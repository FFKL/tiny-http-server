#include "../lib/err.h"
#include "../lib/inout.h"

#include "multiplex.h"

#define MAXLINE 8192

int byte_cnt = 0; /* Counts total bytes received by server */

void mult_init_pool(int listenfd, mult_pool *p)
{
  /* Initially, there are no connected descriptors */
  p->maxi = -1;
  for (int i = 0; i < FD_SETSIZE; i++)
    p->clientfd[i] = -1;

  /* Initially, listenfd is only member of select read set */
  p->maxfd = listenfd;
  FD_ZERO(&p->read_set);
  FD_SET(listenfd, &p->read_set);
}

void mult_add_client(int connfd, mult_pool *p)
{
  int i;
  p->nready--;
  for (i = 0; i < FD_SETSIZE; i++) /* Find an available slot */
    if (p->clientfd[i] < 0)
    {
      /* Add connected descriptor to the pool */
      p->clientfd[i] = connfd;
      Rio_readinitb(&p->clientrio[i], connfd);

      /* Add the descriptor to descriptor set */
      FD_SET(connfd, &p->read_set);

      /* Update max descriptor and pool highwater mark */
      if (connfd > p->maxfd)
        p->maxfd = connfd;
      if (i > p->maxi)
        p->maxi = i;
      break;
    }
  if (i == FD_SETSIZE) /* Couldn’t find an empty slot */
    app_error("add_client error: Too many clients");
}

void mult_check_clients(mult_pool *p)
{
  int connfd, n;
  char buf[MAXLINE];
  rio_t rio;

  for (int i = 0; (i <= p->maxi) && (p->nready > 0); i++)
  {
    connfd = p->clientfd[i];
    rio = p->clientrio[i];

    /* If the descriptor is ready, echo a text line from it */
    if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set)))
    {
      p->nready--;
      if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
      {
        byte_cnt += n;
        printf("Server received %d (%d total) bytes on fd %d\n", n, byte_cnt, connfd);
        Rio_writen(connfd, buf, n);
      }
      else /* EOF detected, remove descriptor from pool */
      {
        Close(connfd);
        FD_CLR(connfd, &p->read_set);
        p->clientfd[i] = -1;
      }
    }
  }
}