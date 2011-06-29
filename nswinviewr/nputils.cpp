#include <windows.h>

#include <set>

#include "npupp.h"
#include "nputils.h"
#include "npp_script.h"

#define FORWARD_CALL(T,f) int f##T() { \
	printf("%s %s\n", #f, #T); \
	return 0; \
}

HWND __netscape_hwnd;
NPObject *__window_object;
NPPluginFuncs __pluginfunc;
NPNetscapeFuncs __netscapefunc;
std::set<NPP> __plugin_instance_list;

int (__stdcall * WP_Shutdown)();
int (__stdcall * WP_GetEntryPoints)(NPPluginFuncs *);
int (__stdcall * WP_Initialize)(NPNetscapeFuncs *);

#define __forward_functions __netscapefunc

FORWARD_CALL(NPN_GetURLProcPtr, geturl);
FORWARD_CALL(NPN_PostURLProcPtr, posturl);
FORWARD_CALL(NPN_RequestReadProcPtr, requestread);
FORWARD_CALL(NPN_NewStreamProcPtr, newstream);
FORWARD_CALL(NPN_WriteProcPtr, write);
FORWARD_CALL(NPN_DestroyStreamProcPtr, destroystream);
FORWARD_CALL(NPN_StatusProcPtr, status);
FORWARD_CALL(NPN_UserAgentProcPtr, uagent);
FORWARD_CALL(NPN_MemAllocProcPtr, memalloc);
FORWARD_CALL(NPN_MemFreeProcPtr, memfree);
FORWARD_CALL(NPN_MemFlushProcPtr, memflush);
FORWARD_CALL(NPN_ReloadPluginsProcPtr, reloadplugins);
FORWARD_CALL(NPN_GetJavaEnvProcPtr, getJavaEnv);
FORWARD_CALL(NPN_GetJavaPeerProcPtr, getJavaPeer);
FORWARD_CALL(NPN_GetURLNotifyProcPtr, geturlnotify);
FORWARD_CALL(NPN_PostURLNotifyProcPtr, posturlnotify);
FORWARD_CALL(NPN_GetValueProcPtr, getvalue);
FORWARD_CALL(NPN_SetValueProcPtr, setvalue);
FORWARD_CALL(NPN_InvalidateRectProcPtr, invalidaterect);
FORWARD_CALL(NPN_InvalidateRegionProcPtr, invalidateregion);
FORWARD_CALL(NPN_ForceRedrawProcPtr, forceredraw);
FORWARD_CALL(NPN_GetStringIdentifierProcPtr, getstringidentifier);
FORWARD_CALL(NPN_GetStringIdentifiersProcPtr, getstringidentifiers);
FORWARD_CALL(NPN_GetIntIdentifierProcPtr, getintidentifier);
FORWARD_CALL(NPN_IdentifierIsStringProcPtr, identifierisstring);
FORWARD_CALL(NPN_UTF8FromIdentifierProcPtr, utf8fromidentifier);
FORWARD_CALL(NPN_IntFromIdentifierProcPtr, intfromidentifier);
FORWARD_CALL(NPN_CreateObjectProcPtr, createobject);
FORWARD_CALL(NPN_RetainObjectProcPtr, retainobject);
FORWARD_CALL(NPN_ReleaseObjectProcPtr, releaseobject);
FORWARD_CALL(NPN_InvokeProcPtr, invoke);
FORWARD_CALL(NPN_InvokeDefaultProcPtr, invokeDefault);
FORWARD_CALL(NPN_GetPropertyProcPtr, getproperty);
FORWARD_CALL(NPN_SetPropertyProcPtr, setproperty);
FORWARD_CALL(NPN_RemovePropertyProcPtr, removeproperty);
FORWARD_CALL(NPN_HasPropertyProcPtr, hasproperty);
FORWARD_CALL(NPN_HasMethodProcPtr, hasmethod);
FORWARD_CALL(NPN_ReleaseVariantValueProcPtr, releasevariantvalue);
FORWARD_CALL(NPN_SetExceptionProcPtr, setexception);
FORWARD_CALL(NPN_PushPopupsEnabledStateProcPtr, pushpopupsenabledstate);
FORWARD_CALL(NPN_PopPopupsEnabledStateProcPtr, poppopupsenabledstate);
FORWARD_CALL(NPN_EnumerateProcPtr, enumerate);
FORWARD_CALL(NPN_PluginThreadAsyncCallProcPtr, pluginthreadasynccall);
FORWARD_CALL(NPN_ConstructProcPtr, construct);

