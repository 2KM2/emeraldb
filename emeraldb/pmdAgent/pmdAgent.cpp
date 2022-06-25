#include "logprint.h"
#include  "linuxinclude.h"
#include  "commontype.h"
#include  "commondef.h"
#include  "ossSocket.h"
#include  "bson/src/bson.h"
#include   "pmdEDU.h"
#include   "pmdKRCB.h"
#include   "pmdEDUMgr.h"

using namespace std;
using namespace bson;

//x变成y的整数倍
#define ossRoundUpToMultipleX(x,y) (((x)+((y)-1))-(((x)+((y)-1))%(y)))
#define PMD_AGENT_RECIEVE_BUFFER_SZ 4096
#define EDB_PAGE_SIZE               4096

struct MsgReply
{
    int a;
};
static int pmdProcessAgentRequest ( char *pReceiveBuffer,
                                    int packetSize,
                                    char **ppResultBuffer,
                                    int *pResultBufferSize,
                                    bool *disconnect,
                                    pmdEDUCB *cb )
{

   EDB_ASSERT ( disconnect, "disconnect can't be NULL" )
   EDB_ASSERT ( pReceiveBuffer, "pReceivedBuffer is NULL" )
   int rc  = EDB_OK;
   unsigned int probe  = 0;
   const char * pInsertorBuffer = NULL;
    **(int **)ppResultBuffer=0;
    *pResultBufferSize = 4;
    OSS_LOG(LOG_DEBUG,"Receive data %s\n ",&pReceiveBuffer[4]); 
    return rc;
}

