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
#include <wups/config/WUPSConfigItemBoolean.h>
#include <nsysnet/nssl.h>
#include <coreinit/cache.h>
#include <coreinit/dynload.h>
#include <coreinit/mcp.h>
#include <coreinit/memory.h>
#include <coreinit/memorymap.h>
#include <coreinit/memexpheap.h>
#include <coreinit/launch.h>
#include <sysapp/launch.h>
#include "wut_extra.h"
#include <utils/logger.h>
#include "url_patches.h"

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("Inkay");
WUPS_PLUGIN_DESCRIPTION("Pretendo Network Patcher");
WUPS_PLUGIN_VERSION("v2.1");
WUPS_PLUGIN_AUTHOR("Pretendo contributors");
WUPS_PLUGIN_LICENSE("ISC");

WUPS_USE_STORAGE("inkay");

bool connect_to_network = true;
bool prevConValue = true;

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

    WUPSStorageError storageRes = WUPS_OpenStorage();
    if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("Failed to open storage %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);
    }
    else {
        // Try to get value from storage
        if ((storageRes = WUPS_GetBool(nullptr, "connect_to_network", &connect_to_network)) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            // Add the value to the storage if it's missing.
            if (WUPS_StoreBool(nullptr, "connect_to_network", connect_to_network) != WUPS_STORAGE_ERROR_SUCCESS) {
                DEBUG_FUNCTION_LINE("Failed to store bool");
            }
        }
        else if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE("Failed to get bool %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);
        }

        prevConValue = connect_to_network;
        // Close storage
        if (WUPS_CloseStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE("Failed to close storage");
        }
    }

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

    if (connect_to_network) {
        if (is555(os_version)) {
            Mocha_IOSUKernelWrite32(0xE1019F78, 0xE3A00001); // mov r0, #1
        }
        else {
            Mocha_IOSUKernelWrite32(0xE1019E84, 0xE3A00001); // mov r0, #1
        }

        for (const auto& patch : url_patches) {
            write_string(patch.address, patch.url);
        }

        DEBUG_FUNCTION_LINE("Pretendo URL and NoSSL patches applied successfully.");
    }
    else {
        DEBUG_FUNCTION_LINE("Pretendo URL and NoSSL patches skipped.");
    }


    MCP_Close(mcp);
}
DEINITIALIZE_PLUGIN() {
    WHBLogUdpDeinit();
    Mocha_DeInitLibrary();
}

void skipPatchesChanged(ConfigItemBoolean* item, bool newValue) {
    DEBUG_FUNCTION_LINE("New value in skipPatchesChanged: %d", newValue);
    connect_to_network = newValue;
    // If the value has changed, we store it in the storage.
    WUPS_StoreInt(nullptr, "connect_to_network", connect_to_network);
}

WUPS_GET_CONFIG() {
    // We open the storage so we can persist the configuration the user did.
    if (WUPS_OpenStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("Failed to open storage");
        return 0;
    }

    WUPSConfigHandle config;
    WUPSConfig_CreateHandled(&config, "Inkay");

    WUPSConfigCategoryHandle cat;
    WUPSConfig_AddCategoryByNameHandled(config, "Patching", &cat);

    WUPSConfigItemBoolean_AddToCategoryHandled(config, cat, "connect_to_network", "Connect to the Pretendo network", connect_to_network, &skipPatchesChanged);

    return config;
}

bool isRelaunching = false;

WUPS_CONFIG_CLOSED() {
    // Save all changes
    if (WUPS_CloseStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("Failed to close storage");
    }

    if (prevConValue != connect_to_network) {
        if (!isRelaunching) {
            // Need to reload the console so the patches reset
            OSForceFullRelaunch();
            SYSLaunchMenu();
            isRelaunching = true;
        }
    }
    prevConvalue = connect_to_network;
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

    if (connect_to_network) {
        OSDynLoad_Acquire("nn_olv", &olv_handle);
        DEBUG_FUNCTION_LINE("Inkay: olv! %08x\n", olv_handle);

        //wish there was a better way than "blow through MEM2"
        uint32_t base_addr, size;
        if (OSGetMemBound(OS_MEM2, &base_addr, &size)) {
            DEBUG_FUNCTION_LINE("Inkay: OSGetMemBound failed!");
            return;
        }

        replace(base_addr, size, original_url, sizeof(original_url), new_url, sizeof(new_url));
    }
    else {
        DEBUG_FUNCTION_LINE("Inkay: Miiverse patches skipped.");
    }

}

ON_APPLICATION_ENDS() {
    DEBUG_FUNCTION_LINE("Inkay: shutting down...\n");
    OSDynLoad_Release(olv_handle);
}
