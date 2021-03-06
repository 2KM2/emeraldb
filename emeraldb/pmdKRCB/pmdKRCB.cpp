#include  "pmdOptions.h"
#include  "linuxinclude.h"
#include  "commontype.h"
#include  "commondef.h"
#include "pmdKRCB.h"
EDB_KRCB pmd_krcb;
extern char _pdDiagLogPath [ OSS_MAX_PATHSIZE+1 ] ;


int EDB_KRCB::init(pmdOptions *options)
{
   setDBStatus ( EDB_DB_NORMAL ) ;
   setDataFilePath ( options->getDBPath () ) ;
   setLogFilePath ( options->getLogPath () ) ;
   strncpy ( _pdDiagLogPath, getLogFilePath(), sizeof(_pdDiagLogPath) ) ;
   setSvcName ( options->getServiceName () ) ;
   setMaxPool ( options->getMaxPool () ) ;
   //return _rtnMgr.rtnInitialize() ;
   return EDB_OK;
}
