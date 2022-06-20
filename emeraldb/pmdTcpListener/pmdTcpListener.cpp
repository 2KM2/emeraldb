#include "pmdEDU.h"
#include "logprint.h"
#include  "linuxinclude.h"
#include  "commondef.h"
#include  "commontype.h"
#include  "pmdKRCB.h"
#include  "pmdEDUMgr.h"
#define PMD_TCPLISTENER_RETRY 5
#define OSS_MAX_SERVICENAME NI_MAXSERV

int pmdTcpListenerEntryPoint(pmdEDUCB*cb , void*arg)
{
   int rc  = EDB_OK;
   pmdEDUMgr *eduMgr = cb->getEDUMgr();
   EDUID myEDUID = cb->getID();

   unsigned int retry = 0;
   EDUID  agentEDU = PMD_INVALID_EDUID;

   char  svcName[OSS_MAX_PATHSIZE+1];

   while(retry<=PMD_TCPLISTENER_RETRY && !EDB_IS_DB_DOWN)
   {
      retry++;
      strcpy(svcName,pmdGetKRCB()->getSvcName());
      OSS_LOG(LOG_EVENT, "Listening on port_test %s\n", svcName ) ;

      int port = 0;
      int len = strlen ( svcName ) ;

      for ( int i = 0; i<len; ++i )
      {
         if ( svcName[i] >= '0' && svcName[i] <= '9' )
         {
            port = port * 10 ;
            port += int( svcName[i] - '0' ) ;
         }
         else
         {
            OSS_LOG ( LOG_ERROR, "service name error!\n" ) ;
         }
      }
      ossSocket sock ( port ) ;
      rc = sock.initSocket () ;
      EDB_VALIDATE_GOTOERROR(EDB_OK==rc,rc,"Failed initialize socket");

       rc = sock.bindListen();
       EDB_VALIDATE_GOTOERROR(EDB_OK==rc, rc,"Failed to bind/listen socket");

       //once bind is successfully
      if ( EDB_OK != ( rc = eduMgr->activateEDU ( myEDUID )) )
      {
         goto error ;
      }

      //master loop for tcp listen
      while(!EDB_IS_DB_DOWN)
      {
         int s;
         rc = sock.accept(&s,NULL,NULL);
         if(EDB_TIMEOUT == rc)
         {
            rc=EDB_OK;
            continue;
         }
         if ( rc && EDB_IS_DB_DOWN )
         {
            rc = EDB_OK;
            goto done;
         }
         else if(rc)
         {
            OSS_LOG(LOG_ERROR,"Failed to accept socket in TcpListener" ) ;
            OSS_LOG(LOG_EVENT,"Restarting socket to listen" ) ;
            break;
         }

         //assing the socket to the arg
         void *pData = NULL ;//new client
         *((int *) &pData) = s ;

         //启动新的edu agent
         rc = eduMgr->startEDU(EDU_TYPE_AGENT,pData,&agentEDU);
         if(rc)
         {
            if(rc ==EDB_QUIESCED)
            {
                 OSS_LOG (LOG_ERROR, "Reject new connection due to quiesced database" ) ;
            }
            else
            {
               OSS_LOG(LOG_ERROR,"Failed to start EDU agent" );
            }
            // close remote connection if we can't create new thread
            ossSocket newsock(&s);
            newsock.close();
            continue;
         }
      }                        
      //理论不会走到这一步,除非数据库停止
      if ( EDB_OK != ( rc = eduMgr->waitEDU ( myEDUID )) )
      {
         goto error ;
      }
   } // while ( retry <= PMD_TCPLISTENER_RETRY )
done :
   return rc;
error :
   switch ( rc )
   {
   case EDB_SYS :
      OSS_LOG ( LOG_SERVER, "System error occured" ) ;
      break ;
   default :
      OSS_LOG ( LOG_SERVER, "Internal error" ) ;
   }
   goto done ;
}