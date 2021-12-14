#ifndef TPOOL_H
#define TPOOL_H

#include <pthread.h>

typedef struct job
{
  void (*routine)(void *arg);
  void *arg;
} job;

typedef struct binsem
{
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int val;
} binsem;

typedef struct job_queue
{
  job **buf;             /* Buffer array */
  int n;                 /* Maximum number of slots */
  int front;             /* buf[(front+1)%n] is first item */
  int rear;              /* buf[rear%n] is last item */
  pthread_mutex_t mutex; /* Protects accesses to buf */
  binsem slots;          /* Protects available slots */
  binsem items;          /* Protects available items */
} job_queue;

typedef struct tpool
{
  job_queue *queue;
  int threads_alive;
  int threads_necessary;
  pthread_mutex_t threads_count_mutex;
  pthread_cond_t threads_wait;
} tpool;

void tpool_init(tpool *pool, int threads_count, int queue_size);
void tpool_free(tpool *pool);

#endif
