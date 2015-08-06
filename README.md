README
======

Zerver is a simple http server supported by epoll(may be replaced with libevent/libev in the future). 


Files         | Description
------------  | -------------
main.c        | as its name implies
config.h      | some configuration. You can change the max events epoll handle via this file.
rio.{h,c}     | robust io functions used in network-programming from CSAPP
sockfd.{h,c}  | wrappers to create listen socket and connection socket

