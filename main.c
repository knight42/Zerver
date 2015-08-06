#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "config.h"
#include "rio.h"
#include "sockfd.h"

#define upper(a) ((a)<91?(a):(a)-('a'-'A'))
#define strhash(s) (*(s))

void disperror(int fd, const char *cause, const char *errnum,
                const char *shortmsg, const char *longmsg);
int parse_uri(char *uri, char *filename, char *cgiargs);
void read_requesthdrs(rio_t *rp, char *cgiargs);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, long size);
int handle_request(int fd);

void SigHandle(int sig){
    switch(sig){
        case SIGPIPE:
            fprintf(stderr, "\nBroken Pipe\n");
            break;
        case SIGCHLD:
            while(waitpid(-1, 0, WNOHANG) > 0) ;
            break;
    }
}

static char basedir[BUFSIZ]={0};

#define VERBOSE

int main(int argc, char *argv[]){
    struct sockaddr_in cli_addr;
    socklen_t cli_len=sizeof(cli_addr);;
    int opt, listenfd, connfd;
    uint16_t port=34696;
    
    //long cpucores = sysconf(_SC_NPROCESSORS_ONLN);

    signal(SIGPIPE, SigHandle);
    signal(SIGCHLD, SigHandle);

    while((opt = getopt(argc, argv, "hp:w:")) != -1){
        char *c;
        struct stat sb;
        switch(opt){
            case 'p':
                port = (uint16_t)atoi(optarg);
                break;
            case 'w':
                strncpy(basedir, optarg, BUFSIZ);
                c = basedir + strlen(basedir) - 1;
                *c = (*c == '/') ? 0 : *c;
                if(stat(basedir, &sb) < 0){
                    diehere("stat");
                }
                if((sb.st_mode & S_IFMT) != S_IFDIR){
                    fprintf(stderr, "Invalid working basedir: %s\n", basedir);
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

    if(*basedir == 0){
        int l = strlen(argv[0]);
        strncpy(basedir, argv[0], BUFSIZ);
        char *c = basedir + l;
        for(; *c != '/'; --c);
        *c = 0;
    }

#ifdef VERBOSE
    printf("working dir: %s\n", basedir);
#endif

    listenfd = open_listenfd(&port);
    {
        int flags;
        flags = fcntl(listenfd, F_GETFD);
        flags |= O_NONBLOCK;
        fcntl(listenfd, F_SETFD, flags);
    }

    printf("listen port: %d\n", port);

    struct epoll_event ev, events[MAXEVENTS];
    int efd, nfds, i;
    efd = epoll_create1(0);
    ev.data.fd = listenfd;
    //ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ev.events = EPOLLIN;
    epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &ev);
    for(;;){
        puts("waiting...");
        nfds = epoll_wait(efd, events, MAXEVENTS, -1);
        puts("new event!");
        for(i=0; i<nfds; ++i){
            if(events[i].events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP) ||
                    !(events[i].events & EPOLLIN)){
                fprintf(stderr, "error on fd: %d: %d\n", events[i].data.fd, events[i].events);
                close(events[i].data.fd);
                continue;
            }
            if(events[i].data.fd == listenfd){
                bzero(&cli_addr, sizeof(cli_addr));
                connfd = accept4(listenfd, (struct sockaddr*)&cli_addr,
                                 &cli_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
                ev.data.fd = connfd;
                if(epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &ev) < 0){
                    fprintf(stderr, "epoll_ctl add error\n");
                    perror("epoll_ctl add");
                }
                #ifdef VERBOSE
                printf("new client fd: %d\n", connfd);
                #endif
            }
            else{
                handle_request(events[i].data.fd);
                /* should we delete the connection fd after request is finished?
                if(epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, NULL) < 0){
                    fprintf(stderr, "epoll_ctl del error\n");
                    perror("epoll_ctl del");
                }
                */
                close(events[i].data.fd);
                #ifdef VERBOSE
                printf("client fd: %d leave\n", events[i].data.fd);
                #endif
            }
        }
    }

    return 0;
}

void disperror(int fd, const char *cause, const char *errnum,
                const char *shortmsg, const char *longmsg)
{
    char buf[BUFSIZ], body[BUFSIZ];
    sprintf(body, "<html><title>%s %s</title>", errnum, shortmsg);
    strcat(body, "<body bgcolor='#ffffff'>\r\n");
    sprintf(body, "%s<h1>%s: %s</h1>\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s</p>\r\n", body, longmsg, cause);
    strcat(body, "<hr/>Powered by <em>Zerver/1.0.0</em></body></html>\r\n");

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

        sprintf(filename, "%s%s", basedir, uri);
        return 1;
    }
}

