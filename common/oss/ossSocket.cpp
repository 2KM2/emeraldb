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

ossSocket::ossSocket(unsigned int port, int timeout)
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

ossSocket::ossSocket(const char *pHostName,unsigned int port,int timeout)
{
    /*
    该结构记录主机的信息，包括主机名、别名、地址类型、地址长度和地址列表
    */
    struct hostent *hp ;
    m_init =false;
    m_timeout = timeout;
    m_fd = 0;
    memset(&m_peerAddress,0,sizeof(sockaddr_in));
    memset(&m_sockAddress,0,sizeof(sockaddr_in));
    m_peerAddressLen =sizeof(m_peerAddress);

    m_sockAddress.sin_family = AF_INET;
    if((hp=gethostbyname(pHostName)))
    {
        m_sockAddress.sin_addr.s_addr = *((int*)hp->h_addr_list[0] ) ;
    }else{
        m_sockAddress.sin_addr.s_addr = inet_addr(pHostName);
    }

    m_sockAddress.sin_port = htons(port);
    m_addressLen = sizeof(m_sockAddress);
}

ossSocket::ossSocket(int *socket,int timeout)
{
    int rc = EDB_OK;
    m_fd = *socket;
    m_init = true;
    m_timeout = timeout;
    m_addressLen = sizeof(m_sockAddress);
    memset(&m_peerAddress,0,sizeof(sockaddr_in));

    m_peerAddressLen = sizeof(m_peerAddress);

    rc = getsockname(m_fd,(sockaddr *)&m_sockAddress,&m_addressLen);
    if(rc)
    {
        printf("Failed to get sock name ,error =%d\n",SOCKET_GETLASTERROR);
        m_init =false;
        goto error;
    }
    else
    {
       rc = getpeername (m_fd, (sockaddr*)&m_peerAddress, &m_peerAddressLen ) ;
       if(rc)
       {
            printf("Failed to get peer name ,error %d\n",SOCKET_GETLASTERROR);
            goto error;
       }
    }
done:
    return ;
error:
    goto done;
}


int ossSocket::initSocket()
{

    int rc = EDB_OK;
    if(m_init)
    {
        goto done;
    }
    memset(&m_peerAddress,0,sizeof(sockaddr_in));
    m_peerAddressLen = sizeof(m_peerAddress);
    m_fd =socket(AF_INET,SOCK_STREAM, IPPROTO_TCP ) ;
    if(-1==m_fd)
    {
        printf("Failed to initialize socket,error =%d",SOCKET_GETLASTERROR);
    }
    m_init =true;
    setTimeout(m_timeout);
done:
    return rc;
error:
    goto done;
}

int ossSocket::setSocketLinger(int lOnOff,int linger)
{
    int rc = EDB_OK;

    struct linger  m_linger;
    m_linger.l_onoff = lOnOff;
    m_linger.l_linger = linger ;
    rc = setsockopt(m_fd,SOL_SOCKET,SO_LINGER,(const char*)&m_linger, sizeof(m_linger));
    return rc;
}


void ossSocket::setAddress(const char *pHostName, unsigned int port )
{
    struct hostent *hp ;
    memset(&m_sockAddress,0,sizeof(sockaddr_in));
    memset(&m_peerAddress,0,sizeof(sockaddr_in));

    m_peerAddressLen = sizeof(m_peerAddress);

    m_sockAddress.sin_family = AF_INET;
    if((hp=gethostbyname(pHostName)))
    {
        m_sockAddress.sin_addr.s_addr = *((int*)hp->h_addr_list[0] ) ;
    }
    else
    {
        m_sockAddress.sin_addr.s_addr  =inet_addr(pHostName);
    }
    m_sockAddress.sin_port = htons(port);
    m_addressLen = sizeof(m_sockAddress);
}


