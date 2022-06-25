#pragma once
#include  "linuxinclude.h"
#include  "commondef.h"
#include  "commontype.h"
#include  "pmdEDU.h"
#include  "ossLatch.h"
#include  "ossUtil.h"
#include  <map>
#define EDU_SYSTEM  0x01
#define EDU_USER    0x02
#define EDU_ALL     (EDU_SYSTEM | EDU_USER)

class pmdEDUMgr
{
public:
    pmdEDUMgr()
    :_EDUID(1)
    ,_isQuiesced(false)
    ,_isDestroyed(false)
    {

    }
    ~pmdEDUMgr()
    {
        reset();
    }

    void reset()
    {
        _destroyAll();
    }

    unsigned int size()
    {
      unsigned int num = 0 ;
      _mutex.get_shared () ;
      num = ( unsigned int ) _runQueue.size() +
            ( unsigned int ) _idleQueue.size () ;
      _mutex.release_shared () ;
      return num ;

    }

    
    unsigned int sizeRun ()
    {
      unsigned int num = 0 ;
      _mutex.get_shared () ;
      num = ( unsigned int ) _runQueue.size () ;
      _mutex.release_shared () ;
      return num ;
    }

    unsigned int sizeIdle ()
   {
      unsigned int num = 0 ;
      _mutex.get_shared () ;
      num = ( unsigned int ) _idleQueue.size () ;
      _mutex.release_shared () ;
      return num ;
   }

    unsigned int sizeSystem ()
   {
      unsigned int num = 0 ;
      _mutex.get_shared () ;
      num = _mapSystemEDUS.size() ;
      _mutex.release_shared () ;
      return num ;
   }

   // 获取系统类型的EDUID
   EDUID getSystemEDU ( EDU_TYPES edu )
   {
      EDUID eduID = PMD_INVALID_EDUID;
      _mutex.get_shared () ;
      std::map<unsigned int, EDUID>::iterator it = _mapSystemEDUS.find( edu ) ;
      if ( it != _mapSystemEDUS.end() )
      {
         eduID = it->second  ;
      }
      _mutex.release_shared () ;
      return eduID ;
   }

    bool isSystemEDU ( EDUID eduID )
   {
      bool isSys = false ;
      _mutex.get_shared () ;
      isSys = _isSystemEDU ( eduID ) ;
      _mutex.release_shared () ;
      return isSys ;
   }
    //注册到系统edu表
   void regSystemEDU(EDU_TYPES edu,EDUID eduid)
   {
      _mutex.get() ;
      _mapSystemEDUS[ edu ] = eduid ;
      _mutex.release () ;

   }
   bool isQuiesced ()
   {
      return _isQuiesced ;
   }
   void setQuiesced ( bool b )
   {
      _isQuiesced = b ;
   }
   bool isDestroyed ()
   {
      return _isDestroyed ;
   }
   //是否可以返回线程池，只有agent可以，tcp listen 死循环
   static bool isPoolable ( EDU_TYPES type )
   {
      return ( EDU_TYPE_AGENT == type ) ;
   }
public:
      /*
       * EDU Status Transition Table
       * C: CREATING
       * R: RUNNING
       * W: WAITING
       * I: IDLE
       * D: DESTROY
       * c: createNewEDU
       * a: activateEDU
       * d: destroyEDU
       * w: waitEDU
       * t: deactivateEDU
       *   C   R   W   I   D  <--- from
       * C c
       * R a   -   a   a   -  <--- Creating/Idle/Waiting status can move to Running status
       * W -   w   -   -   -  <--- Running status move to Waiting
       * I t   -   t   -   -  <--- Creating/Waiting status move to Idle
       * D d   -   d   d   -  <--- Creating / Waiting / Idle can be destroyed
       * ^ To
       */

      // c i w 状态转为 running
      int activateEDU ( EDUID eduID ) ;

      //从执行状态到等待状态
      int waitEDU ( EDUID eduID ) ;

