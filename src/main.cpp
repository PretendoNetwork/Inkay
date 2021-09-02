/*  Copyright 2021 Pretendo Network contributors <pretendo.network>
    Copyright 2019 Ash Logan "quarktheawesome" <ash@heyquark.com>
    Copyright 2019 Maschell

    Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdio.h>
#include <stdint.h>
#include <wups.h>
#include <nsysnet/socket.h>
#include <nsysnet/nssl.h>
#include <coreinit/dynload.h>
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

WUPS_ALLOW_KERNEL();

char original_url[] = "discovery.olv.nintendo.net/v1/endpoint";
char new_url[] =      "discovery.olv.pretendo.cc/v1/endpoint";
_Static_assert(sizeof(original_url) > sizeof(new_url),
               "new_url too long! Must be less than 38chars.");

// We'll keep a handle to nn_olv, just to ensure it doesn't get unloaded
static OSDynLoad_Module olv_handle;

ON_APPLICATION_START(args){
    socket_lib_init();
    log_init();

    if (!args.kernel_access) {
        DEBUG_FUNCTION_LINE("Inkay: No kernel access!\n");
        return;
    }

    OSDynLoad_Acquire("nn_olv", &olv_handle);

    //wish there was a better way than "blow through MEM2"
    //TODO use OSGetMemBounds or w/e
    for (uint32_t addr = 0x10000000; addr < 0x20000000; addr += 4) {
        int ret = memcmp(original_url, (void*)addr, sizeof(original_url));
        if (ret == 0) {
            DEBUG_FUNCTION_LINE("Found string at %08X\n", addr);
            DEBUG_FUNCTION_LINE("string: %s\n", (char*)addr);
            for (uint j = 0; j < sizeof(new_url); j += 4) {
                uint32_t val = *(uint32_t*)(new_url + j);
                WUPS_KernelWrite((void*)(addr + j), val);
            }
            DEBUG_FUNCTION_LINE("string: %s\n", (char*)addr);
            return;
        }
    }
}

ON_APPLICATION_ENDING(){
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
