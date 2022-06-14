#include "ossSocket.hpp"

ossSocket::ossSocket()
{
    m_init =false;
    m_fd =0;
    m_timeout = 0;
    memset(&m_sockAddress,0,sizeof(sockaddr_in));
    memset(&m_peerAddress,0,sizeof(sockaddr_in));
    m_addressLen=sizeof(m_sockAddress);
    m_peerAddressLen=sizeof(m_peerAddress);
}

ossSocket::ossSocket(unsigned int port, int timeout=0)
{
    m_init =false;
    m_fd = 0;
    m_timeout = timeout;
    memset(&m_sockAddress,0,sizeof(sockaddr_in));
    memset(&m_peerAddress,0,sizeof(sockaddr_in));
    m_peerAddressLen = sizeof(m_peerAddress);
    m_peerAddress.sin_family =AF_INET;
    m_peerAddress.sin_addr.s_addr =htonl(INADDR_ANY);
    m_peerAddress.sin_port = htons(port);
    m_addressLen =  sizeof(m_sockAddress);
}

ossSocket::ossSocket(const char *pHostName,unsigned int port,int timeout=0)
{
    /*
    该结构记录主机的信息，包括主机名、别名、地址类型、地址长度和地址列表
    */
    struct hostent *hp ;
    m_init =false;
    m_timeout = timeout;
    m_fd = 0;

}

int ossSocket::setSocketLinger(int lOnOff,int linger);
{

}