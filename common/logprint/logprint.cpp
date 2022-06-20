#include "logprint.h"
const static char *OSSLOGLEVELSTRING[] =
{
   "SEVERE",
   "ERROR",
   "EVENT",
   "WARNING",
   "INFO",
   "DEBUG"
};


const char *getLOGLevelDesp(LOG_LEVEL_E level)
{
    if ( (unsigned int)level > (unsigned int)LOG_LEVEL_E::LOG_DEBUG )
   {
      return "Unknow Level" ;
   }
   return OSSLOGLEVELSTRING[(unsigned int)level] ;
}

static int s_printLevel = LOG_LEVEL_E::LOG_INFO;
static int s_direction = LOG_DIRECTION_E::LOG_TO_STD;
static char s_directionstr[32] = {0};

int LOG_GetFileName(char *filename,int len,const char *file)
{
        const char *offset = strrchr(file, '/');

    if (offset)
    {
        snprintf(filename, len, "%s", offset+1);
    }
    else
    {
        snprintf(filename, len, "%s", file);
    }

    return EDB_OK;
}

int LOG_GetTimeString(char *timestr, int len)
{
    struct tm curtm = {0};
    time_t tmptime = time(NULL);

    localtime_r(&tmptime, &curtm);

    snprintf(timestr, len, "%04d.%02d.%02d-%02d:%02d:%02d", curtm.tm_year+1900, 
        curtm.tm_mon+1, curtm.tm_mday, curtm.tm_hour, curtm.tm_min, curtm.tm_sec);

    return EDB_OK;
}



INTERFACE_API int LOG_API_SetPara(int printlevel, int direction, char *directionstr)
{
    if(LOG_LEVEL_E::LOG_SERVER>printlevel || LOG_LEVEL_E::LOG_DEBUG <printlevel)
    {
           printf("error, invalid printlevel(%d)\n", printlevel);
           return EDB_IO;
    }

    s_printLevel = printlevel;

    if(LOG_DIRECTION_E::LOG_TO_STD >direction && LOG_DIRECTION_E::LOG_TO_NULL <direction)
    {
        printf("error,invalid direction(%d)\n", direction);

        return EDB_IO;
    }

    s_direction = direction;

    switch (direction)
    {
    case LOG_DIRECTION_E::LOG_TO_STD:
    case LOG_DIRECTION_E::LOG_TO_NULL:
        break;
    default:
        break;
    }
    return EDB_OK;
}


INTERFACE_API void LOG_API_Print(int level, const char *file, const char *func, int line, const char *fmt, ...)
{
    if(LOG_DIRECTION_E::LOG_TO_NULL==s_direction)
    {
        return;
    }

    if(level>s_printLevel)
    {
        return;
    }
    char filename[32];
    char timestr[32];        
    char buf[512];
    va_list args;
    LOG_GetFileName(filename, sizeof(filename), file);

    //LOG_GetTimeString(timestr, sizeof(timestr));
    va_start(args, fmt);
    snprintf(buf, sizeof(buf), "[%s:%s:%d] %s", filename, func, line, fmt);
    if(LOG_DIRECTION_E::LOG_TO_STD==s_direction)
    {
        vprintf(buf, args);
    }
    va_end(args);
}
