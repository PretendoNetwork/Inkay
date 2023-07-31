/*  Copyright 2023 Pretendo Network contributors <pretendo.network>
    Copyright 2023 Ash Logan <ash@heyquark.com>
    Copyright 2019 Maschell

    Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
    granted, provided that the above copyright notice and this permission notice appear in all copies.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
    INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
    IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
    PERFORMANCE OF THIS SOFTWARE.
*/

#include "game_matchmaking.h"
#include "utils/logger.h"

#include "ini.h"
#include <array>
#include <optional>
#include <string>
#include <vector>
#include <cstdint>
#include <function_patcher/function_patching.h>

#define MARIO_KART_8_TID_J 0x000500001010EB00
#define MARIO_KART_8_TID_U 0x000500001010EC00
#define MARIO_KART_8_TID_E 0x000500001010ED00

constexpr std::array<uint64_t, 3> mk8_tids = {MARIO_KART_8_TID_J, MARIO_KART_8_TID_U, MARIO_KART_8_TID_E};
std::vector<PatchedFunctionHandle> matchmaking_patches;

struct modpack {
    std::string name = "Mario Kart 8";
    int dlc_id = -1;
};
std::optional<modpack> dlc_modpack;

static int handler(void* user, const char* section, const char* name, const char* value) {
    auto* mod = (modpack*)user;
    auto match = [section, name](const char* s, const char* n) -> bool {
        return strcmp(section, s) == 0 && strcmp(name, n) == 0;
    };

    if (match("pretendo", "name")) {
        mod->name = value;
    } else if (match("pretendo", "dlc_id")) {
        mod->dlc_id = std::strtol(value, nullptr, 16);
    } else return 0;

    return 1;
}

static void check_modpack() {
    modpack mod;
    if (ini_parse("fs:/vol/content/pretendo.ini", handler, &mod)) {
        DEBUG_FUNCTION_LINE("Inkay/MK8: Doesn't look like a modpack");
    }

    DEBUG_FUNCTION_LINE("Inkay/MK8: Playing %s (%08x)", mod.name.c_str(), mod.dlc_id);
    dlc_modpack = mod;
}

DECL_FUNCTION(void, mk8_MatchmakeSessionSearchCriteria_SetAttribute, void* _this, uint32_t attributeIndex, uint32_t attributeValue) {
    if (attributeIndex == 4) {
        if (!dlc_modpack) check_modpack();

        const int dlc_id = dlc_modpack->dlc_id;
        if (dlc_id != -1) {
            DEBUG_FUNCTION_LINE("Inkay/MK8: Searching for %s session (%08x)", dlc_modpack->name.c_str(), dlc_id);
            attributeValue = dlc_id;
        }
    }

    real_mk8_MatchmakeSessionSearchCriteria_SetAttribute(_this, attributeIndex, attributeValue);
}

DECL_FUNCTION(void, mk8_MatchmakeSession_SetAttribute, void* _this, uint32_t attributeIndex, uint32_t attributeValue) {
    if (attributeIndex == 4) {
        if (!dlc_modpack) check_modpack();

        const int dlc_id = dlc_modpack->dlc_id;
        if (dlc_id != -1) {
            DEBUG_FUNCTION_LINE("Inkay/MK8: Creating %s session (%08x)", dlc_modpack->name.c_str(), dlc_id);
            attributeValue = dlc_id;
        }
    }

    real_mk8_MatchmakeSession_SetAttribute(_this, attributeIndex, attributeValue);
}

void install_matchmaking_patches() {
    matchmaking_patches.reserve(2);

    function_replacement_data_t repl = REPLACE_FUNCTION_OF_EXECUTABLE_BY_ADDRESS_WITH_VERSION(
        mk8_MatchmakeSessionSearchCriteria_SetAttribute,
        mk8_tids.data(), mk8_tids.size(),
        "Turbo.rpx",
        0x0098e7b4, 64, 64
    );
    PatchedFunctionHandle handle = 0;
    if (FunctionPatcher_AddFunctionPatch(&repl, &handle, nullptr) != FUNCTION_PATCHER_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE("Inkay/MK8: Failed to patch MatchmakeSessionSearchCriteria::SetAttribute!");
    }
    matchmaking_patches.push_back(handle);

    function_replacement_data_t repl2 = REPLACE_FUNCTION_OF_EXECUTABLE_BY_ADDRESS_WITH_VERSION(
            mk8_MatchmakeSession_SetAttribute,
            mk8_tids.data(), mk8_tids.size(),
            "Turbo.rpx",
            0x0098e52c, 64, 64
    );
    if (FunctionPatcher_AddFunctionPatch(&repl2, &handle, nullptr) != FUNCTION_PATCHER_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE("Inkay/MK8: Failed to patch MatchmakeSession::SetAttribute!");
    }
    matchmaking_patches.push_back(handle);
}

void remove_matchmaking_patches() {
    for (auto handle : matchmaking_patches) {
        FunctionPatcher_RemoveFunctionPatch(handle);
    }
    matchmaking_patches.clear();
}

void matchmaking_notify_titleswitch() {
    dlc_modpack.reset();
}
