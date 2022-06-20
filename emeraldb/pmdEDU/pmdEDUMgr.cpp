#include "pmdEDUMgr.h"
#include "logprint.h"
#include "pmdKRCB.h"
int pmdEDUMgr::_destroyAll()
{
    _setDestroyed(true);
    setQuiesced(true);//不会有新的进来

    //stop all user edus

    //先杀掉用户edu
    unsigned int timeCounter = 0;
    unsigned int  eduCount = _getEDUCount(EDU_USER);

    while(eduCount!=0)
    {
        if(0==timeCounter%50)
        {
            //每5秒发一次
            _forceEDUs(EDU_USER);
        }
        ++timeCounter;
        ossSleepmillis(100);
        eduCount = _getEDUCount(EDU_USER);
    }

    //stop all system edus
   timeCounter = 0 ;
   eduCount = _getEDUCount ( EDU_ALL ) ;
   while ( eduCount != 0 )
   {
      if ( 0 == timeCounter % 50 )
      {
         _forceEDUs ( EDU_ALL ) ;
      }

      ++timeCounter ;
      ossSleepmillis ( 100 ) ;
      eduCount = _getEDUCount ( EDU_ALL ) ;
   }

   return EDB_OK ;
}

// force a specific EDU
int pmdEDUMgr::forceUserEDU ( EDUID eduID )
{
    int rc = EDB_OK ;
   std::map<EDUID, pmdEDUCB*>::iterator it ;
   _mutex.get() ;
   if(isSystemEDU(eduID))
   {
    OSS_LOG(LOG_ERROR,"System EDU %d can not be forced",eduID);
    goto error;
   }

   {
      for ( it = _runQueue.begin () ; it != _runQueue.end () ; ++it )
      {
         if ( (*it).second->getID () == eduID )
         {
            (*it).second->force () ;
            goto done ;
         }
      }
      for ( it = _idleQueue.begin () ; it != _idleQueue.end () ; ++it )
      {
         if ( (*it).second->getID () == eduID )
         {
            (*it).second->force () ;
            goto done ;
         }
      }

   }

done:
    _mutex.release();
error:
    goto done;
}

// block all new request and attempt to terminate existing requests
int pmdEDUMgr::_forceEDUs ( int property )
{   
     std::map<EDUID, pmdEDUCB*>::iterator it ;

     _mutex.get();
     for(it = _runQueue.begin () ; it != _runQueue.end () ; ++it)
     {

        if ( ((EDU_SYSTEM & property) && _isSystemEDU( it->first ))
         || ((EDU_USER & property) && !_isSystemEDU( it->first )) )
      {
         ( *it ).second->force () ;
         OSS_LOG ( LOG_ERROR, "force edu[ID:%lld]", it->first ) ;
      }
     }


    for ( it = _idleQueue.begin () ; it != _idleQueue.end () ; ++it )
     {
      if ( EDU_USER & property )
      {
         ( *it ).second->force () ;
      }
    }
      _mutex.release() ;
      return EDB_OK;
}


unsigned int pmdEDUMgr::_getEDUCount ( int property )
{
     unsigned int eduCount = 0 ;
   std::map<EDUID, pmdEDUCB*>::iterator it ;
   _mutex.get_shared() ;
   for ( it = _runQueue.begin () ; it != _runQueue.end () ; ++it )
   {
      if ( ((EDU_SYSTEM & property) && _isSystemEDU( it->first ))
         || ((EDU_USER & property) && !_isSystemEDU( it->first )) )
      {
         ++eduCount ;
      }
   }

   for ( it = _idleQueue.begin () ; it != _idleQueue.end () ; ++it )
   {
      if ( EDU_USER & property )
      {
         ++eduCount ;
      }
   }
   _mutex.release_shared() ;
   return eduCount ;

}


int pmdEDUMgr::postEDUPost ( EDUID eduID, pmdEDUEventTypes type,bool release , void *pData )
{
    int rc = EDB_OK;
    pmdEDUCB *eduCB = NULL;
    std::map<EDUID, pmdEDUCB*>::iterator it ;
    _mutex.get_shared();
    if ( _runQueue.end () == ( it = _runQueue.find ( eduID )) )
    {
        if ( _idleQueue.end () == ( it = _idleQueue.find ( eduID )) )
        {
            // we can't find edu id anywhere
            rc = EDB_SYS ;
            goto error ;
        }
    }

   eduCB = ( *it ).second ;
   eduCB->postEvent( pmdEDUEvent ( type, release, pData ) ) ;
done :
   _mutex.release_shared () ;
   return rc ;
error :
   goto done ;
}

int pmdEDUMgr::waitEDUPost ( EDUID eduID, pmdEDUEvent& event,long long millsecond = -1 )
{
    int rc = EDB_OK ;
    pmdEDUCB* eduCB = NULL ;
    std::map<EDUID, pmdEDUCB*>::iterator it ;
    _mutex.get_shared () ;
    if(  _runQueue.end () == ( it = _runQueue.find ( eduID )))
    {
        if ( _idleQueue.end () == ( it = _idleQueue.find ( eduID )) )
      {
         // we can't find edu id anywhere
         rc = EDB_SYS ;
         goto error ;
      } 
    }

    eduCB = ( *it ).second ;
   // wait for event. when millsecond is 0, it should always return true
   if ( !eduCB->waitEvent( event, millsecond ) )
   {
      rc = EDB_TIMEOUT ;
      goto error ;
   }

done :
   _mutex.release_shared () ;
   return rc ;
error :
   goto done ;
}


