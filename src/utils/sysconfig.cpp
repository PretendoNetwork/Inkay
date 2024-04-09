/*  Copyright 2024 Pretendo Network contributors <pretendo.network>
    Copyright 2024 Ash Logan <ash@heyquark.com>
    Copyright 2020-2022 V10lator <v10lator@myway.de>
    Copyright 2022 Xpl0itU <DaThinkingChair@protonmail.com>

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

#include "sysconfig.h"
#include "utils/logger.h"

#include <coreinit/userconfig.h>
#include <optional>

nn::swkbd::LanguageType get_system_language() {
    static std::optional<nn::swkbd::LanguageType> cached_language{};
    if (cached_language) return *cached_language;

    UCHandle handle = UCOpen();
    if (handle >= 0) {
        nn::swkbd::LanguageType language;

        UCSysConfig settings __attribute__((__aligned__(0x40))) = {
                .name = "cafe.language",
                .access = 0,
                .dataType = UC_DATATYPE_UNSIGNED_INT,
                .error = UC_ERROR_OK,
                .dataSize = sizeof(language),
                .data = &language,
        };

        UCError err = UCReadSysConfig(handle, 1, &settings);
        UCClose(handle);
        if (err != UC_ERROR_OK) {
            DEBUG_FUNCTION_LINE("Error reading UC: %d!", err);
            return nn::swkbd::LanguageType::English;
        } else {
            DEBUG_FUNCTION_LINE_VERBOSE("System language found: %d", language);
            cached_language = language;
            return language;
        }
    } else {
        DEBUG_FUNCTION_LINE("Error opening UC: %d", handle);
        return nn::swkbd::LanguageType::English;
    }
}
