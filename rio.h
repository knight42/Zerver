#include <stdio.h>
#include <errno.h>


/* Robust IO functions */

typedef struct {
    int rio_fd;
    int rio_cnt;
    char *nextptr;
    char rio_buf[BUFSIZ];
} rio_t;

/* without buffer */
ssize_t rio_read(int fd, void *userbuf, size_t n);
ssize_t rio_write(int fd, void *userbuf, size_t n);

/* with userbuffer */
ssize_t rio_readlineb(rio_t *rp, void *userbuf, size_t maxlen);
ssize_t rio_readb(rio_t *rp, void *userbuf, size_t n);


/* implementation */
ssize_t rio_read(int fd, void *userbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *buf = (char *)userbuf;
    while(nleft > 0){
        if((nread = read(fd, buf, nleft)) < 0){
           // interupt by sig handler return and call read() again
            if(errno == EINTR){
                nread = 0;
            }
            else{
                // errno set by read()
                return -1;
            }
        }
        else if(nread == 0){
            break;
        }
        nleft -= nread;
        buf += nread;
    }
    return n - nleft;
}

ssize_t rio_write(int fd, void *userbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    char *buf = (char *)userbuf;
    while(nleft > 0){
        if((nwritten = write(fd, buf, nleft)) <= 0){
            if(errno == EINTR){
                nwritten = 0;
            }
            else
                return -1;
        }
        nleft -= nwritten;
        buf += nwritten;
    }
    return n;
}
