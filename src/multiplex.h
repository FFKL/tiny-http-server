#ifndef MULTIPLEX_H
#define MULTIPLEX_H

#include "http.h"

#include <sys/select.h>

typedef struct mult_pool /* Represents a pool of connected descriptors */
{
  int maxfd;                   /* Largest descriptor in read_set */
  fd_set read_set;             /* Set of all active descriptors */
  fd_set ready_set;            /* Subset of descriptors ready for reading */
  int nready;                  /* Number of ready descriptors from select */
  int maxi;                    /* Highwater index into client array */
  int clientfd[FD_SETSIZE];    /* Set of active descriptors */
  http_text client_read[FD_SETSIZE]; /* Set of active read buffers */
} mult_pool;

void mult_init_pool(int listenfd, mult_pool *p);
void mult_add_client(int connfd, mult_pool *p);
void mult_check_clients(mult_pool *p, int (*handler)(http_text *));

#endif
