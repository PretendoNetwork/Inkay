//
// Created by ash on 10/12/22.
//

#include "config.h"

#include "utils/logger.h"
#include <wups.h>
#include <wups/config/WUPSConfigItemBoolean.h>

#include <coreinit/launch.h>
#include <sysapp/launch.h>

bool Config::connect_to_network = true;
bool Config::need_relaunch = false;

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
}

static void connect_to_network_changed(ConfigItemBoolean* item, bool new_value) {
    DEBUG_FUNCTION_LINE("New value in skipPatchesChanged: %d", new_value);
    if (new_value != Config::connect_to_network) {
        Config::need_relaunch = true;
    }
    Config::connect_to_network = new_value;
    WUPS_StoreInt(nullptr, "connect_to_network", Config::connect_to_network);
}

WUPS_GET_CONFIG() {
    // We open the storage so we can persist the configuration the user did.
    if (WUPS_OpenStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("Failed to open storage");
        return 0;
    }

    WUPSConfigHandle config;
    WUPSConfig_CreateHandled(&config, "Inkay");

    WUPSConfigCategoryHandle cat;
    WUPSConfig_AddCategoryByNameHandled(config, "Patching", &cat);

    WUPSConfigItemBoolean_AddToCategoryHandled(config, cat, "connect_to_network", "Connect to the Pretendo network", Config::connect_to_network, &connect_to_network_changed);

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
