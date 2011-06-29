#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <gtk/gtk.h>

#include "npapi.h"
#include "npruntime.h"
#include "npfunctions.h"

class CPlugin
{
	private:
		NPP m_pNPInstance;
		NPWindow *m_Window;
		NPBool m_bInitialized;

	private:
		GtkWidget *m_plug, *m_entry;

	public:
		CPlugin(NPP pNPInstance);
		~CPlugin();

		int16_t handleEvent(void *event);
		NPBool init(NPWindow *pNPWindow);
		NPBool isInitialized();
		void shut();

	public:
		char *dupText();
		static NPIdentifier sTextData_id;
		static NPIdentifier sCi1_id; /* Function */
		static NPIdentifier sEchoTest_id; /* Function */
};

NPObject *script_CreateObject(NPP instance);

#endif // __PLUGIN_H__
