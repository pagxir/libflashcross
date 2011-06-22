/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

//////////////////////////////////////////////////
//
// CPlugin class implementation
//
#include <assert.h>

#include <ctype.h>
#define GTK_ENABLE_BROKEN
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "plugin.h"
#include "npupp.h"
#include "des.h"

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

NPBool CPlugin::init(NPWindow* pNPWindow)
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
	char * p, * cipher;
	uint8_t input[8], d_output[80];
	uint8_t des_keys[] = {PLACE_DES3_KEYS_HERE};
	const char tpl[] = "3DES_2_000000000000000000000000000000_";
	const gchar * plain = gtk_entry_get_text(GTK_ENTRY(m_entry));

	if (plain != NULL && *plain != 0) {
		int i, len, cnt;
		des3_context ctx;
		uint8 * output = d_output;
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

int16_t CPlugin::handleEvent(void* event)
{
	return 0;
}
