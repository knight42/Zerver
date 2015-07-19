#include "rio.h"

ssize_t rio_readn(int fd, char *buf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
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

ssize_t rio_writen(int fd, const char *buf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
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


void rio_readinitb(rio_t *rp, int fd)
{
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

static ssize_t rio_read(rio_t *rp, char *userbuf, size_t n)
{
    ssize_t cnt;
    while(rp->rio_cnt <= 0){
        /* ssize_t read( int fd, void *buf, int nbytes ) */
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
        if(rp->rio_cnt < 0){
            if(errno != EINTR)
                return -1;
        }
        else if(rp->rio_cnt == 0)
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf;
    }
    cnt = n;
    if(rp->rio_cnt < n){
        cnt = rp->rio_cnt;
    }
    /* void * memcpy( void *restrict dest, const void *restrict src, int n ) */
    memcpy(userbuf, rp->rio_bufptr, (int)cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

ssize_t rio_readlineb(rio_t *rp, char *buf, size_t maxlen)
{
    int n;
    ssize_t rc;
    char c;
    for(n=1; n<maxlen; ++n){
        // read until '\n'
        if((rc = rio_read(rp, &c, 1)) == 1){
            *buf++ = c;
            if(c == '\n') break;
        }
        // EOF
        else if(rc == 0){
            if(n == 1) return 0;
            else break;
        }
        // Error occurs
        else return -1;
    }
    *buf = 0;
    return n;
}
