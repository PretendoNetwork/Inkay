#pragma once

#include <nn/ac/ac_c.h>

NNResult ACGetAssignedAlternativeDns(uint32_t *ip);
NNResult ACGetAssignedGateway(uint32_t *ip);
NNResult ACGetAssignedPreferedDns(uint32_t *ip); // sic
NNResult ACGetAssignedSubnet(uint32_t *ip);
