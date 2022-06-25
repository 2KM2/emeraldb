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




#define OSS_LOG(level,fmt, ...) LOG_API_Print(level,__FILE__,__FUNCTION__,__LINE__,fmt,##__VA_ARGS__)
#define EDB_ASSERT(cond,str)  {if(cond){ }}
#define EDB_CHECK(cond,str)   {if(cond){}}

#define PD_CHECK(cond,retCode,gotoLabel,level,fmt,...)                \
   do {                                                               \
      if ( !(cond) )                                                  \
      {                                                               \
         rc = (retCode) ;                                             \
         OSS_LOG  ( (level), fmt, ##__VA_ARGS__) ;                     \
         goto gotoLabel ;                                             \
      }                                                               \
   } while ( 0 )                                                      \

#define PD_RC_CHECK(rc,level,fmt,...)                                 \
   do {                                                               \
      PD_CHECK ( (EDB_OK==(rc)), (rc), error, (level),                \
                 fmt, ##__VA_ARGS__) ;                                \
   } while ( 0 )                                                      \


#define EDB_VALIDATE_GOTOERROR(cond,ret,str) if(!(cond)) { LOG_API_Print(LOG_ERROR, __func__, __FILE__, __LINE__, str) ;  rc = ret; goto error ; }
    
const char *getLOGLevelDesp(LOG_LEVEL_E level);
INTERFACE_API int LOG_API_SetPara(int printlevel, int direction, char *directionstr);

INTERFACE_API void LOG_API_Print(LOG_LEVEL_E level, const char *file, const char *func, int line, const char *fmt, ...);