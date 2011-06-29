#ifndef _NPNAPI_H_
#define _NPNAPI_H_

#include <npapi.h>
#include <npruntime.h>
#include <npfunctions.h>

int plugin_Setup(void);
int plugin_Test(NPP npp);
int plugin_Shutdown(NPP npp);

int plugin_New(int xid, const char *url, NPP npp);
int plugin_SetWindow(int width, int height);

#ifdef __GDK_H__
void plugin_NetworkNotify(gpointer data, gint fd, GdkInputCondition condition);
#endif

struct plugin {
	int refcnt;
	NPPluginFuncs methods;
};

struct plugin_object {
	struct plugin *plug;
	struct plugin_object *next;

	NPWindow window;
	NPObject *winref;
};

#define OBJ2WIN(obj)    (&(obj)->window)
#define OBJ2WINOBJ(obj) ((obj)->winref)
#define NPP2OBJ(npp)    ((struct plugin_object *)(npp)->ndata)
#define PLUG2FUNC(plug) (&(plug)->methods)

#endif

