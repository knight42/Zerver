#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>  // open()
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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
void handle_request(int fd);

void SIGPIPE_Handle(int sig){
    //int res;
    //res=close(CLIFD);
    fprintf(stderr, "-----------------------------");
    fprintf(stderr, "Recover!");
    //printf("close result: %d\n",res);
    fprintf(stderr, "-----------------------------");
}

void SIGCHLD_Handle(int sig){
    /*
    int status, pid;
    pid = waitpid(-1, &status, WNOHANG);
    if (WIFEXITED(status)) {   
        printf("The child %d exit with code %d\n", pid, WEXITSTATUS(status));   
    }   
    */
    // wait for all children
    while(waitpid(-1, 0, WNOHANG) > 0) ;
    return;
}

static char *basedir=NULL;

int main(int argc, char *argv[]){
    int opt, listenfd, connfd;
    uint16_t port=34696;
    struct sockaddr_in cli_addr;
    int isproxy=0;
    
    //long cpucores = sysconf(_SC_NPROCESSORS_ONLN);
    
    signal(SIGPIPE, SIGPIPE_Handle);
    signal(SIGCHLD, SIGCHLD_Handle);

    while((opt = getopt(argc, argv, "xhp:w:")) != -1){
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
        basedir = BASEDIR;
    }

#ifdef VERBOSE
    printf("%s\n", basedir);
    exit(0);
#endif

    listenfd = open_listenfd(&port);
    socklen_t cli_len=sizeof(cli_addr);;
    printf("listen port: %d\n", port);
    while(1){
        connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &cli_len);
        if(fork() == 0){
            close(listenfd);
            handle_request(connfd);
            exit(0);
        }
        close(connfd);
    }
    return 0;
}

void disperror(int fd, const char *cause, const char *errnum,
                const char *shortmsg, const char *longmsg)
{
    char buf[BUFSIZ], body[BUFSIZ];
    sprintf(body, "<html><title>Zerver Error</title>");
    strcat(body, "<body bgcolor='#ffffff'>\r\n");
    sprintf(body, "%s<h1>%s: %s</h1>\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s</p>\r\n", body, longmsg, cause);
    strcat(body, "<hr/><em>Zerver/1.0.0</em>");

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

void handle_request(int fd)
{
    char buf[BUFSIZ], method[BUFSIZ], uri[BUFSIZ], version[BUFSIZ], \
         filename[BUFSIZ], cgiargs[BUFSIZ];
    int is_static;
    struct stat sbuf;
    ssize_t nread;
    rio_t rio;

    rio_readinitb(&rio, fd);
    nread = rio_readlineb(&rio, buf, BUFSIZ);
    // blank uri
    if(nread == 0) return;

    sscanf(buf, "%s %s %s", method, uri, version);
    if(uri[strlen(uri)-1] == '/')
        strcat(uri, "index.html");

    read_requesthdrs(&rio, cgiargs);

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
            sprintf(filename, "%s%s", basedir, uri);
            if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
                disperror(fd, filename, "403", "Forbidden", "Zerver couldn't read this file.");
                return;
            }
            serve_dynamic(fd, filename, cgiargs);
            break;
        case HEAD:
            disperror(fd, "HEAD request", "200", "OK", "Succeed");
            break;
        default:
            disperror(fd, "", "501", "Method Not Implemented", "");
            break;
    }
}
