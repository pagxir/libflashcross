#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <map>
#include <fcntl.h>
#include <gdk/gdk.h>

#include "npupp.h"
#include "npnapi.h"
#include "plugin.h"
#include "npp_script.h"

#define DEBUG_OUT 0
#define closesocket(a) close(a)
#define WSAGetLastError() errno

static NPP_t __plugin;
static char *__flv_url = NULL;
static HWND __netscape_hwnd;
static NPWindow __plugin_window;
static NPObject *__window_object;
static std::map<int, NPStream*> __stream_list;
static std::map<int, gint> __stream_list_tag;
NPPluginFuncs __pluginfunc;
static NPNetscapeFuncs __netscapefunc;

char __top_id[] = {"top"};
char __location_id[] = {"location"};
char __toString_id[] = {"toString"};
char __TextData_id[] = {"TextData"};
char ___DoFSCommand_id[] = {"_DoFSCommand"};

static void dbg_alert(int line)
{
	/* char buffer[1024]; */
	exit(0);
}

static void setblockopt(int fd, bool block)
{
	int flags = fcntl(fd, F_GETFL);
	if (block && (flags & O_NONBLOCK))
		fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
	else if (!block && !(flags & O_NONBLOCK))
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static const char *__var(const NPVariant *variant, char *buffer)
{
	size_t count;
	switch(variant->type){
		case NPVariantType_Void:
			sprintf(buffer, "void_var(%p)", variant);
			break;
		case NPVariantType_Null:
			sprintf(buffer, "null_var(%p)", variant);
			break;
		case NPVariantType_Bool:
			sprintf(buffer, "bool_var(%s)",
					variant->value.boolValue?"true":"false");
			break;
		case NPVariantType_Int32:
			sprintf(buffer, "int_var(%x)", variant->value.intValue);
			break;
		case NPVariantType_String:
			count = sprintf(buffer, "string_var(%s)",
					variant->value.stringValue.utf8characters);
			break;
		case NPVariantType_Object:
			sprintf(buffer, "object_var(%p=%p)", variant, variant->value.objectValue);
			break;
		default:
			break;
	}
	return buffer;
}

static const char *VAR(const NPVariant *variant)
{
	char buffer[10240];
	if (variant != NULL)
		return __var(variant, buffer);
	return "var(NULL)";
}

static const char *VAR_X(const NPVariant *variant)
{
	char buffer[10240];
	if (variant != NULL)
		return __var(variant, buffer);
	return "var(NULL)";
}

#define NPVT_String(a, b)  do{\
	a->type=NPVariantType_String; \
	a->value.stringValue.UTF8length = strlen(b); \
	a->value.stringValue.UTF8characters = strdup(b); }while(0)

bool NPN_Invoke(NPP npp, NPObject* obj, NPIdentifier methodName,
		const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	printf("NPN_Invoke: %p %s %ld\n",
		obj, (char *)methodName, sizeof(NPVariant));
	assert (obj && obj->_class);
	assert (obj->_class->invoke);
	bool retval = obj->_class->invoke(obj, methodName, args, argCount, result);
	printf("RET: %s\n", VAR(result));
	return retval;
}

const char *NPN_UserAgent(NPP instance)
{
	static char user_agent[] =
		"Mozilla/5.0 (X11; U; FreeBSD i386; en-US; rv:1.9.0.5) Gecko/2008122208 Firefox/3.0.5";
	return user_agent;
}

NPError NPN_SetValue(NPP instance, NPPVariable variable, void *value)
{
	NPError error = NPERR_GENERIC_ERROR;
#if 0
	if (variable == NPNVnetscapeWindow){
		printf("----------NPN_SetValue: %p, %p\n", value, __plugin_window.window);
		error = NPERR_NO_ERROR;
	}else{
		printf("NPN_SetValue: %d\n", variable);
	}
#endif
	return error;
}

NPError NPN_GetValue(NPP instance, NPNVariable variable, void *ret_alue)
{
	assert(ret_alue);
	NPError error = NPERR_GENERIC_ERROR;
	if (variable == NPNVWindowNPObject){
		*(NPObject**)ret_alue = __window_object;
		__window_object->referenceCount++;
		error = NPERR_NO_ERROR;
	}else if (variable == NPNVnetscapeWindow){
		*(void**)ret_alue = __plugin_window.window;
		error = NPERR_NO_ERROR;
	}else if (variable == NPNVSupportsWindowless){
		*(bool*)ret_alue = true;
		error = NPERR_NO_ERROR;
	}else if (variable == NPNVSupportsXEmbedBool){
		*(bool*)ret_alue = true;
		error = NPERR_NO_ERROR;
	}else if (variable == NPNVToolkit){
		*(NPNToolkitType *)ret_alue = NPNVGtk2;
		error = NPERR_NO_ERROR;
#if 0
	}else if (variable == NPNVprivateModeBool){
		*(bool *)ret_alue = false;
		error = NPERR_NO_ERROR;
#endif
	}else{
		printf("NPN_GetValue: %p 0x%x %d\n", instance, variable, variable);
	}
	return error;
}

