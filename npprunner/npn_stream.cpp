#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <map>
#include <gdk/gdk.h>

#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#ifndef ENAVLE_DEBUG
#define dbg_trace(fmt, args...) (void *)0
#else
#define dbg_trace(fmt, args...) fprintf(stderr, fmt, ##args)
#endif

#include "npnapi.h"
#include "npplugin.h"
#include "npn_stream.h"

#define SEEKABLE(A) (NULL != strstr(A, "Accept-Ranges: bytes"))
#define NOTIFY(waitp) if (waitp != NULL) waitp->wt_callback(waitp->wt_udata)

class socket_stream
{
	public:
		socket_stream();
		~socket_stream();

	public:
		int connect(const sockaddr_in *inaddrp, size_t len);
		int get_error(void);
		int connected(void);
		int close(void);

	public:
		int on_read(void);
		int recv_wait(struct waitcb *cbp);
		int recv_data(void *buf, size_t len);
		int recv_chunk(struct chunkcb *chunkcbp);

	public:
		int on_write(void);
		int sent_wait(struct waitcb *cbp);
		int send_chunk(struct chunkcb *chunkcbp);
		int send_data(const void *buf, size_t len);

	private:
		int m_tag;
		int m_files;
		int m_flags;
		int last_error;
		struct waitcb *m_readp;
		struct waitcb *m_writep;
};

static std::map<int, socket_stream *> __stream_list;
static int parse_content_length(const char *headers);
proto_stream *RedirectURLNotify(const char *url, const char *refer);

