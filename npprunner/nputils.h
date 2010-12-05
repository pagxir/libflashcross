#ifndef _NPUTILS_H_
#define _NPUTILS_H_

#if 0
extern int (* NP_Shutdown)();
extern int (* NP_GetEntryPoints)(NPPluginFuncs *);
extern int (* NP_Initialize)(NPNetscapeFuncs *);
#endif

typedef void * HWND;
int NPPluginLoad(const char *path);
int NPPluginInitialize(HWND hWnd);
int NPPluginExec(const char *url);
int NPPluginShutdown(void);
int NPPluginFree(void);

extern HWND __netscape_hwnd;
extern NPObject *__window_object;
extern NPPluginFuncs __pluginfunc;
extern NPNetscapeFuncs __netscapefunc;

bool NPN_PushPopupsEnabledState_fixup(NPP npp, NPBool enabled);
bool NPN_PopPopupsEnabledState_fixup(NPP npp);

#endif

