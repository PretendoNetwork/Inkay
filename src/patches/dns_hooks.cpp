/*  Copyright 2024 Pretendo Network contributors <pretendo.network>
    Copyright 2024 Ash Logan <ash@heyquark.com>

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

#include <netdb.h>


#include "config.h"
#include "utils/logger.h"
#include <array>
#include <vector>
#include <function_patcher/function_patching.h>

std::vector<PatchedFunctionHandle> dns_patches;

const std::pair<const char *, const char *> dns_replacements[] = {
        // NNCS servers
        { "nncs1.app.nintendowifi.net", "nncs1.app.pretendo.cc" },
        { "nncs2.app.nintendowifi.net", "nncs2.app.pretendo.cc" },
};

static const char * replace_dns_name(const char *dns_name) {
    if (!Config::connect_to_network) return dns_name;

    for (auto [original, replacement] : dns_replacements) {
        if (strcmp(original, dns_name) == 0)
            return replacement;
    }

    return dns_name;
}

DECL_FUNCTION(struct hostent *, gethostbyname, const char *dns_name) {
    return real_gethostbyname(replace_dns_name(dns_name));
}

DECL_FUNCTION(int, getaddrinfo, const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {
    return real_getaddrinfo(replace_dns_name(node), service, hints, res);
}

void patchDNS() {
    dns_patches.reserve(2);

    auto add_patch = [](function_replacement_data_t repl, const char *name) {
        PatchedFunctionHandle handle = 0;
        if (FunctionPatcher_AddFunctionPatch(&repl, &handle, nullptr) != FUNCTION_PATCHER_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE("Inkay/DNS: Failed to patch %s!", name);
        }
        dns_patches.push_back(handle);
    };

    // might need a REPLACE_FUNCTION_FOR_PROCESS for Friends
    add_patch(REPLACE_FUNCTION(gethostbyname, LIBRARY_NSYSNET, gethostbyname), "gethostbyname");

    add_patch(REPLACE_FUNCTION(getaddrinfo, LIBRARY_NSYSNET, getaddrinfo), "getaddrinfo");
}

void unpatchDNS() {
    for (auto handle: dns_patches) {
        FunctionPatcher_RemoveFunctionPatch(handle);
    }
    dns_patches.clear();
}
