//
// Created by ash on 10/12/22.
//

#ifndef INKAY_CONFIG_H
#define INKAY_CONFIG_H

#include <stdint.h>
#include "lang.h"

class Config {
public:
    static void Init();

    // wups config items
    static bool connect_to_network;
    static uint32_t language;

    // private stuff
    static bool need_relaunch;

    // private stuff
    static bool is_wiiu_menu;

    static inkay_language current_language;
    static bool unregister_task_item_pressed;
};

#endif //INKAY_CONFIG_H
