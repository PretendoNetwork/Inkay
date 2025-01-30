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

#include <wups.h>

#include <notifications/notifications.h>
#include <utils/logger.h>
#include "config.h"
#include "module.h"

#define INKAY_VERSION "v2.6.0"

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("Inkay");
WUPS_PLUGIN_DESCRIPTION("Pretendo Network Patcher");
WUPS_PLUGIN_VERSION(INKAY_VERSION);
WUPS_PLUGIN_AUTHOR("Pretendo contributors");
WUPS_PLUGIN_LICENSE("GPLv3");

WUPS_USE_STORAGE("inkay");

WUPS_USE_WUT_DEVOPTAB();

INITIALIZE_PLUGIN() {
    WHBLogCafeInit();
    WHBLogUdpInit();

    Config::Init();

    if (NotificationModule_InitLibrary() != NOTIFICATION_MODULE_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE("NotificationModule_InitLibrary failed");
    }

    // if using pretendo then (try to) apply the ssl patches
    Inkay_Initialize(Config::connect_to_network);
}

DEINITIALIZE_PLUGIN() {
    Inkay_Finalize();
    NotificationModule_DeInitLibrary();

    WHBLogCafeDeinit();
    WHBLogUdpDeinit();
}

ON_APPLICATION_START() {
    // Tell the module the plugin is running!
    Inkay_SetPluginRunning();
}

ON_APPLICATION_ENDS() {

}
