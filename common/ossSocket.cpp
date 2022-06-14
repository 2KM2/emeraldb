#include "ossSocket.hpp"

ossSocket::ossSocket()
{
    
}

/**
 * @brief SO_LINGER选项用来设置延迟关闭的时间，等待套接字发送缓冲区中的数据发送完成
 * 
 * @param lOnOff 
 * @param linger 
 * @return int 
 */
int ossSocket::setSocketLinger(int lOnOff,int linger);
{

}