NPIdentifier NPN_GetStringIdentifier(const NPUTF8* name)
{
	NPIdentifier ident = NULL;
	if (strcmp(name, __location_id)==0)
		ident = (NPIdentifier)__location_id;
	else if (strcmp(__toString_id, name)==0)
		ident = (NPIdentifier)__toString_id;
	else if (strcmp(__top_id, name)==0)
		ident = (NPIdentifier)__top_id;
	else if (strcmp(__TextData_id, name)==0)
		ident = (NPIdentifier)__TextData_id;
	else if (strcmp(___DoFSCommand_id, name)==0)
		ident = (NPIdentifier)___DoFSCommand_id;
	else
		printf("NPN_GetStringIdentifier: %s!\n", name);
	return ident;
}

bool NPN_GetProperty(NPP npp, NPObject *obj, NPIdentifier propertyName, NPVariant *result)
{
	printf("NPN_GetProperty: %p %s %ld\n", obj, (char *)propertyName, sizeof(NPVariant));
	assert(obj && obj->_class);
	assert(obj->_class->getProperty);
	bool retval =  obj->_class->getProperty(obj, propertyName, result);
	printf("RET: %s\n", VAR(result));
	return retval;
}

int plugin_Player()
{
#if 0
	char buffer[4096];
	//HWND hWnd = (HWND)__plugin_window.window;
	if (__flv_url != NULL){
		sprintf(buffer, "mplayer -softvol -cache 231072 -user-agent '%s' '%s' &",
				NPN_UserAgent(NULL), __flv_url);
		system(buffer);
		exit(0);
	}
#endif
	return 0;
}

void NPN_ReleaseVariantValue(NPVariant *variant)
{
	if (variant != NULL)
		switch (variant->type){
			case NPVariantType_String:
				free((void*)variant->value.stringValue.utf8characters);
				variant->value.stringValue.utf8length = 0;
				variant->type = NPVariantType_Void;
				break;
			case NPVariantType_Object:
				NPN_ReleaseObject(variant->value.objectValue);
				variant->value.objectValue = 0;
				variant->type = NPVariantType_Void;
				break;
			case NPVariantType_Void:
				break;
			default:
				break;
		}
}

#include <stack>

static bool __popup_state = true;
static std::stack<bool> __popup_stack;

bool NPN_PushPopupsEnabledState_fixup(NPP npp, NPBool enabled)
{
	bool old = __popup_state;
	__popup_stack.push(__popup_state);
	__popup_state = enabled;
	return old;
}

bool NPN_PopPopupsEnabledState_fixup(NPP npp)
{
	bool old = __popup_state;
	__popup_state = __popup_stack.top();
	__popup_stack.pop();
	return old;
}

struct StreamImpl{
	int sin_file;
	int op_error;
	int op_line;
	u_short sin_port;
	in_addr sin_addr;

	int out_off;
	int out_len;
	char *out_buff;

	int   in_off;
	int   in_len;
	char *in_ptr;
	char *in_bptr;
	char  magic[32];

	char *referto;
};

static int getUrlHost(StreamImpl *impl, const char *url)
{
	char *p;
	char buff[1024];
	assert(url);
	if (0!=strncmp(url, "http://", 7)){
		/*************************/
		printf("getUrlHost Fail: %s\n", url);
		return -1;
	}
	assert(strlen(url)<1024);
	strcpy(buff, url+7);
	if ((p = strchr(buff, '/')) != NULL)
		*p = 0;
	if ((p = strchr(buff, ':')) != NULL)
		*p++ = 0;
	hostent *phent = gethostbyname(buff);
	if (phent == NULL){
		perror("gethostbyname");
		printf("getHost: %s fail\n", buff);
		return -1;
	}
	impl->sin_port = htons(p?atoi(p):80);
	impl->sin_addr = *(in_addr*)phent->h_addr;
	return 0;
}

int streamReadHeader(NPStream *stream)
{
	int nr;
	int count = 0;
	char buffer[40960];
	StreamImpl *impl = (StreamImpl*)stream->ndata;

	assert(impl);

	if (stream->headers != NULL){
		count = strlen(stream->headers);
		memcpy(buffer, stream->headers, count);
		free((void*)stream->headers);
		stream->headers=NULL;
		buffer[count]=0;
	}

	do {
		assert(count+1 < sizeof(buffer));
		nr = recv(impl->sin_file, buffer+count,
				sizeof(buffer)-count-1, 0);
		if (nr <= 0){
			impl->op_error = nr?WSAGetLastError():0;
			return nr;
		}
		count += nr;
		buffer[count] = 0;
	}while(!strstr(buffer, "\r\n\r\n"));

	const char *p1 = NULL;
	impl->op_error = 0;
	stream->headers = (char*)malloc(count+1);
	memcpy((void *)stream->headers, buffer, count + 1);
	if ((p1=strstr(stream->headers, "\r\n\r\n")) != NULL){
		char * p = (char *)p1;
		p[2] = 0; p[3] = 0;
		impl->in_bptr = &p[4];
		impl->in_ptr = &p[4];
		impl->in_len = (stream->headers+count-&p[4]);
		impl->in_off = 0;
	}
	//printf("URL HEADER: %s\n %s\n", stream->url, stream->headers);
	return nr;
}

void getUrlInfo(NPStream *stm)
{
	const char *p;
	char buff[1024]="";
	char *np = buff;
	assert(stm);
	printf("%p\n", stm->headers);
	assert(stm->headers);
	if ((p=strstr(stm->headers, "Content-Length: ")) != NULL){
		while (p[16]==' ')p++;
		while (isdigit(p[16])){
			*np++=p[16];
			*np = 0;
			p++;
		}
		stm->end = atoi(buff);
	}
#if 0
	"Last-Modified: ";
#endif
}