int NPPluginInitialize(HWND hWnd)
{
	__netscape_hwnd = hWnd;
	memset(&__pluginfunc, 0, sizeof(__pluginfunc));
	__pluginfunc.size = 60;
	NPError err = WP_GetEntryPoints(&__pluginfunc);
	assert(err == NPERR_NO_ERROR);

#define F(T,f) __netscapefunc.f = (T)f##T;
	F(NPN_GetURLProcPtr, geturl);
	F(NPN_PostURLProcPtr, posturl);
	F(NPN_RequestReadProcPtr, requestread);
	F(NPN_NewStreamProcPtr, newstream);
	F(NPN_WriteProcPtr, write);
	F(NPN_DestroyStreamProcPtr, destroystream);
	F(NPN_StatusProcPtr, status);
	F(NPN_UserAgentProcPtr, uagent);
	F(NPN_MemAllocProcPtr, memalloc);
	F(NPN_MemFreeProcPtr, memfree);
	F(NPN_MemFlushProcPtr, memflush);
	F(NPN_ReloadPluginsProcPtr, reloadplugins);
	F(NPN_GetJavaEnvProcPtr, getJavaEnv);
	F(NPN_GetJavaPeerProcPtr, getJavaPeer);
	F(NPN_GetURLNotifyProcPtr, geturlnotify);
	F(NPN_PostURLNotifyProcPtr, posturlnotify);
	F(NPN_GetValueProcPtr, getvalue);
	F(NPN_SetValueProcPtr, setvalue);
	F(NPN_InvalidateRectProcPtr, invalidaterect);
	F(NPN_InvalidateRegionProcPtr, invalidateregion);
	F(NPN_ForceRedrawProcPtr, forceredraw);
	F(NPN_GetStringIdentifierProcPtr, getstringidentifier);
	F(NPN_GetStringIdentifiersProcPtr, getstringidentifiers);
	F(NPN_GetIntIdentifierProcPtr, getintidentifier);
	F(NPN_IdentifierIsStringProcPtr, identifierisstring);
	F(NPN_UTF8FromIdentifierProcPtr, utf8fromidentifier);
	F(NPN_IntFromIdentifierProcPtr, intfromidentifier);
	F(NPN_CreateObjectProcPtr, createobject);
	F(NPN_ReleaseObjectProcPtr, releaseobject);
	F(NPN_InvokeProcPtr, invoke);
	F(NPN_InvokeDefaultProcPtr, invokeDefault);
	F(NPN_GetPropertyProcPtr, getproperty);
	F(NPN_SetPropertyProcPtr, setproperty);
	F(NPN_RemovePropertyProcPtr, removeproperty);
	F(NPN_HasPropertyProcPtr, hasproperty);
	F(NPN_HasMethodProcPtr, hasmethod);
	F(NPN_ReleaseVariantValueProcPtr, releasevariantvalue);
	F(NPN_SetExceptionProcPtr, setexception);
	F(NPN_PushPopupsEnabledStateProcPtr, pushpopupsenabledstate);
	F(NPN_PopPopupsEnabledStateProcPtr, poppopupsenabledstate);
	F(NPN_EnumerateProcPtr, enumerate);
	F(NPN_PluginThreadAsyncCallProcPtr, pluginthreadasynccall);
	F(NPN_ConstructProcPtr, construct);
#undef F

	__netscapefunc.size = 0xb8;
	__netscapefunc.version = 0x13;
#define XF(f, n) __netscapefunc.n = f;
	XF(NPN_Invoke, invoke);
	XF(NPN_UserAgent, uagent);
	XF(NPN_GetValue, getvalue);
	XF(NPN_RetainObject, retainobject);
	XF(NPN_CreateObject, createobject);
	XF(NPN_PostURLNotify, posturlnotify);
	XF(NPN_ReleaseObject, releaseobject);
	XF(NPN_Evaluate, evaluate);

	__netscapefunc.memalloc = malloc;
	__netscapefunc.memfree = free;

	XF(NPN_GetStringIdentifier, getstringidentifier);
	XF(NPN_GetProperty, getproperty);
	XF(NPN_GetURLNotify, geturlnotify);
	XF(NPN_ReleaseVariantValue, releasevariantvalue);
	XF(NPN_PushPopupsEnabledState_fixup, pushpopupsenabledstate);
	XF(NPN_PopPopupsEnabledState_fixup, poppopupsenabledstate);
#undef XF

	__window_object = NPN_CreateObject(NULL, getWindowClass());
	err = WP_Initialize(&__netscapefunc);
	assert(err == NPERR_NO_ERROR);
	return 0;
}

static HMODULE g_hModule = NULL;

int NPPluginLoad(const char *path)
{
	FARPROC farproc = 0;

   	g_hModule = LoadLibrary("NPSWF32.DLL");

	if (g_hModule != NULL) {
		farproc = GetProcAddress(g_hModule, "NP_Shutdown");
	   	memcpy(&WP_Shutdown, &farproc, sizeof(WP_Shutdown));
		farproc = GetProcAddress(g_hModule, "NP_Initialize");
	   	memcpy(&WP_Initialize, &farproc, sizeof(WP_Initialize));
		farproc = GetProcAddress(g_hModule, "NP_GetEntryPoints");
	   	memcpy(&WP_GetEntryPoints, &farproc, sizeof(WP_GetEntryPoints));
	}

	if (WP_Shutdown == NULL || WP_Initialize == NULL ||
			WP_GetEntryPoints == NULL) {
		if (g_hModule != NULL)
			FreeLibrary(g_hModule);
		g_hModule = NULL;
		return -1;
	}

	return 0;
}

