#include <map>
#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "npupp.h"
#include "nputils.h"
#include "npplugin.h"
#include "npnetstream.h"

#define DEBUG_OUT 0
#define closesocket(a) close(a)
#define WSAGetLastError() errno
#define WSAAsyncSelect(a, b, c, d)

extern NPPluginFuncs __pluginfunc;

static char user_agent[] = 
"Mozilla/5.0 Gecko/2008122208 Firefox/3.0.5"; 
/* "Mozilla/5.0 (X11; U; FreeBSD i386; zh-CN; rv:1.9.1.4) Gecko/20091114 Firefox/3.5.5"; */

static std::map<int, NPNetStream *> __stream_list;

char * NewStreamName(char *buf, size_t len)
{
	static int m_major = 0;
	static int m_minor = 0;

    snprintf(buf, len, "%04d%c", m_major, 'A' + m_minor++);
    return buf;
}

NPNetStream::NPNetStream(NPP instance,
		const char *_url, void *_notifyData)
:m_total(0)
{
	memset(this, 0, sizeof(NPStream));
	memset(m_magic, 0, sizeof(m_magic));
	assert (_url != NULL);
	url = strdup(_url);
	m_instance = instance;
	notifyData = _notifyData;

	m_dstart = NULL;
	in.off = out.off = 0;
	in.len = out.len = 0;
	in.buf = out.buf = NULL;

	m_lastError = 0;
	m_fildes = -1;
	headers = NULL;
	m_cache_file = NULL;
	m_cache_name = NULL;
	m_xyflags    = 0;//XYF_ENABLE;
}

NPNetStream::~NPNetStream()
{
	char buf[1024];
	free(out.buf);
	free((void *)url);
	free((void *)headers);
	if (m_cache_file != NULL)
		fclose(m_cache_file);
	if (m_cache_name == NULL)
		return;

	snprintf(buf, sizeof(buf), "%s.%dK",
			m_cache_name, (end / 1024));
	rename(buf, m_cache_name);
	free(m_cache_name);
}

int NPNetStream::Connect(struct sockaddr *si, size_t silen)
{
	struct sockaddr * name;
	struct sockaddr_in xyaddr;
	m_fildes = socket(AF_INET, SOCK_STREAM, 0);
	WSAAsyncSelect(m_fildes, __netscape_hwnd,
			WM_NETN, FD_READ|FD_WRITE|FD_CLOSE|FD_CONNECT);

	xyaddr.sin_family = PF_INET;
	xyaddr.sin_port   = htons(1080);
	xyaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	name = (m_xyflags & XYF_ENABLE)? (struct sockaddr *)&xyaddr: si;

	if (-1 == connect(m_fildes, name, silen) &&
			10035 != WSAGetLastError()) {
		printf("connect fail!\n");
		closesocket(m_fildes);
		m_fildes = -1;
		return -1;
	}

	if (m_xyflags & XYF_ENABLE) {
		m_xy_snd[0] = 0x04;
		m_xy_snd[1] = 0x01;
		memcpy(&xyaddr, si, sizeof(xyaddr));
		memcpy(&m_xy_snd[2], &xyaddr.sin_port, sizeof(xyaddr.sin_port));
		memcpy(&m_xy_snd[4], &xyaddr.sin_addr, sizeof(xyaddr.sin_addr));
		m_xy_snd[8] = 0;
	}

	__stream_list[m_fildes] = this;
	this->lastmodified = time(NULL);
	return 0;
}

int GetAddressByURL(const char *url, struct sockaddr_in *si)
{
	char *p;
	char buf[1024];
	hostent *phent;

	if (0 != strncmp(url, "http://", 7))
		return -1;

	strncpy(buf, url + 7, sizeof(buf));

	if ((p = strchr(buf, '/')) != NULL)
		*p = 0;

	if ((p = strchr(buf, ':')) != NULL)
		*p++ = 0;

	phent = gethostbyname(buf);
	if (phent == NULL)
		return -1;

	memcpy(&si->sin_addr,  phent->h_addr, sizeof(in_addr));
	si->sin_port = htons(p? atoi(p): 80);
	si->sin_family = AF_INET;
	return 0;
}