static char *ToMineType(const char *header)
{
	char *p = NULL;
	char *np = NULL;
	static char buffer[1024] = "";
	assert(header);
	if ((p=(char*)strstr(header, "Content-Type: ")) != NULL){
		np = buffer;
		while(p[14]==' ')p++;
		while(isalnum(p[14])||p[14]=='-'||p[14]=='/'){
			*np++ = p[14];
			*np = 0;
			p++;
		}
	}
	return buffer;
}

void streamClose(NPStream *stream, int fd);

#define SEEKABLE(A) (NULL!=strstr(A, "Accept-Ranges: bytes"))

int GetOutBuff(StreamImpl *impl, const char *url, const char *orig_url);

int urlRedirect(NPStream *stream)
{
	char hline[13];
	assert(stream);
	StreamImpl *impl = (StreamImpl*)stream->ndata;
	assert(stream->headers);
	char *headers = strdup(stream->headers);

	strncpy(hline, headers, 12);
	if (hline[7] == '1')
		hline[7] = '0';
	if (hline[11] == '1')
		hline[11] = '2';

	if (strncmp(hline, "HTTP/1.0 302", 12)
			|| !strstr(headers, "Location: ")){
		free(headers);
		return 0;
	}

	char *location = strstr(headers, "Location: ")+10;

	while (*location==' ')
		location++;

	char *p = location;
	while(*p!=' '&&*p!='\r'&&*p!='\n'&&*p)p++;
	*p = 0;

	impl = new StreamImpl();
	if (-1==getUrlHost(impl, location)){
		free(headers);
		delete impl;
		//assert(0);
		return 0;
	}
	int fd = impl->sin_file = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in so_addr;
	so_addr.sin_family = AF_INET;
	so_addr.sin_addr = impl->sin_addr;
	so_addr.sin_port = impl->sin_port;
	setblockopt(fd, false);
	if (-1 == connect(fd, (sockaddr*)&so_addr, sizeof(sockaddr))){
		if (EINPROGRESS != WSAGetLastError()){
			free(headers);
			closesocket(fd);
			delete impl;
			return 0;
		}
	}
	gint tag = gdk_input_add(fd, (GdkInputCondition)(GDK_INPUT_WRITE|GDK_INPUT_READ), plugin_NetworkNotify, NULL);
	__stream_list_tag[fd] = tag;
	impl->in_off = 0;
	impl->in_len = 0;
	impl->in_ptr = NULL;
	impl->in_bptr = NULL;
	impl->referto = NULL;
	memset(impl->magic, 0, 32);
	GetOutBuff(impl, location, stream->url);
	printf("NewStream Start0: fd:%d!\n", fd);
	NPStream *nStream = new NPStream();
	nStream->url = strdup(location);
	nStream->headers = NULL;
	nStream->end = 0;
	nStream->notifyData = stream->notifyData;
	nStream->ndata = impl;
	nStream->lastmodified = time(NULL);
	__stream_list[fd] = nStream;

	impl = (StreamImpl*)stream->ndata;
	assert(impl);
	tag = __stream_list_tag[impl->sin_file];
	__stream_list_tag.erase(impl->sin_file);
	gdk_input_remove(tag);
	__stream_list.erase(impl->sin_file);
	closesocket(impl->sin_file);
	free((void*)stream->url);
	free((void*)stream->headers);
	if (impl->out_buff)
		free(impl->out_buff);
	if (impl->referto)
		free(impl->referto);
	free(headers);
	delete stream;
	delete impl;
	return 1;
}

