#ifndef TPOOL_H
#define TPOOL_H

#include <pthread.h>

typedef void (*routine)(void *arg);
typedef void *routine_arg;

typedef struct job
{
  routine routine;
  routine_arg arg;
} job;

typedef struct job_queue
{
  job **buf;
  int n;
  int front; /* buf[(front+1)%n] is first item */
  int rear;  /* buf[rear%n] is last item */
  int filled;
} job_queue;

typedef struct tpool
{
  job_queue *queue;
  int threads_alive;
  int threads_necessary;
  pthread_mutex_t lock;
  pthread_cond_t threads_wait;
  pthread_cond_t producers_wait;
} tpool;

void tpool_init(tpool *pool, int threads_count, int queue_size);
void tpool_free(tpool *pool);
void tpool_push_job(tpool *pool, routine routine, routine_arg arg);

#endif
