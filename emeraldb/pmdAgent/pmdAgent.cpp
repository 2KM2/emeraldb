#include "logprint.h"
#include  "linuxinclude.h"
#include  "commontype.h"
#include  "commondef.h"
#include  "ossSocket.h"
#include  "bson/src/bson.h"
#include   "pmdEDU.h"
#include   "pmdKRCB.h"



#define ossRoundUpToMultipleX(x,y) (((x)+((y)-1))-(((x)+((y)-1))%(y)))
#define PMD_AGENT_RECIEVE_BUFFER_SZ 4096
#define EDB_PAGE_SIZE               4096

int pmdAgentEntryPoint ( pmdEDUCB *cb, void *arg )
{
    int rc = EDB_OK;
    unsigned int probe  = 0;
    bool disconnect = false;
    char *pReceiveBuffer  = NULL ;
    char *pResultBuffer   = NULL ;







done:
    return rc;
}