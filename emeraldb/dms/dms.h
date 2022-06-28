#pragma once

#include "ossLatch.h"
#include "ossMmapFile.h"
#include "bson.h"
#include "dmsRecord.h"
#include <vector>
using namespace std;
using namespace bson;


#define DMS_EXTEND_SIZE 65536
//4M for page size
#define DMS_PAGESIZE 4194304

#define DMS_MAX_RECORD (DMS_PAGESIZE-sizeof(dmsHeader)-sizeof(dmsRecord)-sizeof(SLOTOFF)) // 一个记录最大长度

#define DMS_MAX_PAGES  262144  //最大数据页

typedef unsigned int SLOTOFF ;

#define DMS_INVALID_SLOTID       0xFFFFFFFF
#define DMS_INVALID_PAGEID       0xFFFFFFFF


#define DMS_KEY_FIELDNAME        "_id"

extern const char *gKeyFieldName ;

#define DMS_HEADER_EYECATCHER_LEN 4

#define DMS_RECORD_FLAG_NORMAL  0 //正常
#define DMS_RECORD_FLAG_DROPPED 1 //删除
typedef struct dmsRecord
{
   unsigned int _size ;
   unsigned int _flag ;
   char         _data[0] ;
}dmsRecord;

// dms header
#define DMS_HEADER_EYECATCHER "DMSH"
#define DMS_HEADER_EYECATCHER_LEN 4
#define DMS_HEADER_FLAG_NORMAL  0 // 
#define DMS_HEADER_FLAG_DROPPED 1


#define DMS_HEADER_VERSION_0   0
#define DMS_HEADER_VERSION_CURRENT DMS_HEADER_VERSION_0

typedef struct dmsHeader
{
   char         _eyeCatcher[DMS_HEADER_EYECATCHER_LEN] ;
   unsigned int _size ;
   unsigned int _flag ;
   unsigned int _version ;
} dmsHeader;


// page structure
/*********************************************************
PAGE STRUCTURE
-------------------------
| PAGE HEADER           |
-------------------------
| Slot List             |
-------------------------
| Free Space            |
-------------------------
| Data                  |
-------------------------
**********************************************************/
//数据页
#define DMS_PAGE_EYECATCHER "PAGH"
#define DMS_PAGE_EYECATCHER_LEN 4
#define DMS_PAGE_FLAG_NORMAL    0
#define DMS_PAGE_FLAG_UNALLOC   1
#define DMS_SLOT_EMPTY 0xFFFFFFFF


typedef struct dmsPageHeader
{
   char             _eyeCatcher[DMS_PAGE_EYECATCHER_LEN] ;
   unsigned int     _size ;
   unsigned int     _flag ;
   unsigned int     _numSlots ;
   unsigned int     _slotOffset ;
   unsigned int     _freeSpace ;
   unsigned int     _freeOffset ;
   char             _data[0] ;
}dmsPageHeader ;


#define DMS_FILE_SEGMENT_SIZE 134217728 //128M
#define DMS_FILE_HEADER_SIZE  65536 //64K 
#define DMS_PAGES_PER_SEGMENT (DMS_FILE_SEGMENT_SIZE/DMS_PAGESIZE)  // 32
#define DMS_MAX_SEGMENTS      (DMS_MAX_PAGES/DMS_PAGES_PER_SEGMENT)  //131,072 128K


class dmsFile:public ossMmapFile
{
private:
   // points to memory where header is located
   dmsHeader            *_header ;
   std::vector<char *>  _body;
   //free space to page id map
    std::multimap<unsigned int, PAGEID> _freeSpaceMap ;
    ossSLatch             _mutex ; //读写锁
    ossXLatch             _extendMutex ; //互斥锁
    char                 *_pFileName ;
public:
    dmsFile () ;
   ~dmsFile () ;
   int initialize(const char * pFilename);

   // record:输入 outRecord:输出(索引用) 
   int insert(bson::BSONObj &record,bson::BSONObj &outRecord,dmsRecordID &rid ) ;
   int remove ( dmsRecordID &rid ) ;
   int find ( dmsRecordID &rid, bson::BSONObj &result ) ;
private:
   
   // create a new segment for the current file
   int _extendSegment () ;
   
   // init from empty file, creating header only
   int _initNew () ;
   
   // extend the file for given bytes
   int _extendFile ( int size ) ;
   
   // load data from beginning
   int _loadData () ;

   // search slot
   int _searchSlot ( char *page, dmsRecordID &recordID,SLOTOFF &slot ) ;

   // reorg
   void _recoverSpace ( char *page ) ;
   
   // update free space
   void _updateFreeSpace ( dmsPageHeader *header, int changeSize,PAGEID pageID ) ;

   // find a page id to insert, return invalid_pageid if there's no page canb e found for required size bytes
   PAGEID _findPage ( size_t requiredSize ) ;   
public:
    inline unsigned int getNumSegments ()
   {
      return _body.size() ;
   }
    inline unsigned int getNumPages ()
   {
      return getNumSegments() * DMS_PAGES_PER_SEGMENT ;
   }
 
   inline char *pageToOffset ( PAGEID pageID )
   {
      if ( pageID >= getNumPages () )
      {
         return NULL ;
      }
      return _body [ pageID / DMS_PAGES_PER_SEGMENT ] + DMS_PAGESIZE * ( pageID % DMS_PAGES_PER_SEGMENT ) ;
   }

   inline bool validSize ( size_t size )
   {
      if ( size < DMS_FILE_HEADER_SIZE )
      {
         return false ;
      }
      size = size - DMS_FILE_HEADER_SIZE ;
      if ( size % DMS_FILE_SEGMENT_SIZE != 0 )
      {
         return false ;
      }
      return true ;
   }
};