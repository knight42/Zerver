#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include "sockfd.h"

int main(int argc, char *argv[]){
    int sockfd;
    char buffer[BUFSIZ];
    uint16_t port=9801;
    
    if(argc == 2)
        port=(uint16_t)atoi(argv[1]);

    sockfd = open_clientfd("127.0.0.1", port);

    ssize_t res;
    /*
    while(1){
        printf("enter some text: ");
        fgets(buffer, BUFSIZ, stdin);
        write(sockfd, buffer, strlen(buffer));
        res = read(sockfd, buffer, BUFSIZ);
        buffer[res] = 0;
        printf("read from server: %s", buffer);
    }
    */
    sprintf(buffer, "GET / HTTP/1.0\r\n\r\n");
    write(sockfd, buffer, strlen(buffer));
    res = read(sockfd, buffer, BUFSIZ);
    buffer[res] = 0;
    printf("read from server:\n%s\n", buffer);
    return 0;
}

