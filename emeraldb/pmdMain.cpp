#include "pmdEDU.h"
#include "logprint.h"
#include  "signalhandle.h"
#include  "pmdKRCB.h"
#include  "pmdOptions.h"
#include  "pmdEDUMgr.h"
using namespace std;
static int pmdResolveArguments ( int argc, char **argv )
{
   int rc = EDB_OK ;
   pmdOptions options ;
   rc = options.init ( argc, argv ) ;
   if ( rc )
   {
      if ( EDB_PMD_HELP_ONLY != rc )
         OSS_LOG ( LOG_ERROR, "Failed to init options, rc = %d", rc ) ;
      goto error ;
   }
   rc = pmdGetKRCB()->init ( &options ) ;
   if( rc )
   {
      OSS_LOG ( LOG_ERROR, "Failed to init krcb, rc = %d", rc ) ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

int pmdMasterThreadMain(int argc,char **argv)
{
    int rc = EDB_OK;
    EDB_KRCB * krcb = pmdGetKRCB();
    pmdEDUMgr *eduMgr   = krcb->getEDUMgr () ;
   EDUID      agentEDU = PMD_INVALID_EDUID ;
    RegisterCommonSignalHandle();

    LOG_API_SetPara(LOG_LEVEL_E::LOG_INFO,LOG_DIRECTION_E::LOG_TO_STD,NULL);

    // arguments
    rc = pmdResolveArguments ( argc, argv ) ;
    rc = eduMgr->startEDU ( EDU_TYPE_TCPLISTENER, NULL, &agentEDU ) ;
    PD_RC_CHECK ( rc, LOG_ERROR, "Failed to start tcplistener edu, rc = %d", rc ) ;
    while ( EDB_IS_DB_UP )
    {
       sleep(1) ;
    }
done:
    return rc;

error:
    goto done;

}


int main(int argc,char **argv)
{
    OSS_LOG(LOG_INFO, "start emeralddb\n");
    return pmdMasterThreadMain ( argc, argv ) ;
}