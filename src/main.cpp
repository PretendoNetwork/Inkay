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
#include <optional>
#include <nsysnet/nssl.h>
#include <coreinit/cache.h>
#include <coreinit/dynload.h>
#include <coreinit/mcp.h>
#include <coreinit/memory.h>
#include <coreinit/memorymap.h>
#include <coreinit/memexpheap.h>
#include <notifications/notifications.h>
#include <utils/logger.h>
#include "url_patches.h"
#include "config.h"
#include "Notification.h"

#include <coreinit/filesystem.h>
#include <cstring>
#include <string>
#include <nn/erreula/erreula_cpp.h>
#include <nn/act/client_cpp.h>

#include <curl/curl.h>
#include "ca_pem.h"

#include <gx2/surface.h>

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("Inkay");
WUPS_PLUGIN_DESCRIPTION("Pretendo Network Patcher");
WUPS_PLUGIN_VERSION("v2.3");
WUPS_PLUGIN_AUTHOR("Pretendo contributors");
WUPS_PLUGIN_LICENSE("ISC");

WUPS_USE_STORAGE("inkay");

#include <kernel/kernel.h>
#include <mocha/mocha.h>

const char original_url[] = "discovery.olv.nintendo.net/v1/endpoint";
const char new_url[] =      "discovery.olv.pretendo.cc/v1/endpoint";
const char wave_original[] = {
	0x68, 0x74, 0x74, 0x70, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x2E, 0x6E, 0x69, 0x6E, 0x74, 0x65, 0x6E, 0x64,
	0x6F, 0x2E, 0x6E, 0x65, 0x74
};
const char wave_new[] = {
	0x68, 0x74, 0x74, 0x70, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x2E, 0x70, 0x72, 0x65, 0x74, 0x65, 0x6E, 0x64,
	0x6F, 0x2E, 0x63, 0x63, 0x00
};

const char miiverse_green_highlight[] = {
	0x82,0xff,0x05,0xff,0x82,0xff,0x05,0xff,0x1d,0xff,0x04,0xff,0x1d,0xff,0x04,0xff
};

const char juxt_purple_highlight[] = {
	0x5d,0x4a,0x9a,0xff,0x5d,0x4a,0x9a,0xff,0x5d,0x4a,0x9a,0xff,0x5d,0x4a,0x9a,0xff 
};

const char miiverse_green_touch1[] = {
	0x94,0xd9,0x2a,0x00,0x57,0xbd,0x12,0xff
};

const char juxt_purple_touch1[] = {
	0x5d,0x4a,0x9a,0x00,0x5d,0x4a,0x9a,0xff
};

const char miiverse_green_touch2[] = {
	0x57,0xbd,0x12,0x00,0x94,0xd9,0x2a,0xff
};

const char juxt_purple_touch2[] = {
	0x5d,0x4a,0x9a,0x00,0x5d,0x4a,0x9a,0xff
};

_Static_assert(sizeof(original_url) > sizeof(new_url),
               "new_url too long! Must be less than 38chars.");

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

static bool is555(MCPSystemVersion version) {
    return (version.major == 5) && (version.minor == 5) && (version.patch >= 5);
}

INITIALIZE_PLUGIN() {
    WHBLogUdpInit();
    WHBLogCafeInit();

    Config::Init();

    auto res = Mocha_InitLibrary();

    if (res != MOCHA_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE("Mocha init failed with code %d!", res);
        return;
    }

    if (NotificationModule_InitLibrary() != NOTIFICATION_MODULE_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE("NotificationModule_InitLibrary failed");
    }

    //get os version
    MCPSystemVersion os_version;
    int mcp = MCP_Open();
    int ret = MCP_GetSystemVersion(mcp, &os_version);
    if (ret < 0) {
        DEBUG_FUNCTION_LINE("getting system version failed (%d/%d)!", mcp, ret);
        os_version = (MCPSystemVersion) {
                .major = 5, .minor = 5, .patch = 5, .region = 'E'
        };
    }
    DEBUG_FUNCTION_LINE("Running on %d.%d.%d%c",
        os_version.major, os_version.minor, os_version.patch, os_version.region
    );

    if (Config::connect_to_network) {
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
        StartNotificationThread("Using Pretendo Network");
    }
    else {
        DEBUG_FUNCTION_LINE("Pretendo URL and NoSSL patches skipped.");
        StartNotificationThread("Using Nintendo Network");
    }


    MCP_Close(mcp);
}
DEINITIALIZE_PLUGIN() {
    WHBLogUdpDeinit();
    Mocha_DeInitLibrary();
    NotificationModule_DeInitLibrary();
}

