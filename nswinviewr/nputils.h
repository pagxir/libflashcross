#ifndef _NPUTILS_H_
#define _NPUTILS_H_

extern int (__stdcall * WP_Shutdown)();
extern int (__stdcall * WP_GetEntryPoints)(NPPluginFuncs *);
extern int (__stdcall * WP_Initialize)(NPNetscapeFuncs *);

int NPPluginLoad(const char *path);
int NPPluginInitialize(HWND hWnd);
int NPPluginExec(const char *url);
int NPPluginShutdown(void);
int NPPluginFree(void);

extern HWND __netscape_hwnd;
extern NPObject *__window_object;
extern NPPluginFuncs __pluginfunc;
extern NPNetscapeFuncs __netscapefunc;

void NPN_PushPopupsEnabledState_fixup(NPP npp, NPBool enabled);
void NPN_PopPopupsEnabledState_fixup(NPP npp);

#endif

