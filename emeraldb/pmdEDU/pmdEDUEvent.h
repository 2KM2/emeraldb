#pragma once
#include "linuxinclude.h"
#include "commontype.h"
#include  "commondef.h"
#include <iostream>
using namespace std;
/**
 * edu 之间得事件通信机制
*/
enum pmdEDUEventTypes
{
   PMD_EDU_EVENT_NONE = 0,
   PMD_EDU_EVENT_TERM,     // terminate EDU 结束EDU
   PMD_EDU_EVENT_RESUME,   // resume a EDU, the data is startEDU's argv
   PMD_EDU_EVENT_ACTIVE,
   PMD_EDU_EVENT_DEACTIVE,
   PMD_EDU_EVENT_MSG,
   PMD_EDU_EVENT_TIMEOUT,  // timeout
   PMD_EDU_EVENT_LOCKWAKEUP // transaction lock wake up
};

class pmdEDUEvent
{
public:
   pmdEDUEvent () :
   _eventType(PMD_EDU_EVENT_NONE),
   _release(false),
   _Data(NULL)
   {
   }

   pmdEDUEvent(pmdEDUEventTypes type ) :
   _eventType(type),
   _release(false),
   _Data(NULL)
   {
   }

   pmdEDUEvent (pmdEDUEventTypes type, bool release, void *data ) :
   _eventType(type),
   _release(release),
   _Data(data)
   {
   
   
   }

   void reset ()
   {
      _eventType = PMD_EDU_EVENT_NONE ;
      _release = false ;
      _Data = NULL ;
   }
   pmdEDUEventTypes _eventType ;
   bool             _release ;
   void            *_Data ;
};



