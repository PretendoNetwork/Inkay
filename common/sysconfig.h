//
// Created by ash on 9/04/24.
//

#ifndef INKAY_SYSCONFIG_H
#define INKAY_SYSCONFIG_H

#include <nn/swkbd.h>
#include <coreinit/mcp.h>

nn::swkbd::LanguageType get_system_language();
const char * get_console_serial();
MCPSystemVersion get_console_os_version();
unsigned short get_console_peertopeer_port();

#endif //INKAY_SYSCONFIG_H
