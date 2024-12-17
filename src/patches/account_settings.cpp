/*  Copyright 2023 Pretendo Network contributors <pretendo.network>
    Copyright 2023 Jemma Poffinbarger <jemma@jemsoftware.dev>
    Copyright 2023 Ash Logan <ash@heyquark.com>
    Copyright 2019 Maschell

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"
#include "olv_urls.h"
#include "utils/logger.h"
#include "utils/replace_mem.h"
#include "inkay_config.h"

#include <function_patcher/function_patching.h>
#include <vector>
#include <optional>
#include <coreinit/debug.h>
#include <coreinit/filesystem.h>
#include <coreinit/title.h>
#include <nsysnet/nssl.h>

#include "ca_pem.h" // generated at buildtime

#define ACCOUNT_SETTINGS_TID_J 0x000500101004B000
#define ACCOUNT_SETTINGS_TID_U 0x000500101004B100
#define ACCOUNT_SETTINGS_TID_E 0x000500101004B200

struct account_settings_allowlist {
    char scheme[16];
    char domain[128];
    char path[128]; // unverified
    uint32_t flags;
};

constexpr struct account_settings_allowlist original_entry = {
    .scheme = "https",
    .domain = "account.nintendo.net",
    .path = "",
    .flags = 0x01010101,
};

constexpr struct account_settings_allowlist new_entry = {
    .scheme = "https",
    .domain = "account." NETWORK_BASEURL,
    .path = "",
    .flags = 0x01010101,
};
constexpr char wave_original[] = "saccount.nintendo.net";

constexpr char wave_new[] =      "saccount." NETWORK_BASEURL;

static bool isAccountSettingsTitle() {
    return (OSGetTitleID() != 0 && (
        OSGetTitleID() == ACCOUNT_SETTINGS_TID_J ||
        OSGetTitleID() == ACCOUNT_SETTINGS_TID_U ||
        OSGetTitleID() == ACCOUNT_SETTINGS_TID_E
        ));
}

static std::optional<FSFileHandle> rootca_pem_handle{};
std::vector<PatchedFunctionHandle> account_patches;

DECL_FUNCTION(int, FSOpenFile_accSettings, FSClient *client, FSCmdBlock *block, char *path, const char *mode, uint32_t *handle,
              int error) {
    if(!isAccountSettingsTitle()) {
        return real_FSOpenFile_accSettings(client, block, path, mode, handle, error);
    }

    if (!Config::connect_to_network) {
        DEBUG_FUNCTION_LINE_VERBOSE("Inkay: account settings patches skipped.");
        return real_FSOpenFile_accSettings(client, block, path, mode, handle, error);
    }

    // Check for root CA file and take note of its handle
    if (strcmp("vol/content/browser/rootca.pem", path) == 0) {
        int ret = real_FSOpenFile_accSettings(client, block, path, mode, handle, error);
        rootca_pem_handle = *handle;
        DEBUG_FUNCTION_LINE_VERBOSE("Inkay: Found account settings CA, replacing...");
        return ret;
    }
    return real_FSOpenFile_accSettings(client, block, path, mode, handle, error);
}

DECL_FUNCTION(FSStatus, FSReadFile_accSettings, FSClient *client, FSCmdBlock *block, uint8_t *buffer, uint32_t size, uint32_t count,
              FSFileHandle handle, uint32_t unk1, uint32_t flags) {
    if(!isAccountSettingsTitle()) {
        return real_FSReadFile_accSettings(client, block, buffer, size, count, handle, unk1, flags);
    }
    if (size != 1) {
        DEBUG_FUNCTION_LINE("Inkay: account settings CA replacement failed!");
    }

    if (rootca_pem_handle && *rootca_pem_handle == handle) {
        strlcpy((char *) buffer, (const char *) ca_pem, size * count);
        return (FSStatus) count;
    }
    return real_FSReadFile_accSettings(client, block, buffer, size, count, handle, unk1, flags);
}

DECL_FUNCTION(FSStatus, FSCloseFile_accSettings, FSClient *client, FSCmdBlock *block, FSFileHandle handle, FSErrorFlag errorMask) {
    if(!isAccountSettingsTitle()) {
        return real_FSCloseFile_accSettings(client, block, handle, errorMask);
    }
    if (handle == rootca_pem_handle) {
        rootca_pem_handle.reset();
    }
    return real_FSCloseFile_accSettings(client, block, handle, errorMask);
}

bool patchAccountSettings() {
    account_patches.reserve(3);

    auto add_patch = [](function_replacement_data_t repl, const char *name) {
        PatchedFunctionHandle handle = 0;
        if (FunctionPatcher_AddFunctionPatch(&repl, &handle, nullptr) != FUNCTION_PATCHER_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE("Inkay/Account: Failed to patch %s!", name);
        }
        account_patches.push_back(handle);
    };

    add_patch(REPLACE_FUNCTION_FOR_PROCESS(FSOpenFile_accSettings, LIBRARY_COREINIT, FSOpenFile, FP_TARGET_PROCESS_GAME), "FSOpenFile_accSettings");
    add_patch(REPLACE_FUNCTION_FOR_PROCESS(FSReadFile_accSettings, LIBRARY_COREINIT, FSReadFile, FP_TARGET_PROCESS_GAME), "FSReadFile_accSettings");
    add_patch(REPLACE_FUNCTION_FOR_PROCESS(FSCloseFile_accSettings, LIBRARY_COREINIT, FSCloseFile, FP_TARGET_PROCESS_GAME), "FSCloseFile_accSettings");
        
    return true;
}

bool hotpatchAccountSettings() {
    if(!isAccountSettingsTitle()) {
        return false;
    }

    if (!Config::connect_to_network) {
        DEBUG_FUNCTION_LINE_VERBOSE("Inkay: account settings patches skipped.");
        return false;
    }

    DEBUG_FUNCTION_LINE_VERBOSE("Inkay: hewwo account settings!\n");

    if (!replace(0x10000000, 0x10000000, wave_original, sizeof(wave_original), wave_new, sizeof(wave_new))) {
        DEBUG_FUNCTION_LINE("Inkay: We didn't find the url /)>~<(\\");
        return false;
    }

    if (!replace(0x10000000, 0x10000000, (const char *)&original_entry, sizeof(original_entry), (const char *)&new_entry, sizeof(new_entry))) {
        DEBUG_FUNCTION_LINE("Inkay: We didn't find the whitelist /)>~<(\\");
        return false;
    }

    return true;
}

void unpatchAccountSettings() {
    for (auto handle: account_patches) {
        FunctionPatcher_RemoveFunctionPatch(handle);
    }
    account_patches.clear();
}