void streamRead(NPStream *stream, int fd)
{
	uint16 stype = 1;
	assert(stream);
	StreamImpl *impl = (StreamImpl*)stream->ndata;
	assert(fd==impl->sin_file);

	if (impl->in_len==0 && impl->in_bptr==NULL){
		int err = streamReadHeader(stream);
		if (err==-1&&impl->op_error!=EAGAIN){
			streamClose(stream, fd);
			return;
		}
		if (impl->in_bptr || err==0){
			if (err == 0){
				printf("REFURL: %s\n", stream->url);
				streamClose(stream, fd);
				return;
			}
			getUrlInfo(stream);
			if (urlRedirect(stream)){
				printf("do URL Redirecting!\n");
				return;
			}
			__pluginfunc.newstream(&__plugin,
					ToMineType(stream->headers), stream,
					SEEKABLE(stream->headers), &stype);
		}
	}

	while (impl->in_len > 0){
		int count = __pluginfunc.writeready(&__plugin, stream);
		if (count == 0){
			printf("plugin buffer full!\n");
			return;
		}
		int n = __pluginfunc.write(&__plugin, stream,
				impl->in_off, impl->in_len, impl->in_ptr);
		if (n == -1){
			impl->op_error = 190;
			printf("plugin request close Stream: %p\n", stream);
			streamClose(stream, fd);
			return;
		}
		if (impl->in_off < 32){
			int idx = impl->in_off;
			const char *ptr = impl->in_ptr;
			for (int i=0; i<impl->in_len&&idx<32; idx++, i++){
				impl->magic[idx] = ptr[i];
			}
			if (memcmp(impl->magic, "FLV", 3)==0){
				__flv_url = strdup(stream->url);
				plugin_Player();
				printf("Found FLV Stream %d: %s\n", stream->end, stream->url);
				if (impl->referto)
					printf("Refer FLV Stream: %s\n", impl->referto);
				printf("HEADER FLV Stream : %s\n", stream->headers);
				impl->magic[2] = '+';
			}
			if (memcmp(impl->magic+4, "ftypisom", 8)==0){
				__flv_url = strdup(stream->url);
				plugin_Player();
				printf("Found MP4 Stream %d: %s\n", stream->end, stream->url);
				if (impl->referto)
					printf("Refer MP4 Stream: %s\n", impl->referto);
				printf("HEADER MP4 Stream : %s\n", stream->headers);
				impl->magic[4] = '+';
			}
		}
		printf("%d %d %d\n", n, count, impl->in_len);
		assert(n>=count||n==impl->in_len);
		impl->in_len -= n;
		impl->in_off += n;
	}

	int count=0, test=0;
	char buffer[4096];
	for (;;){
		int bufsize = __pluginfunc.writeready(&__plugin, stream);
		if (bufsize == 0){
			printf("plugin buffer full!\n");
			return;
		}
		count = recv(impl->sin_file, buffer, bufsize>4096?4096:bufsize, 0);
		if (count <= 0){
			impl->op_error = count?WSAGetLastError():0;
			break;
		}
		test = __pluginfunc.write(&__plugin, stream, impl->in_off, count, buffer);
		if (test == -1){
			impl->op_error = 190;
			printf("plugin request close Stream: %p\n", stream);
			streamClose(stream, fd);
			return;
		}
		if (impl->in_off < 32){
			int idx = impl->in_off;
			const char *ptr = buffer;
			for (int i=0; i<count&&idx<32; idx++, i++){
				impl->magic[idx] = ptr[i];
			}
			if (memcmp(impl->magic, "FLV", 3)==0){
				__flv_url = strdup(stream->url);
				plugin_Player();
				printf("Found FLV Stream %d: %s\n", stream->end, stream->url);
				if (impl->referto)
					printf("Refer FLV Stream: %s\n", impl->referto);
				printf("HEADER FLV Stream : %s\n", stream->headers);
				impl->magic[2] = '+';
			}
			if (memcmp(impl->magic+4, "ftypisom", 8)==0){
				__flv_url = strdup(stream->url);
				plugin_Player();
				printf("Found MP4 Stream %d: %s\n", stream->end, stream->url);
				if (impl->referto)
					printf("Refer MP4 Stream: %s\n", impl->referto);
				printf("HEADER MP4 Stream : %s\n", stream->headers);
				impl->magic[4] = '+';
			}
		}
		assert(test == count);
		impl->in_off += count;
	}

	if (count==-1
			&& WSAGetLastError()==EAGAIN)
		return;
	impl->op_error = 0;
	streamClose(stream, fd);
}

void streamWrite(NPStream *stream, int fd)
{
	StreamImpl *impl = (StreamImpl*)stream->ndata;
	assert(fd == impl->sin_file);

	if (impl->out_off < impl->out_len){
		int wr_off = impl->out_off;
		int wr_count = impl->out_len;

		int count = 0;
		assert(impl->out_buff);
		while (wr_off<wr_count && count!=-1){
			count = send(impl->sin_file,
					impl->out_buff+wr_off, wr_count-wr_off, 0);
			if (count == -1){
				impl->op_error = count?WSAGetLastError():0;
				break;
			}
			wr_off += count;
		}
		if (count==-1 && WSAGetLastError()!=EAGAIN){
			free(impl->out_buff);
			impl->out_buff = NULL;
			impl->out_off  = impl->out_len;
			streamClose(stream, fd);
			return;
		}
		impl->op_error = 0;
		impl->out_off = wr_off;
		if (wr_off >= wr_count){
			free(impl->out_buff);
			impl->out_buff = NULL;
			gint tag = __stream_list_tag[impl->sin_file];
			gdk_input_remove(tag);
			__stream_list_tag[impl->sin_file] =
				gdk_input_add(impl->sin_file, GDK_INPUT_READ, plugin_NetworkNotify, 0);
		}
	}
	
	//printf("stream write: %s\n", stream->url);
}

void streamClose(NPStream *stream, int fd)
{
	NPError err = 0;
	assert(stream);
	StreamImpl *impl = (StreamImpl*)stream->ndata;

	assert(impl->sin_file==fd);
#if 0
	printf("streamClose: fd:%d end:%d off:%d\n%s\n%s\n",
			impl->sin_file, stream->end, impl->in_off,
			impl->out_buff?impl->out_buff:"send ok\n",
			stream->headers);
#endif
	__stream_list.erase(impl->sin_file);
	gint tag = __stream_list_tag[fd];
	__stream_list_tag.erase(impl->sin_file);
	gdk_input_remove(tag);
	closesocket(impl->sin_file);

	int resean = (impl->op_error)?NPRES_NETWORK_ERR:NPRES_DONE;
	if (impl->op_error == 190)
		resean = NPRES_USER_BREAK;

	printf("last op count: %d\n", impl->op_error);
	if (impl->in_bptr)
		err = __pluginfunc.destroystream(&__plugin, stream, resean);
	__pluginfunc.urlnotify(&__plugin, stream->url,
			resean, stream->notifyData);
	if (stream->url)
		free((void*)stream->url);
	if (stream->headers)
		free((void*)stream->headers);
	if (impl->out_buff)
		free(impl->out_buff);
	if (impl->referto)
		free(impl->referto);
	delete stream;
	delete impl;
}

