#pragma once
#include  "core.hpp"
#define SOCKET_GETLASTERROR errno

// by default 10ms timeout
#define OSS_SOCKET_DFT_TIMEOUT 10000

// max hostname
#define OSS_MAX_HOSTNAME NI_MAXHOST
#define OSS_MAX_SERVICENAME NI_MAXSERV

#define MAX_RECV_RETRIES 5
class ossSocket
{
public:
    /**
     * @brief Construct a new oss Socket object
     * 
     */
    ossSocket();
    
    /**
     * @brief create listen socket
     * 
     * @param port 
     * @param timeout 
     */
    ossSocket(unsigned int port, int timeout=0);

    /**
     * @brief create a connection socket such as client socket
     * 
     * @param pHostName 
     * @param port 
     * @param timeout 
     */
    ossSocket(const char *pHostName,unsigned int port,int timeout=0);
    
    /**
     * @brief create from a existing socket such as after accept socket
     * 
     * @param socket 
     * @param timeout 
     */
    ossSocket(int *socket,int timeout=0);
    
    /**
     * @brief Destroy the oss Socket object
     * 
     */
    ~ossSocket()
    {
        close();
    }

    /**
     * @brief Set the Socket Linger object 
     * SO_LINGER选项用来设置延迟关闭的时间，等待套接字发送缓冲区中的数据发送完成
     * @param lOnOff 
     * @param linger 
     * @return int 
     */
    int setSocketLinger(int lOnOff,int linger);

    /**
     * @brief Set the Address object
     * 
     * @param pHostName 
     * @param port 
     */
    void setAddress (const char *pHostName, unsigned int port );
    /**
     * @brief init socket func
     * 
     * @return int 
     */
    int initSocket();
    
    /**
     * @brief bind listen func
     * 
     * @return int 
     */
    int bindListen();
    
    /**
     * @brief adjust is connected
     * 
     * @return true 
     * @return false 
     */
    bool isConnected();
    
    /**
     * @brief send msg
     * 
     * @param pMsg 
     * @param len 
     * @param timeout 
     * @param flags 
     * @return int 
     */
    int send(const char *pMsg,int len,int timeout=OSS_SOCKET_DFT_TIMEOUT,int flags=0);
    
    /**
     * @brief recv msg wait len is arrive to return otherwise is waiting 
     * 
     * @param pMsg 
     * @param len 
     * @param timeout 
     * @return int 
     */
    int recv(char *pMsg,int len,int timeout=OSS_MAX_SERVICENAME,int flags=0);
    
    /**
     * @brief  recv msg return now
     * 
     * @param pMsg 
     * @param len 
     * @param timeout 
     * @return int 
     */
    int recvNF(char *pMsg,int len,int timeout=OSS_SOCKET_DFT_TIMEOUT);


    int  connect();
    
    void close();
    
   int accept ( int *sock, struct sockaddr *addr, socklen_t *addrlen,int timeout = OSS_SOCKET_DFT_TIMEOUT ) ;
    
    int  disableNagle();
    
    unsigned int getPeerPort();
    
    int getPeerAddress ( char *pAddress, unsigned int length ) ;


    unsigned int getLocalPort () ;
    
    int getLocalAddress ( char *pAddress, unsigned int length ) ;
    
    int setTimeout ( int seconds ) ;
    /**
     * @brief Get the Host Name object
     * get local host name
     * @param pName 
     * @param nameLen 
     * @return int 
     */
    static int getHostName ( char *pName, int nameLen ) ;
    /**
     * @brief Get the Port object
     *  service name  convert to port
     * @param pServiceName 
     * @param port 
     * @return int 
     */
    static int getPort ( const char *pServiceName, unsigned short &port ) ;
protected:
    unsigned int getPort(sockaddr_in *addr);
    int getAddress(sockaddr_in *addr,char *pAddress,unsigned int length);
private:
    int m_fd;
    socklen_t  m_addressLen;
    socklen_t  m_peerAddressLen;
    struct sockaddr_in  m_sockAddress;
    struct sockaddr_in  m_peerAddress;
    bool m_init;
    int  m_timeout;
};