int NPNetStream::ReadHeader(void)
{
	int len = 0;
	int ibytes ;
	char *pel = NULL;
	char buf[40960] = "";

	if (headers != NULL) {
		len = strlen(headers);
		memcpy(buf, headers, len + 1);
		free((void*)headers);
		headers = NULL;
	}

	do {
		assert(len + 1 < sizeof(buf));
		ibytes = recv(m_fildes, buf + len, sizeof(buf) - len - 1, 0);
		if (ibytes <= 0) {
			buf[len] = 0;
			headers = strdup(buf);
			m_lastError = WSAGetLastError();
			return ibytes;
		}
		m_total += ibytes;
		len += ibytes;
		buf[len] = 0;
	} while( !strstr(buf, "\r\n\r\n") );

	m_lastError = 0;
	headers = (char *) malloc(len + 1);
	memcpy((void*)headers, buf, len + 1);

	if ((pel = strstr(headers, "\r\n\r\n")) != NULL) {
		pel[2] = pel[3] = 0;
		in.len = (headers + len - &pel[4]);
		m_dstart = &pel[4];
		in.buf = &pel[4];
		in.off = 0;
	}
	return ibytes;
}

void NPNetStream::ParseURLHeader(void)
{
	char buf[1024];
	char *np = buf;
	const char *pcl;

	end = 0;
	buf[0] = 0;
	if ((pcl = strstr(headers, "Content-Length: ")) != NULL){
		pcl += 16;
		while (*pcl == ' ')
			pcl++;

		int len = sizeof(buf);
		while (isdigit(*pcl) && len > 0){
			*np++ = *pcl;
			len --;
			pcl ++;
		}
		*np = 0;

		end = atoi(buf);
	}
#if 0
	"Last-Modified: ";
#endif
}

static char *GetMineType(const char *header, char *buf, size_t len)
{
	char *pct, *np;
	assert(header != NULL && len > 0 && buf != NULL);

	buf[0] = 0;
	if ((pct = strstr(header, "Content-Type: ")) != NULL){
		pct += 14;
		while(*pct == ' ')
			pct++;

		np = buf;
		while (len > 0 &&
				(isalnum(*pct) ||
				 *pct == '-' ||
				 *pct == '/')) {
			*np++ = *pct++;
			len --;
		}
		*np = 0;
	}


	return buf;
}

#define SEEKABLE(A) (NULL != strstr(A, "Accept-Ranges: bytes"))

int NPNetStream::RedirectURL(void)
{
	char *pl;
	char hline[13];
	char *location;
	struct sockaddr_in si;
	char *optheaders = strdup(headers);

	strncpy(hline, headers, 12);

	if (hline[7] == '1')
		hline[7] = '0';

	if (hline[11] == '1')
		hline[11] = '2';

	if (strncmp(hline, "HTTP/1.0 302", 12)
			|| !strstr(optheaders, "Location: ")){
		printf("Not A Redirect URL: %s!\n", optheaders);
		free(optheaders);
		return FALSE;
	}

	location = strstr(optheaders, "Location: ") + 10;
	while (*location == ' ')
		location++;

	pl = location;
	while(*pl != ' ' && *pl != '\r' &&
			*pl != '\n' && *pl)
		pl++;
	*pl = 0;

	if (GetAddressByURL(location, &si) != 0) {
		printf("Hello World!\n");
		return FALSE;
	}

	NPNetStream *nstream = new NPNetStream(m_instance, location, notifyData);
	if (nstream->Connect((struct sockaddr*)&si, sizeof(si)) != 0) {
		printf("NPNetStream::Connect\n");
		nstream->Close(FALSE);
		return FALSE;
	}

	if (nstream->PrepareGetURLHeaders(url) != 0) {
		printf("Redirect PrepareGetURLHeaders\n");
		nstream->Close(FALSE);
		return FALSE;
	}

	printf("redirect url ing!\n");
	return TRUE;
}

