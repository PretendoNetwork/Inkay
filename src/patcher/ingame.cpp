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

#include "ingame.h"
#include "rplinfo.h"
#include "utils/logger.h"
#include "utils/asm.h"

#include <coreinit/title.h>

#include "patches/nn_olv.h"

void RunPatcher() {
    auto orplinfo = TryGetRPLInfo();
    if (!orplinfo) {
        DEBUG_FUNCTION_LINE("Couldn't get RPL info - trying patches");
        bool bret = PatchDynLoadFunctions();
        if (!bret) {
            DEBUG_FUNCTION_LINE("OSDynLoad patches failed! Quitting");
            return;
        }

        //try again
        orplinfo = TryGetRPLInfo();
        if (!orplinfo) {
            DEBUG_FUNCTION_LINE("Still couldn't get RPL info - quitting");
            return;
        }
    }

    auto rpls = *orplinfo;

    /*for (const auto& rpl : rpls) {
        DEBUG_FUNCTION_LINE("rpl[0]: [%s] %08x %08x %08x|%08x %08x %08x|%08x %08x %08x",
                            rpl.name, rpl.textAddr, rpl.textOffset, rpl.textSize,
                            rpl.dataAddr, rpl.dataOffset, rpl.dataSize,
                            rpl.readAddr, rpl.readOffset, rpl.readSize
        );
    }*/

    uint64_t titleId = OSGetTitleID();
    auto titleVer = __OSGetTitleVersion();

    // "always" patches
    Patch_nn_olv(titleVer, titleId, rpls);
}
