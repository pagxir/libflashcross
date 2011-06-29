#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <dlfcn.h>
#include <sys/mman.h>

#include "des.h"
#include "npapi.h"
#include "npfunctions.h"

static void setup_api_ptr();

static void des3_encrypt_fixup(des3_context *ctx,
	 uint8 input[8], uint8 output[8])
{
    printf("des3 enc:\n");
    exit(0);
}

static void des3_decrypt_fixup(des3_context *ctx,
	uint8 input[8], uint8 output[8])
{
    printf("des3 dec:\n");
    exit(0);
}

static int  des3_set_2keys_fixup(des3_context *ctx,
	uint8 key1[8], uint8 key2[8])
{
    printf("Hello World2!\n");
    exit(0);
    return 0;
}

static int  des3_set_3keys_fixup(des3_context *ctx,
	uint8 key1[8], uint8 key2[8], uint8 key3[8])
{
	int i;

    printf("Hello World3: !\n");
    for (i = 0; i < 8; i++)
		 printf("%02x", key1[i]);
    printf(" ");

    printf("Hello World3: !\n");
    for (i = 0; i < 8; i++)
		 printf("%02x", key2[i]);
    printf(" ");

    printf("Hello World3: !\n");
    for (i = 0; i < 8; i++)
		 printf("%02x", key3[i]);
    printf(" ");

    exit(0);
    return 0;
}

static void hook_api(void *orig, void *newp)
{
    uint8_t bytes[7];

    bytes[0] = 0xB8;
    memcpy(bytes + 1, &newp, 4);
    bytes[6] = 0xE0;
    bytes[5] = 0xFF;

    mprotect(orig, 7, PROT_WRITE|PROT_READ|PROT_EXEC);
    printf("orig: %p, newp: %p\n", orig, newp);
    memcpy(orig, bytes, 7);
}

static char * __getmimedescription()
{
    setup_api_ptr();
    return NP_GetMIMEDescription();
}

static NPError __getvalue(void* future, NPPVariable variable, void *value)
{
    setup_api_ptr();
    return NP_GetValue(future, variable, value);
}

static NPError __initialize(NPNetscapeFuncs* pFuncs ,
	 NPPluginFuncs* pluginFuncs)
{
    setup_api_ptr();
    return NP_Initialize(pFuncs,  pluginFuncs);
}

static NPError __shutdown()
{
    setup_api_ptr();
    return NP_Shutdown();
}

static NPError (* np_shutdown)() = __shutdown;
static char *  (* np_getMIMEDescription)() = __getmimedescription;
static NPError (* np_getValue)(void *future, NPPVariable variable, void *value) = __getvalue;
static NPError (* np_initialize)(NPNetscapeFuncs *pFuncs, NPPluginFuncs *pluginFuncs) = __initialize;

static void setup_api_ptr()
{
    void *symbol = NULL;
    static void *paliedit = NULL;

    paliedit = dlopen("libaliedit.so", RTLD_NOW|RTLD_GLOBAL);
    assert(paliedit != NULL);

    symbol = dlsym(paliedit, "NP_GetMIMEDescription");
    assert(symbol != NULL);
    memcpy(&np_getMIMEDescription, &symbol, 4);

    symbol = dlsym(paliedit, "NP_Shutdown");
    assert(symbol != NULL);
    memcpy(&np_shutdown, &symbol, 4);

    symbol = dlsym(paliedit, "NP_Initialize");
    assert(symbol != NULL);
    memcpy(&np_initialize, &symbol, 4);

    symbol = dlsym(paliedit, "NP_GetValue");
    assert(symbol != NULL);
    memcpy(&np_getValue, &symbol, 4);

    hook_api((void *)des3_set_2keys, (void *)des3_set_2keys_fixup);
    hook_api((void *)des3_set_3keys, (void *)des3_set_3keys_fixup);
    hook_api((void *)des3_encrypt, (void *)des3_encrypt_fixup);
    hook_api((void *)des3_decrypt, (void *)des3_decrypt_fixup);
}

char *
NP_GetMIMEDescription()
{
    return np_getMIMEDescription();
}

NPError
NP_GetValue(void* future, NPPVariable variable, void *value)
{
    return np_getValue(future, variable, value);
}

NPError
NP_Initialize(NPNetscapeFuncs* pFuncs , NPPluginFuncs* pluginFuncs)
{
    return np_initialize(pFuncs, pluginFuncs);
}

NPError NP_Shutdown()
{
    return np_shutdown();
}

