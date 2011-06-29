#include <assert.h>

#include <ctype.h>
#define GTK_ENABLE_BROKEN
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "des.h"
#include "plugin.h"
#include "npfunctions.h"

NPIdentifier CPlugin::sCi1_id; /* Function */
NPIdentifier CPlugin::sTextData_id;
NPIdentifier CPlugin::sEchoTest_id; /* Function */

CPlugin::CPlugin(NPP pNPInstance) :
	m_pNPInstance(pNPInstance),
	m_bInitialized(FALSE)
{
	sCi1_id = NPN_GetStringIdentifier("ci1");
	sTextData_id = NPN_GetStringIdentifier("TextData");
	sEchoTest_id = NPN_GetStringIdentifier("EchoTest");
}

CPlugin::~CPlugin()
{
}

NPBool CPlugin::init(NPWindow *pNPWindow)
{
	if(pNPWindow == NULL)
		return FALSE;

	m_plug  = gtk_plug_new((GdkNativeWindow)(size_t)pNPWindow->window);
	m_entry = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(m_plug), m_entry);
	gtk_entry_set_visibility(GTK_ENTRY(m_entry), FALSE);
	gtk_widget_show(m_entry);
	gtk_widget_show(m_plug);
	gtk_widget_set_size_request(m_entry, pNPWindow->width, pNPWindow->height);

	m_Window = pNPWindow;

	m_bInitialized = TRUE;
	return TRUE;
}

char *CPlugin::dupText()
{
	char *p, *cipher;
	uint8_t input[8], d_output[80];
	uint8_t des_keys[] = {PLACE_DES3_KEYS_HERE};
	const char tpl[] = "3DES_2_000000000000000000000000000000_";
	const gchar *plain = gtk_entry_get_text(GTK_ENTRY(m_entry));

	if (plain != NULL && *plain != 0) {
		int i, len, cnt;
		des3_context ctx;
		uint8 *output = d_output;
		des3_set_3keys(&ctx, des_keys);
		fprintf(stderr, "gtk text: %s\n", plain);
		while (*plain != 0 && output < d_output + sizeof(d_output)) {
			for (i = 0; i < 8; i++) {
				input[i] = *plain;
				if (*plain)
					plain++;
			}

			fprintf(stderr, "input %s\n", input);
			des3_encrypt(&ctx, input, output);
			for (i = 0; i < 8; i++)
				fprintf(stderr, "%02X", output[i]);
			fprintf(stderr, "-%s\n", input);
			output += 8;
		}

		cnt = output - d_output;
		len = cnt * 2 + strlen(tpl) + 1;

		cipher = (char *)malloc(len);
		if (cipher == NULL)
			return strdup(tpl);

		strncpy(cipher, tpl, len);
		p = cipher + strlen(tpl);
		for (i = 0; i < cnt; i++) {
			sprintf(p, "%02X", d_output[i] & 0xff);
			p += 2;
		}

		return cipher;
	}

	return strdup(tpl);
}

void CPlugin::shut()
{
	m_bInitialized = FALSE;
}

NPBool CPlugin::isInitialized()
{
	return m_bInitialized;
}

int16_t CPlugin::handleEvent(void *event)
{
	return 0;
}