int pmdAgentEntryPoint ( pmdEDUCB *cb, void *arg )
{
    OSS_LOG( LOG_DEBUG,"pmdAgentEntryPoint \n");
    int rc = EDB_OK;
    unsigned int probe  = 0;
    bool disconnect = false;
    char *pReceiveBuffer  = NULL ;
    char *pResultBuffer   = NULL ;
    int recvSizeBufferSize = ossRoundUpToMultipleX(PMD_AGENT_RECIEVE_BUFFER_SZ,EDB_PAGE_SIZE);
    int resultBufferSize = sizeof(MsgReply);

    int packetLength = 0;
    EDUID  myEDUID =cb->getID();
    pmdEDUMgr *eduMgr = cb->getEDUMgr();


    int s = *( int * )&arg;//socket
    ossSocket sock(&s);
    sock.disableNagle();


    pReceiveBuffer = (char *)malloc(sizeof(char)*recvSizeBufferSize);
    if(!pReceiveBuffer)
    {
        rc =EDB_OOM;
        probe = 10;
        goto error ;
    }

    pResultBuffer = (char*)malloc(sizeof(char)*resultBufferSize);
    if(!pResultBuffer)
    {
        rc = EDB_OOM;
        probe = 20;
        goto error ;
    }


    while (!disconnect)
    {
        //receive next packet
        rc = pmdRecv(pReceiveBuffer,sizeof(int),&sock,cb);
        OSS_LOG(LOG_DEBUG,"Receive head status :%d \n",rc);
        if(rc)
        {
            OSS_LOG ( LOG_ERROR, "recv error\n");
            if(EDB_APP_FORCED==rc)
            {
                disconnect =true;
                continue;
            }
            if(EDB_NETWORK_CLOSE==rc)
            {
                disconnect = true;
                goto error ;   
            }
        }
        packetLength =*(int *)(pReceiveBuffer);
        OSS_LOG(LOG_DEBUG,"Receive packet size=%d\n",packetLength);
        if(packetLength<sizeof(int))
        {
            probe = 40;
            rc = EDB_INVALIDARG;
            goto error ;
        }

        if(recvSizeBufferSize <packetLength+1)
        {
             OSS_LOG(LOG_DEBUG,"Receive buffer size is too small: %d vs %d, increasing...",recvSizeBufferSize, packetLength ) ;
             int newSize = ossRoundUpToMultipleX ( packetLength+1, EDB_PAGE_SIZE ) ;//4096整数倍
            if ( newSize < 0 )
            {
               probe = 50 ;
               rc = EDB_INVALIDARG ;
               goto error ;
            }
            free ( pReceiveBuffer ) ;
            pReceiveBuffer = (char*)malloc ( sizeof(char) * (newSize) ) ;
            if ( !pReceiveBuffer )
            {
               rc = EDB_OOM ;
               probe = 60 ;
               goto error ;
            }
            *(int*)(pReceiveBuffer) = packetLength ;
            recvSizeBufferSize = newSize ; 
        }

        //recvSizeBufferSize
      //正式数据
      rc = pmdRecv ( &pReceiveBuffer[sizeof(int)],
                     packetLength-sizeof(int),
                     &sock, cb ) ;
      OSS_LOG( LOG_DEBUG,"Receive data status : %d\n",rc);
      if ( rc )
      {
         if ( EDB_APP_FORCED == rc )
         {
            disconnect = true ;
            continue ;
         }
         probe = 70 ;
         goto error ;
      }
          // put 0 at end of the packet
      pReceiveBuffer[packetLength] = 0 ;


      //线程从等待变成执行
      if( EDB_OK != ( rc = eduMgr->activateEDU ( myEDUID )))
      {
        goto error;
      }
        //检查返回包的大小
      if( resultBufferSize >(int)sizeof( MsgReply ) )
      {
          resultBufferSize =  (int)sizeof( MsgReply ) ;
          free ( pResultBuffer ) ;
          pResultBuffer = (char*)malloc( sizeof(char) *
                                         resultBufferSize ) ;
          if ( !pResultBuffer )
          {
            rc = EDB_OOM ;
            probe = 20 ;
            goto error ;
          }
      }

      //真正的执行函数
      rc = pmdProcessAgentRequest(pReceiveBuffer,packetLength,&pResultBuffer,&resultBufferSize,&disconnect,cb);
      if(rc)
      {
         OSS_LOG ( LOG_ERROR, "Error processing Agent request, rc=%d", rc ) ;
      }

      //发送返回信息
      if(!disconnect)
      {
        OSS_LOG( LOG_DEBUG,"send buffer%s %d\n",pResultBuffer,*(int*)pResultBuffer);
        rc = pmdSend ( pResultBuffer,*(int *)pResultBuffer, &sock, cb ) ;
        if ( rc )
         {
            if ( EDB_APP_FORCED == rc )
            {
            }
            probe = 80 ;
            goto error ;
         }
      }

      //状态改成等待
      if ( EDB_OK != ( rc = eduMgr->waitEDU ( myEDUID )) )
      {
         goto error ;
      }

    }
done:
    if ( pReceiveBuffer )
      free ( pReceiveBuffer )  ;
   if ( pResultBuffer )
      free ( pResultBuffer )  ;
    sock.close () ;
    return rc;
error:
    switch ( rc )
   {
   case EDB_SYS :
      OSS_LOG ( LOG_ERROR,
              "EDU id %d cannot be found, probe %d\n", myEDUID, probe ) ;
      break ;
   case EDB_EDU_INVAL_STATUS :
      OSS_LOG ( LOG_SERVER,
              "EDU status is not valid, probe %d\n", probe ) ;
      break ;
   case EDB_INVALIDARG :
      OSS_LOG ( LOG_SERVER,
              "Invalid argument receieved by agent, probe %d\n", probe ) ;
      break ;
   case EDB_OOM :
      OSS_LOG ( LOG_SERVER,
              "Failed to allocate memory by agent, probe %d\n", probe ) ;
      break ;
   case EDB_NETWORK :
      OSS_LOG ( LOG_SERVER,
              "Network error occured, probe %d\n", probe ) ;
      break ;
   case EDB_NETWORK_CLOSE :
      OSS_LOG ( LOG_SERVER,"Remote connection closed\n" ) ;
      rc = EDB_OK ;
      break ;
   default :
      OSS_LOG ( LOG_SERVER,"Internal error, probe %d\n", probe ) ;
   }
    goto done;
}