#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"
#include "rio.h"

#define upper(a) ((a)<91?(a):(a)-('a'-'A'))
static const char *BaseDir=NULL;

int strhash(char *s)
{
    return *s;
}

void diehere(const char *prog){
    perror(prog);
    exit(1);
}

int open_listenfd(uint16_t *port)
{
    int serv_sockfd ,optval=1;
    struct sockaddr_in serv_addr;

    serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(serv_sockfd < 0){
        diehere("socket");
    }

    // Eliminates "Address alrealy in use" error from bind
    if(setsockopt(serv_sockfd, SOL_SOCKET, SO_REUSEADDR,
                (const void *)&optval, sizeof(int)) < 0)
        return -1;

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

void disperror(int fd, const char *cause, const char *errnum,
                const char *shortmsg, const char *longmsg)
{
    char buf[BUFSIZ], body[BUFSIZ];
    sprintf(body, "<html><title>Zerver Error</title>");
    strcat(body, "<body bgcolor='#ffffff'>\r\n");
    sprintf(body, "%s<h1>%s: %s</h1>\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s</p>\r\n", body, longmsg, cause);
    strcat(body, "<hr></hr><em>Zerver/1.0.0</em>");

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %lu\r\n\r\n", strlen(body));
    rio_writen(fd, buf, strlen(buf));
    rio_writen(fd, body, strlen(body));
}

void serve_static()
{;}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
    /* return 1 if file is static else 0 */
    char *ptr;
    ptr = strchr(uri, '?');
    if(ptr == NULL){
        *cgiargs = 0;
        sprintf(filename, "%s%s", BaseDir, uri);
        return 1;
    }
    if(ptr != strrchr(uri, '?')){
        fprintf(stderr, "invalid request: %s\n", uri);
        exit(1);
    }

    return 0;
}


void handle_request(int fd)
{
    char buf[BUFSIZ], method[BUFSIZ]={0}, uri[BUFSIZ], version[BUFSIZ];
    char filename[BUFSIZ], cgiargs[BUFSIZ], *c;
    int is_static;
    struct stat sbuf;
    rio_t rio;


    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, BUFSIZ);
    sscanf(buf, "%s %s %s", method, uri, version);
    
    for(c = method; *c; ++c)
        *c = upper(*c);

    is_static = parse_uri(uri, filename, cgiargs);

    switch(strhash(method)){
        case GET:
            disperror(fd, "browser", "200", "server hello", "succeed in handling your request");
            break;
        case POST:
            break;
        case HEAD:
            break;
        default:
            break;
    }
}

int main(int argc, char *argv[]){
    int opt, listenfd, connfd;
    uint16_t port=34696;
    struct sockaddr_in cli_addr;
    int isproxy=0;
    
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
                BaseDir = optarg;
                c = BaseDir + strlen(BaseDir) - 1;
                *c = (*c == '/') ? 0 : *c;
                if(stat(BaseDir, &sb) < 0){
                    diehere("stat");
                }
                if((sb.st_mode & S_IFMT) != S_IFDIR){
                    fprintf(stderr, "Invalid working dir: %s\n", BaseDir);
                    exit(1);
                }
                break;
            default:
                puts("Usage:");
                printf("zerver -p port ");
                printf("-w BaseDir ");
                puts("");
                exit(0);
        }
    }

    if(!BaseDir){
        BaseDir = (char *)malloc(sizeof(char)*256);
        strcpy(BaseDir, BASEDIR);   
    }

#ifdef VERBOSE
    printf("%s\n", BaseDir);
    exit(0);
#endif

    listenfd = open_listenfd(&port);
    socklen_t cli_len=sizeof(cli_addr);;
    printf("listen port: %d\n", port);
    while(1){
        puts("I am listening~");
        connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &cli_len);
        handle_request(connfd);
        close(connfd);
    }
    return 0;
}