void NPNetStream::OnRead(void)
{
	int nbytes;
	int error, count;
	uint16 stype = 1;
	char minetype[256];

	int test_flags = XYF_ENABLE | XYF_SYNGET;
	if ((m_xyflags & test_flags) ==  XYF_ENABLE) {
	   	count = recv(m_fildes, m_xy_rcv, 8, 0);
		if (count == -1 && WSAGetLastError() == 10035)
			return;
		m_xyflags |= XYF_SYNGET;
		printf("proxy recv: %d\n", count);
		assert (count == 8);
		return;
	}

	if (in.len == 0 && m_dstart == NULL) {
		error = ReadHeader();
		if (error == -1 && m_lastError != 10035) {
			Close(TRUE);
			return;
		}
		const char *dstart = m_dstart;
		if (dstart != NULL || error == 0) {
			ParseURLHeader();
			if (RedirectURL() == TRUE) {
				printf("do URL Redirecting!\n");
				m_dstart = NULL;
				Close(FALSE);
				return;
			}
			assert (headers != NULL);
			__pluginfunc.newstream(m_instance,
					GetMineType(headers, minetype, sizeof(minetype)),
					this, SEEKABLE(headers), &stype);
		}
	}

	while (in.off < in.len) {
		count = __pluginfunc.writeready(m_instance, this);
		if (count == 0) {
			printf("plugin buffer full!\n");
			return;
		}
		nbytes = __pluginfunc.write(m_instance, this, in.off,
				in.len - in.off, in.buf);
		CacheWrite(in.buf, in.len - in.off, in.off);
		assert(nbytes >= count || nbytes + in.off == in.len);
		in.off += nbytes;
		in.buf += nbytes;
	}

	char buf[16384];
	int test;
	size_t nready;

	for ( ; ; ) {
		nready = __pluginfunc.writeready(m_instance, this);
		if (nready == 0) {
			printf("plugin buffer full!\n");
			return;
		}
		assert (nready > 0);
		nready = (nready < sizeof(buf))? nready: sizeof(buf);
		count = recv(m_fildes, buf, nready, 0);
		if (count <= 0) {
			m_lastError = WSAGetLastError();
			break;
		}
		m_total += count;
		test = __pluginfunc.write(m_instance, this, in.off, count, buf);
		CacheWrite(buf, count, in.off);
		if (test == -1) {
			printf("plugin request close Stream: %p\n", this);
			m_lastError = 190;
			Close(TRUE);
			return ;
		}
		assert(test == count);
		in.off += test;
	}

	if (count == -1 
			&& WSAGetLastError() == 10035)
		return;

	printf("Size: %ld\n", in.len);
	printf("OnRead Return: (%ld >= %ld + %ld) (%ld <= %d)\n----\n%s\n----\n",
			m_total, strlen(headers), in.off, in.off, end, headers);

	m_lastError = 0;
	Close(TRUE);
}

int NPNetStream::CacheWrite(const void *buf, size_t len, size_t off)
{
	NPPlugin *plugin;
	char name[512];
	char tmp_name[1024];
	size_t todrop = (off + len - 32);

	if (todrop > len)
		todrop = len;
	else if (todrop < 0)
		todrop = 0;

	memcpy(m_magic + off, buf, len - todrop);

	if (m_cache_file == NULL &&
			memcmp(m_magic, "FLV", 3) &&
			memcmp(m_magic + 4, "ftypisom", 8))
		return len;

	if (m_cache_file != NULL)
		return fwrite(buf, 1, len, m_cache_file);

	name[0] = 0;
	plugin = (NPPlugin *)m_instance;
	NewStreamName(name, sizeof(name));

	if (memcmp(m_magic, "FLV", 3) == 0)
		strncat(name, ".flv", sizeof(name));
	else if (memcmp(m_magic + 4, "ftypisom", 8) == 0)
		strncat(name, ".mp4", sizeof(name));
	name[sizeof(name) - 1] =0;

	m_cache_name = strdup(name);
	snprintf(tmp_name, sizeof(tmp_name), "%s.%dK", name, (end / 1024));
	m_cache_file = fopen(tmp_name, "wb");
	fwrite(m_magic, 1,  off, m_cache_file);

	return fwrite(buf, 1, len, m_cache_file);
}

void NPNetStream::OnWrite()
{
	ssize_t count;
	int wr_off, wr_count;

	int test_flags = XYF_ENABLE | XYF_SYNPUT;
	if ((m_xyflags & test_flags) ==  XYF_ENABLE) {
	   	count = send(m_fildes, m_xy_snd, 9, 0);
		if (count == -1 && WSAGetLastError() == 10035)
			return;
		m_xyflags |= XYF_SYNPUT;
		assert (count == 9);
	}

	if (out.off < out.len) {
		wr_off = out.off;
		wr_count = out.len;

		count = 0;
		assert(out.buf != NULL);
		while (wr_off < wr_count && count != -1){
			count = send(m_fildes, out.buf + wr_off,
					wr_count - wr_off, 0);
			if (count == -1) {
				m_lastError = WSAGetLastError();
				break;
			}
			wr_off += count;
		}
		if (count == -1 && m_lastError != 10035){
			Close(TRUE);
			return;
		}
		m_lastError = 0;
		out.off = wr_off;
	}
}

