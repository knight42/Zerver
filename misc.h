#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>

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
