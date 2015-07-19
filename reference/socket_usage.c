#include <stdio.h>

#include <sys/socket.h>
/* Create a new socket of type TYPE in domain DOMAIN, using
   protocol PROTOCOL.  If PROTOCOL is zero, one is chosen automatically.
   Returns a file descriptor for the new socket, or -1 for errors.  */
int socket( int domain, int type, int protocol );
sockfd = socket(AF_INET, SOCK_STREAM, 0);  // network
sockfd = socket(AF_UNIX, SOCK_STREAM, 0);  // local


/* Give the socket FD the local address ADDR (which is LEN bytes long).  */
int bind( int sockfd, struct sockaddr *my_addr, int addrlen );
#include <string.h>
bzero(&serv_addr, sizeof(serv_addr));
serv_addr.sin_family = AF_INET;
#include <netinet/in.h>
serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
serv_addr.sin_port = htons(*port);
bind(serv_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));


/* Prepare to accept connections on socket FD.
   N connection requests will be queued before further requests are refused.
   Returns 0 on success, -1 for errors.  */
int listen( int sockfd, int backlog );
listen(servfd, MAXLISTEN);


/* Await a connection on socket FD.
   When a connection arrives, open a new socket to communicate with it,
   set *ADDR (which is *ADDR_LEN bytes long) to the address of the connecting
   peer and *ADDR_LEN to the address's actual length, and return the
   new socket's descriptor, or -1 for errors.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
int accept(int listenfd, struct sockaddr *cliaddr, int *addrlen);
connfd = accept(servfd, (struct sockaddr*)&cliaddr, &clilen);


/* Open a connection on socket FD to peer at ADDR (which LEN bytes long).
   For connectionless socket types, just set the default address to send to
   and the only address from which to accept transmissions.
   Return 0 on success, -1 for errors.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
int connect( int fd, const struct sockaddr *addr, socklen_t len );
connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
// 得到的连接由套接字对 (client_ip:local_port, serv_addr.sin_addr:serv_addr.sin_port) 确定


/*
server 流程
创建 socket -> bind address -> listen on socket -> accept request -> handle request

client 流程
创建 socket -> connect -> send request
 */
