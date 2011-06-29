#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <ctype.h>
#include <assert.h>

#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <map>
#include <gdk/gdk.h>

#include "npnapi.h"
#include "plugin.h"
#include "npn_stream.h"
#include "npp_script.h"

static const char *get_schema(const char *url, char *schema, size_t len)
{
	if (strncmp(url, "http://", 7) == 0) {
		strncpy(schema, "http://", len);
		return (url + 7);
	}

	fprintf(stderr, "unkown schema: %s\n", url);
	return url;
}

const char *get_hostname(const char *url, char *hostname, size_t len)
{
	while (*url) {
		switch (*url) {
			case '/':
				if (len > 0)
					*hostname = 0;
				return url;

			case ':':
				if (len > 0)
					*hostname = 0;
				return url;

			default:
				if (len > 1) {
					*hostname++ = *url;
					len--;
				}
				break;
		}

		url++;
	}

	if (len > 0)
		*hostname = 0;

	return url;
}

const char *get_porttext(const char *url, char *porttext, size_t len)
{
	if (*url != ':') {
		strncpy(porttext, "", len);
		return url;
	}

	while (*url) {
		switch (*url) {
			case '/':
				if (len > 0)
					*porttext = 0;
				return url;

			default:
				if (len > 1) {
					*porttext ++ = *url;
					len--;
				}
				break;
		}

		url++;
	}

	if (len > 0)
		*porttext = 0;

	return url;
}

const char *get_url_path(const char *url, char *url_path, size_t len)
{
	int c = len;

	if (*url != '/') {
		strncpy(url_path, "/", len);
		return url;
	}

	while ((c-- > 0) && (*url_path++ = *url++));
	return url;
}

static in_addr inaddr_convert(const char *strp)
{
	struct in_addr addr;
	struct hostent *hostp;

	const char *testp = strp;
	while (*testp) {
		char ch = *testp++;

		if (ch == '.' ||
				'0' <= ch && ch <= '9')
			continue;

		addr.s_addr = INADDR_NONE;
		hostp = gethostbyname(strp);
		if (hostp != NULL) {
			//printf("hostname: %s\n", strp);
			memcpy(&addr, hostp->h_addr, sizeof(addr));
		}

		return addr;
	}

	addr.s_addr = inet_addr(strp);
	return addr;
}


NPError NPN_GetURLNotify(NPP instance, const char* url,
		const char* window, void* notifyData)
{
	char schema[8];
	char porttext[8];
	char hostname[256];
	char url_path[512];
	const char *partial_url;

	if (window != NULL){
		fprintf(stderr, "GetURLNotify: %s %s\n", window, url);
		return NPERR_NO_ERROR;
	}

	partial_url = get_schema(url, schema, sizeof(schema));
	partial_url = get_hostname(partial_url, hostname, sizeof(hostname));
	partial_url = get_porttext(partial_url, porttext, sizeof(porttext));
	partial_url = get_url_path(partial_url, url_path, sizeof(url_path));

	if (strncmp(schema, "http://", 7)) {
		fprintf(stderr, "GetURLNotify failure: %s %s %d\n",
				window, url, errno);
		return NPERR_GENERIC_ERROR;
	}

	char *foop;
	char headers[8192];
	NPNetStream *stream;
	proto_stream *protop;
	struct sockaddr_in name;

	name.sin_family = AF_INET;
	name.sin_port   = htons(*porttext? atoi(porttext + 1): 80);
	name.sin_addr   = inaddr_convert(hostname);
	//fprintf(stderr, "%s:%s\n", inet_ntoa(name.sin_addr), porttext);

	protop = new proto_stream();
	assert(protop != NULL);

	if (protop->connect(&name, sizeof(name))) {
		fprintf(stderr, "GetURLNotify connect: %s %s %d\n",
				window, url, errno);
		protop->rel();
		return NPERR_GENERIC_ERROR;
	}

	foop = headers;
	foop += sprintf(foop, "GET %s HTTP/1.0\r\n", url_path);
	foop += sprintf(foop, "Host: %s%s\r\n", hostname, porttext);
	foop += sprintf(foop, "User-Agent: %s\r\n", NPN_UserAgent(0));
	foop += sprintf(foop, "Accept: application/xml;q=0.9,*/*,q=0.8\r\n");
	foop += sprintf(foop, "Connection: close\r\n");
	foop += sprintf(foop, "\r\n");

	stream = new NPNetStream(instance, notifyData, url, protop);
	assert(stream != NULL);

	protop->set_request(headers);
	stream->startup();
	protop->rel();
	return NPERR_NO_ERROR;
}