static void setblockopt(int fd, bool block)
{
	int flags = fcntl(fd, F_GETFL);
	if (block && (flags & O_NONBLOCK))
		fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
	else if (!block && !(flags & O_NONBLOCK))
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

	socket_stream::socket_stream(void)
:m_flags(0), m_readp(0), m_writep(0)
{
	m_files = socket(AF_INET, SOCK_STREAM, 0);
	setblockopt(m_files, false);
	last_error = 0;
}

socket_stream::~socket_stream()
{
	close();
}

int socket_stream::close(void)
{
	if ((m_flags & SF_CLOSED) == 0) {
		m_flags |= SF_CLOSED;
		__stream_list.erase(m_files);
		shutdown(m_files, SHUT_RDWR);
		gdk_input_remove(m_tag);
		::close(m_files);
	}
	return 0;
}

int socket_stream::connected(void)
{
	return (m_flags & SF_CONNECTED);
}

int socket_stream::get_error(void)
{
	return last_error;
}

int socket_stream::on_read(void)
{
	m_flags |= SF_CONNECTED;
	NOTIFY(m_readp);
	return 0;
}

int socket_stream::on_write(void)
{
	if ((m_flags & SF_CONNECTED) == 0) {
		gdk_input_remove(m_tag);
		m_tag = gdk_input_add(m_files, GDK_INPUT_READ, plugin_NetworkNotify, 0);
	}

	m_flags |= SF_CONNECTED;
	NOTIFY(m_writep);
	return 0;
}

int socket_stream::recv_wait(struct waitcb *waitp)
{
	m_readp = waitp;
	return 0;
}

int socket_stream::sent_wait(struct waitcb *waitp)
{
	m_writep = waitp;
	return 0;
}

int socket_stream::recv_data(void *buf, size_t len)
{
	int error;
	error = recv(m_files, (char *)buf, len, 0);
	last_error = (error == -1)? errno: 0;
	return error;
}

int socket_stream::send_data(const void *buf, size_t len)
{
	int error;
	error = send(m_files, (char *)buf, len, 0);
	last_error = (error == -1)? errno: 0;
	return error;
}

int socket_stream::recv_chunk(struct chunkcb *cbp)
{
	int error = 0;

	while (cbp->off < cbp->len) {
		error = recv_data(cbp->buf + cbp->off, cbp->len - cbp->off);
		if (error == -1)
			break;
		if (error == 0)
			break;
		cbp->off += error;
	}

	return error;
}

int socket_stream::send_chunk(struct chunkcb *cbp)
{
	int error = 0;

	while (cbp->off < cbp->len) {
		error = send_data(cbp->buf + cbp->off, cbp->len - cbp->off);
		if (error == -1)
			break;
		cbp->off += error;
	}

	return error;
}

int socket_stream::connect(const sockaddr_in *inaddrp, size_t len)
{
	int error;
	GdkInputCondition cond;
	cond = (GdkInputCondition)(GDK_INPUT_READ| GDK_INPUT_WRITE);

	m_tag = gdk_input_add(m_files, cond, plugin_NetworkNotify, NULL);
	__stream_list[m_files] = this;

	error = ::connect(m_files, (const sockaddr *)inaddrp, len);
	return (errno == EINPROGRESS)? 0: error;
}

static int read_chunk(struct chunkcb *chunkcbp, void *buf, size_t len)
{
	size_t min;

	min = len;
	if (int(len + chunkcbp->off) > chunkcbp->len)
		min = chunkcbp->len - chunkcbp->off;
	memcpy(buf, chunkcbp->buf + chunkcbp->off, min);
	chunkcbp->off += min;
	return min;
}

int proto_stream_wrapper::m_salt = 0;

proto_stream_wrapper::proto_stream_wrapper(proto_stream *protop)
	:m_protop(protop)
{
	int salt;
	char path[1024];
	protop->ref();
	salt = m_salt++;
	sprintf(path, "stream-%d.mp4", salt);
	m_file = fopen(path, "wb");
	return;
}

int proto_stream_wrapper::recv_data(void *buf, size_t len)
{
	int count = m_protop->recv_data(buf, len);
	if (count > 0)
		fwrite(buf, 1, count, m_file);
	return count;
}

proto_stream_wrapper::~proto_stream_wrapper()
{
	m_protop->rel();
	fclose(m_file);
	return;
}

proto_stream::proto_stream(void)
:m_ref(1),  m_flags(0)
{
	m_write.wt_callback = proto_write;
	m_write.wt_udata = this;

	m_read.wt_callback = proto_read;
	m_read.wt_udata = this;

	m_stream = new socket_stream();
	assert(m_stream != NULL);

	memset(&m_header, 0, sizeof(m_header));
	memset(&m_chunk, 0, sizeof(m_chunk));
	m_waitp = 0;
}

proto_stream::~proto_stream()
{
	char *chunk;
	m_stream->close();
	delete m_stream;

	chunk = m_header.buf;
	if (chunk != NULL)
		free(chunk);

	chunk = m_chunk.buf;
	if (chunk != NULL)
		free(chunk);
}

void proto_stream::proto_read(void *upp)
{
	proto_stream *streamp;
	streamp = (proto_stream *)upp;
	streamp->on_read();	
}

static int check_video(char *bound)
{
	if (memcmp(bound, "FLV", 3) &&
			memcmp(bound + 4, "ftyp", 4))
		return 0;
	return PF_VIDEO;
}

void proto_stream::on_read(void)
{
	int len;
	int error;
	char *bound = 0;

	if (m_flags & PF_CREATED) {
		NOTIFY(m_waitp);
		return;
	}

	if (m_chunk.buf == NULL) {
		m_chunk.buf = (char *)malloc(1024 + 1);
		assert(m_chunk.buf != NULL);
		m_chunk.len = 1024;
		m_chunk.off = 0;
	}

	error = m_stream->recv_chunk(&m_chunk);
	if (error == -1 && m_stream->get_error() != EWOULDBLOCK) {
		m_flags |= (PF_CREATED| PF_ERROR);
		NOTIFY(m_waitp);
		return;
	}

	m_chunk.buf[m_chunk.off] = 0;
	//dbg_trace("Hello World: %d\n", __LINE__);
	bound = strstr(m_chunk.buf, "\r\n\r\n");
	if (bound != NULL) {
		len = parse_content_length(m_chunk.buf);
		bound += 4;
	}

	if (bound != NULL && len < 120000) {
		m_chunk.len = m_chunk.off;
		m_chunk.off = (bound - m_chunk.buf);
		m_flags |= PF_CREATED;
		*(bound - 2) = 0;
		NOTIFY(m_waitp);
		return;
	}

	if (bound != NULL &&
			m_chunk.off >= (bound + 12 - m_chunk.buf)) {
		m_chunk.len = m_chunk.off;
		m_chunk.off = (bound - m_chunk.buf);
		m_flags |= check_video(bound);
		m_flags |= PF_CREATED;
		*(bound - 2) = 0;
		NOTIFY(m_waitp);
		return;
	}

	if (error == 0) {
		m_flags |= PF_CREATED;
		m_flags |= PF_ERROR;
		NOTIFY(m_waitp);
	}

	return;
}

void proto_stream::proto_write(void *upp)
{
	proto_stream *streamp;

	streamp = (proto_stream *)upp;
	streamp->on_write();
}

void proto_stream::on_write(void)
{
	int error;
	int writed;

	writed = m_stream->send_chunk(&m_header);
	if (writed >= 0) {
		return;
	}

	error = m_stream->get_error();
	if (error == EWOULDBLOCK) {
		return;
	}

	m_flags |= PF_CREATED;
	m_flags |= PF_ERROR;
	NOTIFY(m_waitp);
	return;
}

void proto_stream::ref(void)
{
	assert(m_ref > 0);
	m_ref++;
}

void proto_stream::rel(void)
{
	assert(m_ref > 0);
	m_ref--;
	if (m_ref == 0)
		delete this;
	return;
}

int proto_stream::recv_wait(struct waitcb *waitp)
{
	m_waitp = waitp;
	return 0;
}

int proto_stream::get_error(void)
{
	/* NPRES_NETWORK_ERR NPRES_DONE */
	return NPRES_DONE;
}

int proto_stream::cantain_video(void)
{
	return (m_flags & PF_VIDEO);
}

int proto_stream::connected(void)
{
	int cond_flag;
	cond_flag = (PF_CREATED| PF_ERROR);
	return (m_flags & cond_flag) == PF_CREATED;
}

const char *proto_stream::response(void)
{
	return m_chunk.buf;
}

int proto_stream::block(void)
{
	int error = m_stream->get_error();
	return (error == EWOULDBLOCK)? 0: -1;
}

int proto_stream::recv_data(void *buf, size_t len)
{
	int min = -1;

	if (m_flags & PF_CREATED) {
		if (m_flags & PF_ERROR) {
			return -1;
		}

		if (m_chunk.off < m_chunk.len) {
			min = read_chunk(&m_chunk, buf, len);
			return min;
		}

		min = m_stream->recv_data(buf, len);
		return min;
	}

	return min;
}

int proto_stream::connect(const sockaddr_in *inaddrp, size_t len)
{
	int error;
	error = m_stream->connect(inaddrp, len);
	m_stream->sent_wait(&m_write);
	return error;
}

int proto_stream::set_request(const char *header)
{
	m_header.off = 0;
	m_header.len = strlen(header);
	m_header.buf = strdup(header);
	m_stream->recv_wait(&m_read);

	if (m_stream->connected())
		proto_write(this);
	return 0;
}

int proto_stream::set_request(const char *header, const char *buf, size_t len)
{
	m_header.off = 0;
	m_header.len = strlen(header);
	m_header.buf = (char *)malloc(m_header.len + len + 1);
	memcpy(m_header.buf, header, m_header.len);
	memcpy(m_header.buf + m_header.len, buf, len);
	m_header.len += len;
	m_header.buf[m_header.len] = 0;

	m_stream->recv_wait(&m_read);
	if (m_stream->connected())
		proto_write(this);
	return 0;
}

void plugin_NetworkNotify(gpointer data, gint fd, GdkInputCondition condition)
{
	if ((condition & GDK_INPUT_READ) &&
			(__stream_list.find(fd) != __stream_list.end()) )
		__stream_list[fd]->on_read();

	if ((condition & GDK_INPUT_WRITE) &&
			(__stream_list.find(fd) != __stream_list.end()) ) {
		__stream_list[fd]->on_write();
	}
}

static int parse_redirect_location(const char *headers, char **plocation)
{
	char *parse;
	char title[13];
	char *location;
	char *optheaders = strdup(headers);

	strncpy(title, headers, 12);
	title[0x7] = (title[0x7] ^ '1')? title[0x7]: '0';
	title[0xB] = (title[0xB] ^ '1')? title[0xB]: '2';
	title[0xC] = 0;

	if (strncmp(title, "HTTP/1.0 302", 12)
			|| !strstr(optheaders, "Location: ")) {
		free(optheaders);
		return false;
	}

	location = strstr(optheaders, "Location: ") + 9;
	while (*++location == ' ');

	parse = location;
	while (*parse) {
		switch(*parse) {
			case ' ':
			case '\r':
			case '\n':
				*parse = 0;
				break;

			default:
				parse++;
				break;
		}
	}

	*plocation = strdup(location);
	free(optheaders);
	return true;
}

static int parse_content_length(const char *headers)
{
	int count;
	char title[512];
	const char *parse;
	size_t length = 0;

	parse = strstr(headers, "Content-Length: ") + 15;
	if (parse - 15 != NULL) {
		count = 0;
		while (*++parse == ' ');
		while (isdigit(*parse) &&
				count + 1 < (int)sizeof(title))
			title[count++] = *parse++;
		title[count] = 0;
		length = atoi(title);
	}

	return length;
}

static char *parse_mime_type(const char *header, char *title, size_t len)
{
	int idx;
	const char *parse;

	assert(len > 0 && title != NULL);
	*title = 0;

	parse = strstr(header, "Content-Type: ") + 13;
	if (parse - 13 != NULL) {
		idx = 0;
		while (*++parse == ' ');
		while (idx + 1 < int(len)) {
			switch(*parse) {
				case '-':
				case '/':
					title[idx++] = *parse++;
					break;

				default:
					if (!isalnum(*parse)) {
						len = idx + 1;
						break;
					}
					title[idx++] = *parse++;
					break;
			}
		}
		title[idx] = 0;
	}

	return title;
}

NPNetStream::NPNetStream(NPP instance,
		void *_notifyData, const char *url, proto_stream *protop)
:m_instance(instance), m_protop(protop), m_flags(0), m_offset(0)
{
	memset(&m_stream, 0, sizeof(m_stream));
	m_stream.notifyData = _notifyData;
	m_stream.headers = "badvalue";
	m_read.wt_callback = recv_wrapper;
	m_read.wt_udata = this;

	m_url = strdup(url);
	m_url0 = strdup(url);
	m_stream.url = m_url;
	m_protop->ref();
}

NPNetStream::~NPNetStream()
{
	int error;
	int resean = NPRES_DONE;

	struct plugin_object *object = NPP2OBJ(m_instance);
	NPPluginFuncs *pluginfuncs = PLUG2FUNC(object->plug);

	if (m_flags & NF_CREATED) {
		resean = m_protop->get_error();
		error = pluginfuncs->destroystream(m_instance, &m_stream, resean);
		assert(NPERR_NO_ERROR == error);
	}

	pluginfuncs->urlnotify(m_instance, m_url, resean, m_stream.notifyData);
	m_protop->rel();
	free(m_url0);
	free(m_url);
	return;
}

#define SWAP(a, b) { char *p = a; a = b; b = p; }

int NPNetStream::OnRead(void)
{
	int len;
	int count;
	int error;
	uint16_t stype;
	char mimetype[256];
	NPPluginFuncs *pluginfunc;
	struct plugin_object *object;

	if (m_protop == NULL) {
		dbg_trace("proto stream not support!\n");
		return -1;
	}

	object = NPP2OBJ(m_instance);
	pluginfunc = PLUG2FUNC(object->plug);

	if (m_protop->connected() && 
			(m_flags & NF_CREATED) == 0) {
		char *location = 0;
		const char *response = 0;
		proto_stream *stream = 0;

		response = m_protop->response();
		if (parse_redirect_location(response, &location)) {
			stream = RedirectURLNotify(location, m_url);
			dbg_trace("redirect: %s\n", location);
			if (stream != NULL) {
				SWAP(m_url, location);
				m_protop->recv_wait(0);
				m_protop->rel();
				m_protop = stream;
				startup();
			}

			free(location);
			return 0;
		}
	}

	if (m_protop->connected() && 
			(m_flags & NF_CREATED) == 0) {
		stype = 1;
		m_offset = 0;
		m_stream.url = m_url;
		m_stream.headers = m_protop->response();
		assert(m_stream.headers != NULL);
		m_stream.end = parse_content_length(m_protop->response());

		if (m_protop->cantain_video()) {
#if 0
			proto_stream *protop = new proto_stream_wrapper(m_protop);
			if (protop != NULL) {
				m_protop->rel();
				m_protop = protop;
			}
			int const limit = 256 * 1024 / 4;
			if (m_stream.end < (size_t)limit) {
				dbg_trace("turn of advise\n");
				m_stream.end /= 2;
			} else {
				m_stream.end = limit;
				play_video(m_url, m_url0);
			}
#endif
		}

		parse_mime_type(m_protop->response(), mimetype, sizeof(mimetype));
		dbg_trace("newstream: %s %d\n", mimetype, m_stream.end);
		error = pluginfunc->newstream(m_instance,
				mimetype, &m_stream, SEEKABLE(m_protop->response()), &stype);
		if (error != NPERR_NO_ERROR) {
			dbg_trace("newstream error: %d", error);
			return -1;
		}

		m_flags |= NF_CREATED;
	}

	len = 0;
	while (m_flags & NF_CREATED) {
		count = pluginfunc->writeready(m_instance, &m_stream);
		if (count == 0) {
			dbg_trace("write not ready!\n");
			return 0;
		}

		count = count < (int)sizeof(m_cache)? count: sizeof(m_cache);
		len = m_protop->recv_data(m_cache, count);
		if (len == -1 || len == 0) {
			break;
		}

		assert(m_stream.headers != NULL);
		count = pluginfunc->write(m_instance,
				&m_stream, m_offset, len, m_cache);
		if (count != len) {
			return -1;
		}

		m_offset += count;
#if 0
		if (m_protop->cantain_video() &&
				m_offset > m_stream.end && m_stream.end > 0) {
			break;
		}
#endif
	}

	return m_protop->block();
}

void NPNetStream::startup(void)
{
	m_protop->recv_wait(&m_read);
	return;
}

void NPNetStream::recv_wrapper(void *upp)
{
	NPNetStream *streamp;
	static int _locked = 0;

	streamp = (NPNetStream *)upp;
	assert(_locked == 0);

	_locked++;
	if (streamp->OnRead() == -1) {
		//dbg_trace("length: %p %d\n", streamp, streamp->m_stream.end);
		//dbg_trace("offset: %p %d\n", streamp, streamp->m_offset);
		delete streamp;
	}
	_locked--;

	return;
}

int CloseAllStream(NPP instance)
{
	return 0;
}

