/*  Copyright 2022 Pretendo Network contributors <pretendo.network>
    Copyright 2022 Ash Logan <ash@heyquark.com>

    Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
    granted, provided that the above copyright notice and this permission notice appear in all copies.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
    INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
    IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
    PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef INKAY_RPLINFO_H
#define INKAY_RPLINFO_H

#include <optional>
#include <vector>
#include <string>
#include <algorithm>

#include <coreinit/dynload.h>

using rplinfo = std::vector<OSDynLoad_NotifyData>;

std::optional<rplinfo> TryGetRPLInfo();
bool PatchDynLoadFunctions();

constexpr inline std::optional<OSDynLoad_NotifyData> FindRPL(const rplinfo& rpls, const std::string& name) {
    auto res = std::find_if(rpls.cbegin(), rpls.cend(), [&](const OSDynLoad_NotifyData& data){ return std::string(data.name).ends_with(name); });
    if (res == rpls.cend()) return std::nullopt;
    return *res;
}

#endif //INKAY_RPLINFO_H
