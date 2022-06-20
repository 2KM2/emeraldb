#include "pmdEDU.h"
#include "logprint.h"
#include  "signalhandle.h"
using namespace std;
int main()
{
    RegisterCommonSignalHandle();

    LOG_API_SetPara(LOG_LEVEL_E::LOG_INFO, LOG_DIRECTION_E::LOG_TO_STD, NULL);
    OSS_LOG(LOG_INFO, "start emeralddb\n");
    pmdTcpListenerEntryPoint();
}