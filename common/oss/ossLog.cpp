#include "ossLog.hpp"
#include "ossLatch.hpp"
#include  "ossPrimitiveFileOp.hpp"

const static char *OSSLOGLEVELSTRING[] =
{
   "SEVERE",
   "ERROR",
   "EVENT",
   "WARNING",
   "INFO",
   "DEBUG"
};


const char *getLOGLevelDesp(LOGLEVEL level)
{
    if ( (unsigned int)level > (unsigned int)LOGDEBUG )
   {
      return "Unknow Level" ;
   }
   return OSSLOGLEVELSTRING[(unsigned int)level] ;
}
//time  fromat
const static char *OSS_LOG_HEADER_FORMAT ="%04d-%02d-%02d-%02d.%02d.%02d.%06d\
               \
Level:%s"OSS_NEWLINE"PID:%-37dTID:%d"OSS_NEWLINE"Function:%-32sLine:%d"\
OSS_NEWLINE"File:%s"OSS_NEWLINE"Message:"OSS_NEWLINE"%s"OSS_NEWLINE OSS_NEWLINE;


 LOGLEVEL _curLogLevel=LOG_DEF_DIAGLEVEL;

 char _ossDiagLogPath[OSS_MAX_PATHSIZE+1]={0};


 ossXLatch _ossLogMutex;
 ossPrimitiveFileOp   _ossLogFile;

 //open log file

 static int _ossLogFileReopen()
 {
    int rc = EDB_OK;
    _ossLogFile.Close();
    rc = _ossLogFile.Open(_ossDiagLogPath);
    if(rc)
    {
        printf ( "Failed to open log file, errno = %d"OSS_NEWLINE, rc ) ;
        goto error ;
    }
    _ossLogFile.seekToEnd();
done:
    return rc;
error:
    goto done ;

 }


 //write log file

static int _ossLogFileWrite(const char *pData)
{
    int rc = EDB_OK;
    size_t dataSize = strlen(pData);
    _ossLogMutex.get();
    if(!_ossLogFile.isValid())
    {
        rc = _ossLogFileReopen();
        if(rc)
        {
            printf ( "Failed to open log file, errno = %d"OSS_NEWLINE,
                  rc ) ;
         goto error ;
        }
    }

    //write into log file
    rc = _ossLogFile.Write(pData,dataSize);
   if ( rc )
   {
      printf ( "Failed to write into log file, errno = %d"OSS_NEWLINE,
               rc ) ;
      goto error ;
   }
done:
    _ossLogMutex.release();
    return rc;
error:
    goto done;
}



