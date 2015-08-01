#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

void SetNonBlocking(int fd)
{
    int flag;
    flag = fcntl(fd, F_GETFL, 0);
    if(flag < 0){
        perror("fcntl");
        exit(1);
    }
    flag |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flag) < 0){
        perror("fcntl");
        exit(1);
    }
}
