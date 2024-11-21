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

static void minecraft_peertopeer_patch() {
    std::optional<OSDynLoad_NotifyData> minecraft = search_for_rpl("Minecraft.Client.rpx");
    if (!minecraft) {
        DEBUG_FUNCTION_LINE("Couldn't find minecraft rpx!");
        return;
    }

    auto port = get_console_peertopeer_port();
    DEBUG_FUNCTION_LINE_VERBOSE("Will use port %d. %08x", port, minecraft->textAddr);

    uint32_t *target_func = rpl_addr(*minecraft, 0x03579530);
    replace_instruction(&target_func[0], 0x3c600001, 0x3c600000);        // li r3, 0
    replace_instruction(&target_func[1], 0x3863c000, 0x60630000 | port); // ori r3, r3, port
    // blr

    target_func = rpl_addr(*minecraft, 0x0357953c);
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
        DEBUG_FUNCTION_LINE_VERBOSE("Game has no p2p patches, will skip.\n");
    }
}
