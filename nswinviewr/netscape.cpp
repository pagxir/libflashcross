#include <windows.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <set>
#include <map>

#include "rpcvm.h"
#include "npupp.h"
#include "npnapi.h"
#include "plugin.h"
#include "nputils.h"
#include "npplugin.h"
#include "npp_window.h"
#include "npnetstream.h"

#define DEBUG_OUT 0

extern std::set<NPP> __plugin_instance_list;

char __top_id[] = {"top"};
char __location_id[] = {"location"};
char __toString_id[] = {"toString"};
char __TextData_id[] = {"TextData"};
char ___DoFSCommand_id[] = {"_DoFSCommand"};

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

static void dbg_alert(int line)
{
	char buffer[1024];
	sprintf(buffer, "crash at line __%d@%p__\n", line, line);
	MessageBox(NULL, buffer, "crash report", 0);
	exit(0);
}

static const char *__var(const NPVariant *variant, char *buffer)
{
	size_t count, len;
	switch(variant->type) {
		case NPVariantType_Void:
			sprintf(buffer, "void_var(%p)", variant);
			break;
		case NPVariantType_Null:
			sprintf(buffer, "null_var(%p)", variant);
			break;
		case NPVariantType_Bool:
			sprintf(buffer, "bool_var(%p=%x)", variant, variant->value.boolValue);
			break;
		case NPVariantType_Int32:
			sprintf(buffer, "int_var(%p=%x)", variant, variant->value.intValue);
			break;
		case NPVariantType_String:
			count = sprintf(buffer, "string_var(%p=%s)",
					variant, variant->value.stringValue.UTF8Characters);
#if 0
			len = variant->value.stringValue.UTF8Length;
			memcpy(buffer,  len);
			strcpy(buffer+count+len, "\n");
#endif
			break;
		case NPVariantType_Object:
			sprintf(buffer, "object_var(%p=%p)", variant,
					variant->value.objectValue);
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
	assert (obj && obj->_class);
	bool retval = obj->_class->invoke(obj, methodName, args, argCount, result);
	//printf("RET: %s\n", VAR(result));
	return retval;
}

const char *NPN_UserAgent(NPP instance)
{
	static char user_agent[] = "Mozilla/5.0 Gecko/2008122208 Firefox/3.0.5";
	return user_agent;
}

NPError NPN_GetValue(NPP instance, NPNVariable variable, void *ret_alue)
{
	NPError error = NPERR_GENERIC_ERROR;
	//printf("NPN_GetValue: %p\n", instance);
	if (variable == NPNVWindowNPObject){
		//fprintf(stderr, "Get NPNVWindowNPObject\n");
		*(NPObject**)ret_alue = __window_object;
		__window_object->referenceCount++;
		error = NPERR_NO_ERROR;
	}
	return error;
}

NPIdentifier NPN_GetStringIdentifier(const NPUTF8* name)
{
	NPIdentifier ident = (NPIdentifier)name;
	if (!strcmp(name, __location_id))
		ident = (NPIdentifier)__location_id;
	else if (!strcmp(__toString_id, name))
		ident = (NPIdentifier)__toString_id;
	else if (!strcmp(__top_id, name))
		ident = (NPIdentifier)__top_id;
	else if (!strcmp(___DoFSCommand_id, name))
		ident = (NPIdentifier)___DoFSCommand_id;
	else if (!strcmp(__TextData_id, name))
		ident = (NPIdentifier)__TextData_id;
	else
		fprintf(stderr, "NPN_GetStringIdentifier: %s!\n", name);
	//printf("NPN_GetStringIdentifier\n");
	return ident;
}

bool NPN_GetProperty(NPP npp, NPObject *obj,
		NPIdentifier propertyName, NPVariant *result)
{
	//printf("NPN_GetProperty: %p %s\n", obj, propertyName);
	assert(obj && obj->_class);
	bool retval =  obj->_class->getProperty(obj, propertyName, result);

#if 0
	char * buf = (char*)result;
	for (int i = 0; i < sizeof(NPVariant); i++)
		buf[i] = i;
	result->type = NPVariantType_Object;
#endif
	printf("RET: %s\n", VAR(result));
	return retval;
}

void NPN_ReleaseVariantValue(NPVariant *variant)
{
	assert (variant != NULL);

	switch (variant->type) {
		case NPVariantType_String:
			printf("RET: %s\n", VAR(variant));
			free((void*)variant->value.stringValue.UTF8Characters);
			variant->value.stringValue.UTF8Length = 0;
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
			dbg_alert(__LINE__);
			break;
	}
}

#include <stack>

static bool __popup_state = true;
static std::stack<bool> __popup_stack;

void NPN_PushPopupsEnabledState_fixup(NPP npp, NPBool enabled)
{
	__popup_stack.push(enabled);
	return;
}

void NPN_PopPopupsEnabledState_fixup(NPP npp)
{
	__popup_stack.top();
	return;
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
				window, url, WSAGetLastError());
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
				window, url, WSAGetLastError());
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

