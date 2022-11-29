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
#include <coreinit/filesystem.h>
#include <cstring>
#include <string>
#include <sdutils/sdutils.h>
#include <nn/erreula/erreula_cpp.h>
#include <nn/act/client_cpp.h>

#include <curl/curl.h>

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

bool skipPatches = false;
bool prevSkipValue = false;

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

char16_t* output;

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

static const int B64index[256] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62, 63, 62, 62, 63, 52, 53, 54, 55,
56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,
7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,
0,  0,  0, 63,  0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };

std::string b64decode(const void* data, const size_t len)
{
    unsigned char* p = (unsigned char*)data;
    int pad = len > 0 && (len % 4 || p[len - 1] == '=');
    const size_t L = ((len + 3) / 4 - pad) * 4;
    std::string str(L / 4 * 3 + pad, '\0');

    for (size_t i = 0, j = 0; i < L; i += 4)
    {
        int n = B64index[p[i]] << 18 | B64index[p[i + 1]] << 12 | B64index[p[i + 2]] << 6 | B64index[p[i + 3]];
        str[j++] = n >> 16;
        str[j++] = n >> 8 & 0xFF;
        str[j++] = n & 0xFF;
    }
    if (pad)
    {
        int n = B64index[p[L]] << 18 | B64index[p[L + 1]] << 12;
        str[str.size() - 1] = n >> 16;

        if (len > L + 2 && p[L + 2] != '=')
        {
            n |= B64index[p[L + 2]] << 6;
            str.push_back(n >> 8 & 0xFF);
        }
    }
    return str;
}

static bool is555(MCP_SystemVersion version) {
    return (version.major == 5) && (version.minor == 5) && (version.patch >= 5);
}

WUPS_USE_WUT_DEVOPTAB();

