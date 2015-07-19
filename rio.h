#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* Robust IO functions */

typedef struct {
    char rio_buf[BUFSIZ];
    char *rio_bufptr;
    ssize_t rio_cnt;
    int rio_fd;
} rio_t;

/* without buffer */
ssize_t rio_readn(int fd, char *buf, size_t n);
ssize_t rio_writen(int fd, const char *buf, size_t n);

/* with buffer */
void rio_readinitb(rio_t *rp, int fd);
ssize_t rio_readlineb(rio_t *rp, char *userbuf, size_t maxlen);
