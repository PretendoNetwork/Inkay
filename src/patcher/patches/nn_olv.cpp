#include "nn_olv.h"
#include "utils/logger.h"
#include "patcher/patcher.h"

#include <cstring>

const char original_url[] = "discovery.olv.nintendo.net/v1/endpoint";
const char new_url[] =      "discovery.olv.pretendo.cc/v1/endpoint";
_Static_assert(sizeof(original_url) > sizeof(new_url),
               "new_url too long! Must be less than 38chars.");

std::optional<OSDynLoad_NotifyData> checkForOlvLibs(const rplinfo& rpls) {
    auto res = FindRPL(rpls, "nn_olv.rpl");
    if (res) return res;

    res = FindRPL(rpls, "nn_olv2.rpl");
    if (res) return res;

    return std::nullopt;
}

void Patch_nn_olv(uint32_t titleVer, uint64_t titleId, const rplinfo& rpls) {
    auto olv_rpl = checkForOlvLibs(rpls);
    if (!olv_rpl) {
        DEBUG_FUNCTION_LINE("olv not loaded?");
        return;
    }

    replace_string(olv_rpl->dataAddr, olv_rpl->dataSize, original_url, sizeof(original_url), new_url, sizeof(new_url));
}
