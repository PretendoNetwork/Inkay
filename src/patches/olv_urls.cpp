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

#include "olv_urls.h"
#include "config.h"
#include "utils/logger.h"
#include "utils/replace_mem.h"

#include <cstring>
#include <coreinit/dynload.h>
#include <coreinit/memory.h>

bool check_olv_libs() {
    OSDynLoad_Module olv_handle = 0;
    OSDynLoad_Error dret;

    dret = OSDynLoad_IsModuleLoaded("nn_olv", &olv_handle);
    if (dret == OS_DYNLOAD_OK && olv_handle != 0) {
        return true;
    }

    dret = OSDynLoad_IsModuleLoaded("nn_olv2", &olv_handle);
    if (dret == OS_DYNLOAD_OK && olv_handle != 0) {
        return true;
    }

    return false;
}

#define OLV_RPL "nn_olv.rpl"
bool path_is_olv(const char* path) {
    auto path_len = strlen(path);
    auto* path_suffix = path + path_len - (sizeof(OLV_RPL) - 1); //1 for null
    return strcmp(path_suffix, OLV_RPL) == 0;
}

void new_rpl_loaded(OSDynLoad_Module module, void* ctx, OSDynLoad_NotifyReason reason, OSDynLoad_NotifyData* rpl) {
    if (!Config::connect_to_network) {
        DEBUG_FUNCTION_LINE("Inkay: Miiverse patches skipped.");
        return;
    }

    // Loaded olv?
    if (reason != OS_DYNLOAD_NOTIFY_LOADED) return;
    if (!path_is_olv(rpl->name)) return;

    replace(rpl->dataAddr, rpl->dataSize, original_url, sizeof(original_url), new_url, sizeof(new_url));
}

bool setup_olv_libs() {
    if (!Config::connect_to_network) {
        DEBUG_FUNCTION_LINE("Inkay: Miiverse patches skipped.");
        return false;
    }

    OSDynLoad_AddNotifyCallback(&new_rpl_loaded, nullptr);

    auto olvLoaded = check_olv_libs();
    if (!olvLoaded) {
        DEBUG_FUNCTION_LINE("Inkay: no olv, quitting for now\n");
        return false;
    }

    //wish there was a better way than "blow through MEM2"
    uint32_t base_addr, size;
    if (OSGetMemBound(OS_MEM2, &base_addr, &size)) {
        DEBUG_FUNCTION_LINE("Inkay: OSGetMemBound failed!");
        return false;
    }

    return replace(base_addr, size, original_url, sizeof(original_url), new_url, sizeof(new_url));
}
