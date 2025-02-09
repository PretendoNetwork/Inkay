/*  Copyright 2024 Pretendo Network contributors <pretendo.network>
    Copyright 2024 Ash Logan <ash@heyquark.com>

    Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
    granted, provided that the above copyright notice and this permission notice appear in all copies.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
    INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
    IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
    PERFORMANCE OF THIS SOFTWARE.
*/

#include <coreinit/title.h>
#include <coreinit/dynload.h>
#include "game_peertopeer.h"

#include "config.h"
#include "sysconfig.h"
#include "utils/logger.h"
#include "utils/rpl_info.h"
#include "utils/replace_mem.h"

#include <optional>
#include <algorithm>
#include <string_view>
using namespace std::string_view_literals;

static struct {
    std::array<uint64_t, 3> tid;
    uint16_t version;
    uint32_t min_port_addr;
    uint32_t max_port_addr;
    std::string_view rpx;
} generic_patch_games[] = {
    {
        // MARIO KART 8
        {0x00050000'1010ec00, 0x00050000'1010ed00, 0x00050000'1010eb00},
        81,
        0x101a9a52,
        0x101a9a54,
        "Turbo.rpx"sv,
    },
    {
        // Splatoon
        {0x00050000'10176900, 0x00050000'10176a00, 0x00050000'10162b00},
        288,
        0x101e8952,
        0x101e8954,
        "Gambit.rpx"sv,
    },
};

static void generic_peertopeer_patch() {
    uint64_t tid = OSGetTitleID();
    uint16_t title_version = 0;
    if (const auto version_opt = get_current_title_version(); !version_opt) {
        DEBUG_FUNCTION_LINE("Failed to detect current title version");
        return;
    } else {
        title_version = *version_opt;
        DEBUG_FUNCTION_LINE("Title version detected: %d", title_version);
    }

    for (const auto &patch: generic_patch_games) {
        if (std::ranges::find(patch.tid, tid) == patch.tid.end()) continue;

        std::optional<OSDynLoad_NotifyData> game = search_for_rpl(patch.rpx);
        if (!game) {
            DEBUG_FUNCTION_LINE("Couldn't find game rpx! (%s)", patch.rpx.data());
            return;
        }

        if (title_version != patch.version) {
            DEBUG_FUNCTION_LINE("Unexpected title version. Expected %d but got %d (%s)", patch.version, title_version,
                                patch.rpx.data());
            continue;
        }

        auto port = get_console_peertopeer_port();
        DEBUG_FUNCTION_LINE_VERBOSE("Will use port %d. %08x", port, game->textAddr);

        auto target = (uint16_t *)rpl_addr(*game, patch.min_port_addr);
        replace_unsigned<uint16_t>(target, 0xc000, port);

        target = (uint16_t *)rpl_addr(*game, patch.max_port_addr);
        replace_unsigned<uint16_t>(target, 0xffff, port);
        break;
    }
}

static void minecraft_peertopeer_patch() {
    std::optional<OSDynLoad_NotifyData> minecraft = search_for_rpl("Minecraft.Client.rpx"sv);
    if (!minecraft) {
        DEBUG_FUNCTION_LINE("Couldn't find minecraft rpx!");
        return;
    }
    if (const auto version_opt = get_current_title_version(); !version_opt || *version_opt != 688) {
        DEBUG_FUNCTION_LINE("Wrong mincecraft version detected");
        return;
    }

    auto port = get_console_peertopeer_port();
    DEBUG_FUNCTION_LINE_VERBOSE("Will use port %d. %08x", port, minecraft->textAddr);

    auto target_func = (uint32_t *)rpl_addr(*minecraft, 0x03579530);
    replace_instruction(&target_func[0], 0x3c600001, 0x3c600000);        // li r3, 0
    replace_instruction(&target_func[1], 0x3863c000, 0x60630000 | port); // ori r3, r3, port
    // blr

    target_func = (uint32_t *)rpl_addr(*minecraft, 0x0357953c);
    replace_instruction(&target_func[0], 0x3c600001, 0x3c600000);        // li r3, 0
    replace_instruction(&target_func[1], 0x3863ffff, 0x60630000 | port); // ori r3, r3, port
    // blr
}

void peertopeer_patch() {
    if (!Config::connect_to_network) {
        return;
    }

    uint64_t tid = OSGetTitleID();
    if (tid == 0x00050000'101D7500 || // EUR
        tid == 0x00050000'101D9D00 || // USA
        tid == 0x00050000'101DBE00) { // JPN

        minecraft_peertopeer_patch();
    } else {
        generic_peertopeer_patch();
    }
}