proto_stream *RedirectURLNotify(const char *url, const char *refer)
{
	char schema[8];
	char porttext[8];
	char hostname[256];
	char url_path[512];
	const char *partial_url;

	partial_url = get_schema(url, schema, sizeof(schema));
	partial_url = get_hostname(partial_url, hostname, sizeof(hostname));
	partial_url = get_porttext(partial_url, porttext, sizeof(porttext));
	partial_url = get_url_path(partial_url, url_path, sizeof(url_path));

	if (strncmp(schema, "http://", 7)) {
		return NULL;
	}

	char *foop;
	char headers[8192];
	proto_stream *protop;
	struct sockaddr_in name;

	name.sin_family = AF_INET;
	name.sin_port   = htons(*porttext? atoi(porttext + 1): 80);
	name.sin_addr   = inaddr_convert(hostname);

	protop = new proto_stream();
	assert(protop != NULL);

	if (protop->connect(&name, sizeof(name))) {
		protop->rel();
		return NULL;
	}

	foop = headers;
	foop += sprintf(foop, "GET %s HTTP/1.0\r\n", url_path);
	foop += sprintf(foop, "Host: %s%s\r\n", hostname, porttext);
	foop += sprintf(foop, "User-Agent: %s\r\n", NPN_UserAgent(0));
	foop += sprintf(foop, "Accept: application/xml;q=0.9,*/*,q=0.8\r\n");
	foop += sprintf(foop, "Refer: %s\r\n", refer);
	foop += sprintf(foop, "Connection: close\r\n");
	foop += sprintf(foop, "\r\n");

	protop->set_request(headers);
	return protop;
}

NPError NPN_PostURLNotify(NPP instance, const char* url, const char* target,
		uint32_t len, const char* buf, NPBool file, void* notifyData)
{
	char schema[8];
	char porttext[8];
	char hostname[256];
	char url_path[512];
	const char *partial_url;

	if (target != NULL){
		fprintf(stderr, "GetURLNotify: %s %s\n", target, url);
		return NPERR_NO_ERROR;
	}

	partial_url = get_schema(url, schema, sizeof(schema));
	partial_url = get_hostname(partial_url, hostname, sizeof(hostname));
	partial_url = get_porttext(partial_url, porttext, sizeof(porttext));
	partial_url = get_url_path(partial_url, url_path, sizeof(url_path));

	if (strncmp(schema, "http://", 7)) {
		return NPERR_GENERIC_ERROR;
	}

	char *foop;
	char headers[8192];
	NPNetStream *stream;
	proto_stream *protop;
	struct sockaddr_in name;

	name.sin_family = AF_INET;
	name.sin_port   = htons(*porttext? atoi(porttext + 1): 80);
	name.sin_addr   = inaddr_convert(hostname);

	protop = new proto_stream();
	assert(protop != NULL);

	if (protop->connect(&name, sizeof(name))) {
		protop->rel();
		return NPERR_GENERIC_ERROR;
	}

	foop = headers;
	foop += sprintf(foop, "POST %s HTTP/1.0\r\n", url_path);
	foop += sprintf(foop, "Host: %s%s\r\n", hostname, porttext);
	foop += sprintf(foop, "User-Agent: %s\r\n", NPN_UserAgent(0));
	foop += sprintf(foop, "Accept: application/xml;q=0.9,*/*,q=0.8\r\n");
	foop += sprintf(foop, "Connection: close\r\n");

	stream = new NPNetStream(instance, notifyData, url, protop);
	assert(stream != NULL);

	protop->set_request(headers, buf, len);
	stream->startup();
	protop->rel();
	return NPERR_NO_ERROR;
}