int GetOutBuff(StreamImpl *impl, const char *url, const char *orig_url)
{
	char *p;
	char buff[1024];
	char tmpbuff[4096];
	assert(url&&impl);
	if (0!=strncmp(url, "http://", 7)){
		/*************************/
		printf("buildOutBuff Fail: %s\n", url);
		assert(0);
		return -1;
	}
	assert(strlen(url)<1024);
	strcpy(buff, url+7);
	if ((p=strchr(buff, '/')) != NULL)
		*p = 0;
	int olen = sprintf(tmpbuff, "GET /%s HTTP/1.0\r\n", p?p+1:"");
	olen += sprintf(tmpbuff+olen, "Host: %s\r\n", buff);
	olen += sprintf(tmpbuff+olen, "User-Agent: %s\r\n", NPN_UserAgent(NULL));
	olen += sprintf(tmpbuff+olen, "Accept: text/html,application/xml;q=0.9,*/*;q=0.8\r\n");
	if (orig_url != NULL){
		impl->referto = strdup(orig_url);
		olen += sprintf(tmpbuff+olen, "Refer: %s\r\n", orig_url);
	}
	olen += sprintf(tmpbuff+olen, "Connection: close\r\n\r\n");
	assert(olen<4096);
	impl->out_off = 0;
	impl->out_len = olen;
	impl->out_buff = strdup(tmpbuff);
}

void plugin_NetworkNotify(gpointer data, gint fd, GdkInputCondition condition)
{
	//printf("plugin_NetworkNotify\n");

	if ((condition & GDK_INPUT_READ) &&
			(__stream_list.find(fd) != __stream_list.end()) )
		streamRead(__stream_list[fd], fd);

	if ((condition & GDK_INPUT_WRITE) &&
			(__stream_list.find(fd) != __stream_list.end()) )
		streamWrite(__stream_list[fd], fd);

}

NPError NPN_GetURLNotify(NPP instance, const char* url, const char* window, void* notifyData)
{
	if (window != NULL){
		printf("GetURL: %s %s\n", window, url);
		return NPERR_NO_ERROR;
	}
	assert(url);
	StreamImpl *impl = new StreamImpl();
	if (-1==getUrlHost(impl, url)){
		delete impl;
		return NPERR_GENERIC_ERROR;
	}
	printf("GetURLNotify: %s\n", url);
	int fd = impl->sin_file = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in so_addr;
	so_addr.sin_family = AF_INET;
	so_addr.sin_addr = impl->sin_addr;
	so_addr.sin_port = impl->sin_port;
	setblockopt(fd, false);
	if (-1==connect(fd, (sockaddr*)&so_addr, sizeof(sockaddr))){
		if (EINPROGRESS != WSAGetLastError()){
			closesocket(fd);
			delete impl;
			printf("NPN_GetURLNotify fail!\n");
			return NPERR_GENERIC_ERROR;
		}
	}
	gint tag = gdk_input_add(fd, (GdkInputCondition)(GDK_INPUT_WRITE|GDK_INPUT_READ), plugin_NetworkNotify, NULL);
	__stream_list_tag[fd] = tag;
	impl->in_off = 0;
	impl->in_len = 0;
	impl->in_ptr = NULL;
	impl->in_bptr = NULL;
	impl->referto = NULL;
	memset(impl->magic, 0, 32);
	GetOutBuff(impl, url, NULL);
	printf("NewStream Start1: fd:%d!\n", fd);
	NPStream *pStream = new NPStream();
	pStream->url = strdup(url);
	pStream->headers = NULL;
	pStream->end = 0;
	pStream->notifyData = notifyData;
	pStream->ndata = impl;
	pStream->lastmodified = time(NULL);
	__stream_list[fd] = pStream;
	printf("NewStream Start---: fd:%d!\n", fd);
	return NPERR_NO_ERROR;
}

void NPN_ReleaseObject(NPObject *obj)
{
	printf("NPN_ReleaseObject: %p\n", obj);
	assert(obj && obj->_class);
	obj->referenceCount--;
	if (obj->referenceCount > 0)
		return;
	obj->_class->deallocate(obj);
}

int PostOutBuff(StreamImpl *impl, const char *url, const char *data, size_t len)
{
	char *p;
	char buff[1024];
	char tmpbuff[409600];
	if (0!=strncmp(url, "http://", 7)){
		/*************************/
		printf("PostOutBuff Fail: %s\n", url);
		assert(0);
		return -1;
	}
	assert(strlen(url)<1024);
	strcpy(buff, url+7);
	if ((p=strchr(buff, '/')) != NULL)
		*p = 0;
	int olen = sprintf(tmpbuff, "POST /%s HTTP/1.0\r\n", p?p+1:"");
	olen += sprintf(tmpbuff+olen, "Host: %s\r\n", buff);
	olen += sprintf(tmpbuff+olen, "User-Agent: %s\r\n", NPN_UserAgent(NULL));
	olen += sprintf(tmpbuff+olen, "Accept: text/html,application/xml;q=0.9,*/*;q=0.8\r\n");
	olen += sprintf(tmpbuff+olen, "Connection: close\r\n");
	impl->out_off = 0;
	impl->out_len = (olen+len);
	impl->out_buff = new char[olen+len];
	memcpy(impl->out_buff, tmpbuff, olen);
	assert(data);
	memcpy(impl->out_buff+olen, data, len);
	return 0;
}

