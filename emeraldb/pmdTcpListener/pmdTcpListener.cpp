#include "pmdEDU.h"




int pmdTcpListenerEntryPoint()
{
    int rc = EDB_OK;
    int port = 48127;
    ossSocket sock(port);
    rc = sock.initSocket();
    if(rc)
    {
      printf ( "Failed to initialize socket, rc = %d\n", rc ) ;
      goto error ;
    }
    rc = sock.bindListen();
    if(rc)
    {
       printf("Failed to bind socket,rc=%d\n", rc);
       goto error;
    }

    while (true)
    {
        int client_fd;
        rc = sock.accept(&client_fd, NULL, NULL);
       if ( EDB_TIMEOUT == rc )
       {
        rc = EDB_OK ;
        continue ;
       }
       else if(rc)
       {
          printf(" Failed to accept socket in TcpListener\n");
          break;
       }
       char buffer[1024];
       int size;
       ossSocket client_sock(&client_fd);
       client_sock.disableNagle();
       printf("get new connect\n");
        do
        {
        rc = client_sock.recv ((char*)&size,4 ) ;
        if ( rc && rc != EDB_TIMEOUT )
        {
           printf ( "Failed to receive size, rc = %d\n", rc ) ;
           goto error ;
        }
        }  while ( EDB_TIMEOUT == rc ) ;
       printf ( "%s\n", buffer ) ;
       client_sock.close () ;
    }

done:
   return rc ;
error :
    switch ( rc )
   {
   case EDB_SYS :
      printf ( "System error occured" ) ;
      break ;
   default :
      printf ( "Internal error" ) ;
   }
   goto done ;
}