// release control from a given EDU
// EDUMgr should decide whether put the EDU to pool or destroy it
// EDU Status must be in waiting or creating
int pmdEDUMgr::returnEDU ( EDUID eduID, bool force, bool* destroyed )
{
    int rc = EDB_OK ;
    EDU_TYPES type = EDU_TYPE_UNKNOWN;
    pmdEDUCB *educb = NULL;
    std::map<EDUID, pmdEDUCB*>::iterator it ;

    _mutex.get_shared();
    if(_runQueue.end () == ( it = _runQueue.find ( eduID )))
    {
        if( _idleQueue.end () == ( it = _idleQueue.find ( eduID )))
        {
            rc = EDB_SYS;
            *destroyed = false;
            _mutex.release_shared();
            goto error;
        }
    }
    educb = (*it).second ;
       // if we are trying to destry EDU manager, or enforce destroy, or
   // if the total number of threads are more than what we need
   // we need to destroy this EDU
   //
   // Currentl we only able to pool agent and coordagent
   if ( educb )
   {
      type = educb->getType() ;
   }
   // if the EDU type can't be pooled, or if we forced, or if the EDU is
   // destroied, or we exceed max pooled edus, let's destroy it
   if ( !isPoolable(type) || force || isDestroyed () ||
        size() > (unsigned int)pmdGetKRCB()->getMaxPool () )
   {
      rc = _destroyEDU ( eduID ) ;
      if ( destroyed )
      {
         // we consider the EDU is destroyed when destroyEDU returns
         // OK or EDB_SYS (edu can't be found), so that thread can terminate
         // itself
         if ( EDB_OK == rc || EDB_SYS == rc )
            *destroyed = true ;
         else
            *destroyed = false ;
      }
   }
   else
   {
      // in this case, we don't need to care whether the EDU is agent or not
      // as long as we treat EDB_SYS as "destroyed" signal, we should be
      // safe here
      rc = _deactivateEDU ( eduID ) ;
      if ( destroyed )
      {
         // when we try to pool the EDU, destroyed set to true only when
         // the EDU can't be found in the list
         if ( EDB_SYS == rc )
            *destroyed = true ;
         else
            *destroyed = false ;
      }
   }


done:   
    return rc;
error:
    goto done ;
}

// get an EDU from idle pool, if idle pool is empty, create new one
int pmdEDUMgr::startEDU ( EDU_TYPES type, void* arg, EDUID *eduid )
{
   int     rc = EDB_OK ;
   EDUID     eduID = 0 ;
   pmdEDUCB* eduCB = NULL ;
   std::map<EDUID, pmdEDUCB*>::iterator it ;

   if ( isQuiesced () )
   {
      rc = EDB_QUIESCED ;
      goto done ;
   }
   /****************** CRITICAL SECTION **********************/
   // get exclusive latch, we don't latch the entire function
   // in order to avoid creating new thread while holding latch
   _mutex.get () ;
   // if there's any pooled EDU?
   // or is the request type can be pooled ?
   if ( true == _idleQueue.empty () || !isPoolable ( type ) )
   {
      // note that EDU types other than "agent" shouldn't be pooled at all
      // release latch before calling createNewEDU
      _mutex.release () ;
      rc = _createNewEDU ( type, arg, eduid ) ;
      if ( EDB_OK == rc )
         goto done ;
      goto error ;
   }

   // if we can find something in idle queue, let's get the first of it
   for ( it = _idleQueue.begin () ;
         ( _idleQueue.end () != it ) &&
         ( PMD_EDU_IDLE != ( *it ).second->getStatus ()) ;
         it ++ ) ;

   // if everything in idleQueue are in DESTROY status, we still need to
   // create a new EDU
   if ( _idleQueue.end () == it )
   {
      // release latch before calling createNewEDU
      _mutex.release () ;
      rc = _createNewEDU ( type, arg, eduid  ) ;
      if ( EDB_OK == rc )
         goto done ;
      goto error ;
   }

   // now "it" is pointing to an idle EDU
   // note that all EDUs in the idleQueue should be AGENT type
   eduID = ( *it ).first ;
   eduCB = ( *it ).second ;
   _idleQueue.erase ( eduID ) ;//从空闲队列移除
   EDB_ASSERT ( isPoolable ( type ),
                "must be agent" )
   // switch agent type for the EDU ( call different agent entry point )
   eduCB->setType ( type ) ;
   eduCB->setStatus ( PMD_EDU_WAITING ) ;
   _runQueue [ eduID ] = eduCB ;//添加到运行队列
   *eduid = eduID ;
   _mutex.release () ;
   /*************** END CRITICAL SECTION **********************/

   //The edu is start, need post a resum event
   eduCB->postEvent( pmdEDUEvent( PMD_EDU_EVENT_RESUME, false, arg ) ) ;

done :
   return rc ;
error :
   goto done ;
}
