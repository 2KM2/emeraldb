#pragma once
#include "core.hpp"
#include <string>
#define OSS_LOG_STRINGMAX 4096



#define OSS_LOG(level,fmt,...)                                          \
    do {                                                                \
     if(_curLogLevel >=level)                                           \
     {                                                                  \
        ossLog(level,__func__,__FILE__,__LINE__,fmt,##__VA_ARGS__);     \
     }                                                                  \
    }while(0)                                                           \

//check cond whether  ok
#define OSS_CHECK(cond, retCode,gotoLabel,level,fmt, ...)               \
    do {                                                                \
        if(!(cond))                                                     \
        {                                                               \
            rc = (retCode);                                             \
            OSS_LOG((level),fmt,##__VA_ARGS__);                         \
            goto gotoLabel;                                             \
        }                                                               \                                                  
    }while (0)                                                          \


#define OSS_RC_CHECK(rc,level,fmt, ...)                                \
    do{                                                                \
        OSS_CHECK((EDB_OK==(rc)),(rc),error,(level),fmt,##__VA_ARGS__) \
    }while(0)                                                          \


#define EDB_VALIDATE_GOTOERROR(cond,ret,str)                            \
   {if(!(cond)) { ossLog(LOGERROR, __func__, __FILE__, __LINE__, str) ; \
    rc = ret; goto error ; }}                                           \


#ifdef _DEBUG
#define EDB_ASSERT(cond,str)                                          \
   { if ( !(cond) ) {                                                 \
      ossassert ( str,__func__, __FILE__,__LINE__);}}                  \

#define EDB_CHECK(cond,str)                                           \
   { if ( !(cond) ) {                                                 \
      osscheck ( str, __func__, __FILE__, __LINE__ ) ; }}              \

#else
#define EDB_ASSERT(cond,str)  {if(cond){}}
#define EDB_CHECK(cond,str)   {if(cond){}}
#endif



typedef enum LOGLEVEL
{
    LOGSEVERE = 0,
    LOGERROR,
    LOGEVENT,
    LOGWARNING,
    LOGINFO,
    LOGDEBUG
}LOGLEVEL;
extern LOGLEVEL _curLogLevel;

const char *getLOGLevelDesp(LOGLEVEL level);

#define LOG_DEF_DIAGLEVEL  LOGWARNING 

void ossLog(LOGLEVEL level,const char *func,const char *file,unsigned int  line,const char *format, ...);
void ossLog(LOGLEVEL level,const char *func,const char* file,unsigned int line,std::string message);
//void ossassert(const char *str,const char *func,const char *file,unsigned int line);
//void osscheck(const char *str,const char *func,const char *file,unsigned int line);