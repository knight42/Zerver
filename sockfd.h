#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#define MAXLISTEN 5
#define diehere(prog) do{ perror(prog); exit(1); }while(0)

int open_listenfd(uint16_t *port);
int open_clientfd(const char *hostorip, uint16_t port);
