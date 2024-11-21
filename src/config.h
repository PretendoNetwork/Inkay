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

#endif //INKAY_CONFIG_H
