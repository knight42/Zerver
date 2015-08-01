#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>

void SIGPIPE_Handle(int sig){
    fprintf(stderr, "------------------\nBroken Pipe\n");
    fprintf(stderr, "------------------\n");
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
}
