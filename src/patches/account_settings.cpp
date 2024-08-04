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

#include <wups.h>
#include <optional>
#include <coreinit/debug.h>
#include <coreinit/filesystem.h>
#include <coreinit/title.h>
#include <nsysnet/nssl.h>

#include "ca_pem.h" // generated at buildtime

#define ACCOUNT_SETTINGS_TID_J 0x000500101004B000
#define ACCOUNT_SETTINGS_TID_U 0x000500101004B100
#define ACCOUNT_SETTINGS_TID_E 0x000500101004B200

const char whitelist_original[] = {
        0x68, 0x74, 0x74, 0x70, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x61, 0x63, 0x63, 0x6F, 0x75, 0x6E, 0x74, 0x2E,
        0x6E, 0x69, 0x6E, 0x74, 0x65, 0x6E, 0x64, 0x6F, 0x2E, 0x6E, 0x65, 0x74
};

const char whitelist_new[] = {
        0x68, 0x74, 0x74, 0x70, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x61, 0x63, 0x63, 0x6F, 0x75, 0x6E, 0x74, 0x2E,
        0x70, 0x72, 0x65, 0x74, 0x65, 0x6E, 0x64, 0x6F, 0x2E, 0x63, 0x63, 0x00
};

const char wave_original[] = "saccount.nintendo.net";

const char wave_new[] =      "saccount.pretendo.cc";

bool isAccountSettingsTitle();

static std::optional<FSFileHandle> rootca_pem_handle{};

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

bool isAccountSettingsTitle() {
    return (OSGetTitleID() != 0 && (
        OSGetTitleID() == ACCOUNT_SETTINGS_TID_J ||
        OSGetTitleID() == ACCOUNT_SETTINGS_TID_U ||
        OSGetTitleID() == ACCOUNT_SETTINGS_TID_E
        ));
}

bool patchAccountSettings() {
    if(!isAccountSettingsTitle()) {
        return false;
    }

    if (!Config::connect_to_network) {
        DEBUG_FUNCTION_LINE_VERBOSE("Inkay: account settings patches skipped.");
        return false;
    }

    DEBUG_FUNCTION_LINE_VERBOSE("Inkay: hewwo account settings!\n");

    if (!replace(0x10000000, 0x10000000, wave_original, sizeof(wave_original), wave_new, sizeof(wave_new))) {
        DEBUG_FUNCTION_LINE_VERBOSE("Inkay: We didn't find the url /)>~<(\\");
        return false;
    }

    if (!replace(0x10000000, 0x10000000, whitelist_original, sizeof(whitelist_original), whitelist_new, sizeof(whitelist_new))) {
        DEBUG_FUNCTION_LINE_VERBOSE("Inkay: We didn't find the whitelist /)>~<(\\");
        return false;
    }
        
    return true;
}

WUPS_MUST_REPLACE_FOR_PROCESS(FSOpenFile_accSettings, WUPS_LOADER_LIBRARY_COREINIT, FSOpenFile, WUPS_FP_TARGET_PROCESS_GAME);
WUPS_MUST_REPLACE_FOR_PROCESS(FSReadFile_accSettings, WUPS_LOADER_LIBRARY_COREINIT, FSReadFile, WUPS_FP_TARGET_PROCESS_GAME);
WUPS_MUST_REPLACE_FOR_PROCESS(FSCloseFile_accSettings, WUPS_LOADER_LIBRARY_COREINIT, FSCloseFile, WUPS_FP_TARGET_PROCESS_GAME);
