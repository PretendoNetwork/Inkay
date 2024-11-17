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

#pragma once

#include <optional>
#include <string_view>
#include <coreinit/dynload.h>

std::optional<OSDynLoad_NotifyData> search_for_rpl(std::string_view name);

constexpr uint32_t *rpl_addr(OSDynLoad_NotifyData rpl, uint32_t cemu_addr) {
    if (cemu_addr < 0x1000'0000) {
        return (uint32_t *)(rpl.textAddr + cemu_addr - 0x0200'0000);
    } else {
        return (uint32_t *)(rpl.dataAddr + cemu_addr - 0x1000'0000);
    }
}
