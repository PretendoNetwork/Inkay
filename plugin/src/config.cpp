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
#include <wups/config/WUPSConfigItemMultipleValues.h>
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
bool Config::show_startup_toast = true;
bool Config::need_relaunch = false;
bool Config::unregister_task_item_pressed = false;
bool Config::is_wiiu_menu = false;
uint32_t Config::language = 13;
inkay_language Config::current_language = English;

static WUPSConfigAPICallbackStatus report_error(WUPSConfigAPIStatus err) {
    DEBUG_FUNCTION_LINE_VERBOSE("WUPS config error: %s", WUPSConfigAPI_GetStatusStr(err));
    return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
}

static void report_storage_error(WUPSStorageError err) {
    DEBUG_FUNCTION_LINE_VERBOSE("WUPS storage error: %s", WUPSStorageAPI_GetStatusStr(err));
}

static void connect_to_network_changed(ConfigItemBoolean *item, bool new_value) {
    DEBUG_FUNCTION_LINE_VERBOSE("connect_to_network changed to: %d", new_value);
    if (new_value != Config::connect_to_network) {
        Config::need_relaunch = true;
    }
    Config::connect_to_network = new_value;

    WUPSStorageError res = WUPSStorageAPI_StoreBool(nullptr, "connect_to_network", Config::connect_to_network);
    if (res != WUPS_STORAGE_ERROR_SUCCESS) return report_storage_error(res);
}

static void language_changed(ConfigItemMultipleValues *item, uint32_t new_value) {
    DEBUG_FUNCTION_LINE_VERBOSE("language changed to: %d", new_value);
    if (new_value != Config::language) {
        if (new_value == inkay_language::System)
            Config::current_language = (inkay_language) get_system_language();
        else
            Config::current_language = (inkay_language) new_value;
    }
    Config::language = new_value;

    WUPSStorageError res = WUPSStorageAPI_StoreU32(nullptr, "language", Config::language);
    if (res != WUPS_STORAGE_ERROR_SUCCESS) return report_storage_error(res);
}

static void show_startup_toast_changed(ConfigItemBoolean *item, bool new_value) {
    DEBUG_FUNCTION_LINE_VERBOSE("show_startup_toast changed to: %d", new_value);
    Config::show_startup_toast = new_value;

    WUPSStorageError res = WUPSStorageAPI_StoreBool(nullptr, "show_startup_toast", Config::show_startup_toast);
    if (res != WUPS_STORAGE_ERROR_SUCCESS) return report_storage_error(res);
}

