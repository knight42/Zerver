#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>  // open()
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"
#include "rio.h"

#define upper(a) ((a)<91?(a):(a)-('a'-'A'))
static char *basedir=NULL;

#define strhash(s) (*(s))
/*
int strhash(char *s)
{
    return *s;
}
*/

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

int parse_uri(char *uri, char *filename, char *cgiargs)
{
    /* return 1 if file is static else 0 */
    char *ptr;
    if(strncmp(uri, "/cgi-bin", strlen("/cgi-bin")) == 0){
        ptr = strchr(uri, '?');
        if(ptr){
            sprintf(cgiargs, "%s", ptr+1);
            *ptr = 0;
        }
        else *cgiargs = 0;

        sprintf(filename, "%s%s", basedir, uri);
        return 0;
    }
    else{
        *cgiargs = 0;
        if(uri[strlen(uri)-1] == '/')
            strcat(uri, "index.html");

        sprintf(filename, "%s%s", basedir, uri);
        return 1;
    }
}

void read_requesthdrs(rio_t *rp)
{
    char buf[BUFSIZ];
    rio_readlineb(rp, buf, BUFSIZ);
    while(strcmp(buf, "\r\n")){
        rio_readlineb(rp, buf, BUFSIZ);
        printf("%s", buf);
    }
}

void get_filetype(char *filename, char *filetype)
{
    char *p = strrchr(filename, '.');
    if(strstr(p, ".html"))
        strcpy(filetype, "text/html");
    else if(strstr(p, ".css"))
        strcpy(filetype, "text/css");
    else if(strstr(p, ".js"))
        strcpy(filetype, "text/javascript");
    else if(strstr(p, ".gif"))
        strcpy(filetype, "image/gif");
    else if(strstr(p, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else if(strstr(p, ".png"))
        strcpy(filetype, "image/png");
    else
        strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    ;
}

void serve_static(int fd, char *filename, long size)
{
    int srcfd, nread;
    char filetype[BUFSIZ], buf[BUFSIZ];
    get_filetype(filename, filetype);
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    strcat(buf, SERVER_STRING);
    sprintf(buf, "%sContent-length: %ld\r\n", buf, size);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    rio_writen(fd, buf, strlen(buf));

    srcfd = open(filename, O_RDONLY);
    while((nread = (int)rio_readn(srcfd, buf, BUFSIZ)) > 0){
        rio_writen(fd, buf, nread);
    }
    close(srcfd);
}

void handle_request(int fd)
{
    char buf[BUFSIZ], method[BUFSIZ]={0}, uri[BUFSIZ], version[BUFSIZ];
    char filename[BUFSIZ], cgiargs[BUFSIZ];
    int is_static;
    struct stat sbuf;
    rio_t rio;


    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, BUFSIZ);
    sscanf(buf, "%s %s %s", method, uri, version);

    /*
    for(c = method; *c; ++c)
        *c = upper(*c);
    */
    //
    read_requesthdrs(&rio);
    // 
    *method = upper(*method);
    switch(strhash(method)){
        case GET:
            is_static = parse_uri(uri, filename, cgiargs);
            if(stat(filename, &sbuf) < 0){
                disperror(fd, filename, "404", "Not found", "Zerver couldnt find this file.");
                return;
            }
            if(is_static){
                if(!(S_IRUSR & sbuf.st_mode) || !(S_ISREG(sbuf.st_mode))){
                    disperror(fd, filename, "403", "Forbidden", "Zerver couldnt read this file.");
                    return;
                }
                serve_static(fd, filename, sbuf.st_size);
            }
            else{
                if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
                    disperror(fd, filename, "403", "Forbidden", "Zerver couldn't read this file.");
                    return;
                }
                serve_dynamic(fd, filename, cgiargs);
            }
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
        strcpy(basedir, BASEDIR);   
    }

#ifdef VERBOSE
    printf("%s\n", basedir);
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
