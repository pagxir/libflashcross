#include <npapi.h>
#include <npfunctions.h>

#ifndef HIBYTE
#define HIBYTE(x) ((((uint32_t)(x)) & 0xff00) >> 8)
#endif

NPNetscapeFuncs NPNFuncs;
NPError NPP_Initialize(void);

char *NPP_GetMIMEDescription();

char *NP_GetMIMEDescription()
{
	return NPP_GetMIMEDescription();
}

NPError NP_GetValue(void *future, NPPVariable variable, void *value)
{
	return NPP_GetValue((NPP_t *)future, variable, value);
}

NPError defProc()
{

	return 0;
}

NPError OSCALL
NP_Initialize(NPNetscapeFuncs *pFuncs
#ifdef XP_UNIX
		, NPPluginFuncs *pluginFuncs
#endif
		)
{
	if(pFuncs == NULL)
		return NPERR_INVALID_FUNCTABLE_ERROR;

	if(HIBYTE(pFuncs->version) > NP_VERSION_MAJOR)
		return NPERR_INCOMPATIBLE_VERSION_ERROR;

	if(pFuncs->size < sizeof(NPNetscapeFuncs)) {
		printf("plugin init call is %u %lu!\n",
				pFuncs->size, sizeof(NPNetscapeFuncs));
		return NPERR_INVALID_FUNCTABLE_ERROR;
	}

	NPNFuncs.size 		   = pFuncs->size;
	NPNFuncs.version		   = pFuncs->version;
	NPNFuncs.geturlnotify 	   = pFuncs->geturlnotify;
	NPNFuncs.geturl		   = pFuncs->geturl;
	NPNFuncs.posturlnotify	   = pFuncs->posturlnotify;
	NPNFuncs.posturl		   = pFuncs->posturl;
	NPNFuncs.requestread		   = pFuncs->requestread;
	NPNFuncs.newstream		   = pFuncs->newstream;
	NPNFuncs.write		   = pFuncs->write;
	NPNFuncs.destroystream	   = pFuncs->destroystream;
	NPNFuncs.status		   = pFuncs->status;
	NPNFuncs.uagent		   = pFuncs->uagent;
	NPNFuncs.memalloc		   = pFuncs->memalloc;
	NPNFuncs.memfree		   = pFuncs->memfree;
	NPNFuncs.memflush		   = pFuncs->memflush;
	NPNFuncs.reloadplugins	   = pFuncs->reloadplugins;
	NPNFuncs.getJavaEnv		   = pFuncs->getJavaEnv;
	NPNFuncs.getJavaPeer		   = pFuncs->getJavaPeer;
	NPNFuncs.getvalue		   = pFuncs->getvalue;
	NPNFuncs.setvalue		   = pFuncs->setvalue;
	NPNFuncs.invalidaterect	   = pFuncs->invalidaterect;
	NPNFuncs.invalidateregion	   = pFuncs->invalidateregion;
	NPNFuncs.forceredraw		   = pFuncs->forceredraw;
	NPNFuncs.getstringidentifier	   = pFuncs->getstringidentifier;
	NPNFuncs.getstringidentifiers    = pFuncs->getstringidentifiers;
	NPNFuncs.getintidentifier	   = pFuncs->getintidentifier;
	NPNFuncs.identifierisstring	   = pFuncs->identifierisstring;
	NPNFuncs.utf8fromidentifier	   = pFuncs->utf8fromidentifier;
	NPNFuncs.intfromidentifier	   = pFuncs->intfromidentifier;
	NPNFuncs.createobject 	   = pFuncs->createobject;
	NPNFuncs.retainobject 	   = pFuncs->retainobject;
	NPNFuncs.releaseobject	   = pFuncs->releaseobject;
	NPNFuncs.invoke		   = pFuncs->invoke;
	NPNFuncs.invokeDefault	   = pFuncs->invokeDefault;
	NPNFuncs.evaluate		   = pFuncs->evaluate;
	NPNFuncs.getproperty		   = pFuncs->getproperty;
	NPNFuncs.setproperty		   = pFuncs->setproperty;
	NPNFuncs.removeproperty	   = pFuncs->removeproperty;
	NPNFuncs.hasproperty		   = pFuncs->hasproperty;
	NPNFuncs.hasmethod		   = pFuncs->hasmethod;
	NPNFuncs.releasevariantvalue	   = pFuncs->releasevariantvalue;
	NPNFuncs.setexception 	   = pFuncs->setexception;

#ifdef XP_UNIX
	/*
	 * Set up the plugin function table that Netscape will use to
	 * call us.  Netscape needs to know about our version and size
	 * and have a UniversalProcPointer for every function we
	 * implement.
	 */
	pluginFuncs->version	  = (NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR;
	pluginFuncs->size	  = sizeof(NPPluginFuncs);
	pluginFuncs->newp	  = (NPP_New);
	pluginFuncs->destroy	  = (NPP_Destroy);
	pluginFuncs->setwindow  = (NPP_SetWindow);
	pluginFuncs->newstream  = (NPP_NewStream);
	pluginFuncs->destroystream = (NPP_DestroyStream);
	pluginFuncs->asfile	  = (NPP_StreamAsFile);
	pluginFuncs->writeready = (NPP_WriteReady);
	pluginFuncs->write	  = (NPP_Write);
	pluginFuncs->print	  = (NPP_Print);
	pluginFuncs->urlnotify  = (NPP_URLNotify);
	*(void **)&pluginFuncs->event	    = (void *)defProc;
	pluginFuncs->getvalue   = (NPP_GetValue);
#ifdef OJI
	//pluginFuncs->javaClass  = NPP_GetJavaClass();
#endif

	NPP_Initialize();
#endif

	return NPERR_NO_ERROR;
}

NPError OSCALL NP_Shutdown()
{
	return NPERR_NO_ERROR;
}

