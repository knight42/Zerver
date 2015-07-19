#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

int open_clientfd(const char *hostname, uint16_t port)
{
    int clifd;
    struct hostent *hp;
    struct sockaddr_in servaddr;
    if((clifd=socket(AF_INET, SOCK_STREAM, 0))<0)
        return -1;
    // netdb.h
    if((hp=gethostbyname(hostname))==NULL)
        return -2;
    bzero((char *)&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr_list[0], 
          (char *)&servaddr.sin_addr.s_addr, hp->h_length);
    
    servaddr.sin_port = htons(port);
    if(connect(clifd, (struct sockaddr*)&servaddr, sizeof(servaddr))<0)
        return -1;
    return clifd;
}

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

