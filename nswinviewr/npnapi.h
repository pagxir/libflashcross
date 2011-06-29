#ifndef _NPNAPI_H_
#define _NPNAPI_H_

int PluginShutdown(void);
int PluginInitialize(HWND hWnd);

int PluginExec(const char *url);
int plugin_NetworkNotify(int fd, int event);

extern int WINAPI (*WP_Shutdown)();
extern int WINAPI (*WP_GetEntryPoints)(NPPluginFuncs *);
extern int WINAPI (*WP_Initialize)(NPNetscapeFuncs *);

#endif
