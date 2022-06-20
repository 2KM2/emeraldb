#pragma once
#include "linuxinclude.h"
#include "commontype.h"
#include "commondef.h"
#define OSS_LOG_STRINGMAX 4096

typedef enum _log_direction_e
{
    LOG_TO_STD  = 0,
    LOG_TO_FILE = 1,
    LOG_TO_NULL = 2
} LOG_DIRECTION_E;

typedef enum _log_level_e
{
    LOG_SERVER  = 0,
    LOG_ERROR   = 1,
    LOG_EVENT   = 2,
    LOG_WARNING = 3,
    LOG_INFO    = 4,
    LOG_DEBUG   = 5,
} LOG_LEVEL_E;
const char *getLOGLevelDesp(LOG_LEVEL_E level);

INTERFACE_API int LOG_API_SetPara(int printlevel, int direction, char *directionstr);

INTERFACE_API void LOG_API_Print(LOG_LEVEL_E level, const char *file, const char *func, int line, const char *fmt, ...);

#define OSS_LOG(level,fmt, ...) LOG_API_Print(level,__FILE__,__FUNCTION__,__LINE__,fmt,##__VA_ARGS__)