int ossSocket::bindListen()
{
    int rc = EDB_OK;
    int temp = 1;
    rc = setsockopt(m_fd,SOL_SOCKET,SO_REUSEADDR,(char*)&temp,sizeof(int));

    if(rc)
    {
        printf(" Failed to setsockopt SO_REUSEADDR ,rc =%d\n",SOCKET_GETLASTERROR);
    }
    rc = setSocketLinger(1,30);//设置延迟关闭30
    if(rc)
    {
        printf("Failed to setsockopt SO_LINGER ,rc = %d\n",SOCKET_GETLASTERROR);
    }

    rc = ::bind(m_fd,(struct sockaddr*)&m_sockAddress, m_addressLen ) ;
    if(rc)
    {
        printf("Failed to bind socket ,rc=%d\n",SOCKET_GETLASTERROR);
    }

done:
    return rc;

error:
    close();
    goto done;
}

int ossSocket::send(const char *pMsg, int len,int timeout, int flags)
{

    int rc = EDB_OK;
    int maxFD  = m_fd;
    struct timeval maxSelectTime;
    maxSelectTime.tv_sec = timeout / 1000000 ;
    maxSelectTime.tv_usec = timeout % 1000000 ;
    fd_set fds ;
    
    if(0==len)
    {
        return EDB_OK;
    }

    while(true)
    {
        FD_ZERO(&fds);
        FD_SET(m_fd,&fds);
        rc  = select(maxFD+1,NULL,&fds,NULL,timeout>=0?&maxSelectTime:NULL);
        if (0==rc)
        {
            rc = EDB_TIMEOUT;
            goto done;
        }
        
        // if < 0, something wrong
        if(0>rc)
        {
          rc = SOCKET_GETLASTERROR ;
         // if we failed due to interrupt, let's continue
         if ( EINTR == rc )
         {
            continue ;
         }
         printf("Failed to select from socket ,rc=%d\n",rc);
        }
        if(FD_ISSET(m_fd,&fds))
        {
            break;   
        }
    }

    while(len>0)
    {
        // MSG_NOSIGNAL: Requests not to send SIGPIPE on errors on stream oriented sockets
        // when the other end breaks the connection. The EPIPE error is still returned
        //MSG_NOSIGNAL：禁止send函数向系统发送异常消息
        rc=::send(m_fd,pMsg,len,MSG_NOSIGNAL | flags);
        if(-1==rc)
        {
            printf("Failed to send ,rc = %d\n",SOCKET_GETLASTERROR);
        }
        len -=rc;
        pMsg +=rc;
    }
    rc = EDB_OK;
done:
    return rc;
error:
    goto done;
}


bool ossSocket::isConnected()
{
    int rc = EDB_OK;
    rc  = ::send(m_fd,"",0,MSG_NOSIGNAL);
    if(0>rc)
    {
        return false;
    }
    return true;
}


int ossSocket::recv(char *pMsg, int len,int timeout, int flags )
{
    int rc = EDB_OK;
    int retries = 0;
    int maxFD = m_fd;
    struct timeval maxSelectTime;
    fd_set fds;
    if(0==len)
    {
        return EDB_OK;
    }
   maxSelectTime.tv_sec = timeout / 1000000 ;
   maxSelectTime.tv_usec = timeout % 1000000 ;
   while (true)
   {
       FD_ZERO(&fds);
       FD_SET(m_fd,&fds);
       rc = select(maxFD + 1, &fds, NULL, NULL, timeout >= 0 ? &maxSelectTime : NULL);
       //0 means timeout
       if(0==rc)
       {
           rc = EDB_TIMEOUT;
           goto done;
       }

       //if <0 something wrong
       if(0>rc)
       {
           rc = SOCKET_GETLASTERROR;
           //系统调用被中断
           if(EINTR ==rc)
           {
            continue;
           }
           printf("Failed to select from socket,rc = %d\n", rc);
       }

       if(FD_ISSET(m_fd,&fds))
       {
        break;
       }
   }

    //this socket can read
    while(len>0)
    {
        rc=::recv(m_fd,pMsg,len, MSG_NOSIGNAL|flags ) ;
        if(rc>0)
        {
            if(flags &MSG_NOSIGNAL)
            {
                goto done;
            }
            len -= rc;
            pMsg += rc;
        }
        else if(rc==0)
        {
            printf("Peer unexpected shutdown\n");
        }
        else
        {
            rc = SOCKET_GETLASTERROR;
            if(( EAGAIN == rc || EWOULDBLOCK == rc )  && m_timeout >0)
            {
                printf("Recv timeout: rc=:%d\n", rc);
            }
            if(( EINTR == rc ) && (retries < MAX_RECV_RETRIES ))
            {
                retries++;
                continue;
            }
            printf("Recv Failed:rc =%d\n", rc);
        }
    }
    rc = EDB_OK;
done:
    return rc;
error :
   goto done ;
}

