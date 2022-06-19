#include "pmdEDU.h"
#include "logprint.h"
#include  "signalhandle.h"
#include  "pmdKRCB.h"
#include  "pmdOptions.h"
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

    RegisterCommonSignalHandle();

    LOG_API_SetPara(LOG_LEVEL_E::LOG_INFO,LOG_DIRECTION_E::LOG_TO_STD,NULL);

    // arguments
    rc = pmdResolveArguments ( argc, argv ) ;
    if ( EDB_PMD_HELP_ONLY == rc )
    {
       goto done ;
    }
    while(EDB_IS_DB_DOWN)
    {
        sleep(1);
    }
done:
    return rc;

error:
    goto done;

}


int main(int argc,char **argv)
{
    RegisterCommonSignalHandle();

    LOG_API_SetPara(LOG_LEVEL_E::LOG_INFO, LOG_DIRECTION_E::LOG_TO_STD, NULL);
    OSS_LOG(LOG_INFO, "start emeralddb\n");
    pmdTcpListenerEntryPoint();
}