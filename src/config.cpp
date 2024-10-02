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
#include <wups/config_api.h>
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

static config_strings strings;

config_strings get_config_strings(nn::swkbd::LanguageType language) {
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
                .need_menu_action = "From WiiU menu only",
                .using_nintendo_network = "Using Nintendo Network",
                .using_pretendo_network = "Using Pretendo Network",
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
                .using_nintendo_network = "Usando Nintendo Network",
                .using_pretendo_network = "Usando Pretendo Network",
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
                .using_nintendo_network = "Sur Nintendo Network",
                .using_pretendo_network = "Sur Pretendo Network",
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
                .using_nintendo_network = "Usando Nintendo Network",
                .using_pretendo_network = "Usando Pretendo Network",
            };

        case nn::swkbd::LanguageType::German:
            return {
                .plugin_name = "Inkay",
                .network_category = "Netzwerkauswahl",
                .connect_to_network_setting = "Verbinde zum Pretendo Network",
                .other_category = "Andere Einstellungen",
                .reset_wwp_setting = "Wara Wara Plaza zurücksetzen",
                .press_a_action = "Drücke A",
                .restart_to_apply_action = "Neustarten zum Anwenden",
                .need_menu_action = "Nur vom Wii U-Menü aus",
                .using_nintendo_network = "Nutze Nintendo Network",
                .using_pretendo_network = "Nutze Pretendo Network",
            };

        case nn::swkbd::LanguageType::SimplifiedChinese:
            return {
                .plugin_name = "Inkay",
                .network_category = "选择网络",
                .connect_to_network_setting = "连接到Pretendo network",
                .other_category = "其他设置",
                .reset_wwp_setting = "重置Wara Wara Plaza",
                .press_a_action = "请按 A",
                .restart_to_apply_action = "重启以应用设置",
                .need_menu_action = "仅来自WiiU Menu",
                .using_nintendo_network = "使用 Nintendo Network",
                .using_pretendo_network = "使用 Pretendo Network",
            };

        case nn::swkbd::LanguageType::TraditionalChinese:
            return {
                .plugin_name = "Inkay",
                .network_category = "選擇網路",
                .connect_to_network_setting = "連接到Pretendo network",
                .other_category = "其他設定",
                .reset_wwp_setting = "重置Wara Wara Plaza",
                .press_a_action = "請按 A",
                .restart_to_apply_action = "重啓以套用設定",
                .need_menu_action = "僅來自WiiU Menu",
                .using_nintendo_network = "使用 Nintendo Network",
                .using_pretendo_network = "使用 Pretendo Network",
            };

        case nn::swkbd::LanguageType::Portuguese:
            return {
                    .plugin_name = "Inkay",
                    .network_category = "Selecionar rede",
                    .connect_to_network_setting = "Conecta-se à Pretendo Network",
                    .other_category = "Outras configurações",
                    .reset_wwp_setting = "Resetar Wara Wara Plaza",
                    .press_a_action = "Aperte A",
                    .restart_to_apply_action = "Reinicie para aplicar",
                    .need_menu_action = "Apenas no menu do Wii U",
                    .using_nintendo_network = "Usando Nintendo Network",
                    .using_pretendo_network = "Usando Pretendo Network",
            };

        case nn::swkbd::LanguageType::Japanese:
            return {
                    .plugin_name = "Inkay",
                    .network_category = "ネットワークの選択",
                    .connect_to_network_setting = "Pretendoネットワークに接続",
                    .other_category = "その他の設定",
                    .reset_wwp_setting = "わらわら広場をリセット",
                    .press_a_action = "Aボタンを押す",
                    .restart_to_apply_action = "再起動して適用",
                    .need_menu_action = "WiiUメニューからのみ実行可能",
                    .using_nintendo_network = "ニンテンドーネットワークを使用中",
                    .using_pretendo_network = "Pretendoネットワークを使用中",
            };

        case nn::swkbd::LanguageType::Dutch:
            return {
                    .plugin_name = "Inkay",
                    .network_category = "Netwerkselectie",
                    .connect_to_network_setting = "Verbind met het Pretendo-netwerk",
                    .other_category = "Overige instellingen",
                    .reset_wwp_setting = "Reset het Wara Wara Plaza",
                    .press_a_action = "Druk A",
                    .restart_to_apply_action = "Herstart om toe te passen",
                    .need_menu_action = "Alleen vanuit het WiiU-menu",
                    .using_nintendo_network = "Nintendo Network wordt gebruikt",
                    .using_pretendo_network = "Pretendo Network wordt gebruikt",
            };

        case nn::swkbd::LanguageType::Russian:
            return {
                    .plugin_name = "Inkay",
                    .network_category = "Выбор сети",
                    .connect_to_network_setting = "Подключиться к Pretendo Network",
                    .other_category = "Другие настройки",
                    .reset_wwp_setting = "Сбросить Wara Wara Plaza",
                    .press_a_action = "Нажмите A",
                    .restart_to_apply_action = "Перезагрузите для применения изменений",
                    .need_menu_action = "Только из меню Wii U",
                    .using_nintendo_network = "Используется Nintendo Network",
                    .using_pretendo_network = "Используется Pretendo Network",
            };
    }
}

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