INITIALIZE_PLUGIN() {
    WHBLogUdpInit();

    WUPSStorageError storageRes = WUPS_OpenStorage();
    if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("Failed to open storage %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);
    }
    else {
        // Try to get value from storage
        if ((storageRes = WUPS_GetBool(nullptr, "skipPatches", &skipPatches)) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            // Add the value to the storage if it's missing.
            if (WUPS_StoreBool(nullptr, "skipPatches", skipPatches) != WUPS_STORAGE_ERROR_SUCCESS) {
                DEBUG_FUNCTION_LINE("Failed to store bool");
            }
        }
        else if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE("Failed to get bool %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);
        }

        prevSkipValue = skipPatches;

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

    if (!skipPatches) {
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
    skipPatches = newValue;
    // If the value has changed, we store it in the storage.
    WUPS_StoreInt(nullptr, "skipPatches", skipPatches);
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

    WUPSConfigItemBoolean_AddToCategoryHandled(config, cat, "skipPatches", "Skip Pretendo Network patches", skipPatches, &skipPatchesChanged);

    return config;
}

bool isRelaunching = false;

WUPS_CONFIG_CLOSED() {
    // Save all changes
    if (WUPS_CloseStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("Failed to close storage");
    }

    if (prevSkipValue != skipPatches) {
        if (!isRelaunching) {
            // Need to reload the console so the patches reset
            OSForceFullRelaunch();
            SYSLaunchMenu();
            isRelaunching = true;
        }
    }
    prevSkipValue = skipPatches;
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

uint32_t find(uint32_t start, uint32_t size, const char* original_val, size_t original_val_sz) {
    for (uint32_t addr = start; addr < start + size - original_val_sz; addr++) {
        int ret = memcmp(original_val, (void*)addr, original_val_sz);
        if (ret == 0) {
            return addr;
        }
    }

    return 0;
}

ON_APPLICATION_START() {
    WHBLogUdpInit();

    DEBUG_FUNCTION_LINE("Inkay: hewwo!\n");

    auto olvLoaded = checkForOlvLibs();

    if (!olvLoaded) {
        DEBUG_FUNCTION_LINE("Inkay: no olv, quitting for now\n");
        return;
    }

    if (!skipPatches) {
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

DECL_FUNCTION(int, FSOpenFile, FSClient *client, FSCmdBlock *block, char *path, const char *mode, uint32_t *handle, int error) {
    const char *initialOma   = "vol/content/initial.oma";

    if (strcmp(initialOma, path) == 0) {
        //below is a hacky (yet functional!) way to get Inkay to redirect URLs from the Miiverse applet
        //we do it when loading this file since it should only load once, preventing massive lag spikes as it searches all of MEM2 xD
        WHBLogUdpInit();

        DEBUG_FUNCTION_LINE("Inkay: hewwo!\n");

        auto olvLoaded = checkForOlvLibs();

        if (!olvLoaded) {
            DEBUG_FUNCTION_LINE("Inkay: no olv, quitting for now\n");
        }else{
            if (!skipPatches) {
                OSDynLoad_Acquire("nn_olv", &olv_handle);
                DEBUG_FUNCTION_LINE("Inkay: olv! %08x\n", olv_handle);

                //wish there was a better way than "blow through MEM2"
                uint32_t base_addr, size;
                if (OSGetMemBound(OS_MEM2, &base_addr, &size)) {
                    DEBUG_FUNCTION_LINE("Inkay: OSGetMemBound failed!");
                }else{
                    //We replace 2 times here.
                    //The first is for nn_olv, the second Wave (the applet itself)
                    replace(base_addr, size, original_url, sizeof(original_url), new_url, sizeof(new_url));
                    replace(0x10000000, 0x40000000, wave_original, sizeof(wave_original), wave_new, sizeof(wave_new));
                }
            }
            else {
                DEBUG_FUNCTION_LINE("Inkay: Miiverse patches skipped.");
            }
        }
    }
    return real_FSOpenFile(client, block, path, mode, handle, error);
}

DECL_FUNCTION(FSStatus, FSReadFile, FSClient *client, FSCmdBlock *block, uint8_t *buffer, uint32_t size, uint32_t count, FSFileHandle handle, uint32_t unk1, uint32_t flags) {
    FSStatus result = real_FSReadFile(client, block, buffer, size, count, handle, unk1, flags);
    if(strstr((char*)buffer, "# rootca.pem") && strstr((char*)buffer, "4CE7Y259RF06alPvERck/VSyWmxzViHJbC2XpEKzJ2EFIWNt6ii8TxpvQtyYq1XT")){
        memset(buffer, 0, size);
        strcpy((char*)buffer, "-----BEGIN CERTIFICATE-----\nMIIDaTCCAlGgAwIBAgIQWWi/WdPuVr1BiNarQaD06TANBgkqhkiG9w0BAQsFADBG\nMQwwCgYDVQQLEwNQS0kxEzARBgNVBAoTCkthZXJ1IFRlYW0xCzAJBgNVBAYTAkdC\nMRQwEgYDVQQDEwtQS0NBIFogUm9vdDAgFw0yMTEwMDMxODIzMTlaGA8yMDUxMTAw\nMzE4MzMxOFowRjEMMAoGA1UECxMDUEtJMRMwEQYDVQQKEwpLYWVydSBUZWFtMQsw\nCQYDVQQGEwJHQjEUMBIGA1UEAxMLUEtDQSBaIFJvb3QwggEiMA0GCSqGSIb3DQEB\nAQUAA4IBDwAwggEKAoIBAQC5QC4VGxY9xeI6svqIPEd/nxJXQPFPRj3l1neu5xNC\n5Q6u4g5i0OYXBXR+u2CTHfzOeimr5Jvxb6jvGHKQWNVChGY0ncKhP9dkjIqQZruw\niLBcB8PUCYP3VMKprUD+aSheSRAdkTLYf2JiayedepTUHPYP1SkaLa6gYfoGBFgR\nK7pYSx5W4xTq4kBnn3Ua9CEfnoOSZPsL7OpYb7Xxnnzap3ro48RYtWLSOEeq0q1R\nUEtE84Vy+QnbCfM/TYeP+lkUZO3zTWkta5+cNEgFxX1ME68rImJsl6SAnvPpJjMf\nMudU3YSFOp5mLjkiWTuldYZPkwQLHWo+3j7NsHgBp/0dAgMBAAGjUTBPMAsGA1Ud\nDwQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQWBBTS9mt1zZQoDkARVBBR\nEjHUg+r3mzAQBgkrBgEEAYI3FQEEAwIBADANBgkqhkiG9w0BAQsFAAOCAQEAix6C\nXdS6A2wfLmXlccGLHNv+ammx7lib3dA4/1n+pc9oIuB9No9JTzANmCMhHsY7cWyW\nyLgJP590tqfsAfP1IfIhwunIUACL1FnQnwxkJPIGjG2NGoQYy9/33geG2Ajn3gXk\n5GWtcDa/CJq/30nlrr9Fx1JQU+UxHEMHXwSKydBP89P2igRm7bHwcXQFdOVgkAyk\nONTsCq4BU3gt/dbItSN8010lFx4sbG366DP0/MtYjeHEFlX+hlUDwtIoCV8zqKVz\nwOgPI/hg6ZO0IR8Ifa65d12gxpKy+xMfiwLhNZQsR62h2hr1iCuaGtoTiXQvM/9S\nv3bMEooCcYmxZkoxqA==\n-----END CERTIFICATE-----");
    }
    
    return result;
}

size_t writeFunction(void* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

DECL_FUNCTION(uint32_t, ErrEulaAppearError__3RplFRCQ3_2nn7erreula9AppearArg, nn::erreula::AppearArg AppearArg) {
    if (!skipPatches) {
        if(AppearArg.errorArg.errorCode == 1022802){
            auto curl = curl_easy_init();
            if (curl) {
                uint32_t pid = (uint32_t)(nn::act::GetPrincipalId());
                std::string str = "http://192.168.0.93:8888/api/banreason?pid=";
                str += std::to_string(pid);
                curl_easy_setopt(curl, CURLOPT_URL, str.c_str());
                curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
                curl_easy_setopt(curl, CURLOPT_USERAGENT, "Pretendo Inkay");
                curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
                curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
                
                std::string response_string;
                std::string header_string;
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
                curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);
                
                char* url;
                long response_code;
                double elapsed;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
                curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &elapsed);
                curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
                
                curl_easy_perform(curl);
                curl_easy_cleanup(curl);
                curl = NULL;
                AppearArg.errorArg.errorType = nn::erreula::ErrorType::Message1Button;
                AppearArg.errorArg.errorMessage = (char16_t*)b64decode((char*)response_string.c_str(), strlen(response_string.c_str())).c_str();
            }
        }
    }
    uint32_t result = real_ErrEulaAppearError__3RplFRCQ3_2nn7erreula9AppearArg(AppearArg);
    output = 0;
    return result;
}

WUPS_MUST_REPLACE(ErrEulaAppearError__3RplFRCQ3_2nn7erreula9AppearArg, WUPS_LOADER_LIBRARY_ERREULA, ErrEulaAppearError__3RplFRCQ3_2nn7erreula9AppearArg);
WUPS_MUST_REPLACE_FOR_PROCESS(FSOpenFile, WUPS_LOADER_LIBRARY_COREINIT, FSOpenFile, WUPS_FP_TARGET_PROCESS_MIIVERSE);
WUPS_MUST_REPLACE_FOR_PROCESS(FSReadFile, WUPS_LOADER_LIBRARY_COREINIT, FSReadFile, WUPS_FP_TARGET_PROCESS_MIIVERSE);