NPError NPN_PostURLNotify(NPP instance, const char* url, const char* target,
		uint32 len, const char* buf, NPBool file, void* notifyData)
{
	assert(url!=0);
	assert(file==0);
	if (target != NULL){
		printf("PostURL: %s %s\n", target, url);
		return NPERR_NO_ERROR;
	}
	StreamImpl *impl = new StreamImpl();
	//printf("data@%d(%s): %s /%s/\n", len, url, target?target:"nullTarget", buf);
	if (-1==getUrlHost(impl, url)){
		delete impl;
		return NPERR_GENERIC_ERROR;
	}
	printf("PostURLNotify: %s\n", url);
	int fd = impl->sin_file = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in so_addr;
	so_addr.sin_family = AF_INET;
	so_addr.sin_addr = impl->sin_addr;
	so_addr.sin_port = impl->sin_port;
	setblockopt(fd, false);
	if (-1==connect(fd, (sockaddr*)&so_addr, sizeof(sockaddr))){
		if (EINPROGRESS != WSAGetLastError()){
			closesocket(fd);
			delete impl;
			return NPERR_GENERIC_ERROR;
		}
	}
	gint tag = gdk_input_add(fd, (GdkInputCondition)(GDK_INPUT_WRITE|GDK_INPUT_READ), plugin_NetworkNotify, NULL);
	__stream_list_tag[fd] = tag;
	impl->in_off = 0;
	impl->in_len = 0;
	impl->in_ptr = NULL;
	impl->in_bptr = NULL;
	impl->referto = NULL;
	memset(impl->magic, 0, 32);
	PostOutBuff(impl, url, buf, len);
	printf("NewStream Start2: fd:%d!\n", fd);
	NPStream *pStream = new NPStream();
	pStream->url = strdup(url);
	pStream->headers = NULL;
	pStream->end = 0;
	pStream->notifyData = notifyData;
	pStream->ndata = impl;
	pStream->lastmodified = time(NULL);
	__stream_list[fd] = pStream;
	return NPERR_NO_ERROR;
}

int plugin_SetWindow(int width, int height)
{
	NPRect rect;

	rect.top = 1;
	rect.left = 1;
	rect.right = width;
	rect.bottom = height;

	__plugin_window.x = 0;
	__plugin_window.y = 0;
	__plugin_window.clipRect = rect;

	__plugin_window.width = width;
	__plugin_window.height = height;
	__plugin_window.type = NPWindowTypeWindow;

	printf("call Set window\n");
	static NPSetWindowCallbackStruct ws_info = {0};
	__plugin_window.ws_info = &ws_info;
	NPError err = __pluginfunc.setwindow(&__plugin, &__plugin_window);
	assert(err == NPERR_NO_ERROR);
	return err;
}

NPObject *NPN_CreateObject(NPP npp, NPClass *aClass)
{
	printf("NPN_CreateObject is called!\n");
	NPObject *obj = aClass->allocate(npp, aClass);
	if (obj != NULL){
		obj->_class = aClass;
		obj->referenceCount = 1;
	}
	return obj;
}

#define __forward_functions __netscapefunc

FORWARD_CALL(NPN_GetURL,geturl);
FORWARD_CALL(NPN_PostURL,posturl);
FORWARD_CALL(NPN_RequestRead,requestread);
FORWARD_CALL(NPN_NewStream,newstream);
FORWARD_CALL(NPN_Write,write);
FORWARD_CALL(NPN_DestroyStream,destroystream);
FORWARD_CALL(NPN_Status,status);
FORWARD_CALL(NPN_UserAgent,uagent);
FORWARD_CALL(NPN_MemAlloc,memalloc);
FORWARD_CALL(NPN_MemFree,memfree);
FORWARD_CALL(NPN_MemFlush,memflush);
FORWARD_CALL(NPN_ReloadPlugins,reloadplugins);
FORWARD_CALL(NPN_GetJavaEnv,getJavaEnv);
FORWARD_CALL(NPN_GetJavaPeer,getJavaPeer);
FORWARD_CALL(NPN_GetURLNotify,geturlnotify);
FORWARD_CALL(NPN_PostURLNotify,posturlnotify);
FORWARD_CALL(NPN_GetValue,getvalue);
FORWARD_CALL(NPN_SetValue,setvalue);
FORWARD_CALL(NPN_InvalidateRect,invalidaterect);
FORWARD_CALL(NPN_InvalidateRegion,invalidateregion);
FORWARD_CALL(NPN_ForceRedraw,forceredraw);
FORWARD_CALL(NPN_GetStringIdentifier,getstringidentifier);
FORWARD_CALL(NPN_GetStringIdentifiers,getstringidentifiers);
FORWARD_CALL(NPN_GetIntIdentifier,getintidentifier);
FORWARD_CALL(NPN_IdentifierIsString,identifierisstring);
FORWARD_CALL(NPN_UTF8FromIdentifier,utf8fromidentifier);
FORWARD_CALL(NPN_IntFromIdentifier,intfromidentifier);
FORWARD_CALL(NPN_CreateObject,createobject);
FORWARD_CALL(NPN_RetainObject,retainobject);
FORWARD_CALL(NPN_ReleaseObject,releaseobject);
FORWARD_CALL(NPN_Invoke,invoke);
FORWARD_CALL(NPN_InvokeDefault,invokeDefault);
FORWARD_CALL(NPN_GetProperty,getproperty);
FORWARD_CALL(NPN_SetProperty,setproperty);
FORWARD_CALL(NPN_RemoveProperty,removeproperty);
FORWARD_CALL(NPN_HasProperty,hasproperty);
FORWARD_CALL(NPN_HasMethod,hasmethod);
FORWARD_CALL(NPN_ReleaseVariantValue,releasevariantvalue);
FORWARD_CALL(NPN_SetException,setexception);
FORWARD_CALL(NPN_PushPopupsEnabledState,pushpopupsenabledstate);
FORWARD_CALL(NPN_PopPopupsEnabledState,poppopupsenabledstate);
FORWARD_CALL(NPN_Enumerate,enumerate);
FORWARD_CALL(NPN_PluginThreadAsyncCall,pluginthreadasynccall);
FORWARD_CALL(NPN_Construct,construct);

