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
#include "utils/sysconfig.h"
#include <wups.h>
#include <wups/storage.h>
#include <wups/config.h>
#include <wups/config/WUPSConfigItemBoolean.h>

#include <coreinit/title.h>
#include <coreinit/launch.h>
#include <sysapp/title.h>
#include <sysapp/launch.h>
#include <nn/act.h>

bool Config::connect_to_network = true;
bool Config::need_relaunch = false;
bool Config::unregister_task_item_pressed = false;
bool Config::is_wiiu_menu = false;

void Config::Init() {
    WUPSStorageError storageRes = WUPS_OpenStorage();
    if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("Failed to open storage %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);
    }
    else {
        // Try to get value from storage
        if ((storageRes = WUPS_GetBool(nullptr, "connect_to_network", &connect_to_network)) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            bool skipPatches = false;
            if ((storageRes = WUPS_GetBool(nullptr, "skipPatches", &skipPatches)) != WUPS_STORAGE_ERROR_NOT_FOUND) {
                // Migrate old config value
                connect_to_network = !skipPatches;
            }
            // Add the value to the storage if it's missing.
            if (WUPS_StoreBool(nullptr, "connect_to_network", connect_to_network) != WUPS_STORAGE_ERROR_SUCCESS) {
                DEBUG_FUNCTION_LINE("Failed to store bool");
            }
        }
        else if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE("Failed to get bool %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);
        }

        // Close storage
        if (WUPS_CloseStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE("Failed to close storage");
        }
    }
    
    uint64_t current_title_id = OSGetTitleID();
    uint64_t wiiu_menu_tid = _SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_WII_U_MENU);
    Config::is_wiiu_menu = (current_title_id == wiiu_menu_tid);
}

static void connect_to_network_changed(ConfigItemBoolean* item, bool new_value) {
    DEBUG_FUNCTION_LINE("New value in skipPatchesChanged: %d", new_value);
    if (new_value != Config::connect_to_network) {
        Config::need_relaunch = true;
    }
    Config::connect_to_network = new_value;
    WUPS_StoreInt(nullptr, "connect_to_network", Config::connect_to_network);
}

static void unregister_task_item_pressed_cb(void *context, WUPSConfigButtons button) {
    if (!Config::unregister_task_item_pressed && Config::is_wiiu_menu && button == WUPS_CONFIG_BUTTON_A) {

        nn::act::Initialize();
        Initialize__Q2_2nn4bossFv();

        for (uint8_t i = 1; i <= nn::act::GetNumOfAccounts(); i++)
        {
            if (nn::act::IsSlotOccupied(i) == true)
            {
                if (nn::act::IsNetworkAccountEx(i) == true)
                {
                    nn::boss::Task task;
                    nn::act::PersistentId persistentId = nn::act::GetPersistentIdEx(i);

                    __ct__Q3_2nn4boss4TaskFv(&task);
                    Initialize__Q3_2nn4boss4TaskFPCcUi(&task, "oltopic", persistentId);

                    uint32_t res = Unregister__Q3_2nn4boss4TaskFv(&task);
                    WHBLogPrintf("Unregistered oltopic for: SlotNo %d | Persistent ID %08x -> 0x%08x", i, persistentId, res);
                }
            }
        }

        Finalize__Q2_2nn4bossFv();
        nn::act::Finalize();

        Config::unregister_task_item_pressed = !Config::unregister_task_item_pressed;
        Config::need_relaunch = true;
    }
}

static bool unregister_task_item_is_movement_allowed(void *context) {
    return true;
}

struct config_strings {
    const char* plugin_name;
    const char* network_category;
    const char* connect_to_network_setting;
    const char* other_category;
    const char* reset_wwp_setting;
    const char* press_a_action;
    const char* restart_to_apply_action;
    const char* need_menu_action;
};
config_strings strings;

