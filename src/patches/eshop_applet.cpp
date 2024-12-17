/*  Copyright 2023 Pretendo Network contributors <pretendo.network>
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

#include <vector>
#include <function_patcher/function_patching.h>
#include <optional>
#include <coreinit/debug.h>
#include <coreinit/filesystem.h>
#include <nsysnet/nssl.h>

#include "ca_pem.h" // generated at buildtime

constexpr char wave_original[] = "https://ninja.wup.shop.nintendo.net/ninja/wood_index.html?";
constexpr char wave_new[] =      "http://samurai.wup.shop." NETWORK_BASEURL "/ninja/wood_index.html?";

struct eshop_allowlist {
    char scheme[16];
    char domain[128];
    char path[128]; // unverified
    unsigned char flags[5];
};

constexpr struct eshop_allowlist original_entry = {
    .scheme = "https",
    .domain = "samurai.wup.shop.nintendo.net",
    .path = "",
    .flags = {1, 1, 1, 1, 0},
};

constexpr struct eshop_allowlist new_entry = {
    .scheme = "http",
    .domain = "samurai.wup.shop." NETWORK_BASEURL,
    .path = "",
    .flags = {1, 1, 1, 1, 0},
};

static std::optional<FSFileHandle> rootca_pem_handle{};
std::vector<PatchedFunctionHandle> eshop_patches;

DECL_FUNCTION(int, FSOpenFile_eShop, FSClient *client, FSCmdBlock *block, char *path, const char *mode, uint32_t *handle,
              int error) {
    const char *initialOma = "vol/content/initial.oma";

    if (!Config::connect_to_network) {
        DEBUG_FUNCTION_LINE_VERBOSE("Inkay: eShop patches skipped.");
        return real_FSOpenFile_eShop(client, block, path, mode, handle, error);
    }

    if (strcmp(initialOma, path) == 0) {
        //below is a hacky (yet functional!) way to get Inkay to redirect URLs from the Miiverse applet
        //we do it when loading this file since it should only load once, preventing massive lag spikes as it searches all of MEM2 xD

        DEBUG_FUNCTION_LINE_VERBOSE("Inkay: hewwo eShop!\n");

        if (!replace(0x10000000, 0x10000000, wave_original, sizeof(wave_original), wave_new, sizeof(wave_new)))
            DEBUG_FUNCTION_LINE_VERBOSE("Inkay: We didn't find the url /)>~<(\\");

        if (!replace(0x10000000, 0x10000000, (const char *)&original_entry, sizeof(original_entry), (const char *)&new_entry, sizeof(new_entry)))
            DEBUG_FUNCTION_LINE_VERBOSE("Inkay: We didn't find the whitelist /)>~<(\\");

    // Check for root CA file and take note of its handle
    } else if (strcmp("vol/content/browser/rootca.pem", path) == 0) {
        int ret = real_FSOpenFile_eShop(client, block, path, mode, handle, error);
        rootca_pem_handle = *handle;
        DEBUG_FUNCTION_LINE_VERBOSE("Inkay: Found eShop CA, replacing...");
        return ret;
    }

    return real_FSOpenFile_eShop(client, block, path, mode, handle, error);
}

DECL_FUNCTION(FSStatus, FSReadFile_eShop, FSClient *client, FSCmdBlock *block, uint8_t *buffer, uint32_t size, uint32_t count,
              FSFileHandle handle, uint32_t unk1, uint32_t flags) {
    if (size != 1) {
        DEBUG_FUNCTION_LINE("Inkay: eShop CA replacement failed!");
    }

    if (rootca_pem_handle && *rootca_pem_handle == handle) {
        strlcpy((char *) buffer, (const char *) ca_pem, size * count);
        return (FSStatus) count;
    }

    return real_FSReadFile_eShop(client, block, buffer, size, count, handle, unk1, flags);
}

DECL_FUNCTION(FSStatus, FSCloseFile_eShop, FSClient *client, FSCmdBlock *block, FSFileHandle handle, FSErrorFlag errorMask) {
    if (handle == rootca_pem_handle) {
        rootca_pem_handle.reset();
    }

    return real_FSCloseFile_eShop(client, block, handle, errorMask);
}

void patchEshop() {
    eshop_patches.reserve(3);

    auto add_patch = [](function_replacement_data_t repl, const char *name) {
        PatchedFunctionHandle handle = 0;
        if (FunctionPatcher_AddFunctionPatch(&repl, &handle, nullptr) != FUNCTION_PATCHER_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE("Inkay/eShop: Failed to patch %s!", name);
        }
        eshop_patches.push_back(handle);
    };

    add_patch(REPLACE_FUNCTION_FOR_PROCESS(FSOpenFile_eShop, LIBRARY_COREINIT, FSOpenFile, FP_TARGET_PROCESS_ESHOP), "FSOpenFile_eShop");
    add_patch(REPLACE_FUNCTION_FOR_PROCESS(FSReadFile_eShop, LIBRARY_COREINIT, FSReadFile, FP_TARGET_PROCESS_ESHOP), "FSReadFile_eShop");
    add_patch(REPLACE_FUNCTION_FOR_PROCESS(FSCloseFile_eShop, LIBRARY_COREINIT, FSCloseFile, FP_TARGET_PROCESS_ESHOP), "FSCloseFile_eShop");
}

void unpatchEshop() {
    for (auto handle: eshop_patches) {
        FunctionPatcher_RemoveFunctionPatch(handle);
    }
    eshop_patches.clear();
}
