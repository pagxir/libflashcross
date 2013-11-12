#include "plugin.h"

char *NPP_GetMIMEDescription(void)
{
	static char MIMEDescription[] = {
		"application/aliedit::Aliedit"
	};
	return MIMEDescription;
}

NPError NPP_Initialize(void)
{
	return NPERR_NO_ERROR;
}

void NPP_Shutdown(void)
{

}

NPError NPP_New(NPMIMEType pluginType,
		NPP instance, uint16_t mode, int16_t argc,
		char *argn[], char *argv[], NPSavedData *saved)
{
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	NPError rv = NPERR_NO_ERROR;

	CPlugin *pPlugin = new CPlugin(instance);
	if (pPlugin == NULL)
		return NPERR_OUT_OF_MEMORY_ERROR;

	instance->pdata = (void *)pPlugin;
	return rv;
}

NPError NPP_Destroy (NPP instance, NPSavedData **save)
{
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	NPError rv = NPERR_NO_ERROR;

	CPlugin *pPlugin = (CPlugin *)instance->pdata;
	if (pPlugin != NULL) {
		instance->pdata=NULL;
		pPlugin->shut();
		delete pPlugin;
	}

	return rv;
}

NPError NPP_SetWindow (NPP instance, NPWindow *pNPWindow)
{
	if(instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	NPError rv = NPERR_NO_ERROR;

	if(pNPWindow == NULL)
		return NPERR_GENERIC_ERROR;

#if 0
	NPSetWindowCallbackStruct *cbs =
		(NPSetWindowCallbackStruct*)pNPWindow->ws_info;

	printf("%p\n", cbs);
	printf("type: %x\n", cbs->type);
	printf("display: %p\n", cbs->display);
	printf("visual: %p\n", cbs->visual);
	printf("depth: %x\n", cbs->depth);
	printf("wtype: %x\n", pNPWindow->type);
#endif

	CPlugin *pPlugin = (CPlugin *)instance->pdata;

	if (pPlugin == NULL)
		return NPERR_GENERIC_ERROR;


	if (!pPlugin->isInitialized() && (pNPWindow->window != NULL)) {
		if (!pPlugin->init(pNPWindow)) {
			delete pPlugin;
			pPlugin = NULL;
			instance->pdata = NULL;
			return NPERR_MODULE_LOAD_FAILED_ERROR;
		}
	}

	if ((pNPWindow->window == NULL) && pPlugin->isInitialized())
		return NPERR_NO_ERROR;

	if (pPlugin->isInitialized() && (pNPWindow->window != NULL))
		return NPERR_NO_ERROR;

	if ((pNPWindow->window == NULL) && !pPlugin->isInitialized())
		return NPERR_NO_ERROR;

	return rv;
}

static char __nameString[] = "Aliedit (www.alipay.com security control)";

NPError NPP_GetValue(NPP instance, NPPVariable variable, void *value)
{
	NPError rv = NPERR_NO_ERROR;

#if 0
	if(instance == NULL)
		return NPERR_GENERIC_ERROR;

	CPlugin * plugin = (CPlugin *)instance->pdata;
	if(plugin == NULL)
		return NPERR_GENERIC_ERROR;
#endif

	switch (variable) {
		case NPPVpluginNameString:
			*((const char **)value) = "Aliedit";
			break;

		case NPPVpluginDescriptionString:
			*((char **)value) = __nameString;
			break;

		case NPPVpluginNeedsXEmbed:
			*((bool *)value) = true;
			break;

		case NPPVpluginScriptableNPObject:
			rv = NPERR_INVALID_INSTANCE_ERROR;
			if (instance != NULL){
				*(NPObject **)value = script_CreateObject(instance);
				rv = NPERR_NO_ERROR;
			}
			break;

		default:
			rv = NPERR_GENERIC_ERROR;
	}

	return rv;
}

NPError NPP_NewStream(NPP instance,
		NPMIMEType type,
		NPStream *stream,
		NPBool seekable,
		uint16_t *stype)
{
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	CPlugin *pPlugin = (CPlugin *)instance->pdata;

#if 0
	if (stream&&stream->url&&pPlugin)
		pPlugin->addURL(stream->url);
#endif

	pPlugin = pPlugin;

	NPError rv = NPERR_NO_ERROR;
	return rv;
}

int32_t NPP_WriteReady (NPP instance, NPStream *stream)
{
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	int32_t rv = 0x0fffffff;
	return rv;
}

int32_t NPP_Write (NPP instance, NPStream *stream, int32_t offset, int32_t len, void *buffer)
{
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	int32_t rv = len;
	return rv;
}

NPError NPP_DestroyStream (NPP instance, NPStream *stream, NPError reason)
{
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	NPError rv = NPERR_NO_ERROR;
	return rv;
}

void NPP_StreamAsFile (NPP instance, NPStream *stream, const char *fname)
{
	if (instance == NULL)
		return;
}

void NPP_Print (NPP instance, NPPrint *printInfo)
{
	if (instance == NULL)
		return;
}

void NPP_URLNotify(NPP instance, const char *url, NPReason reason, void *notifyData)
{
	if (instance == NULL)
		return;
}

NPError NPP_SetValue(NPP instance, NPNVariable variable, void *value)
{
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	NPError rv = NPERR_NO_ERROR;
	return rv;
}

int16_t	NPP_HandleEvent(NPP instance, void *event)
{
	if(instance == NULL)
		return 0;

	int16_t rv = 0;
	CPlugin *pPlugin = (CPlugin *)instance->pdata;
	if (pPlugin)
		rv = pPlugin->handleEvent(event);

	return rv;
}

