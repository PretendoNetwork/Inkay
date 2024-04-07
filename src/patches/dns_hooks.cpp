#include <wups.h>
#include <netdb.h>

#include "utils/logger.h"
#include "config.h"
#include <array>

const std::pair<const char *, const char *> dns_replacements[] = {
        // NNCS servers. TODO use Pretendo instead of borrowing Nintendo's
        { "nncs1.app.nintendowifi.net", "nncs1-lp1.n.n.srv.nintendo.net" },
        { "nncs2.app.nintendowifi.net", "nncs2-lp1.n.n.srv.nintendo.net" },
};

static const char * replace_dns_name(const char *dns_name) {
    DEBUG_FUNCTION_LINE("DNS request for %s", dns_name);

//    if (!Config::connect_to_network) return dns_name;

    for (auto [original, replacement] : dns_replacements) {
        if (strcmp(original, dns_name) == 0)
            return replacement;
    }

    return dns_name;
}

DECL_FUNCTION(struct hostent *, gethostbyname, const char *dns_name) {
    return real_gethostbyname(replace_dns_name(dns_name));
}
// might need a WUPS_MUST_REPLACE_FOR_PROCESS for Friends
WUPS_MUST_REPLACE(gethostbyname, WUPS_LOADER_LIBRARY_NSYSNET, gethostbyname);

DECL_FUNCTION(int, getaddrinfo, const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {
    return real_getaddrinfo(replace_dns_name(node), service, hints, res);
}
WUPS_MUST_REPLACE(getaddrinfo, WUPS_LOADER_LIBRARY_NSYSNET, getaddrinfo);