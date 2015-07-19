#include <stdio.h>
#include "rio.h"
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define MAXLISTEN 3

void diehere(const char *prog){
    perror(prog);
    exit(1);
}

int open_listenfd(uint16_t *port)
{
    int serv_sockfd ;
    struct sockaddr_in serv_addr;

    serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(serv_sockfd < 0){
        diehere("socket");
    }

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(*port);

    if(bind(serv_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        diehere("bind");

    // get actual port when it is allocated dynamically
    if(*port == 0){
        socklen_t addrlen = sizeof(serv_addr);
        if((getsockname(serv_sockfd, (struct sockaddr*)&serv_addr, &addrlen)) < 0){
            diehere("getsockname");
        }
        *port = ntohs(serv_addr.sin_port);
    }

    if(listen(serv_sockfd, MAXLISTEN) < 0)
        diehere("listen");

    return serv_sockfd;
}

void echo(int fd){
    size_t n;
    char buf[BUFSIZ];
    rio_t rio;
    rio_readinitb(&rio, fd);
    while((n=rio_readlineb(&rio, buf, BUFSIZ))!=0){
        printf("server received: %zu bytes\n", n);
        rio_writen(fd, buf, n);
    }
}

int main(int argc, char *argv[]){
    int servfd, connfd;
    struct sockaddr_in cliaddr;
    struct hostent *hp;
    char *haddrp;
    socklen_t clilen;
    uint16_t port;

    if(argc == 2)
        port=(uint16_t)atoi(argv[1]);
    else port=0;
    servfd=open_listenfd(&port);

    printf("port is: %d\n", port);
    while(1){
        clilen=sizeof(cliaddr);
        connfd = accept(servfd, (struct sockaddr*)&cliaddr, &clilen);
        // netdb.h
        hp=gethostbyaddr((const char*)&cliaddr.sin_addr.s_addr, sizeof(cliaddr.sin_addr.s_addr), AF_INET);
        haddrp=inet_ntoa(cliaddr.sin_addr);
        printf("connected to %s (%s)\n", hp->h_name, haddrp);
        echo(connfd);
        close(connfd);
    }
    return 0;
}