constexpr config_strings get_config_strings(nn::swkbd::LanguageType language) {
    switch (language) {
        case nn::swkbd::LanguageType::English:
        default:
            return {
                .plugin_name = "Inkay",
                .network_category = "Network selection",
                .connect_to_network_setting = "Connect to the Pretendo network",
                .other_category = "Other settings",
                .reset_wwp_setting = "Reset Wara Wara Plaza",
                .press_a_action = "Press A",
                .restart_to_apply_action = "Restart to apply",
                .need_menu_action = "From WiiU menu only"
            };
        case nn::swkbd::LanguageType::Spanish:
            return {
                .plugin_name = "Inkay",
                .network_category = "Selección de red",
                .connect_to_network_setting = "Conectar a la red Pretendo",
                .other_category = "Otros ajustes",
                .reset_wwp_setting = "Restablecer Wara Wara Plaza",
                .press_a_action = "Pulsa A",
                .restart_to_apply_action = "Reinicia para confirmar",
                .need_menu_action = "Sólo desde el menú de WiiU",
            };
        case nn::swkbd::LanguageType::French:
            return {
                .plugin_name = "Inkay",
                .network_category = "Sélection du réseau",
                .connect_to_network_setting = "Connexion à Pretendo",
                .other_category = "Autres paramètres",
                .reset_wwp_setting = "Réinitialiser la place WaraWara",
                .press_a_action = "Appuyez sur A",
                .restart_to_apply_action = "Redémarrer pour appliquer",
                .need_menu_action = "Depuis le menu Wii U seulement",
            };
        case nn::swkbd::LanguageType::Italian:
            return {
                .plugin_name = "Inkay",
                .network_category = "Selezione della rete",
                .connect_to_network_setting = "Connettiti alla rete Pretendo",
                .other_category = "Altre categorie",
                .reset_wwp_setting = "Ripristina Wara Wara Plaza",
                .press_a_action = "Premi A",
                .restart_to_apply_action = "Riavvia per applicare",
                .need_menu_action = "Solo dal menu WiiU",
            };
    }
}

static int32_t unregister_task_item_get_display_value(void *context, char *out_buf, int32_t out_size) {
    if(!Config::is_wiiu_menu)
        strncpy(out_buf, strings.need_menu_action, out_size);
    else
        strncpy(out_buf, Config::unregister_task_item_pressed ? strings.restart_to_apply_action : strings.press_a_action, out_size);
    return 0;
}

WUPS_GET_CONFIG() {
    // We open the storage so we can persist the configuration the user did.
    if (WUPS_OpenStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("Failed to open storage");
        return 0;
    }

    strings = get_config_strings(get_system_language());

    WUPSConfigHandle config;
    WUPSConfig_CreateHandled(&config, strings.plugin_name);

    WUPSConfigCategoryHandle patching_cat;
    WUPSConfig_AddCategoryByNameHandled(config, strings.network_category, &patching_cat);

    WUPSConfigItemBoolean_AddToCategoryHandled(config, patching_cat, "connect_to_network", strings.connect_to_network_setting, Config::connect_to_network, &connect_to_network_changed);
    
    WUPSConfigCategoryHandle boss_cat;
    WUPSConfig_AddCategoryByNameHandled(config, strings.other_category, &boss_cat);

    WUPSConfigCallbacks_t unregisterTasksItemCallbacks = {
        .getCurrentValueDisplay = unregister_task_item_get_display_value,
        .getCurrentValueSelectedDisplay = unregister_task_item_get_display_value,
        .onSelected = nullptr,
        .restoreDefault = nullptr,
        .isMovementAllowed = unregister_task_item_is_movement_allowed,
        .callCallback = nullptr,
        .onButtonPressed = unregister_task_item_pressed_cb,
        .onDelete = nullptr
    };

    WUPSConfigItemHandle unregisterTasksItem;
    WUPSConfigItem_Create(&unregisterTasksItem, "unregister_task_item", strings.reset_wwp_setting, unregisterTasksItemCallbacks, &Config::unregister_task_item_pressed);
    WUPSConfigCategory_AddItem(boss_cat, unregisterTasksItem);

    return config;
}

WUPS_CONFIG_CLOSED() {
    // Save all changes
    if (WUPS_CloseStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("Failed to close storage");
    }

    if (Config::need_relaunch) {
        // Need to reload the console so the patches reset
        OSForceFullRelaunch();
        SYSLaunchMenu();
        Config::need_relaunch = false;
    }
}
