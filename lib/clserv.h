#ifndef CLSERV_H
#define CLSERV_H

#include <sys/socket.h>

typedef struct sockaddr SA;
#define LISTENQ 1024 /* second argument to listen() */

int open_clientfd(char *hostname, int portno);
int open_listenfd(int portno);

int Open_listenfd(int port);

#endif
