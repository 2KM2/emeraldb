#include "signalhandle.h"
#include "logprint.h"

#define MAX_BACKTRACE_SIZE 32


static void CommonSignalHandle(int signalNum)
{
    pid_t pid = getpid();
    pid_t tid = syscall(__NR_gettid);

    char tName[32] = {0};
    prctl(PR_GET_NAME, (unsigned long)tName);

    OSS_LOG(LOG_INFO,"pid:%d, tid:%d, threadname:%s. coming signal:%d(%s)\n", pid, tid, tName, signalNum, sys_siglist[signalNum]);

    if (SIGPIPE == signalNum)
    {
        //
    }
    else if (SIGCHLD == signalNum)
    {
        int status = 0;
        pid_t subPid = waitpid(-1, &status, 0);
        OSS_LOG(LOG_DEBUG,"sub process(%d) status: %d\n", subPid, status);
    }
    else
    {
        #if 0
        void *buffer[MAX_BACKTRACE_SIZE];
        int cnt = backtrace(buffer, MAX_BACKTRACE_SIZE);
        OSS_LOG(LOG_DEBUG,"backtrace count:%d\n",cnt);
        backtrace_symbols_fd(buffer, cnt, STDOUT_FILENO);
		usleep(100*1000);
        #endif

        exit(-1);
    }
    
}

INTERFACE_API void RegisterCommonSignalHandle()
{
    for (int num = 0; num < 32; num ++)
    {
        signal(num, CommonSignalHandle);
    }
}