int ossSocket::recvNF(char *pMsg,int len,int timeout)
{
    int rc = EDB_OK;
    int retries = 0;
    int maxFD = m_fd;
    struct timeval maxSelectTime;
    fd_set fds;
    if( 0==len)
    {
        return EDB_OK;
    }

   maxSelectTime.tv_sec = timeout / 1000000 ;
   maxSelectTime.tv_usec = timeout % 1000000 ;
    // wait loop until either we timeout or get a message
    while(true)
    {
        FD_ZERO(&fds);
        FD_SET(m_fd, &fds);
        rc = select(maxFD+1,&fds,NULL,NULL,timeout>=0?&maxSelectTime:NULL ) ;

        //0 means timeout
        if(0==rc)
        {
            rc = EDB_TIMEOUT;
            goto error;
        }

        // if < 0, means something wrong
        if(0>rc)
        {
            rc = SOCKET_GETLASTERROR;
            if(EINTR ==rc)
            {
                continue;
            }
            printf("Failed to select from socket, rc = %d\n ", rc);
        }
        // if the socket we interested is not receiving anything, let's continue
        if ( FD_ISSET ( m_fd, &fds ) )
        {
           break ;
        }
    }

    rc =::recv(m_fd,pMsg,len, MSG_NOSIGNAL ) ;

    if(rc >0)
    {
        len = rc;
    }
    else if(rc==0)
    {
        printf("Peer unexpected shutdown\n");
    }
    else{
        rc = SOCKET_GETLASTERROR;
        if( (EAGAIN == rc || EWOULDBLOCK == rc) && m_timeout >0)
        {
            printf("Recv timeout :rc =%d\n", rc);
        }
        else if((EINTR ==rc) && ( retries < MAX_RECV_RETRIES ))
        {
          retries ++ ;
        }
    }
    rc = EDB_OK;
done:
    return rc;
error :
   goto done ;
}

int ossSocket::connect()
{
    int rc = EDB_OK;
    rc = ::connect(m_fd, (struct sockaddr *)&m_sockAddress, m_addressLen);
    if(rc)
    {
        printf("Failed to connect,rc = %d\n", SOCKET_GETLASTERROR);
    }
    rc = getsockname(m_fd, (sockaddr *)&m_sockAddress, &m_addressLen);
    if(rc)
    {
        printf("Failed to get local address,rc = %d\n", rc);
    }

done:
    return rc;
error:
    goto done;
}


void ossSocket::close()
{

    if(m_init)
    {
        int i = 0;
        i = ::close(m_fd);
        if(i<0)
        {
            i = -1;
        }
        m_init = false;
    }
}


