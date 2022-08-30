/*  Copyright 2022 Pretendo Network contributors <pretendo.network>
    Copyright 2022 Ash Logan <ash@heyquark.com>
    Copyright 2019 Maschell

    Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
    granted, provided that the above copyright notice and this permission notice appear in all copies.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
    INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
    IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
    PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <wups.h>
#include <nsysnet/nssl.h>
#include <coreinit/cache.h>
#include <coreinit/dynload.h>
#include <coreinit/memory.h>
#include <coreinit/memorymap.h>
#include <coreinit/memexpheap.h>
#include <utils/logger.h>

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("Inkay");
WUPS_PLUGIN_DESCRIPTION("In-game patches for Juxtaposition");
WUPS_PLUGIN_VERSION("v0.1");
WUPS_PLUGIN_AUTHOR("Pretendo contributors");
WUPS_PLUGIN_LICENSE("ISC");

#include <kernel/kernel.h>

//#ifdef OLD_WUPS
extern "C" {
OSDynLoad_Error
OSDynLoad_IsModuleLoaded(char const *name,
                         OSDynLoad_Module *outModule);
}
//#endif

const char original_url[] = "discovery.olv.nintendo.net/v1/endpoint";
const char new_url[] =      "discovery.olv.pretendo.cc/v1/endpoint";
_Static_assert(sizeof(original_url) > sizeof(new_url),
               "new_url too long! Must be less than 38chars.");

// We'll keep a handle to nn_olv, just to ensure it doesn't get unloaded
static OSDynLoad_Module olv_handle;

#ifndef OLD_WUPS
INITIALIZE_PLUGIN() {
    WHBLogUdpInit();
}
DEINITIALIZE_PLUGIN() {
    WHBLogUdpDeinit();
}
#endif

bool checkForOlvLibs() {
    OSDynLoad_Module olv_handle = 0;
    OSDynLoad_Error dret;

    dret = OSDynLoad_IsModuleLoaded("nn_olv", &olv_handle);
    if (dret == OS_DYNLOAD_OK && olv_handle != 0) {
        return true;
    }

    dret = OSDynLoad_IsModuleLoaded("nn_olv2", &olv_handle);
    if (dret == OS_DYNLOAD_OK && olv_handle != 0) {
        return true;
    }

    return false;
}

bool replace(uint32_t start, uint32_t size, const char* original_val, size_t original_val_sz, const char* new_val, size_t new_val_sz) {
    for (uint32_t addr = start; addr < start + size - original_val_sz; addr++) {
        int ret = memcmp(original_val, (void*)addr, original_val_sz);
        if (ret == 0) {
            DEBUG_FUNCTION_LINE("found str @%08x: %s", addr, (const char*)addr);
            KernelCopyData(OSEffectiveToPhysical(addr), OSEffectiveToPhysical((uint32_t)new_val), new_val_sz);
            DEBUG_FUNCTION_LINE("new str   @%08x: %s", addr, (const char*)addr);
            return true;
        }
    }

    return false;
}

ON_APPLICATION_START() {
    WHBLogUdpInit();

    DEBUG_FUNCTION_LINE("Inkay: hewwo!\n");

    auto olvLoaded = checkForOlvLibs();

    if (!olvLoaded) {
        DEBUG_FUNCTION_LINE("Inkay: no olv, quitting for now\n");
        return;
    }

    OSDynLoad_Acquire("nn_olv", &olv_handle);
    DEBUG_FUNCTION_LINE("Inkay: olv! %08x\n", olv_handle);

    //wish there was a better way than "blow through MEM2"
    uint32_t base_addr, size;
    if(OSGetMemBound(OS_MEM2, &base_addr, &size)) {
        DEBUG_FUNCTION_LINE("Inkay: OSGetMemBound failed!");
        return;
    }

    replace(base_addr, size, original_url, sizeof(original_url), new_url, sizeof(new_url));
}

ON_APPLICATION_ENDS() {
    DEBUG_FUNCTION_LINE("Inkay: shutting down...\n");
    OSDynLoad_Release(olv_handle);
}

DECL_FUNCTION(NSSLContextHandle, NSSLCreateContext, int32_t unk) {
    NSSLContextHandle context = real_NSSLCreateContext(unk);

    //Add all commercial certs
    for (int cert = NSSL_SERVER_CERT_GROUP_COMMERCIAL_FIRST;
        cert <= NSSL_SERVER_CERT_GROUP_COMMERCIAL_LAST; cert++) {
        NSSLAddServerPKI(context, (NSSLServerCertId)cert);
    }
    for (int cert = NSSL_SERVER_CERT_GROUP_COMMERCIAL_4096_FIRST;
        cert <= NSSL_SERVER_CERT_GROUP_COMMERCIAL_4096_LAST; cert++) {
        NSSLAddServerPKI(context, (NSSLServerCertId)cert);
    }

    return context;
}

WUPS_MUST_REPLACE(NSSLCreateContext, WUPS_LOADER_LIBRARY_NSYSNET, NSSLCreateContext);