void read_requesthdrs(rio_t *rp, char *cgiargs)
{
    char buf[BUFSIZ], *p;
    int has_content=0, nread=0;
    rio_readlineb(rp, buf, BUFSIZ);
    while(strcmp(buf, "\r\n")){
        if(strcasestr(buf, "content-length")){
            has_content = 1;
            p = strchr(buf, ' ');
            nread = atoi(p);
        }
        rio_readlineb(rp, buf, BUFSIZ);
        if(errno == EAGAIN) break;
        #ifdef VERBOSE
        printf("%s", buf);
        #endif
    }
    if(has_content){
        rio_readnb(rp, cgiargs, nread);
        cgiargs[nread] = 0;
        #ifdef VERBOSE
        printf("%s\n", cgiargs);
        #endif
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
    char buf[BUFSIZ], *emptylist[] = {NULL};
    sprintf(buf, "%s", "HTTP/1.0 200 OK\r\n");
    strcat(buf, SERVER_STRING);
    rio_writen(fd, buf, strlen(buf));
    if(fork() == 0){
        setenv("QUERY_STR", cgiargs, 1);
        dup2(fd, STDOUT_FILENO);
        execve(filename, emptylist, __environ);
    }
    wait(NULL);
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

int handle_request(int fd)
{
    char buf[BUFSIZ], method[BUFSIZ], uri[BUFSIZ], version[BUFSIZ], \
         filename[BUFSIZ], cgiargs[BUFSIZ];
    int is_static;
    struct stat sbuf;
    ssize_t nread;
    rio_t rio;

    rio_readinitb(&rio, fd);
    nread = rio_readlineb(&rio, buf, BUFSIZ);

    #ifdef VERBOSE
    printf("%s", buf);
    #endif

    // blank uri
    if(nread == 0) return 0;

    sscanf(buf, "%s %s %s", method, uri, version);
    if(uri[strlen(uri)-1] == '/')
        strncat(uri, "index.html", BUFSIZ);

    read_requesthdrs(&rio, cgiargs);

    *method = upper(*method);
    switch(strhash(method)){
        case GET:
            is_static = parse_uri(uri, filename, cgiargs);
            if(stat(filename, &sbuf) < 0){
                disperror(fd, filename, "404", "Not found", "Zerver couldnt find this file.");
                return 1;
            }
            if(is_static){
                if(!(S_IRUSR & sbuf.st_mode) || !(S_ISREG(sbuf.st_mode))){
                    disperror(fd, filename, "403", "Forbidden", "Zerver couldnt read this file.");
                    return 1;
                }
                serve_static(fd, filename, sbuf.st_size);
            }
            else{
                if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
                    disperror(fd, filename, "403", "Forbidden", "Zerver couldn't read this file.");
                    return 1;
                }
                serve_dynamic(fd, filename, cgiargs);
            }
            break;
        case POST:
            sprintf(filename, "%s%s", basedir, uri);
            if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
                disperror(fd, filename, "403", "Forbidden", "Zerver couldn't read this file.");
                return 1;
            }
            serve_dynamic(fd, filename, cgiargs);
            break;
        case HEAD:
            disperror(fd, "HEAD request", "200", "OK", "Succeed");
            break;
        default:
            disperror(fd, "", "501", "Method Not Implemented", "");
            return 2;
    }
    return 0;
}