      //启动edu 1.创建新的 2 从线程池中获取
      int startEDU ( EDU_TYPES type, void* arg, EDUID *eduid ) ;

      //线程通信
      int postEDUPost ( EDUID eduID, pmdEDUEventTypes type,bool release = false, void *pData = NULL ) ;
      int waitEDUPost ( EDUID eduID, pmdEDUEvent& event,long long millsecond ) ;
      
      
      //当线程结束了,征求线程池，由线程决定 传入destoryed
      int returnEDU ( EDUID eduID, bool force, bool* destroyed ) ;

    
      int forceUserEDU ( EDUID eduID ) ;
      
      //通过线程id获取edu
      pmdEDUCB *getEDU ( unsigned int tid ) ;

     //获取当前线程的edu  
      pmdEDUCB *getEDU () ;
      
      //通过eudid获取edu
      pmdEDUCB *getEDUByID ( EDUID eduID ) ;
      
      void setEDU ( unsigned int tid, EDUID eduid ) ;
private:
    //创建新的edu
    int _createNewEDU(EDU_TYPES type,void *arg,EDUID *eduid);
    // 销毁所有线程
    int _destroyAll();

    //强制退出某种类型的edu
    int _forceEDUs(int property = EDU_ALL);
    
    
    unsigned int _getEDUCount ( int property = EDU_ALL ) ;
    
    //线程池设置为销毁
    void _setDestroyed ( bool b )
    {
       _isDestroyed = b ;
    }
    
   /*
    * This function must be called against a thread that either in
    * PMD_EDU_WAITING or PMD_EDU_IDLE or PMD_EDU_CREATING status
    * This function set the status to PMD_EDU_DESTROY and remove
    * the control block from manager
    * Parameter:
    *   EDU ID (UINt64)
    * Return:
    *   EDB_OK (success)
    *   EDB_SYS (the given eduid can't be found)
    *   EDB_EDU_INVAL_STATUS (EDU is found but not with expected status)
    */
    int  _destroyEDU(EDUID eduID);

   /*
    * This function must be called against a thread that either in creating
    * or waiting status, it will return without any change if the agent is
    * already in pool
    * This function will change the status to PMD_EDU_IDLE and put to
    * idlequeue, representing the EDU is pooled (no longer associate with
    * any user activities)
    * deactivateEDU supposed only happened to AGENT EDUs
    * Any EDUs other than AGENT will be forwarded to destroyEDU and return
    * SDB_SYS
    * Parameter:
    *   EDU ID (UINt64)
    * Return:
    *   EDB_OK (success)
    *   EDB_SYS (the given eduid can't be found, or it's not AGENT)
    *           (note that deactivate an non-AGENT EDU will cause the EDU
    *            destroyed and SDB_SYS return)
    *   EDB_EDU_INVAL_STATUS (EDU is found but not with expected status)
    *  把一个edu放到线程池中
    */
    int _deactivateEDU( EDUID eduID ) ;

    //判断是不是系统edu
    bool _isSystemEDU( EDUID eduID)
    {
        std::map<unsigned int,EDUID>::iterator it = _mapSystemEDUS.begin();
        while(it!=_mapSystemEDUS.end())
        {
            if(eduID ==it->second)
            {
                return true;
            }
            ++it;
        }
        return false;
    }
private:
    std::map<EDUID,pmdEDUCB *> _runQueue;  //运行队列
    std::map<EDUID,pmdEDUCB*>  _idleQueue; //空闲队列
    std::map<unsigned int,EDUID> _tid_eduid_map; //线程ID和EDUID映射
    ossSLatch  _mutex;
    EDUID      _EDUID;
    std::map<unsigned int,EDUID> _mapSystemEDUS; //系统类型和eduid映射

    bool _isQuiesced; //标志数据库是否挂起，退出的时候 不能有新的连接进来
    bool _isDestroyed;
};