NPObject *NPN_RetainObject(NPObject *npobj)
{
	printf("NPN_RetainObject: %p %p\n", npobj, __window_object);
	npobj->referenceCount++;
	return npobj;
}

JRIEnv *NPN_GetJavaEnv()
{
	return NULL;
}

jref NPN_GetJavaPeer(NPP npp_)
{
	return NULL;
}

bool NPN_Evaluate(NPP npp, NPObject *npobj, NPString *stript, NPVariant *result)
{
	printf("__window_object: %p\n", __window_object);
	printf("NPN_Evaluate %p %p: %s\n", npp, npobj, stript->utf8characters);
	return false;
}

int plugin_Setup(HWND hWnd)
{
	NPError err;
	__netscape_hwnd = hWnd;
	memset(&__pluginfunc, 0, sizeof(__pluginfunc));
	__pluginfunc.size = 60;
#if 0
	NPError err = NP_GetEntryPoints(&__pluginfunc);
	assert(err == NPERR_NO_ERROR);
#endif

#define F(T,f) __netscapefunc.f = (T##UPP)f##T;
	F(NPN_GetURL,geturl);
	F(NPN_PostURL,posturl);
	F(NPN_RequestRead,requestread);
	F(NPN_NewStream,newstream);
	F(NPN_Write,write);
	F(NPN_DestroyStream,destroystream);
	F(NPN_Status,status);
	F(NPN_UserAgent,uagent);
	F(NPN_MemAlloc,memalloc);
	F(NPN_MemFree,memfree);
	F(NPN_MemFlush,memflush);
	F(NPN_ReloadPlugins,reloadplugins);
	F(NPN_GetJavaEnv,getJavaEnv);
	F(NPN_GetJavaPeer,getJavaPeer);
	F(NPN_GetURLNotify,geturlnotify);
	F(NPN_PostURLNotify,posturlnotify);
	F(NPN_GetValue,getvalue);
	F(NPN_InvalidateRect,invalidaterect);
	F(NPN_InvalidateRegion,invalidateregion);
	F(NPN_ForceRedraw,forceredraw);
	F(NPN_GetStringIdentifier,getstringidentifier);
	F(NPN_GetStringIdentifiers,getstringidentifiers);
	F(NPN_GetIntIdentifier,getintidentifier);
	F(NPN_IdentifierIsString,identifierisstring);
	F(NPN_UTF8FromIdentifier,utf8fromidentifier);
	F(NPN_IntFromIdentifier,intfromidentifier);
	F(NPN_CreateObject,createobject);
	F(NPN_ReleaseObject,releaseobject);
	F(NPN_Invoke,invoke);
	F(NPN_InvokeDefault,invokeDefault);
	F(NPN_GetProperty,getproperty);
	F(NPN_SetProperty,setproperty);
	F(NPN_RemoveProperty,removeproperty);
	F(NPN_HasProperty,hasproperty);
	F(NPN_HasMethod,hasmethod);
	F(NPN_ReleaseVariantValue,releasevariantvalue);
	F(NPN_SetException,setexception);
	F(NPN_PushPopupsEnabledState,pushpopupsenabledstate);
	F(NPN_PopPopupsEnabledState,poppopupsenabledstate);
	F(NPN_Enumerate,enumerate);
	F(NPN_PluginThreadAsyncCall,pluginthreadasynccall);
	F(NPN_Construct,construct);
#undef F

	__netscapefunc.size = sizeof(__netscapefunc);
	__netscapefunc.version = 0x13;
#define XF(f, n) __netscapefunc.n = f;
	XF(NPN_Invoke,invoke);
	XF(NPN_UserAgent,uagent);
	XF(NPN_GetValue,getvalue);
	XF(NPN_GetJavaEnv,getJavaEnv);
	XF(NPN_GetJavaPeer,getJavaPeer);
	XF(NPN_RetainObject,retainobject);
	XF(NPN_CreateObject,createobject);
	XF(NPN_PostURLNotify,posturlnotify);
	XF(NPN_ReleaseObject, releaseobject);
	XF(NPN_Evaluate,evaluate);
	XF(NPN_SetValue,setvalue);
	XF(NPN_GetStringIdentifier,getstringidentifier);
	XF(NPN_GetProperty,getproperty);
	XF(NPN_GetURLNotify,geturlnotify);
	XF(NPN_ReleaseVariantValue,releasevariantvalue);
	XF(NPN_PushPopupsEnabledState_fixup,pushpopupsenabledstate);
	XF(NPN_PopPopupsEnabledState_fixup,poppopupsenabledstate);
#undef XF

	err = NP_Initialize(&__netscapefunc, &__pluginfunc);
	assert(err == NPERR_NO_ERROR);
	return 0;
}

