#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <whb/log.h>
#include <whb/log_module.h>
#include <whb/log_cafe.h>
#include <whb/log_udp.h>

#define __FILENAME_X__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILENAME_X__)

#define OSFATAL_FUNCTION_LINE(FMT, ARGS...)do { \
    OSFatal_printf("[(P)             Inkay][%23s]%30s@L%04d: " FMT "",__FILENAME__,__FUNCTION__, __LINE__, ## ARGS); \
    } while (0)

#define DEBUG_FUNCTION_LINE(FMT, ARGS...)do { \
    WHBLogPrintf("[(P)             Inkay][%23s]%30s@L%04d: " FMT "",__FILENAME__,__FUNCTION__, __LINE__, ## ARGS); \
    } while (0);

#define DEBUG_FUNCTION_LINE_WRITE(FMT, ARGS...)do { \
    WHBLogWritef("[(P)             Inkay][%23s]%30s@L%04d: " FMT "",__FILENAME__,__FUNCTION__, __LINE__, ## ARGS); \
    } while (0);

#ifdef DEBUG
#define DEBUG_FUNCTION_LINE_VERBOSE(FMT, ARGS...) DEBUG_FUNCTION_LINE(FMT, ##ARGS)
#else
#define DEBUG_FUNCTION_LINE_VERBOSE(FMT, ARGS...) while (0)
#endif

#ifdef __cplusplus
}
#endif
