#include "sockfd.h"

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

    bzero(&serv_addr, sizeof(serv_addr));
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

int open_clientfd(const char *hostorip, uint16_t port)
{
    int clifd;
    struct hostent *hp;
    struct sockaddr_in servaddr;

    if((clifd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;

    if(((int)inet_addr(hostorip)) == -1){  // hostname
        // in netdb.h
        if((hp = gethostbyname(hostorip)) == NULL)
            return -2;
        strncpy((char *)hp->h_addr_list[0], 
                (char *)&servaddr.sin_addr.s_addr,
                hp->h_length);
    }
    else{  // ip address
        servaddr.sin_addr.s_addr = inet_addr(hostorip);
    }

    servaddr.sin_port = htons(port);

    if(connect(clifd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        return -1;

    return clifd;
}
