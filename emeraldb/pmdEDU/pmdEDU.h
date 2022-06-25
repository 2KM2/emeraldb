#pragma once

#include  "pmdEDUEvent.h"
#include  "ossQueue.h"
#include  "ossSocket.h"



#define PMD_INVALID_EDUID 0
#define PMD_IS_EDU_CREATING(x)    ( PMD_EDU_CREATING == x )
#define PMD_IS_EDU_RUNNING(x)     ( PMD_EDU_RUNNING  == x )
#define PMD_IS_EDU_WAITING(x)     ( PMD_EDU_WAITING  == x )
#define PMD_IS_EDU_IDLE(x)        ( PMD_EDU_IDLE     == x )
#define PMD_IS_EDU_DESTROY(x)     ( PMD_EDU_DESTROY  == x )


/**
 * edu 引擎调度单元
 * 线程池对edu进行调度,通过传入不同的EDU类型,EDU内部调用不同得函数执行相应得请求
 * EDU回池后可以分派给其他任务
 * 
 * 优势
 * 1.直接处理用户sock,不需要其他转接
 * 2.用户线程数等于用户连接数量
 * 缺点：
 * 大量请求回创建很多线程，资源浪费(数据分片)
 * 
 * 另一种线程模式
 * 1.通过tcp监听 队列 代理线程从队列中
 * 
*/
typedef unsigned long long EDUID;

enum EDU_TYPES
{
   // System EDU Type
   EDU_TYPE_TCPLISTENER = 0,
   // Agent EDU Type
   EDU_TYPE_AGENT,

   EDU_TYPE_UNKNOWN,
   EDU_TYPE_MAXIMUM = EDU_TYPE_UNKNOWN
} ;
enum EDU_STATUS
{
   PMD_EDU_CREATING = 0,         // 创建
   PMD_EDU_RUNNING,             //  运行
   PMD_EDU_WAITING,            //   等待 等待已经连接好的客户端
   PMD_EDU_IDLE,              //    空闲 客户端断开连接 线程回池            
   PMD_EDU_DESTROY,
   PMD_EDU_UNKNOWN,
   PMD_EDU_STATUS_MAXIMUM = PMD_EDU_UNKNOWN
} ;

class pmdEDUMgr ;

class pmdEDUCB
{
public:
   pmdEDUCB ( pmdEDUMgr *mgr, EDU_TYPES type ) ;

   inline EDUID getID()
   {
      return _id ;
   }
    // 发送事件
   inline void postEvent ( pmdEDUEvent const &data )
   {
      _queue.push ( data ) ;
   }
   // 等待事件    
   bool waitEvent(pmdEDUEvent &data,long long millsec)
   {
     // if millsec is not 0, that means we want timeout
     bool waitMsg = false ;

     //不是空闲状态设置为等待状态
     if(PMD_EDU_IDLE != _status)
     {
        _status = PMD_EDU_WAITING;
     }


     if(0 > millsec)
     {
        //always waitMsg
        _queue.wait_and_pop( data);
        waitMsg = true;        
     }
     else
     {
        waitMsg = _queue.timed_wait_and_pop(data,millsec);
     }


     if(waitMsg)
     {  
        //结束事件得话设置为 断开连接
        if(PMD_EDU_EVENT_TERM==data._eventType)
        {
            _isDisconnected = true;
        }else
        {
            _status = PMD_EDU_RUNNING;
        }
     }

     return waitMsg;
   }
    //其他线程强制停止该线程，比如退出引擎让退出线程发命令
    inline void force ()
   {
      _isForced = true ;
   }

   inline void disconnect ()
   {
      _isDisconnected = true ;
   }

   inline EDU_TYPES getType ()
   {
      return _type ;
   }

   inline EDU_STATUS getStatus ()
   {
      return _status ;
   }

   inline void setType ( EDU_TYPES type )
   {
      _type = type ;
   }
   inline void setID ( EDUID id )
   {
      _id = id ;
   }
   inline void setStatus ( EDU_STATUS status )
   {
      _status = status ;
   }
   inline bool isForced ()
   {
      return _isForced ;
   }
   //当前线程池得地址
   inline pmdEDUMgr *getEDUMgr ()
   {
      return _mgr ;
   }
private :
   EDU_TYPES    _type ;
   pmdEDUMgr *  _mgr ;// 线程池
   EDU_STATUS   _status ;
   EDUID        _id ;
   bool         _isForced ;
   bool         _isDisconnected ;
   ossQueue<pmdEDUEvent> _queue ; //每个edu都有自身的事件队列
};

//线程函数的入口函数
typedef int (*pmdEntryPoint) ( pmdEDUCB *, void * ) ;

/**
 * 给定类型返回函数指针 代理线程 或者 监听线程
*/
pmdEntryPoint getEntryFuncByType ( EDU_TYPES type );

 // 代理线程函数指针:处理用户请求
int pmdAgentEntryPoint ( pmdEDUCB *cb, void *arg ) ;      


//监听线程函数指针
int pmdTcpListenerEntryPoint ( pmdEDUCB *cb, void *arg ) ;





int pmdEDUEntryPoint ( EDU_TYPES type, pmdEDUCB *cb, void *arg ) ;
 
//tcp 接受
int pmdRecv ( char *pBuffer, int recvSize, ossSocket *sock, pmdEDUCB *cb ) ;

//tcp 发送
int pmdSend ( const char *pBuffer, int sendSize, ossSocket *sock, pmdEDUCB *cb ) ;