static int parse_argument(const char *url, char ***argn, char ***argv, char **wurl)
{
	int count = 0;
	static char *_argv[100], *_argn[100];
	const char *ctxp=url, *xp, *vp, *np;

	for (count=0; count<100; count++){
		xp = strchr(ctxp, '=');
		if (xp==NULL || xp==ctxp)
			break;
		for (vp=xp+1; vp[0]&&*vp!=' '; vp++){/**/};
		for (np=xp-1; np>=ctxp&&isalnum(*np); np--){/**/};
		_argn[count] = new char[xp-np];
		memcpy(_argn[count], np+1, xp-np-1);
		_argn[count][xp-np-1] = 0;
		_argv[count] = NULL;
		if (vp > xp+1){
			_argv[count] = new char[vp-xp];
			if (xp[1]=='\'' || xp[1]=='\"'){
				memcpy(_argv[count], xp+2, vp-xp-3);
				_argv[count][vp-xp-3] = 0;
			}else{
				memcpy(_argv[count], xp+1, vp-xp-1);
				_argv[count][vp-xp-1] = 0;
			}
		}
		if (strcmp(_argn[count], "SRC")==0)
			*wurl = _argv[count];
		else if (strcmp(_argn[count], "src")==0)
			*wurl = _argv[count];
		ctxp = xp>vp?xp:vp;
	}
	*argn = _argn;
	*argv = _argv;
	return count;
}

int plugin_New(HWND hPlugin, const char *url)
{
	NPRect rect;
	char  *wurl = NULL;
	char  **argn, **argv;
	uint16 stype = NP_NORMAL;
	int count = parse_argument(url, &argn, &argv, &wurl);
	memset(&__plugin, 0, sizeof(__plugin));
	__window_object = NPN_CreateObject(NULL, getWindowClass());
	__pluginfunc.newp("application/x-shockwave-flash",
			&__plugin, 1, count, argn, argv, NULL);
#if 0
	__pluginfunc.newp("application/x-shockwave-flash",
			&__plugin, 1, count, argn, argv, NULL);
#endif

	rect.top = 4;
	rect.left = 4;
	rect.right = 600;
	rect.bottom = 480;

	__plugin_window.window = (void*)hPlugin;
	__plugin_window.x = 8;
	__plugin_window.y = 8;
	__plugin_window.clipRect = rect;
	__plugin_window.width = 600;
	__plugin_window.height = 480;
	__plugin_window.type = NPWindowTypeWindow;

	printf("call Set window 00\n");
	static NPSetWindowCallbackStruct ws_info = {0};
	__plugin_window.ws_info = &ws_info;
	NPError err = __pluginfunc.setwindow(&__plugin, &__plugin_window);
	assert(err==NPERR_NO_ERROR);
	if (wurl != NULL)
		NPN_GetURLNotify(&__plugin, wurl, NULL, NULL);

#if 0
	uint16 stype0 = 1;
	size_t off = 0;
	char buf[163840];
	NPStream stream;
	FILE * fp = fopen("/home/pagxir/Downloads/bsb110300a_100816.swf", "rb");

	stream.url = "http://www.baidu.com";
	stream.end = 23605;
	stream.notifyData = 0;
	stream.lastmodified = 0;
	stream.headers = "HTTP/1.0 200 OK\r\n";
	printf("newstream\n");
	__pluginfunc.newstream(&__plugin,
			"application/x-shockwave-flash", &stream, TRUE, &stype0);

	while (feof(fp) == 0) {
		int count = fread(buf, 1, sizeof(buf), fp);
		printf("write stream\n");
		__pluginfunc.write(&__plugin, &stream, off, count, buf);
		off += count;
		break;
	}
	printf("destroystream\n");
	__pluginfunc.destroystream(&__plugin, &stream, NPRES_DONE);
	fclose(fp);
#endif
	return 0;
}

int plugin_Shutdown()
{
	__pluginfunc.destroy(&__plugin, NULL);

	return NP_Shutdown();
}

int plugin_Test(HWND hWnd)
{
	NPObject *obj;
	int retval = __pluginfunc.getvalue(&__plugin,
			NPPVpluginScriptableNPObject, &obj);
	if (retval == NPERR_NO_ERROR){
		NPVariant result;
		if (NPN_GetProperty(&__plugin, obj, __TextData_id, &result)){
			if (NPVARIANT_IS_STRING(result)){
				printf("UTF-8: %s\n", result.value.stringValue.utf8characters);
				//SetWindowText(hWnd, result.value.stringValue.utf8characters);
			}
			NPN_ReleaseVariantValue(&result);
		}
	}
	return 0;
}
