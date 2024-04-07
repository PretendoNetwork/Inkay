//
// Created by ash on 10/12/22.
//

#include "config.h"

#include "wut_extra.h"
#include "utils/logger.h"
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

static int32_t unregister_task_item_get_display_value(void *context, char *out_buf, int32_t out_size) {
    if(!Config::is_wiiu_menu)
        strncpy(out_buf, "From WiiU menu only", out_size);
    else
        strncpy(out_buf, Config::unregister_task_item_pressed ? "Restart to apply" : "Press A", out_size);
    return 0;
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

WUPS_GET_CONFIG() {
    // We open the storage so we can persist the configuration the user did.
    if (WUPS_OpenStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("Failed to open storage");
        return 0;
    }

    WUPSConfigHandle config;
    WUPSConfig_CreateHandled(&config, "Inkay");

    WUPSConfigCategoryHandle patching_cat;
    WUPSConfig_AddCategoryByNameHandled(config, "Patching", &patching_cat);

    WUPSConfigItemBoolean_AddToCategoryHandled(config, patching_cat, "connect_to_network", "Connect to the Pretendo network", Config::connect_to_network, &connect_to_network_changed);
    
    WUPSConfigCategoryHandle boss_cat;
    WUPSConfig_AddCategoryByNameHandled(config, "BOSS settings", &boss_cat);

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
    WUPSConfigItem_Create(&unregisterTasksItem, "unregister_task_item", "Unregister Wara Wara Plaza BOSS tasks", unregisterTasksItemCallbacks, &Config::unregister_task_item_pressed);
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