int ossSocket::accept(int *sock, struct sockaddr *addr, socklen_t*addrlen, int timeout)
{
    int rc = EDB_OK;
    int maxFD = m_fd;
    struct timeval maxSelectTime ;
    fd_set fds;
    maxSelectTime.tv_sec = timeout / 1000000 ;
    maxSelectTime.tv_usec = timeout % 1000000 ;
    while (true)
    {
        FD_ZERO(&fds);
        FD_SET(m_fd, &fds);
        rc = select (maxFD+1,&fds,NULL,NULL,timeout>=0?&maxSelectTime:NULL ) ;
        //0 means timeout
        if(0 == rc)
        {
            *sock = 0;
            rc = EDB_TIMEOUT;
            goto done;
        }

        if(0>rc)
        {
             rc = SOCKET_GETLASTERROR ;
             if ( EINTR == rc )
            {
            continue ;
            }
            printf ( "Failed to select from socket, rc = %d", SOCKET_GETLASTERROR);
            rc = EDB_NETWORK ;
            goto error ;
        }
        if(FD_ISSET(m_fd,&fds))
        {
            break;
        }
    }
    rc = EDB_OK;
    *sock = ::accept(m_fd, addr, addrlen);
    if(-1==*sock)
    {
        printf ( "Failed to accept socket, rc = %d", SOCKET_GETLASTERROR ) ;
        rc = EDB_NETWORK ;
        goto error ;
    }

done:
    return rc;
error:
    close();
    goto done;
}


int ossSocket::disableNagle ()
{
    int rc = EDB_OK;

    int temp = 1;   
    rc =  setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, (char *) &temp,
                     sizeof ( int ) ) ;
       if ( rc )
    {
      printf ( "Failed to setsockopt, rc = %d", SOCKET_GETLASTERROR ) ;
    }

    rc = setsockopt ( m_fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &temp,
                     sizeof ( int ) ) ;
   if ( rc )
   {
      printf ( "Failed to setsockopt, rc = %d", SOCKET_GETLASTERROR ) ;
   }
   return rc ;
    
}

unsigned int ossSocket::getPort(sockaddr_in *addr )
{
    return ntohs ( addr->sin_port ) ;
}


int ossSocket::getAddress(sockaddr_in *addr, char *pAddress, unsigned int length)
{
    int rc = EDB_OK;
    length = length < NI_MAXHOST ? length : NI_MAXHOST ;
    rc = getnameinfo ( (struct sockaddr *)addr, sizeof(sockaddr), pAddress,length,NULL, 0, NI_NUMERICHOST ) ;
   if ( rc )
   {
      printf ( "Failed to getnameinfo, rc = %d", SOCKET_GETLASTERROR ) ;
      rc = EDB_NETWORK ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

unsigned int ossSocket::getLocalPort ()
{
    return getPort(&m_sockAddress);
}

unsigned int ossSocket::getPeerPort ()
{
   return getPort ( &m_peerAddress ) ;
}


int ossSocket::getLocalAddress ( char * pAddress, unsigned int length )
{
    return getAddress(&m_sockAddress,pAddress, length ) ;
}


int ossSocket::getPeerAddress ( char * pAddress, unsigned int length )
{
   return getAddress ( &m_peerAddress, pAddress, length ) ;
}


int ossSocket::setTimeout ( int seconds )
{
  int rc = EDB_OK ;
   struct timeval tv ;
   tv.tv_sec = seconds ;
   tv.tv_usec = 0 ;
    rc = setsockopt ( m_fd, SOL_SOCKET, SO_RCVTIMEO, ( char* ) &tv,sizeof ( tv ) ) ;

    if ( rc )
    {
      printf ( "Failed to setsockopt, rc = %d", SOCKET_GETLASTERROR ) ;
    }

    rc = setsockopt ( m_fd, SOL_SOCKET, SO_SNDTIMEO, ( char* ) &tv,sizeof ( tv ) ) ;

    if ( rc )
   {
      printf ( "Failed to setsockopt, rc = %d", SOCKET_GETLASTERROR ) ;
   }

   return rc ;
}

int ossSocket::getHostName(char *pName, int nameLen )
{
    return gethostname ( pName, nameLen ) ;
}


int ossSocket::getPort(const char *pServiceName, unsigned short &port )
{
    int rc = EDB_OK ;
   struct servent *servinfo ;
   servinfo = getservbyname ( pServiceName, "tcp" ) ;
   if ( !servinfo )
      port = atoi ( pServiceName ) ;
   else
      port = (unsigned short)ntohs(servinfo->s_port) ;
   return rc ;
}