#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"

/* return a listen fd according to the port */
int open_listenfd(uint16_t port);

int main(int argc, char *argv[]){
    int opt, serv_listenfd;
    uint16_t port=34696;
    int isproxy=0;
    while((opt=getopt(argc, argv, "xhp:")) != -1){
        switch(opt){
            case 'p':
                port = (uint16_t)atoi(optarg);
                break;
            case 'x':
                isproxy=1;
                break;
            default:
                printf("Usage:\nzerver -p port\n");
                break;
        }
    }

    if((serv_listenfd = open_listenfd(port)) < 0){
        fprintf(stderr, "fail to open listen socket\n");
        exit(1);
    }
    
    return 0;
}

int open_listenfd(uint16_t port)
{
    int serv_sockfd ;
    struct sockaddr_in serv_addr;

    serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(serv_sockfd < 0) return -1;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
    if(bind(serv_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        return -1;

    if(listen(serv_sockfd, MAXLISTEN) < 0)
        return -1;

    return serv_sockfd;
}
