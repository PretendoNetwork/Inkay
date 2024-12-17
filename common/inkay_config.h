//
// Created by ash on 17/12/24.
//

#ifndef INKAY_INKAY_CONFIG_H
#define INKAY_INKAY_CONFIG_H

#ifdef __has_include
#if __has_include("inkay_config.local.h")

#include "inkay_config.local.h"
#define INKAY_CUSTOM 1

#endif
#endif

#ifndef NETWORK_BASEURL
#define NETWORK_BASEURL "pretendo.cc"
#endif

#endif //INKAY_INKAY_CONFIG_H