bool check_olv_libs() {
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

#define OLV_RPL "nn_olv.rpl"
bool path_is_olv(const char* path) {
    auto path_len = strlen(path);
    auto* path_suffix = path + path_len - (sizeof(OLV_RPL) - 1); //1 for null
    return strcmp(path_suffix, OLV_RPL) == 0;
}

void new_rpl_loaded(OSDynLoad_Module module, void* ctx, OSDynLoad_NotifyReason reason, OSDynLoad_NotifyData* rpl) {
    if (!Config::connect_to_network) {
        DEBUG_FUNCTION_LINE("Inkay: Miiverse patches skipped.");
        return;
    }

    // Loaded olv?
    if (reason != OS_DYNLOAD_NOTIFY_LOADED) return;
    if (!path_is_olv(rpl->name)) return;

    replace(rpl->dataAddr, rpl->dataSize, original_url, sizeof(original_url), new_url, sizeof(new_url));
}

void replaceBulk(uint32_t start, uint32_t size, const char* original_val, size_t original_val_sz, const char* new_val, size_t new_val_sz) {
    for (uint32_t addr = start; addr < start + size - original_val_sz; addr++) {
        int ret = memcmp(original_val, (void*)addr, original_val_sz);
        if (ret == 0) {
            DEBUG_FUNCTION_LINE("found bulk @%08x", addr);
            KernelCopyData(OSEffectiveToPhysical(addr), OSEffectiveToPhysical((uint32_t)new_val), new_val_sz);
            DEBUG_FUNCTION_LINE("new bulk   @%08x", addr);
        }
    }
}

ON_APPLICATION_START() {
    WHBLogUdpInit();
    WHBLogCafeInit();

    DEBUG_FUNCTION_LINE("Inkay: hewwo!\n");

    OSDynLoad_AddNotifyCallback(&new_rpl_loaded, nullptr);

    auto olvLoaded = check_olv_libs();
    if (!olvLoaded) {
        DEBUG_FUNCTION_LINE("Inkay: no olv, quitting for now\n");
        return;
    }

    if (!Config::connect_to_network) {
        DEBUG_FUNCTION_LINE("Inkay: Miiverse patches skipped.");
        return;
    }

    //wish there was a better way than "blow through MEM2"
    uint32_t base_addr, size;
    if (OSGetMemBound(OS_MEM2, &base_addr, &size)) {
        DEBUG_FUNCTION_LINE("Inkay: OSGetMemBound failed!");
        return;
    }

    replace(base_addr, size, original_url, sizeof(original_url), new_url, sizeof(new_url));
}

ON_APPLICATION_ENDS() {
    DEBUG_FUNCTION_LINE("Inkay: shutting down...\n");
    StopNotificationThread();
}

static std::optional<FSFileHandle> rootca_pem_handle{};

DECL_FUNCTION(int, FSOpenFile, FSClient *client, FSCmdBlock *block, char *path, const char *mode, uint32_t *handle, int error) {
    const char *initialOma   = "vol/content/initial.oma";

    if (!Config::connect_to_network) {
        DEBUG_FUNCTION_LINE("Inkay: Miiverse patches skipped.");
        return real_FSOpenFile(client, block, path, mode, handle, error);
    }

    if (strcmp(initialOma, path) == 0) {
        //below is a hacky (yet functional!) way to get Inkay to redirect URLs from the Miiverse applet
        //we do it when loading this file since it should only load once, preventing massive lag spikes as it searches all of MEM2 xD
        //WHBLogUdpInit();

        DEBUG_FUNCTION_LINE("Inkay: hewwo!\n");

        auto olvLoaded = check_olv_libs();

        if (!olvLoaded) {
            DEBUG_FUNCTION_LINE("Inkay: no olv, quitting for now\n");
        } else {
            uint32_t base_addr, size;
            if (OSGetMemBound(OS_MEM2, &base_addr, &size)) {
                DEBUG_FUNCTION_LINE("Inkay: OSGetMemBound failed!");
            } else {
                //We replace 2 times here.
                //The first is for nn_olv, the second Wave (the applet itself)
                replace(base_addr, size, original_url, sizeof(original_url), new_url, sizeof(new_url));
                replace(0x10000000, 0x10000000, wave_original, sizeof(wave_original), wave_new, sizeof(wave_new));
            }
        }
    // Check for root CA file and take note of its handle
    } else if (strcmp("vol/content/browser/rootca.pem", path) == 0) {
        int ret = real_FSOpenFile(client, block, path, mode, handle, error);
        rootca_pem_handle = *handle;
        DEBUG_FUNCTION_LINE("Inkay: Found Miiverse CA, replacing...");
        return ret;
    }

    return real_FSOpenFile(client, block, path, mode, handle, error);
}

DECL_FUNCTION(FSStatus, FSReadFile, FSClient *client, FSCmdBlock *block, uint8_t *buffer, uint32_t size, uint32_t count, FSFileHandle handle, uint32_t unk1, uint32_t flags) {
    if (size != 1) {
        DEBUG_FUNCTION_LINE("Inkay: Miiverse CA replacement failed!");
    }

    if (rootca_pem_handle && *rootca_pem_handle == handle) {
        memset(buffer, 0, size);
        strcpy((char*)buffer, (const char*)ca_pem);

        //this can't be done above (in the FSOpenFile hook) since it's not loaded yet.
        replaceBulk(0x10000000, 0x10000000, miiverse_green_highlight, sizeof(miiverse_green_highlight), juxt_purple_highlight, sizeof(juxt_purple_highlight));
        replaceBulk(0x10000000, 0x10000000, miiverse_green_touch1, sizeof(miiverse_green_touch1), juxt_purple_touch1, sizeof(juxt_purple_touch1));
        replaceBulk(0x10000000, 0x10000000, miiverse_green_touch2, sizeof(miiverse_green_touch2), juxt_purple_touch2, sizeof(juxt_purple_touch2));

        return (FSStatus)count;
    }
    
    return real_FSReadFile(client, block, buffer, size, count, handle, unk1, flags);
}

DECL_FUNCTION(FSStatus, FSCloseFile, FSClient * client, FSCmdBlock * block, FSFileHandle handle, FSErrorFlag errorMask) {
    if (handle == rootca_pem_handle) {
        rootca_pem_handle.reset();
    }

    return real_FSCloseFile(client, block, handle, errorMask);
}

DECL_FUNCTION(uint32_t, NSSLExportInternalServerCertificate, NSSLServerCertId cert, int unk, void* unk2, void* unk3) {
    if (cert == NSSL_SERVER_CERT_THAWTE_PREMIUM_SERVER_CA) { // Martini patches
        OSFatal("[598-0069] Please uninstall Martini patches to continue.\n" \
                "See pretendo.network/docs/search for more info.\n\n" \
                "Hold the POWER button for 4 seconds to shut down.\n\n"
                "            .\n"
                ".---------.'---.\n"
                "'.       :    .'\n"
                "  '.  .:::  .'\n"
                "    '.'::'.'\n"
                "      '||'\n"
                "       ||\n"
                "       ||\n"
                "mrz    ||\n"
                "   ---====---");
    }
    return real_NSSLExportInternalServerCertificate(cert, unk, unk2, unk3);
}

WUPS_MUST_REPLACE_FOR_PROCESS(FSOpenFile, WUPS_LOADER_LIBRARY_COREINIT, FSOpenFile, WUPS_FP_TARGET_PROCESS_MIIVERSE);
WUPS_MUST_REPLACE_FOR_PROCESS(FSReadFile, WUPS_LOADER_LIBRARY_COREINIT, FSReadFile, WUPS_FP_TARGET_PROCESS_MIIVERSE);
WUPS_MUST_REPLACE_FOR_PROCESS(FSCloseFile, WUPS_LOADER_LIBRARY_COREINIT, FSCloseFile, WUPS_FP_TARGET_PROCESS_MIIVERSE);
WUPS_MUST_REPLACE_FOR_PROCESS(NSSLExportInternalServerCertificate, WUPS_LOADER_LIBRARY_NSYSNET, NSSLExportInternalServerCertificate, WUPS_FP_TARGET_PROCESS_MIIVERSE);