void NPNetStream::Close(bool notify)
{
	NPError error = 0;
#if 0
	printf("streamClose: fd:%d end:%d off:%d\n%s\n%s\n",
			impl->sin_file, stream->end, impl->in_off, 
			impl->out_buff?impl->out_buff:"send ok\n",
			stream->headers);
#endif
	if (m_fildes != -1) {
		WSAAsyncSelect(m_fildes, __netscape_hwnd, 0, 0);
		__stream_list.erase(m_fildes);
		closesocket(m_fildes);
		m_fildes = -1;
	}

	int resean = m_lastError? NPRES_NETWORK_ERR: NPRES_DONE;
	if (m_lastError == 190)
		resean = NPRES_USER_BREAK;

	printf("last op count: %d\n", m_lastError);

	if (m_dstart != NULL)
		error = __pluginfunc.destroystream(m_instance, this, resean);

	if (notify == TRUE)
		__pluginfunc.urlnotify(m_instance, url, resean, notifyData);

	delete this;
}

int NPNetStream::PrepareGetURLHeaders(const char *refer)
{
	char * p;
	char buf[1024];
	char headers[8192];
	char * foo = headers;

	if (0 != strncmp(this->url, "http://", 7)){
		printf("PrepareGetURLHeaders fail: %s\n", this->url);
		/*************************/
		assert(0);
		return -1;
	}

	strncpy(buf, (this->url + 7), sizeof(buf));
	if ((p = strchr(buf, '/')) != NULL)
		*p++ = 0;

	foo += sprintf(foo, "GET /%s HTTP/1.0\r\n", p? p: "");
	foo += sprintf(foo, "Host: %s\r\n", buf);
	foo += sprintf(foo, "User-Agent: %s\r\n", user_agent);
	foo += sprintf(foo, "Accept: application/xml;q=0.9,*/*;q=0.8\r\n");
	if (refer != NULL)
		foo += sprintf(foo, "Refer: %s\r\n", refer);
	foo += sprintf(foo, "Connection: close\r\n\r\n");

	out.buf = strdup(headers);
	out.len = (foo - headers);
	out.off = 0;
	return 0;
}

int NPNetStream::PreparePostURLHeader(const char *url,
		const void *data, size_t len)
{
	char *p;
	char buf[1024];
	char headers[409600];
	char *foo = headers;

	if (0 != strncmp(url, "http://", 7)) {
		printf("PreparePostURLHeader Fail: %s\n", url);
		assert(0);
		return -1;
	}

	strncpy(buf, (url + 7), sizeof(buf));
	if ((p = strchr(buf, '/')) != NULL)
		*p++ = 0;

	foo += sprintf(foo, "POST /%s HTTP/1.0\r\n", p? p: "");
	foo += sprintf(foo, "Host: %s\r\n", buf);
	foo += sprintf(foo, "User-Agent: %s\r\n", user_agent);
	foo += sprintf(foo, "Accept: application/xml;q=0.9,*/*;q=0.8\r\n");
	foo += sprintf(foo, "Connection: close\r\n");
	out.len = (foo - headers);
	out.buf = (char *)malloc(out.len + len);
	memcpy(out.buf, headers, out.len);
	memcpy(out.buf + out.len, data, len);
	out.len += len;
	out.off = 0;
	return 0;
}

int plugin_NetworkNotify(int fd, int event)
{
#if 0
	switch(WSAGETSELECTEVENT(event))
	{
		case FD_CONNECT:
			printf("FD_CONNECT: %d %d\n", fd, WSAGETSELECTERROR(event));
			break;
		case FD_CLOSE:
		case FD_READ:
			if (__stream_list.find(fd)
					!= __stream_list.end())
				__stream_list[fd]->OnRead();
			break;
		case FD_WRITE:
			if (__stream_list.find(fd)
					!= __stream_list.end())
				__stream_list[fd]->OnWrite();
			break;
		default:
			printf("Unkown Event!\n");
			break;
	}
#endif
	return 0;
}

int CloseAllStream(NPP instance)
{
	std::map<int, NPNetStream*>::iterator iter;
	iter = __stream_list.begin();

	NPNetStream * stream = NULL;
	while (iter != __stream_list.end()) {
		if (iter->second->m_instance == instance)
			stream = iter->second;
		++iter;
		if (stream != NULL) {
			stream->Close(FALSE);
			stream = NULL;
		}
	}

	return 0;
}

