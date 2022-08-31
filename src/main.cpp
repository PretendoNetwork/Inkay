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
#include <coreinit/mcp.h>
#include <coreinit/memory.h>
#include <coreinit/memorymap.h>
#include <coreinit/memexpheap.h>
#include "wut_extra.h"
#include <utils/logger.h>
#include "url_patches.h"

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
#include <mocha/mocha.h>

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

//thanks @Gary#4139 :p
static void write_string(uint32_t addr, const char* str)
{
    int len = strlen(str) + 1;
    int remaining = len % 4;
    int num = len - remaining;

    for (int i = 0; i < (num / 4); i++) {
        Mocha_IOSUKernelWrite32(addr + i * 4, *(uint32_t*)(str + i * 4));
    }

    if (remaining > 0) {
        uint8_t buf[4];
        Mocha_IOSUKernelRead32(addr + num, (uint32_t*)&buf);

        for (int i = 0; i < remaining; i++) {
            buf[i] = *(str + num + i);
        }

        Mocha_IOSUKernelWrite32(addr + num, *(uint32_t*)&buf);
    }
}

static bool is555(MCP_SystemVersion version) {
    return (version.major == 5) && (version.minor == 5) && (version.patch >= 5);
}

INITIALIZE_PLUGIN() {
    WHBLogUdpInit();
    auto res = Mocha_InitLibrary();

    if (res != MOCHA_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE("Mocha init failed with code %d!", res);
        return;
    }

    //get os version
    MCP_SystemVersion os_version;
    int mcp = MCP_Open();
    int ret = MCP_GetSystemVersion(mcp, &os_version);
    if (ret < 0) {
        DEBUG_FUNCTION_LINE("getting system version failed (%d/%d)!", mcp, ret);
        os_version = (MCP_SystemVersion) {
                .major = 5, .minor = 5, .patch = 5, .region = 'E'
        };
    }
    DEBUG_FUNCTION_LINE("Running on %d.%d.%d%c",
        os_version.major, os_version.minor, os_version.patch, os_version.region
    );

    if (is555(os_version)) {
        Mocha_IOSUKernelWrite32(0xE1019F78, 0xE3A00001); // mov r0, #1
    } else {
        Mocha_IOSUKernelWrite32(0xE1019E84, 0xE3A00001); // mov r0, #1
    }

    for (const auto& patch : url_patches) {
        write_string(patch.address, patch.url);
    }

    DEBUG_FUNCTION_LINE("Pretendo URL and NoSSL patches applied successfully.")
}
DEINITIALIZE_PLUGIN() {
    WHBLogUdpDeinit();
    Mocha_DeinitLibrary();
}

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
