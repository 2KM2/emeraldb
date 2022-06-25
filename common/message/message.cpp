#include "message.h"
#include "logprint.h"
#include "linuxinclude.h"
#include "commondef.h"
#include "commontype.h"

using namespace bson;
using namespace std;

//检测缓冲区大小是否合法
static int msgCheckBuffer(char **ppBuffer, int* pBufferSize,int length)
{
   int rc = EDB_OK ;
   if ( length > *pBufferSize )
   {
      char *pOldBuf = *ppBuffer ;
      if ( length < 0 )
      {
         OSS_LOG ( LOG_ERROR, "invalid length: %d\n", length ) ;
         rc = EDB_INVALIDARG ;
         goto error ;
      }
      *ppBuffer = (char*)realloc ( *ppBuffer, sizeof(char)*length ) ;
      if ( !*ppBuffer )
      {
         OSS_LOG ( LOG_ERROR, "Failed to allocate %d bytes buffer\n", length ) ;
         rc = EDB_OOM ;
         *ppBuffer = pOldBuf ;
         goto error ;
      }
      *pBufferSize = length ;
   }
done :
   return rc ;
error :
   goto done ;
}










