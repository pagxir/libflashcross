#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <time.h>
#include <ctype.h>

#include <map>

#include "npnapi.h"
#include "plugin.h"
#include "npp_script.h"

#ifndef ENAVLE_DEBUG
#define dbg_trace(fmt, args...) (void *)0
#else
#define dbg_trace(fmt, args...) fprintf(stderr, fmt, ##args)
#endif

static struct plugin __plugin;

char __top_id[] = {"top"};
char __location_id[] = {"location"};
char __toString_id[] = {"toString"};
char __TextData_id[] = {"TextData"};
char ___DoFSCommand_id[] = {"_DoFSCommand"};

static const char *__var(const NPVariant *variant, char *buffer)
{
	size_t count;
	switch(variant->type) {
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
					variant->value.stringValue.UTF8Characters);
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

const char *VAR_X(const NPVariant *variant)
{
	char buffer[10240];
	if (variant != NULL)
		return __var(variant, buffer);
	return "var(NULL)";
}

bool NPN_Invoke(NPP npp, NPObject* obj, NPIdentifier methodName,
		const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	dbg_trace("NPN_Invoke: %p %s %ld\n", obj, (char *)methodName, sizeof(NPVariant));
	assert (obj && obj->_class);
	assert (obj->_class->invoke);
	bool retval = obj->_class->invoke(obj, methodName, args, argCount, result);
	dbg_trace("RET: %s\n", VAR(result));
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
	dbg_trace("NPN_SetValue: %d\n", variable);
	return error;
}

NPError NPN_GetValue(NPP instance, NPNVariable variable, void *ret_alue)
{
	struct plugin_object *object;
	NPError error = NPERR_GENERIC_ERROR;
	assert(ret_alue != NULL);

	object = instance? NPP2OBJ(instance): NULL;

	if (variable == NPNVWindowNPObject){
		NPObject *obj =  OBJ2WINOBJ(object);
		obj->referenceCount++;
		*(NPObject**)ret_alue = obj;
		error = NPERR_NO_ERROR;
	}else if (variable == NPNVnetscapeWindow){
		*(void**)ret_alue = OBJ2WIN(object);
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
	}else{
		dbg_trace("NPN_GetValue: %p 0x%x %d\n", instance, variable, variable);
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
		dbg_trace("NPN_GetStringIdentifier: %s!\n", name);
	return ident;
}

bool NPN_GetProperty(NPP npp, NPObject *obj, NPIdentifier propertyName, NPVariant *result)
{
	dbg_trace("NPN_GetProperty: %p %s %ld\n", obj, (char *)propertyName, sizeof(NPVariant));
	assert(obj && obj->_class);
	assert(obj->_class->getProperty);
	bool retval =  obj->_class->getProperty(obj, propertyName, result);
	dbg_trace("RET: %s\n", VAR(result));
	return retval;
}

void NPN_ReleaseVariantValue(NPVariant *variant)
{
	if (variant != NULL)
		switch (variant->type){
			case NPVariantType_String:
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
				break;
		}
}

#include <stack>

static std::stack<bool> __popup_stack;

void NPN_PushPopupsEnabledState(NPP npp, NPBool enabled)
{
	__popup_stack.push(enabled);
	return ;
}

void NPN_PopPopupsEnabledState(NPP npp)
{
	__popup_stack.pop();
	return;
}

void NPN_ReleaseObject(NPObject *obj)
{
	dbg_trace("NPN_ReleaseObject: %p\n", obj);
	assert(obj && obj->_class);
	obj->referenceCount--;
	if (obj->referenceCount > 0)
		return;
	obj->_class->deallocate(obj);
}

#if 0
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

	dbg_trace("call Set window\n");
	static NPSetWindowCallbackStruct ws_info = {0};
	__plugin_window.ws_info = &ws_info;
	NPError err = __pluginfunc.setwindow(&__plugin, &__plugin_window);
	assert(err == NPERR_NO_ERROR);
	return err;
}
#endif

NPObject *NPN_CreateObject(NPP npp, NPClass *aClass)
{
	dbg_trace("NPN_CreateObject is called!\n");
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
	dbg_trace("NPN_RetainObject: %p\n", npobj);
	npobj->referenceCount++;
	return npobj;
}

void *NPN_GetJavaEnv()
{
	return NULL;
}

void *NPN_GetJavaPeer(NPP npp_)
{
	return NULL;
}

bool NPN_Evaluate(NPP npp, NPObject *npobj, NPString *stript, NPVariant *result)
{
	dbg_trace("NPN_Evaluate %p %p: %s\n", npp, npobj, stript->UTF8Characters);
	return false;
}

int plugin_Setup(void)
{
	NPError err;
	static NPNetscapeFuncs __netscapefunc;
	__netscapefunc.version = 0x13;
	__netscapefunc.size = sizeof(__netscapefunc);

#define F(T,f) __netscapefunc.f = (T##ProcPtr)f##T;
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

#define XF(f, n) __netscapefunc.n = (f##ProcPtr)f;
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
	XF(NPN_PushPopupsEnabledState,pushpopupsenabledstate);
	XF(NPN_PopPopupsEnabledState,poppopupsenabledstate);
#undef XF

	memset(&__plugin, 0, sizeof(__plugin));
	PLUG2FUNC(&__plugin)->size = sizeof(NPPluginFuncs);

#if 0
	err = NP_GetEntryPoints(PLUG2FUNC(&__plugin));
	assert(err == NPERR_NO_ERROR);
#endif

	err = NP_Initialize(&__netscapefunc, PLUG2FUNC(&__plugin));
	assert(err == NPERR_NO_ERROR);

	dbg_trace("NP_Initialize: %d\n", err);
	return err;
}

static int parse_argument(const char *url, char ***argn, char ***argv, char **wurl)
{
	int count = 0;
	static char *_argv[100], *_argn[100];
	const char *ctxp=url, *xp, *vp, *np;

	static char name[] = "allowFullScreen";
	static char value[] = "true";

	_argn[count] = name;
	_argv[count++] = value;

	for (; count<100; count++){
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

int plugin_New(int xid, const char *url, NPP npp)
{
	int count;
	char *wurl = NULL;
	char **argn, **argv;
	char mime_type[] = {"application/x-shockwave-flash"};

	NPRect rect;
	NPError error;
	struct plugin_object *object;
	NPPluginFuncs *pluginfuncs = PLUG2FUNC(&__plugin);
	static NPSetWindowCallbackStruct ws_info = {0};

	object = (struct plugin_object *)malloc(sizeof(*object));
	memset(object, 0, sizeof(*object));
	object->plug = &__plugin;
	npp->ndata = object;
	npp->pdata = 0;

	count = parse_argument(url, &argn, &argv, &wurl);
	object->winref = NPN_CreateObject(npp, getWindowClass());
	error = pluginfuncs->newp(mime_type, npp, 1, count, argn, argv, NULL);
	dbg_trace("plugin_newp %d\n", error);
	__plugin.refcnt++;

	rect.top = 4;
	rect.left = 4;
	rect.right = 600;
	rect.bottom = 480;

	object->window.window = (void *)xid;
	object->window.x = 0;
	object->window.y = 0;
	object->window.clipRect = rect;
	object->window.width = rect.right;
	object->window.height = rect.bottom;
	object->window.type = NPWindowTypeWindow;
	object->window.ws_info = &ws_info;

	error = pluginfuncs->setwindow(npp, OBJ2WIN(object));
	assert(error == NPERR_NO_ERROR);

	dbg_trace("plugin_new %d\n", error);

	if (wurl != NULL) {
		NPN_GetURLNotify(npp, wurl, NULL, NULL);
		return 0;
	}

	return 0;
}

int plugin_Shutdown(NPP npp)
{
	NPPluginFuncs *pluginfuncs;
	struct plugin_object *object;

	object = NPP2OBJ(npp);
	pluginfuncs = PLUG2FUNC(object->plug);

	pluginfuncs->destroy(npp, NULL);
	free(object);

	return NP_Shutdown();
}

int plugin_Test(NPP npp)
{
	NPError err;
	NPObject *obj;
	NPVariant result;
	NPPluginFuncs *pluginfuncs;
	struct plugin_object *object;

	object = NPP2OBJ(npp);
	pluginfuncs = PLUG2FUNC(object->plug);

	err = pluginfuncs->getvalue(npp, NPPVpluginScriptableNPObject, &obj);
	if (err == NPERR_NO_ERROR) {
		if (NPN_GetProperty(npp, obj, __TextData_id, &result)) {
			if (NPVARIANT_IS_STRING(result)) {
				dbg_trace("UTF-8: %s\n", result.value.stringValue.UTF8Characters);
				//SetWindowText(hWnd, result.value.stringValue.utf8characters);
			}

			NPN_ReleaseVariantValue(&result);
		}
	}

	return 0;
}

