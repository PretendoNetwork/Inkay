/*  Copyright 2022 Pretendo Network contributors <pretendo.network>
    Copyright 2022 Ash Logan <ash@heyquark.com>
    Copyright 2019 Maschell

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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <wups.h>
#include <optional>
#include <nsysnet/nssl.h>
#include <sysapp/title.h>
#include <coreinit/cache.h>
#include <coreinit/dynload.h>
#include <coreinit/mcp.h>
#include <coreinit/memory.h>
#include <coreinit/memorymap.h>
#include <coreinit/memexpheap.h>
#include <coreinit/title.h>
#include <notifications/notifications.h>
#include <utils/logger.h>
#include "iosu_url_patches.h"
#include "config.h"
#include "Notification.h"
#include "patches/olv_urls.h"
#include "patches/game_matchmaking.h"

#include <coreinit/filesystem.h>
#include <cstring>
#include <string>
#include <nn/erreula/erreula_cpp.h>
#include <nn/act/client_cpp.h>

#include <curl/curl.h>
#include "ca_pem.h"

#include <gx2/surface.h>

#define INKAY_VERSION "v3.0"

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("Inkay");
WUPS_PLUGIN_DESCRIPTION("Pretendo Network Patcher");
WUPS_PLUGIN_VERSION(INKAY_VERSION);
WUPS_PLUGIN_AUTHOR("Pretendo contributors");
WUPS_PLUGIN_LICENSE("ISC");

WUPS_USE_STORAGE("inkay");
WUPS_USE_WUT_DEVOPTAB();

#include <kernel/kernel.h>
#include <mocha/mocha.h>
#include <function_patcher/function_patching.h>
#include "patches/account_settings.h"
#include "utils/sysconfig.h"

//thanks @Gary#4139 :p
static void write_string(uint32_t addr, const char* str)
{
    int len = strlen(str) + 1;
    int remaining = len % 4;
    int num = len - remaining;

    for (int i = 0; i < (num / 4); i++) {
        Mocha_IOSUKernelWrite32(addr + i * 4, *(uint32_t*)(str + i * 4));
    }

    if (remaining > 0) {
        uint8_t buf[4];
        Mocha_IOSUKernelRead32(addr + num, (uint32_t*)&buf);

        for (int i = 0; i < remaining; i++) {
            buf[i] = *(str + num + i);
        }

        Mocha_IOSUKernelWrite32(addr + num, *(uint32_t*)&buf);
    }
}

static bool is555(MCPSystemVersion version) {
    return (version.major == 5) && (version.minor == 5) && (version.patch >= 5);
}

static const char * get_nintendo_network_message() {
    // TL note: "Nintendo Network" is a proper noun - "Network" is part of the name
    // TL note: "Using" instead of "Connected" is deliberate - we don't know if a successful connection exists, we are
    // only specifying what we'll *attempt* to connect to
    switch (get_system_language()) {
        case nn::swkbd::LanguageType::English:
        default:
            return "Using Nintendo Network";
        case nn::swkbd::LanguageType::Spanish:
            return "Usando Nintendo Network";
        case nn::swkbd::LanguageType::French:
            return "Sur Nintendo Network";
         case nn::swkbd::LanguageType::Italian:
            return "Usando Nintendo Network";
        case nn::swkbd::LanguageType::German:
            return "Nutze Nintendo Network";
    }
}
static const char * get_pretendo_message() {
    // TL note: "Pretendo Network" is also a proper noun - though "Pretendo" alone can refer to us as a project
    // TL note: "Using" instead of "Connected" is deliberate - we don't know if a successful connection exists, we are
    // only specifying what we'll *attempt* to connect to
    switch (get_system_language()) {
        case nn::swkbd::LanguageType::English:
        default:
            return "Using Pretendo Network";
        case nn::swkbd::LanguageType::Spanish:
            return "Usando Pretendo Network";
        case nn::swkbd::LanguageType::French:
            return "Sur Pretendo Network";
        case nn::swkbd::LanguageType::Italian:
            return "Usando Pretendo Network";
        case nn::swkbd::LanguageType::German:
            return "Nutze Pretendo Network";
    }
}

INITIALIZE_PLUGIN() {
	WHBLogModuleInit();
	WHBLogCafeInit();
    WHBLogUdpInit();

    Config::Init();

    auto res = Mocha_InitLibrary();

    if (res != MOCHA_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE("Mocha init failed with code %d!", res);
        return;
    }

    if (NotificationModule_InitLibrary() != NOTIFICATION_MODULE_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE("NotificationModule_InitLibrary failed");
    }

    //get os version
    MCPSystemVersion os_version;
    int mcp = MCP_Open();
    int ret = MCP_GetSystemVersion(mcp, &os_version);
    if (ret < 0) {
        DEBUG_FUNCTION_LINE("getting system version failed (%d/%d)!", mcp, ret);
        os_version = (MCPSystemVersion) {
                .major = 5, .minor = 5, .patch = 5, .region = 'E'
        };
    }
    DEBUG_FUNCTION_LINE_VERBOSE("Running on %d.%d.%d%c",
        os_version.major, os_version.minor, os_version.patch, os_version.region
    );

	// if using pretendo then (try to) apply the ssl patches
    if (Config::connect_to_network) {
        if (is555(os_version)) {
            Mocha_IOSUKernelWrite32(0xE1019F78, 0xE3A00001); // mov r0, #1
        }
        else {
            Mocha_IOSUKernelWrite32(0xE1019E84, 0xE3A00001); // mov r0, #1
        }

        for (const auto& patch : url_patches) {
            write_string(patch.address, patch.url);
        }
        DEBUG_FUNCTION_LINE_VERBOSE("Pretendo URL and NoSSL patches applied successfully.");
    }
    else {
        DEBUG_FUNCTION_LINE_VERBOSE("Pretendo URL and NoSSL patches skipped.");
    }

    MCP_Close(mcp);

    if (FunctionPatcher_InitLibrary() == FUNCTION_PATCHER_RESULT_SUCCESS) {
        install_matchmaking_patches();
    }
}

DEINITIALIZE_PLUGIN() {
    remove_matchmaking_patches();

    Mocha_DeInitLibrary();
    NotificationModule_DeInitLibrary();
    FunctionPatcher_DeInitLibrary();
	
	WHBLogModuleDeinit();
	WHBLogCafeDeinit();
    WHBLogUdpDeinit();
}

ON_APPLICATION_START() {
    DEBUG_FUNCTION_LINE_VERBOSE("Inkay " INKAY_VERSION " starting up...\n");

    uint64_t titleID = OSGetTitleID();
    if (titleID == _SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_WII_U_MENU)) {
        if (Config::connect_to_network) {
            ShowNotification(get_pretendo_message());
        }
        else {
            ShowNotification(get_nintendo_network_message());
        }
    }

    setup_olv_libs();
    matchmaking_notify_titleswitch();
    patchAccountSettings();
}

ON_APPLICATION_ENDS() {
	// commented because it doesnt really unload inkay
	//DEBUG_FUNCTION_LINE_VERBOSE("Unloading Inkay...\n");
}
