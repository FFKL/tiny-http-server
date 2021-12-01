#include "../lib/err.h"
#include "../lib/inout.h"
#include "../lib/rio.h"

#include "multiplex.h"

#define MAXLINE 8192

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
      http_text_init(connfd, &p->client_read[i]);
      p->clientfd[i] = p->client_read[i].fd;

      /* Add the descriptor to descriptor set */
      FD_SET(p->clientfd[i], &p->read_set);

      /* Update max descriptor and pool highwater mark */
      if (p->clientfd[i] > p->maxfd)
        p->maxfd = p->clientfd[i];
      if (i > p->maxi)
        p->maxi = i;
      break;
    }
  if (i == FD_SETSIZE) /* Couldn’t find an empty slot */
    app_error("add_client error: Too many clients");
}

void mult_check_clients(mult_pool *p, int (*handler)(http_text *))
{
  int connfd, n;
  http_text *rio;

  for (int i = 0; (i <= p->maxi) && (p->nready > 0); i++)
  {
    connfd = p->clientfd[i];
    rio = &p->client_read[i];

    /* If the descriptor is ready, echo a text line from it */
    if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set)))
    {
      p->nready--;
      if ((n = http_consume(rio)) == CRLF_HAPPENED)
      {
        int res = handler(rio);
        if (res == -1)
        {
          http_text_free(rio);
          FD_CLR(connfd, &p->read_set);
          p->clientfd[i] = -1;
        }
      }
      else if (n == TEXT_END) /* EOF detected, remove descriptor from pool */
      {
        http_text_free(rio);
        FD_CLR(connfd, &p->read_set);
        p->clientfd[i] = -1;
      }
    }
  }
}