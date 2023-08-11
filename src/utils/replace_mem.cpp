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

#include "replace_mem.h"
#include "utils/logger.h"

#include <kernel/kernel.h>
#include <coreinit/memorymap.h>
#include <algorithm>

bool replace(uint32_t start, uint32_t size, const char* original_val, size_t original_val_sz, const char* new_val, size_t new_val_sz) {
    for (uint32_t addr = start; addr < start + size - original_val_sz; addr++) {
        int ret = memcmp(original_val, (void*)addr, original_val_sz);
        if (ret == 0) {
            DEBUG_FUNCTION_LINE("found str @%08x: %s", addr, (const char*)addr);
            KernelCopyData(OSEffectiveToPhysical(addr), OSEffectiveToPhysical((uint32_t)new_val), new_val_sz);
            DEBUG_FUNCTION_LINE("new str   @%08x: %s", addr, (const char*)addr);
            return true;
        }
    }

    return false;
}

void replaceBulk(uint32_t start, uint32_t size, std::span<const replacement> replacements) {
    // work out the biggest input replacement
    auto max_sz = std::max_element(replacements.begin(), replacements.end(), [](auto& a, auto& b) {
        return a.orig.size_bytes() < b.orig.size_bytes();
    })->orig.size_bytes();

    int counts[replacements.size()];
    for (auto& c : counts) {
        c = 0;
    }

    for (uint32_t addr = start; addr < start + size - max_sz; addr++) {
        for (int i = 0; i < (int)replacements.size(); i++) {
            const auto& replacement = replacements[i];

            int ret = memcmp((void*)addr, replacement.orig.data(), replacement.orig.size_bytes());
            if (ret == 0) {
                KernelCopyData(
                        OSEffectiveToPhysical(addr),
                        OSEffectiveToPhysical((uint32_t)replacement.repl.data()),
                        replacement.repl.size_bytes()
                );
                counts[i]++;
                break; // don't check the other replacements
            }
        }
    }
    for (auto c : counts) {
        DEBUG_FUNCTION_LINE("replaced %d times", c);
    }
}
