#pragma once

#include  "linuxinclude.h"
#include  "logprint.h"
#include  "commontype.h"
#include  "commondef.h"
#include  "primitivefileop.h"
#include  "ossLatch.h"
#include <vector>


class ossMmapFile
{
protected:
    class ossMmapSegment
    {
    public:
        void *_ptr;//映射的地址
        unsigned int  _length; //长度
        unsigned long long _offset;//偏移
        ossMmapSegment( void *ptr,
                        unsigned int length,
                        unsigned long long offset)
        {
            _ptr = ptr ;
            _length = length ;
            _offset = offset ;
        }
    };
    typedef std::vector<ossMmapSegment>::const_iterator CONST_ITR;
public:
    ossMmapFile()
    {
        _opened = false;
        memset(_fileName, 0, sizeof(_fileName));
    }
    ~ossMmapFile ()
    {
        close(); 
    }
    int open(const char * pFilename,unsigned int options);
    void close();
    int map(unsigned long long offset,unsigned int length,void **pAddress);
public:
    inline CONST_ITR begin()
    {
        return _segments.begin();
    }
    inline CONST_ITR end()
    {
        return _segments.end();
    }
    inline unsigned int segmentSize()
    {
        return _segments.size();
    }
private:
     std::vector<ossMmapSegment> _segments ;//文件中定义多个段
     ossPrimitiveFileOp  _fileOp;
     ossXLatch  _mutex;
     bool _opened;//是否打开
     char _fileName[OSS_MAX_PATHSIZE];
};