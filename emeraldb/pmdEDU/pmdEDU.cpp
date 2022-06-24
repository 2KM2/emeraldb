#include "pmdEDUEvent.h"
#include  "pmdEDU.h"
#include  "logprint.h"
#include  "pmdEDUMgr.h"
#include  "pmdKRCB.h"
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


const char *getEDUName(EDU_TYPES type)
{
    std::map<EDU_TYPES,std::string>::iterator it=mapEDUName.find(type);
    if(it!=mapEDUName.end())
    {
        return it->second.c_str();
    }

    return "Unknown";
}



bool isSystemEDU(EDU_TYPES type)
{
    std::map<EDU_TYPES,EDU_TYPES>::iterator it=mapEDUTypeSys.find(type);
    return it==mapEDUTypeSys.end()?false:true;
}


pmdEDUCB::pmdEDUCB(pmdEDUMgr *mgr,EDU_TYPES type)
:_type(type)
,_mgr(mgr)
,_status(PMD_EDU_CREATING)
,_id(0)
,_isForced(false)
,_isDisconnected(false)
{

}


struct _eduEntryInfo
{
    EDU_TYPES type;
    int       regResult;
    pmdEntryPoint  entryFunc;//回调函数
};

#define ON_EDUTYPE_TO_ENTRY(type,system,entry,desp) \
    {type,registerEDUName(type,desp,system),entry}

pmdEntryPoint getEntryFuncByType ( EDU_TYPES type )
{
    pmdEntryPoint rt = NULL;
    static const _eduEntryInfo entry[]={
        ON_EDUTYPE_TO_ENTRY(EDU_TYPE_AGENT,false,pmdAgentEntryPoint,"Agent"),
        ON_EDUTYPE_TO_ENTRY(EDU_TYPE_TCPLISTENER,true,pmdTcpListenerEntryPoint,"TCPListener"),
        ON_EDUTYPE_TO_ENTRY(EDU_TYPE_MAXIMUM,false,NULL,"Unknown")
    };

    static const unsigned int number =sizeof(entry) /sizeof(_eduEntryInfo);

    unsigned int index =0;
    for(;index<number;++index)
    {
        if(entry[index].type==type)
        {
            rt = entry[index].entryFunc;
            goto done;
        }
    }
done:
    return rt;
}

int pmdRecv(char *pBuffer,int recvSize,ossSocket *sock,pmdEDUCB *cb)
{
    int rc = EDB_OK;
    assert(sock!=NULL);
    assert(cb!=NULL);
    while(true)
    {
        if(cb->isForced())
        {
            rc = EDB_APP_FORCED;
            goto done;
        }
        rc = sock->recv(pBuffer,recvSize);
        if(EDB_TIMEOUT == rc)
        {
            continue;
        }
        goto done;
    }

done:
    return rc;

}

int pmdSend(const char *pBuffer,int sendSize,ossSocket *sock,pmdEDUCB *cb)
{
    int rc = EDB_OK;
    assert(sock!=NULL);
    assert(cb!=NULL);

    while (true)
    {
        if(cb->isForced())
        {
            rc = EDB_APP_FORCED;
            goto done;
        }
        rc = sock->send(pBuffer,sendSize);
        if(EDB_TIMEOUT == rc)
        {
            continue;
        }
        goto done ;
    }
    
done:
    return rc;

}
//线程运行函数
int pmdEDUEntryPoint( EDU_TYPES type,pmdEDUCB *cb, void *arg)
{
    int rc = EDB_OK;
    EDUID myEDUID = cb->getID();
    pmdEDUMgr *eduMgr = cb->getEDUMgr();
    pmdEDUEvent event;
    bool eduDestroyed = false;
    bool  isForced = false;
    //main loop
    while(!eduDestroyed)
    {
        type = cb->getType();
        //wait for 1000 millseconds for event
        // 1秒等到一次事件
        if(!cb->waitEvent(event,1000))
        {
            if(cb->isForced())
            {
                OSS_LOG(LOG_EVENT,"EDU %lld is forced ",myEDUID);
                isForced = true;
            }
            else 
                continue;
        }
        //PMD_EDU_EVENT_RESUME事件
        if(!isForced && PMD_EDU_EVENT_RESUME ==event._eventType)
        {
            //set EDU status to wait
            eduMgr->waitEDU(myEDUID);
            pmdEntryPoint entryFunc = getEntryFuncByType(type);
            if(!entryFunc)
            {
                OSS_LOG(LOG_ERROR, "EDU %lld type %d entry point func is NULL",myEDUID, type ) ;
                EDB_SHUTDOWN_DB;
                rc = EDB_SYS ;
            }
            else
            {
                //传入pmdEDU和数据
                rc = entryFunc(cb,event._Data);
            }

            if(EDB_IS_DB_UP)
            {
                if(isSystemEDU(cb->getType()))
                {
                    OSS_LOG ( LOG_ERROR, "System EDU: %lld, type %s exits with %d",myEDUID, getEDUName(type), rc ) ;
                     EDB_SHUTDOWN_DB
                }
                else if(rc)
                {

                }
            }
            eduMgr->waitEDU(myEDUID);
        }
        else if(!isForced &&PMD_EDU_EVENT_TERM !=event._eventType)
        {
            OSS_LOG(LOG_ERROR, "Receive the wrong event %d in EDU %lld, type %s",
                  event._eventType, myEDUID, getEDUName(type) ) ;

            rc = EDB_SYS;
        }
        else if ( isForced && PMD_EDU_EVENT_TERM == event._eventType && cb->isForced() )
        {
          OSS_LOG ( LOG_ERROR, "EDU %lld, type %s is forced", myEDUID, type ) ;
             isForced = true ;
        }

        if ( !isForced && event._Data && event._release )
        {
           free ( event._Data ) ;
           event.reset () ;
        }

          rc = eduMgr->returnEDU ( myEDUID, isForced, &eduDestroyed ) ;

        if ( rc )
        {
           OSS_LOG ( LOG_ERROR, "Invalid EDU Status for EDU: %lld, type %s", myEDUID, getEDUName(type) ) ;
        }
        OSS_LOG ( LOG_DEBUG, "Terminating thread for EDU: %lld, type %s",myEDUID, getEDUName(type) ) ;
    }
    return 0;
}