static void unregister_task_item_on_input_cb(void *context, WUPSConfigSimplePadData input) {
    if (!Config::unregister_task_item_pressed && Config::is_wiiu_menu && (
            (input.buttons_d & WUPS_CONFIG_BUTTON_A) == WUPS_CONFIG_BUTTON_A)) {
        nn::act::Initialize();
        Initialize__Q2_2nn4bossFv();

        for (uint8_t i = 1; i <= nn::act::GetNumOfAccounts(); i++) {
            if (nn::act::IsSlotOccupied(i) && nn::act::IsNetworkAccountEx(i)) {
                nn::boss::Task task{};
                nn::act::PersistentId persistentId = nn::act::GetPersistentIdEx(i);

                __ct__Q3_2nn4boss4TaskFv(&task);
                Initialize__Q3_2nn4boss4TaskFPCcUi(&task, "oltopic", persistentId);

                // bypasses compiler warning about unused variable
#ifdef DEBUG
                uint32_t res = Unregister__Q3_2nn4boss4TaskFv(&task);
                DEBUG_FUNCTION_LINE_VERBOSE("Unregistered oltopic for: SlotNo %d | Persistent ID %08x -> 0x%08x", i,
                                            persistentId, res);
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

    if ((int) string.length() > out_size - 1) return -1;
    string.copy(out_buf, string.length());
    out_buf[string.length()] = '\0';

    return 0;
}

static WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle root_cat) {
    WUPSConfigAPIStatus err;

    uint64_t current_title_id = OSGetTitleID();
    uint64_t wiiu_menu_tid = _SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_WII_U_MENU);
    Config::is_wiiu_menu = (current_title_id == wiiu_menu_tid);

    // get translation strings
    strings = get_config_strings(Config::current_language);

    // Network submenu
    //    Connect to Pretendo Network        Yes/No
    //    Using UDP port 5000 for multiplayer

    WUPSConfigCategoryHandle network_cat;
    err = WUPSConfigAPI_Category_Create({strings.network_category.data()}, &network_cat);
    if (err != WUPSCONFIG_API_RESULT_SUCCESS) return report_error(err);

    WUPSConfigItemHandle connect_item;
    err = WUPSConfigItemBoolean_CreateEx(
        "connect_to_network", strings.connect_to_network_setting.data(), true, Config::connect_to_network,
        &connect_to_network_changed, strings.setting_yes.data(), strings.setting_no.data(), &connect_item);
    if (err != WUPSCONFIG_API_RESULT_SUCCESS) return report_error(err);

    err = WUPSConfigAPI_Category_AddItem(network_cat, connect_item);
    if (err != WUPSCONFIG_API_RESULT_SUCCESS) return report_error(err);

    const uint16_t port = get_console_peertopeer_port();
    char buffer[256];
    snprintf(buffer, sizeof(buffer), strings.multiplayer_port_display.data(), port);

    WUPSConfigItemHandle multiplayer_port_display;
    err = WUPSConfigItemStub_Create(buffer, &multiplayer_port_display);
    if (err != WUPSCONFIG_API_RESULT_SUCCESS) return report_error(err);

    err = WUPSConfigAPI_Category_AddItem(network_cat, multiplayer_port_display);
    if (err != WUPSCONFIG_API_RESULT_SUCCESS) return report_error(err);

    // Add to root and finish
    err = WUPSConfigAPI_Category_AddCategory(root_cat, network_cat);
    if (err != WUPSCONFIG_API_RESULT_SUCCESS) return report_error(err);

    // Other category
    //    Reset Wara Wara Plaza  Press A
    //    Language               English/Spanish/etc..
    //    Show startup toast     Yes/No

    WUPSConfigCategoryHandle other_cat;
    err = WUPSConfigAPI_Category_Create({strings.other_category.data()}, &other_cat);
    if (err != WUPSCONFIG_API_RESULT_SUCCESS) return report_error(err);

    WUPSConfigItemHandle unregisterTasksItem;
    err = WUPSConfigAPI_Item_Create({
                                        .displayName = strings.reset_wwp_setting.data(),
                                        .context = nullptr,
                                        .callbacks = {
                                            .getCurrentValueDisplay = unregister_task_item_get_display_value,
                                            .getCurrentValueSelectedDisplay = unregister_task_item_get_display_value,
                                            .onSelected = nullptr,
                                            .restoreDefault = nullptr,
                                            .isMovementAllowed = nullptr,
                                            .onCloseCallback = nullptr,
                                            .onInput = unregister_task_item_on_input_cb,
                                            .onInputEx = nullptr,
                                            .onDelete = nullptr
                                        },
                                    }, &unregisterTasksItem);
    if (err != WUPSCONFIG_API_RESULT_SUCCESS) return report_error(err);

    err = WUPSConfigAPI_Category_AddItem(other_cat, unregisterTasksItem);
    if (err != WUPSCONFIG_API_RESULT_SUCCESS) return report_error(err);

    // TODO localise these!
    constexpr std::array<ConfigItemMultipleValuesPair, 14> languages = {
        {
            {0, "Japanese"},
            {1, "English"},
            {2, "French"},
            {3, "German"},
            {4, "Italian"},
            {5, "Spanish"},
            {6, "Simplified Chinese"},
            {7, "Korean"},
            {8, "Dutch"},
            {9, "Portuguese"},
            {10, "Russian"},
            {11, "Traditional Chinese"},
            {13, "System"},
            {14, "English (UwU)"},
        }
    };

    constexpr auto find_lang = [languages](const uint32_t lang) -> int {
        return std::ranges::find_if(languages, [lang](auto const l) { return l.value == lang; }) -
               std::begin(languages);
    };
    constexpr int system_language = find_lang(13 /* System */);
    const int current_language = find_lang(Config::language);

    WUPSConfigItemHandle language_item;
    err = WUPSConfigItemMultipleValues_Create("language", strings.language_setting.data(), system_language,
                                              current_language,
                                              const_cast<ConfigItemMultipleValuesPair *>(languages.data()), // Yikes!
                                              languages.size(), &language_changed, &language_item);
    if (err != WUPSCONFIG_API_RESULT_SUCCESS) return report_error(err);

    err = WUPSConfigAPI_Category_AddItem(other_cat, language_item);
    if (err != WUPSCONFIG_API_RESULT_SUCCESS) return report_error(err);

    WUPSConfigItemHandle startup_toast_item;
    err = WUPSConfigItemBoolean_CreateEx("show_startup_toast", strings.show_startup_toast_setting.data(), true,
                                         Config::show_startup_toast, &show_startup_toast_changed,
                                         strings.setting_yes.data(), strings.setting_no.data(), &startup_toast_item);
    if (err != WUPSCONFIG_API_RESULT_SUCCESS) return report_error(err);

    err = WUPSConfigAPI_Category_AddItem(other_cat, startup_toast_item);
    if (err != WUPSCONFIG_API_RESULT_SUCCESS) return report_error(err);

    // finish up
    err = WUPSConfigAPI_Category_AddCategory(root_cat, other_cat);
    if (err != WUPSCONFIG_API_RESULT_SUCCESS) return report_error(err);

    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
}

static void ConfigMenuClosedCallback() {
    // Save all changes
    WUPSStorageError res = WUPSStorageAPI_SaveStorage(false);
    if (res != WUPS_STORAGE_ERROR_SUCCESS) return report_storage_error(res);

    if (Config::need_relaunch) {
        // Need to reload the console so the patches reset
        OSForceFullRelaunch();
        SYSLaunchMenu();
        Config::need_relaunch = false;
    }
}

void Config::Init() {
    // Init the config api
    WUPSConfigAPIStatus cres =
            WUPSConfigAPI_Init({.name = "Inkay"}, ConfigMenuOpenedCallback, ConfigMenuClosedCallback);
    if (cres != WUPSCONFIG_API_RESULT_SUCCESS) return (void) report_error(cres);

    WUPSStorageError res;
    // Try to get values from storage
    res = WUPSStorageAPI_GetBool(nullptr, "connect_to_network", &Config::connect_to_network);
    if (res == WUPS_STORAGE_ERROR_NOT_FOUND) {
        DEBUG_FUNCTION_LINE_VERBOSE("Connect to network value not found, attempting to migrate/create");

        bool skipPatches = false;
        if (WUPSStorageAPI_GetBool(nullptr, "skipPatches", &skipPatches) == WUPS_STORAGE_ERROR_SUCCESS) {
            // Migrate old config value
            Config::connect_to_network = !skipPatches;
            WUPSStorageAPI_DeleteItem(nullptr, "skipPatches");
        }

        // Add the value to the storage if it's missing.
        res = WUPSStorageAPI_StoreBool(nullptr, "connect_to_network", connect_to_network);
        if (res != WUPS_STORAGE_ERROR_SUCCESS) return report_storage_error(res);
    } else if (res != WUPS_STORAGE_ERROR_SUCCESS) return report_storage_error(res);

    res = WUPSStorageAPI_GetU32(nullptr, "language", &Config::language);
    if (res == WUPS_STORAGE_ERROR_NOT_FOUND) {
        DEBUG_FUNCTION_LINE_VERBOSE("Language value not found, attempting to create");

        // Add the value to the storage.
        res = WUPSStorageAPI_StoreU32(nullptr, "language", Config::language);
        if (res != WUPS_STORAGE_ERROR_SUCCESS) return report_storage_error(res);
    } else if (res != WUPS_STORAGE_ERROR_SUCCESS) return report_storage_error(res);

    res = WUPSStorageAPI_GetBool(nullptr, "show_startup_toast", &Config::show_startup_toast);
    if (res == WUPS_STORAGE_ERROR_NOT_FOUND) {
        DEBUG_FUNCTION_LINE_VERBOSE("Show startup toast value not found, attempting to create");

        // Add the value to the storage if it's missing.
        res = WUPSStorageAPI_StoreBool(nullptr, "show_startup_toast", show_startup_toast);
        if (res != WUPS_STORAGE_ERROR_SUCCESS) return report_storage_error(res);
    } else if (res != WUPS_STORAGE_ERROR_SUCCESS) return report_storage_error(res);

    // Set the language that's currently used
    if (Config::language == inkay_language::System)
        Config::current_language = (inkay_language) get_system_language();
    else
        Config::current_language = (inkay_language) Config::language;

    // Save storage
    res = WUPSStorageAPI_SaveStorage(false);
    if (res != WUPS_STORAGE_ERROR_SUCCESS) return report_storage_error(res);
}
