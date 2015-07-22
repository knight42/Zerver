#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "sockfd.h"

int main(int argc, char *argv[]){
    int sockfd;
    char buffer[BUFSIZ];
    uint16_t port;
    
    if(argc == 2)
        port=(uint16_t)atoi(argv[1]);
    else
        port=9801;

    sockfd=open_clientfd("localhost", port);
    if(sockfd<0){
        fprintf(stderr, "Fail!\n");
        exit(1);
    }
    ssize_t res;
    while(1){
        printf("enter some text: ");
        fgets(buffer, BUFSIZ, stdin);
        write(sockfd, buffer, strlen(buffer));
        res = read(sockfd, buffer, BUFSIZ);
        buffer[res] = 0;
        printf("read from server: %s", buffer);
    }
    return 0;
}

