#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#define 
//typedef 

int main(int argc, char *argv[]){
    int opt;
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

    int serv_sockfd, cli_sockfd;
    struct sockaddr_in serv_address, cli_address;
    unsigned serv_len, cli_len;
    serv_len = cli_len = sizeof(struct sockaddr_in);
    serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_address.sin_family = AF_INET;
    serv_address.sin_port = htons(port);
    bind(serv_sockfd, (struct sockaddr*)&serv_address, serv_len);

    return 0;
}

