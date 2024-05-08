//
// Created by ash on 10/12/22.
//

#ifndef INKAY_CONFIG_H
#define INKAY_CONFIG_H

class Config {
public:
    static void Init();

    // wups config items
    static bool connect_to_network;

    // private stuff
    static bool need_relaunch;
    
    // private stuff
    static bool is_wiiu_menu;

    static bool unregister_task_item_pressed;
	
	// for notifications to work correctly in aroma beta 17+
	static bool has_displayed_popup;
};

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

#endif //INKAY_CONFIG_H
