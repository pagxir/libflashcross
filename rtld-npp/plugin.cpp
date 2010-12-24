#include "npapi.h"
#include "npupp.h"
#include <assert.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "rtld.h"
#define MOZILLA_API extern "C"

static char plugin_path[512];
static const Obj_Entry * m_plugin = 0;

static NPError (* np_shutdown)( );
static char *  (* np_getMIMEDescription)( );
static NPError (* np_getValue)(void * future, NPPVariable variable, void * value);
static NPError (* np_initialize)(NPNetscapeFuncs* pFuncs , NPPluginFuncs* pluginFuncs);

static void
setup_api_ptr(const Obj_Entry * obj)
{
	const void *symbol = NULL;
	symbol = elf_dlsym(obj, "NP_GetMIMEDescription");
	assert(symbol != NULL);
	memcpy(&np_getMIMEDescription, &symbol, sizeof(symbol));
	symbol = elf_dlsym(obj, "NP_Shutdown");
	assert(symbol != NULL);
	memcpy(&np_shutdown, &symbol, sizeof(symbol));
	symbol = elf_dlsym(obj, "NP_Initialize");
	assert(symbol != NULL);
	memcpy(&np_initialize, &symbol, sizeof(symbol));
	symbol = elf_dlsym(obj, "NP_GetValue");
	assert(symbol != NULL);
	memcpy(&np_getValue, &symbol, sizeof(symbol));

	fprintf(stderr, "MimeDescription: %s\n", np_getMIMEDescription());
	const char * name = NULL;
	np_getValue(NULL, NPPVpluginNameString, &name);
	fprintf(stderr, "NameString: %s\n", name);
	np_getValue(NULL, NPPVpluginDescriptionString, &name);
	fprintf(stderr, "DescriptionString: %s\n", name);
}

MOZILLA_API char *
NP_GetMIMEDescription()
{
	if (m_plugin == NULL)
		return "";
	assert(np_getMIMEDescription);
	return np_getMIMEDescription();
}

MOZILLA_API NPError
NP_GetValue(void* future, NPPVariable variable, void *value)
{
	if (m_plugin == NULL)
		return NPERR_MODULE_LOAD_FAILED_ERROR;
	assert(np_getValue);
	return np_getValue(future, variable, value);
}

MOZILLA_API NPError
NP_Initialize(NPNetscapeFuncs* pFuncs , NPPluginFuncs* pluginFuncs)
{
	if (m_plugin == NULL) {
		m_plugin = elf_dlopen(plugin_path);
		setup_api_ptr(m_plugin);
	}

	if (m_plugin == NULL)
		return NPERR_MODULE_LOAD_FAILED_ERROR;

	assert(np_initialize);
	return np_initialize(pFuncs, pluginFuncs);
}

MOZILLA_API NPError
NP_Shutdown()
{
	if (m_plugin == NULL)
		return NPERR_MODULE_LOAD_FAILED_ERROR;
	assert(np_shutdown);
	int error = np_shutdown();
	if (m_plugin != NULL)
		elf_dlclose(m_plugin);
	m_plugin = 0;
	return error;
}

class PluginInitFini
{
	public:
		PluginInitFini();
		~PluginInitFini();
};

PluginInitFini::PluginInitFini()
{
	size_t len;
	const char * subpath;
	const char * homedir = getenv("HOME");
	assert(homedir != NULL);

	strncpy(plugin_path, homedir, sizeof(plugin_path));
	plugin_path[sizeof(plugin_path) - 1] = 0;

	len = strlen(plugin_path);
	assert(len > 0);

	subpath = ".mozilla/libflashplayer.so";
	if (plugin_path[len - 1] != '/')
		subpath = "/.mozilla/libflashplayer.so";

	strlcat(plugin_path, subpath, sizeof(plugin_path));
	fprintf(stderr, "load flash player: %s\n", plugin_path);
	m_plugin = elf_dlopen(plugin_path);
	setup_api_ptr(m_plugin);
}

PluginInitFini::~PluginInitFini()
{
	if (m_plugin != NULL)
		elf_dlclose(m_plugin);
}

static PluginInitFini __plugin;