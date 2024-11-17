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

#include "rpl_info.h"

#include <vector>
#include <algorithm>
#include <string_view>

// if we get more than like.. two callsites for this, it should really be refactored out rather than doing a fresh
// search every time
std::optional<OSDynLoad_NotifyData> search_for_rpl(std::string_view name) {
    int num_rpls = OSDynLoad_GetNumberOfRPLs();
    if (!num_rpls)
        return std::nullopt;

    // allocates but we free it at return
    std::vector<OSDynLoad_NotifyData> rpls(num_rpls);
    if (!OSDynLoad_GetRPLInfo(0, num_rpls, rpls.data()))
        return std::nullopt;

    auto rpl = std::find_if(rpls.begin(), rpls.end(), [=](const OSDynLoad_NotifyData &rpl) {
        return std::string_view(rpl.name).ends_with(name);
    });
    if (rpl == rpls.end())
        return std::nullopt;

    return *rpl;
}
