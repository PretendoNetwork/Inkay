/*  Copyright 2022 Pretendo Network contributors <pretendo.network>
    Copyright 2022 Ash Logan <ash@heyquark.com>

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

#include "config.h"

#include "wut_extra.h"
#include "utils/logger.h"
#include "sysconfig.h"
#include "lang.h"

#include <wups.h>
#include <wups/storage.h>
#include <wups/config_api.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/config/WUPSConfigItemStub.h>

#include <coreinit/title.h>
#include <coreinit/launch.h>
#include <sysapp/title.h>
#include <sysapp/launch.h>
#include <nn/act.h>
#include <format>

static config_strings strings;

bool Config::connect_to_network = true;
bool Config::need_relaunch = false;
bool Config::unregister_task_item_pressed = false;
bool Config::is_wiiu_menu = false;

static WUPSConfigAPICallbackStatus report_error(WUPSConfigAPIStatus err) {
    DEBUG_FUNCTION_LINE_VERBOSE("WUPS config error: %s", WUPSConfigAPI_GetStatusStr(err));
    return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
}

static void report_storage_error(WUPSStorageError err) {
    DEBUG_FUNCTION_LINE_VERBOSE("WUPS storage error: %s", WUPSStorageAPI_GetStatusStr(err));
}

static void connect_to_network_changed(ConfigItemBoolean* item, bool new_value) {
    DEBUG_FUNCTION_LINE_VERBOSE("connect_to_network changed to: %d", new_value);
    if (new_value != Config::connect_to_network) {
        Config::need_relaunch = true;
    }
    Config::connect_to_network = new_value;

    WUPSStorageError res;
    res = WUPSStorageAPI::Store<bool>("connect_to_network", Config::connect_to_network);
    if (res != WUPS_STORAGE_ERROR_SUCCESS) return report_storage_error(res);
}

static void unregister_task_item_on_input_cb(void *context, WUPSConfigSimplePadData input) {
    if (!Config::unregister_task_item_pressed && Config::is_wiiu_menu && ((input.buttons_d & WUPS_CONFIG_BUTTON_A) == WUPS_CONFIG_BUTTON_A)) {

        nn::act::Initialize();
        Initialize__Q2_2nn4bossFv();

        for (uint8_t i = 1; i <= nn::act::GetNumOfAccounts(); i++)
        {
            if (nn::act::IsSlotOccupied(i) && nn::act::IsNetworkAccountEx(i))
            {
              nn::boss::Task task{};
              nn::act::PersistentId persistentId = nn::act::GetPersistentIdEx(i);

              __ct__Q3_2nn4boss4TaskFv(&task);
              Initialize__Q3_2nn4boss4TaskFPCcUi(&task, "oltopic", persistentId);

      // bypasses compiler warning about unused variable
      #ifdef DEBUG
              uint32_t res = Unregister__Q3_2nn4boss4TaskFv(&task);
              DEBUG_FUNCTION_LINE_VERBOSE("Unregistered oltopic for: SlotNo %d | Persistent ID %08x -> 0x%08x", i, persistentId, res);
      #else
              Unregister__Q3_2nn4boss4TaskFv(&task);
      #endif
          }
      }

        Finalize__Q2_2nn4bossFv();
        nn::act::Finalize();

        Config::unregister_task_item_pressed = !Config::unregister_task_item_pressed;
        Config::need_relaunch = true;
    }
}

static int32_t unregister_task_item_get_display_value(void *context, char *out_buf, int32_t out_size) {
    auto string = strings.need_menu_action;
    if (Config::is_wiiu_menu) {
        if (Config::unregister_task_item_pressed) {
            string = strings.restart_to_apply_action;
        } else {
            string = strings.press_a_action;
        }
    }

    if ((int)string.length() > out_size - 1) return -1;
    string.copy(out_buf, string.length());
    out_buf[string.length()] = '\0';

    return 0;
}

static WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {
    WUPSConfigAPIStatus err;
    bool res;

    uint64_t current_title_id = OSGetTitleID();
    uint64_t wiiu_menu_tid = _SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_WII_U_MENU);
    Config::is_wiiu_menu = (current_title_id == wiiu_menu_tid);

    // get translation strings
    strings = get_config_strings(get_system_language());

    // create root config category
    WUPSConfigCategory root = WUPSConfigCategory(rootHandle);

    auto network_cat = WUPSConfigCategory::Create(strings.network_category, err);
    if (!network_cat) return report_error(err);

    //                                                  config id                   display name            default        current value             changed callback
    auto connect_item = WUPSConfigItemBoolean::Create("connect_to_network", strings.connect_to_network_setting, true, Config::connect_to_network, &connect_to_network_changed, err);
    if (!connect_item) return report_error(err);

    res = network_cat->add(std::move(*connect_item), err);
    if (!res) return report_error(err);

    {
        uint16_t port = get_console_peertopeer_port();
        std::string multiplayer_port_text = std::vformat(strings.multiplayer_port_display, std::make_format_args(port));
        res = network_cat->add(WUPSConfigItemStub::Create(multiplayer_port_text), err);
        if (!res) return report_error(err);
    }

    res = root.add(std::move(*network_cat), err);
    if (!res) return report_error(err);

    auto other_cat = WUPSConfigCategory::Create(strings.other_category, err);
    if (!other_cat) return report_error(err);

    WUPSConfigAPIItemCallbacksV2 unregisterTasksItemCallbacks = {
            .getCurrentValueDisplay = unregister_task_item_get_display_value,
            .getCurrentValueSelectedDisplay = unregister_task_item_get_display_value,
            .onSelected = nullptr,
            .restoreDefault = nullptr,
            .isMovementAllowed = nullptr,
            .onCloseCallback = nullptr,
            .onInput = unregister_task_item_on_input_cb,
            .onInputEx = nullptr,
            .onDelete = nullptr
    };

    WUPSConfigAPIItemOptionsV2 unregisterTasksItemOptions = {
            .displayName = strings.reset_wwp_setting.data(),
            .context = nullptr,
            .callbacks = unregisterTasksItemCallbacks,
    };

    WUPSConfigItemHandle unregisterTasksItem;
    err = WUPSConfigAPI_Item_Create(unregisterTasksItemOptions, &unregisterTasksItem);
    if (err != WUPSCONFIG_API_RESULT_SUCCESS) return report_error(err);

    err = WUPSConfigAPI_Category_AddItem(other_cat->getHandle(), unregisterTasksItem);
    if (err != WUPSCONFIG_API_RESULT_SUCCESS) return report_error(err);

    res = root.add(std::move(*other_cat), err);
    if (!res) return report_error(err);

    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
}

static void ConfigMenuClosedCallback() {
    // Save all changes
    WUPSStorageError res;
    res = WUPSStorageAPI::SaveStorage();
    if (res != WUPS_STORAGE_ERROR_SUCCESS) return report_storage_error(res);

    if (Config::need_relaunch) {
        // Need to reload the console so the patches reset
        OSForceFullRelaunch();
        SYSLaunchMenu();
        Config::need_relaunch = false;
    }
}

void Config::Init() {
    WUPSConfigAPIStatus cres;

    // Init the config api
    WUPSConfigAPIOptionsV1 configOptions = { .name = "Inkay" };
    cres = WUPSConfigAPI_Init(configOptions, ConfigMenuOpenedCallback, ConfigMenuClosedCallback);
    if (cres != WUPSCONFIG_API_RESULT_SUCCESS) return (void)report_error(cres);

    WUPSStorageError res;
    // Try to get value from storage
    res = WUPSStorageAPI::Get<bool>("connect_to_network", Config::connect_to_network);
    if (res == WUPS_STORAGE_ERROR_NOT_FOUND) {
        DEBUG_FUNCTION_LINE("Connect to network value not found, attempting to migrate/create");

        bool skipPatches = false;
        if (WUPSStorageAPI::Get<bool>("skipPatches", skipPatches) == WUPS_STORAGE_ERROR_SUCCESS) {
            // Migrate old config value
            Config::connect_to_network = !skipPatches;
            WUPSStorageAPI::DeleteItem("skipPatches");
        }
    
        // Add the value to the storage if it's missing.
        res = WUPSStorageAPI::Store<bool>("connect_to_network", connect_to_network);
        if (res != WUPS_STORAGE_ERROR_SUCCESS) return report_storage_error(res);
    }
    else if (res != WUPS_STORAGE_ERROR_SUCCESS) return report_storage_error(res);

    // Save storage
    res = WUPSStorageAPI::SaveStorage();
    if (res != WUPS_STORAGE_ERROR_SUCCESS) return report_storage_error(res);
}
