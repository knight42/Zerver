#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"

/* return a listen fd according to the port */
//int open_listenfd(uint16_t *port);
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

    memset(&serv_addr, 0, sizeof(serv_addr));
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

void handle_request(int fd)
{
    char buf[BUFSIZ];
}

void serve_static()
{;}

int main(int argc, char *argv[]){
    int opt, listen_sock, conn_sock;
    uint16_t port=34696;
    struct sockaddr_in cli_addr;
    int isproxy=0;
    char *basedir = NULL;

    //long cpucores = sysconf(_SC_NPROCESSORS_ONLN);
    
    while((opt=getopt(argc, argv, "xhp:w:")) != -1){
        char *c;
        struct stat sb;
        switch(opt){
            case 'p':
                port = (uint16_t)atoi(optarg);
                break;
            case 'x':
                isproxy=1;
                break;
            case 'w':
                basedir = optarg;
                c = basedir + strlen(basedir) - 1;
                *c = (*c == '/') ? 0 : *c;
                if(stat(basedir, &sb) < 0){
                    diehere("stat");
                }
                if((sb.st_mode & S_IFMT) != S_IFDIR){
                    fprintf(stderr, "Invalid working dir: %s\n", basedir);
                    exit(1);
                }
                break;
            default:
                puts("Usage:");
                printf("zerver -p port ");
                printf("-w basedir ");
                puts("");
                exit(0);
        }
    }

    if(!basedir){
        basedir = (char *)malloc(sizeof(char)*256);
        strcpy(basedir, ".");   
    }

#ifdef VERBOSE
    printf("%s\n", basedir);
    exit(0);
#endif

    listen_sock = open_listenfd(&port);
    socklen_t cli_len;
    while(1){
        cli_len = sizeof(cli_addr);
        conn_sock = accept(listen_sock, (struct sockaddr*)&cli_addr, &cli_len);
        handle_request(conn_sock);
    }
    return 0;
}
