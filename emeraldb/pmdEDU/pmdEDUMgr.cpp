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
        ossSleepmillis(100);//每0.1秒检查
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
    return rc ;
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
         OSS_LOG ( LOG_ERROR, "force edu[ID:%lld]\n", it->first ) ;
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
   eduCB->postEvent( pmdEDUEvent ( type, release, pData ) ) ;//pmdEDUEvent 用于数据传递
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

//  edu结束的时候调用 
int pmdEDUMgr::returnEDU ( EDUID eduID, bool force, bool* destroyed )
{
   OSS_LOG(LOG_DEBUG,"pmdEDUMgr::returnEDU\n");
    int rc = EDB_OK ;
    EDU_TYPES type = EDU_TYPE_UNKNOWN;
    pmdEDUCB *educb = NULL;
    std::map<EDUID, pmdEDUCB*>::iterator it ;

    _mutex.get_shared();
    //判断eduID存在不存在 runqueue
    if(_runQueue.end () == ( it = _runQueue.find ( eduID )))
    {
        if( _idleQueue.end () == ( it = _idleQueue.find ( eduID )))
        {
            rc = EDB_SYS;
            *destroyed = false;
            _mutex.release_shared();//释放锁
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
      type = educb->getType() ;//获取edu类型1
   }
    _mutex.release_shared () ;//释放锁
   // if the EDU type can't be pooled, or if we forced, or if the EDU is
   // destroied, or we exceed max pooled edus, let's destroy it
   //如果不是agent 或者强制退出 或者销毁  或者 超出了线程池大小 
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
      //回归线程池 
      rc = _deactivateEDU ( eduID ) ;
      
      if ( destroyed )
      {
         // when we try to pool the EDU, destroyed set to true only when
         // the EDU can't be found in the list
         //回池错误就销毁
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
   OSS_LOG(LOG_DEBUG,"pmdEDUMgr::startEDU\n");
   int     rc = EDB_OK ;
   EDUID     eduID = 0 ;
   pmdEDUCB* eduCB = NULL ;
   std::map<EDUID, pmdEDUCB*>::iterator it ;

   //挂起状态直接返回
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
   /**
    * 空间队列为空 或者不能从线程池中获取
   */
   if ( true == _idleQueue.empty () || !isPoolable ( type ) )
   {
      // note that EDU types other than "agent" shouldn't be pooled at all
      // release latch before calling createNewEDU
      _mutex.release () ;
      rc = _createNewEDU ( type, arg, eduid ) ; //创建新的线程
      if ( EDB_OK == rc )
         goto done ;
      goto error ;
   }

   // if we can find something in idle queue, let's get the first of it
   //从空闲队列中找到一个edu
   for ( it = _idleQueue.begin () ;
         ( _idleQueue.end () != it ) &&
         ( PMD_EDU_IDLE != ( *it ).second->getStatus ()) ;
         it ++ ) ;

   // if everything in idleQueue are in DESTROY status, we still need to
   // create a new EDU
   //如果没找到就重新创建一个edu
   if ( _idleQueue.end () == it )
   {
      // release latch before calling createNewEDU
      _mutex.release () ;
      rc = _createNewEDU ( type, arg, eduid  ) ;//创建新的edu
      if ( EDB_OK == rc )
         goto done ;
      goto error ;
   }

   // now "it" is pointing to an idle EDU
   // note that all EDUs in the idleQueue should be AGENT type
   /*
      找到了一个空间的ide
      获取eduid 和 educb
   */
   eduID = ( *it ).first ;
   eduCB = ( *it ).second ;
   _idleQueue.erase ( eduID ) ;//从空闲队列移除
   EDB_ASSERT ( isPoolable ( type ),"must be agent" )
   // switch agent type for the EDU ( call different agent entry point )
   eduCB->setType ( type ) ;//设置educb的类型
   eduCB->setStatus ( PMD_EDU_WAITING ) ;//把状态设置为等待状态
   _runQueue [ eduID ] = eduCB ;//添加到运行队列
   *eduid = eduID ;
   _mutex.release () ;
   /*************** END CRITICAL SECTION **********************/

   //The edu is start, need post a resum event
   //激活事件 传入参数
   eduCB->postEvent( pmdEDUEvent( PMD_EDU_EVENT_RESUME, false, arg ) ) ;

done :
   return rc ;
error :
   goto done ;
}

int pmdEDUMgr::_createNewEDU ( EDU_TYPES type, void* arg, EDUID *eduid )
{
   int rc = EDB_OK;
   unsigned int probe = 0;
   pmdEDUCB  *cb =NULL;
   EDUID  myEDUID = 0;

   //如果是挂起状态直接返回
   if(isQuiesced())
   {
      rc = EDB_QUIESCED;
      goto done;
   }

   //判断是不是agent 或者listen的
   if(!getEntryFuncByType(type))
   {
      OSS_LOG(LOG_ERROR,"The edu[type:%d] not exist or function is null", type ) ;
      rc =EDB_INVALIDARG;
      probe = 30;
      goto error;
   }
   //创建一个新的edu
   cb = new(std::nothrow)pmdEDUCB(this,type);
   EDB_VALIDATE_GOTOERROR(cb,EDB_OOM,"Out of memory to create agent control block");
   cb->setStatus(PMD_EDU_CREATING);//设置为创建状态

   _mutex.get();
     // if the EDU exist in runqueue
   //在运行队列中找edu id
   if(_runQueue.end()!=_runQueue.find(_EDUID))
   {
      _mutex.release();
      rc = EDB_SYS;
      probe = 10;
      goto error;
   }
    // if the EDU exist in idle queue
    //判断是否存在在 空闲队列
   if ( _idleQueue.end() != _idleQueue.find ( _EDUID )  )
   {
      _mutex.release () ;
      rc = EDB_SYS ;
      probe = 15 ;
      goto error ;
   }

   // assign EDU id and increment global EDUID
   //edu 分配一个id 
   cb->setID ( _EDUID ) ;
   if ( eduid )
      *eduid = _EDUID ;
      // place cb into runqueue
   _runQueue [ _EDUID ] = ( pmdEDUCB* ) cb ;//插入运行队列
   myEDUID = _EDUID ;
   ++_EDUID ;//global 
   _mutex.release () ;
    
    //真正的创建线程
   try
   {
      //创建一个线程 传入类型 edu 参数 
      boost::thread agentThread (pmdEDUEntryPoint, type, cb, arg ) ;
      // detach the agent so that he's all on his own
      // we only track based on CB
      agentThread.detach () ;//设置分离
   }
   catch(const std::exception& e)
   {
      // if we failed to create thread, make sure to clean runqueue
      _runQueue.erase ( myEDUID ) ;
      rc = EDB_SYS ;
      probe = 20 ;
      goto error ;
   }
   

   //The edu is create, need post a resum event
   //线程创建完成后唤醒事件
   cb->postEvent(pmdEDUEvent( PMD_EDU_EVENT_RESUME, false, arg ) ) ;

done:
   return rc;
error:
   if(cb)
   {
      delete cb;
   }
   OSS_LOG(LOG_ERROR,"Failed to create new agent, probe = %d", probe ) ;
   goto done;
}

int pmdEDUMgr::_destroyEDU( EDUID eduID)
{
   int rc = EDB_OK ;
   pmdEDUCB *eduCB = NULL;

   unsigned int eduStatus  = PMD_EDU_CREATING;

   std::map<EDUID, pmdEDUCB*>::iterator it ;
   std::map<unsigned int,EDUID>::iterator it1;

   _mutex.get();
      // try to find the edu id in runqueue
   // Since this is private function, no latch is needed
   if(_runQueue.end ()==( it = _runQueue.find ( eduID )))
   {
      if ( _idleQueue.end () == ( it = _idleQueue.find ( eduID )) )
      {
         // we can't find edu id anywhere
         rc = EDB_SYS ;
         goto error ;
      }
      //在空闲队列中找到
      eduCB = ( *it ).second ;
      if(!PMD_IS_EDU_IDLE(eduCB->getStatus()))
      {
         rc = EDB_EDU_INVAL_STATUS;
         goto error ;
      }
      //set the status to destroyed
      eduCB->setStatus(PMD_EDU_DESTROY);
      _idleQueue.erase(eduID);
   }
   else //if we find in run queue we expect waiting status
   {
     //在运行队列中
      eduCB = ( *it ).second ;
      eduStatus = eduCB->getStatus () ;
      if(!PMD_IS_EDU_WAITING(eduStatus)&&!PMD_IS_EDU_CREATING ( eduStatus ))
      {
         rc = EDB_EDU_INVAL_STATUS;
         goto error;
      }
      eduCB->setStatus ( PMD_EDU_DESTROY ) ;
      _runQueue.erase ( eduID ) ;
   }

   // clean up tid/eduid map
   for ( it1 = _tid_eduid_map.begin(); it1 != _tid_eduid_map.end();
         ++it1 )
   {
      if ( (*it1).second == eduID )
      {
         _tid_eduid_map.erase ( it1 ) ;
         break ;
      }
   }
   if ( eduCB )
      delete ( eduCB ) ;
done :
   _mutex.release () ;
   return rc ;
error :
   goto done ;
}

// change edu status from running to waiting

int pmdEDUMgr::waitEDU( EDUID eduID)
{
   int rc = EDB_OK ;
   pmdEDUCB *eduCB  = NULL ;
   unsigned int eduStatus = PMD_EDU_CREATING ;
   std::map<EDUID, pmdEDUCB*>::iterator it ;
   _mutex.get_shared () ;
   if ( _runQueue.end () == ( it = _runQueue.find ( eduID )) )
   {
      // we can't find EDU in run queue
      rc = EDB_SYS ;
      goto error ;
   }
   
   eduCB = ( *it ).second ;
   eduStatus = eduCB->getStatus () ;

   if(PMD_IS_EDU_WAITING(eduStatus))
   {
      goto done;
   }
   if(!PMD_IS_EDU_RUNNING(eduStatus))
   {
      rc = EDB_EDU_INVAL_STATUS;
      goto error;
   }
   eduCB->setStatus ( PMD_EDU_WAITING ) ;//设为等待状态
done :
   _mutex.release_shared () ;
   /************* END CRITICAL SECTION **************/
   return rc ;
error :
   goto done ;
}

// creating/waiting status edu can be deactivated (pooled)
// deactivateEDU supposed only happened to AGENT EDUs
// any EDUs other than AGENT will be destroyed and EDB_SYS will be returned
//线程runqueue 放入 idleQueue
int pmdEDUMgr::_deactivateEDU( EDUID eduID)
{
   OSS_LOG(LOG_DEBUG," pmdEDUMgr::_deactivateEDU %d\n",eduID);
   int rc = EDB_OK;
   unsigned int eduStatus  = PMD_EDU_CREATING ;
   pmdEDUCB * eduCB = NULL ;
   std::map<EDUID, pmdEDUCB*>::iterator it ;
   _mutex.get () ;
   if ( _runQueue.end () == ( it = _runQueue.find ( eduID )) )
   {
      if ( _idleQueue.end() != _idleQueue.find ( eduID )  )
      {
         OSS_LOG(LOG_DEBUG,"eduID is int idleQueue\n");
         goto done ;
      }
      OSS_LOG(LOG_DEBUG,"fadfasf\n");
      rc =EDB_SYS;
      goto error ;
   }
   printf("11111\n");
   eduCB  = ( *it ).second ;

   eduStatus = eduCB->getStatus();
   OSS_LOG(LOG_DEBUG,"eduStatus:%d\n",eduStatus);
   if(PMD_IS_EDU_IDLE(eduStatus))
   {
      OSS_LOG(LOG_DEBUG,"pmdEDUMgr::_deactivateEDU  PMD_IS_EDU_IDLE\n");
      goto done ;
   }
   //不是等待状态或者创建状态
   if(!PMD_IS_EDU_WAITING(eduStatus)&&!PMD_IS_EDU_CREATING( eduStatus))
   {
      rc = EDB_EDU_INVAL_STATUS;
      goto error;
   }
   // only Agent can be deactivated (pooled), other system
   // EDUs can only be destroyed
   EDB_ASSERT(isPoolable(eduCB->getType()),"Only agent can be pooled\n");

   _runQueue.erase( eduID);//移除
   eduCB->setStatus ( PMD_EDU_IDLE ) ;
   _idleQueue [ eduID ] = eduCB ;//添加
   OSS_LOG( LOG_DEBUG,"edu status is PMD_EDU_IDLE\n");
done:
   _mutex.release();
   return rc;

error :
   OSS_LOG(LOG_DEBUG,"error\n");
   goto done ;

}
//空闲队列 转为运行队列
int pmdEDUMgr::activateEDU ( EDUID eduID )
{
   int rc = EDB_OK ;
   unsigned int eduStatus  = PMD_EDU_CREATING ;
   pmdEDUCB *eduCB = NULL ;
   std::map<EDUID, pmdEDUCB*>::iterator it ;
   
   _mutex.get();

   if(_idleQueue.end () == ( it = _idleQueue.find ( eduID )))
   {
      if(_runQueue.end () == ( it = _runQueue.find ( eduID )))
      {
         rc = EDB_SYS;
         goto error;
      }

      eduCB = ( *it ).second ;

      // in runqueue we may have creating/waiting status
      eduStatus = eduCB->getStatus () ;

      if(PMD_IS_EDU_RUNNING(eduStatus))
      {
         goto done ;
      }
      if(!PMD_IS_EDU_WAITING(eduStatus) &&!PMD_IS_EDU_CREATING( eduStatus))
      {
         rc = EDB_EDU_INVAL_STATUS;
         goto error;
      }
      eduCB->setStatus( PMD_EDU_RUNNING);
      goto done;
   }

   eduCB = ( *it ).second ;
   eduStatus = eduCB->getStatus () ;
   if ( PMD_IS_EDU_RUNNING ( eduStatus ) )
      goto done ;
   if ( !PMD_IS_EDU_IDLE ( eduStatus ) )
   {
      rc = EDB_EDU_INVAL_STATUS ;
      goto error ;
   }
    // now the EDU status is idle, let's bring it to RUNNING
   _idleQueue.erase ( eduID ) ;
   eduCB->setStatus ( PMD_EDU_RUNNING ) ;
   _runQueue [ eduID ] = eduCB ;
done :
   _mutex.release () ;
   return rc ;
error :
   goto done ;
}


// get pmdEDUCB for the given thread id
pmdEDUCB *pmdEDUMgr::getEDU ( unsigned int tid )
{
   std::map<unsigned int, EDUID>::iterator it ;
   std::map<EDUID, pmdEDUCB*>::iterator it1 ;
   EDUID eduid ;
   pmdEDUCB *pResult = NULL ;
   _mutex.get_shared () ;
   it = _tid_eduid_map.find ( tid ) ;
   if ( _tid_eduid_map.end() == it )
   {
      pResult = NULL ;
      goto done ;
   }
   eduid = (*it).second ;
   it1 = _runQueue.find ( eduid ) ;
   if ( _runQueue.end() != it1 )
   {
      pResult = (*it1).second ;
      goto done ;
   }
   it1 = _idleQueue.find ( eduid ) ;
   if ( _idleQueue.end() != it1 )
   {
      pResult = (*it1).second ;
      goto done ;
   }
done :
   _mutex.release_shared () ;
   return pResult ;
}


void pmdEDUMgr::setEDU ( unsigned int tid, EDUID eduid )
{
   _mutex.get() ;
   _tid_eduid_map [ tid ] = eduid ;
   _mutex.release () ;
}

// get pmdEDUCB for the current thread
pmdEDUCB *pmdEDUMgr::getEDU ()
{
   return getEDU ( ossGetCurrentThreadID() ) ;
}

pmdEDUCB *pmdEDUMgr::getEDUByID ( EDUID eduID )
{
   std::map<EDUID, pmdEDUCB*>::iterator it ;
   pmdEDUCB *pResult = NULL ;
   // shared lock the block, since we don't change anything
   _mutex.get_shared () ;
   if ( _runQueue.end () == ( it = _runQueue.find ( eduID )) )
   {
      // if we cannot find it in runqueue, we search for idle queue
      // note that during the time, we already have EDUMgr locked,
      // so thread cannot change queue from idle to run
      // that means we are safe to exame both queues
      if ( _idleQueue.end () == ( it = _idleQueue.find ( eduID )) )
      {
         // we can't find edu id anywhere
         goto done ;
      }
   }
   pResult = it->second ;
done :
   _mutex.release_shared () ;
   return pResult ;
}
