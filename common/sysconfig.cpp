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
#include "utils/scope_exit.h"

#include <coreinit/mcp.h>
#include <coreinit/userconfig.h>
#include <optional>

nn::swkbd::LanguageType get_system_language() {
    static std::optional <nn::swkbd::LanguageType> cached_language{};
    if (cached_language) return *cached_language;

    UCHandle handle = UCOpen();
    if (handle < 0) {
        DEBUG_FUNCTION_LINE("Error opening UC: %d", handle);
        return nn::swkbd::LanguageType::English;
    }
    scope_exit uc_c([&] { UCClose(handle); });

    nn::swkbd::LanguageType language;

    alignas(0x40) UCSysConfig settings = {
            .name = "cafe.language",
            .access = 0,
            .dataType = UC_DATATYPE_UNSIGNED_INT,
            .error = UC_ERROR_OK,
            .dataSize = sizeof(language),
            .data = &language,
    };

    UCError err = UCReadSysConfig(handle, 1, &settings);
    if (err != UC_ERROR_OK) {
        DEBUG_FUNCTION_LINE("Error reading UC: %d!", err);
        return nn::swkbd::LanguageType::English;
    }

    DEBUG_FUNCTION_LINE_VERBOSE("System language found: %d", language);
    cached_language = language;
    return language;
}

static std::optional<MCPSysProdSettings> mcp_config;
static std::optional<MCPSystemVersion> mcp_os_version;
static void get_mcp_config() {
    int mcp = MCP_Open();
    scope_exit mcp_c([&] { MCP_Close(mcp); });

    alignas(0x40) MCPSysProdSettings config {};
    if (MCP_GetSysProdSettings(mcp, &config)) {
        DEBUG_FUNCTION_LINE("Could not get MCP system config!");
        return;
    }
    mcp_config = config;

    //get os version
    MCPSystemVersion os_version;
    if (MCP_GetSystemVersion(mcp, &os_version)) {
        DEBUG_FUNCTION_LINE("Could not get MCP system config!");
        return;
    }
    mcp_os_version = os_version;

    DEBUG_FUNCTION_LINE_VERBOSE("Running on %d.%d.%d%c; %s%s",
                                os_version.major, os_version.minor, os_version.patch, os_version.region
                                        config.code_id, config.serial_id
    );
}

const char * get_console_serial() {
    if (!mcp_config) get_mcp_config();

    return mcp_config ? mcp_config->serial_id : "123456789";
}

MCPSystemVersion get_console_os_version() {
    if (!mcp_os_version) get_mcp_config();

    return mcp_os_version.value_or((MCPSystemVersion) { .major = 5, .minor = 5, .patch = 5, .region = 'E' });
}

static inline int digit(char a) {
    if (a < '0' || a > '9') return 0;
    return a - '0';
}

unsigned short get_console_peertopeer_port() {
    const char * serial = get_console_serial();

    unsigned short port = 50000 +
                          (digit(serial[4]) * 1000) +
                          (digit(serial[5]) * 100 ) +
                          (digit(serial[6]) * 10  ) +
                          (digit(serial[7]) * 1   );

    return port;
}
