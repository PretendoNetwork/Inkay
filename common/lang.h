#pragma once

#include <string_view>

enum inkay_language {
    // Wii U System Languages
    Japanese = 0,
    English,
    French,
    German,
    Italian,
    Spanish,
    SimplifiedChinese,
    Korean,
    Dutch,
    Portuguese,
    Russian,
    TraditionalChinese,
    Invalid,
    // Custom Languages
    System,
    EnUwU,
};

struct config_strings {
    const char *plugin_name;
    std::string_view network_category;
    std::string_view connect_to_network_setting;
    std::string_view other_category;
    std::string_view reset_wwp_setting;
    std::string_view press_a_action;
    std::string_view restart_to_apply_action;
    std::string_view need_menu_action;
    std::string_view using_nintendo_network;
    std::string_view using_pretendo_network;
    std::string_view multiplayer_port_display;
    std::string_view module_not_found;
    std::string_view module_init_not_found;
};

config_strings get_config_strings(inkay_language language);