void NPN_ReleaseObject(NPObject *obj)
{
	//printf("NPN_ReleaseObject: %p\n", obj);
	assert(obj && obj->_class);
	obj->referenceCount--;
	if (obj->referenceCount > 0)
		return;
	obj->_class->deallocate(obj);
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

NPObject *NPN_CreateObject(NPP npp, NPClass *aClass)
{
	//printf("NPN_CreateObject is called!\n");
	NPObject *obj = aClass->allocate(npp, aClass);
	if (obj != NULL){
		obj->_class = aClass;
		obj->referenceCount = 1;
	}
	return obj;
}

NPObject *NPN_RetainObject(NPObject *npobj)
{
	//printf("NPN_RetainObject: %p %p\n", npobj, __window_object);
	npobj->referenceCount++;
	return npobj;
}

bool NPN_Evaluate(NPP npp, NPObject *npobj, NPString *stript, NPVariant *result)
{
	fprintf(stderr, "__window_object: %p\n", __window_object);
	//printf("NPN_Evaluate %p %p: %s\n", npp, npobj, stript->UTF8Characters);
	return false;
}

static int ParseVarList(char *var_list, char **argn, char **argv, size_t count)
{
	int total = 0;
	char *ctxp = var_list;
	char *xp, *vp, *np;

	while (count > 0) {
		xp = strchr(ctxp, '=');
		if (xp == NULL || xp == ctxp)
			break;

		for (vp = xp + 1; *vp && *vp != ' '; vp ++) { /**/ };
		for (np = xp - 1; np >= ctxp && isalnum(*np); np --) { /**/ };

		*argn = np + 1;
		*xp = 0;

		*argv = NULL;
		if (vp > xp + 1)
			*argv = (xp + 1);

		ctxp = *vp? (vp + 1): vp;
		*vp = 0;

		total ++, count --;
		argn ++, argv ++;
	}

	return total;
}

int NPPluginShutdown()
{
	std::set<NPP>::iterator iter;
	iter = __plugin_instance_list.begin();
	while (iter != __plugin_instance_list.end()) {
		__pluginfunc.destroy(*iter, NULL);
		++iter;
	}

	return WP_Shutdown();
}

int NPPluginExec(const char *var_list)
{
	char  *wurl = NULL;
	uint16_t stype = NP_NORMAL;
	char *svar_list = strdup(var_list);
	char *argn[100], *argv[100];
	int count = ParseVarList(svar_list, argn, argv, 100);

#ifdef VIEWER
	for (int i = 0; i < count; i++)
		if (stricmp("src", argn[i]) == 0)
			wurl = argv[i];

	fprintf(stderr, "count: %s\n", wurl);
	if (count == 0 || wurl == NULL) {
		free(svar_list);
		return 0;
	}

	fprintf(stderr, "URL: %s\n", wurl);
	NPPlugin * plugin = new NPPlugin(__netscape_hwnd);
	__plugin_instance_list.insert(plugin);
	memset(plugin, 0, sizeof(NPP_t));
	__pluginfunc.newp("application/x-shockwave-flash",
			plugin, 1, count, argn, argv, NULL);
	plugin->SetWindow();
	plugin->AttachWindow();
	NPN_GetURLNotify(plugin, wurl, NULL, NULL);
#endif
	free(svar_list);
	return 0;
}

void DoTest(NPP npp)
{
	NPError err;
	NPObject *obj;
	NPVariant result;

	err = __pluginfunc.getvalue(npp,
		NPPVpluginScriptableNPObject, &obj);

	if (err != NPERR_NO_ERROR)
		return;

	if (!NPN_GetProperty(npp, obj, __TextData_id, &result))
		return;

	if (NPVARIANT_IS_STRING(result)) {
		printf("Hello World 1: sizeof(%d)\n", sizeof(result));
	}

	NPN_ReleaseVariantValue(&result);
	return;
}

