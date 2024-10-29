//
// Created by ash on 10/12/22.
//

#ifndef INKAY_CONFIG_H
#define INKAY_CONFIG_H

#include <string_view>
#include <nn/swkbd.h>

class Config {
public:
    static bool connect_to_network;

    static bool initialized;

    static bool shown_uninitialized_warning;
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

config_strings get_config_strings(nn::swkbd::LanguageType language);

#endif //INKAY_CONFIG_H
