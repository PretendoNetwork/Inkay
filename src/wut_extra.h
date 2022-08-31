#pragma once

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

//remove all this if wut adds it and it causes errors
typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    char region;
} MCP_SystemVersion;
static_assert(sizeof(MCP_SystemVersion) == 0x10);
int MCP_GetSystemVersion(int handle, MCP_SystemVersion* version);

#ifdef __cplusplus
}
#endif
