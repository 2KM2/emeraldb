#include "pmdEDUEvent.h"
#include  "pmdEDU.h"
#include  "logprint.h"

static std::map<EDU_TYPES,std::string> mapEDUName;
static std::map<EDU_TYPES,EDU_TYPES> mapEDUTypeSys;


//注册edu名字
int  registerEDUName(EDU_TYPES type,const char *name,bool system)
{
    int rc = EDB_OK;
    std::map<EDU_TYPES,std::string>::iterator it = mapEDUName.find(type);

    if(it!=mapEDUName.end())
    {
        OSS_LOG( LOG_ERROR,"EDU Type conflict[type:%d,%s<->%s",(int)type, it->second.c_str(), name ) ;
        rc =EDB_SYS;
        goto error ;
    }
    mapEDUName[type]=std::string(name);
    if(system)
    {
        mapEDUTypeSys[type]=type;
    }
done:
    return rc;
error:
    goto done;
}