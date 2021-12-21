#include <stdio.h>

#include "../lib/concurrency.h"
#include "../lib/err.h"
#include "../lib/memory.h"

#include "tpool.h"

#define MAX_THREADS 16
#define MIN_THREADS 2

static job *tpool_pull_job(tpool *pool);

static job_queue *job_queue_create(int n);
static void job_queue_destroy(job_queue *queue);

static void threads_create(tpool *pool, int n);
static void threads_spawn(tpool *pool);
static void threads_reduce(tpool *pool);

static void thread_create(tpool *pool);
static void *thread_routine(tpool *pool);

static int is_power_of_two(int n);

void tpool_init(tpool *pool, int threads_count, int queue_size)
{
  if (!is_power_of_two(threads_count))
    app_error("Number of threads must be a power of two");

  Pthread_mutex_init(&pool->lock, NULL);
  Pthread_cond_init(&pool->threads_wait, NULL);
  pool->threads_alive = 0;
  pool->threads_necessary = threads_count;
  pool->queue = job_queue_create(queue_size);

  threads_create(pool, threads_count);
  printf("Thread pool initialized with %d queue size and %d threads count\n", queue_size, threads_count);
}

void tpool_free(tpool *pool)
{
  Pthread_mutex_destroy(&pool->lock);
  Pthread_cond_destroy(&pool->threads_wait);
  job_queue_destroy(pool->queue);
}

static job_queue *job_queue_create(int n)
{
  job_queue *queue = Malloc(sizeof(job_queue));
  queue->buf = Malloc(n * sizeof(job));
  queue->n = n;
  queue->front = queue->rear = 0;
  queue->filled = 0;
  return queue;
}

static void job_queue_destroy(job_queue *queue)
{
  // TODO: wait for jobs finishing and iterate over the job queue to free memory ??
  Free(queue);
}

void tpool_push_job(tpool *pool, routine routine, routine_arg arg)
{
  printf("Thread %ld: Push the job\n", Pthread_self());
  job_queue *queue = pool->queue;

  job *new_job = Malloc(sizeof(job));
  new_job->routine = routine;
  new_job->arg = arg;

  Pthread_mutex_lock(&pool->lock); // 🔒

  while (queue->filled == queue->n)
    Pthread_cond_wait(&pool->producers_wait, &pool->lock); // 💤
  queue->buf[(++queue->rear) % (queue->n)] = new_job;
  queue->filled += 1;

  if (queue->filled == queue->n)
    threads_spawn(pool);

  Pthread_cond_signal(&pool->threads_wait); // ⚡

  Pthread_mutex_unlock(&pool->lock); // 🔓
  printf("Thread %ld: Job pushed\n", Pthread_self());
}

static job *tpool_pull_job(tpool *pool)
{
  job_queue *queue = pool->queue;
  job *job = queue->buf[(++queue->front) % (queue->n)];

  queue->filled -= 1;

  return job;
}

static void threads_create(tpool *pool, int n)
{
  for (int i = 0; i < n; i++)
    thread_create(pool);
}

static void threads_spawn(tpool *pool)
{
  if (pool->threads_necessary == MAX_THREADS)
    return;
  pool->threads_necessary = pool->threads_necessary * 2;
  threads_create(pool, pool->threads_necessary);
  printf("Number of threads spawned to %d\n", pool->threads_necessary);
}

static void threads_reduce(tpool *pool)
{
  if (pool->threads_necessary == MIN_THREADS)
    return;
  pool->threads_necessary = pool->threads_necessary / 2;
  Pthread_cond_broadcast(&pool->threads_wait);
  printf("Number of threads reduced to %d\n", pool->threads_necessary);
}

static void thread_create(tpool *pool)
{
  pthread_t tid;
  Pthread_create(&tid, NULL, (void *(*)(void *))thread_routine, pool);
  Pthread_detach(tid);
}

static void *thread_routine(tpool *pool)
{
  printf("Thread %ld: Initialization\n", Pthread_self());

  Pthread_mutex_lock(&pool->lock); // 🔒
  pool->threads_alive += 1;
  Pthread_mutex_unlock(&pool->lock); // 🔓

  printf("Threads alive %d\n", pool->threads_alive);

  while (1)
  {
    Pthread_mutex_lock(&pool->lock); // 🔒
    while ((pool->queue->filled == 0) && (pool->threads_alive <= pool->threads_necessary))
      Pthread_cond_wait(&pool->threads_wait, &pool->lock); // 💤

    if (pool->threads_alive > pool->threads_necessary)
    {
      pool->threads_alive -= 1;
      Pthread_mutex_unlock(&pool->lock); // 🔓
      break;
    }

    job *job = tpool_pull_job(pool);

    if (pool->queue->filled == 0)
      threads_reduce(pool);
    printf("Thread %ld: Start the job\n", Pthread_self());
    Pthread_cond_signal(&pool->producers_wait); // ⚡

    Pthread_mutex_unlock(&pool->lock); // 🔓

    job->routine(job->arg);
    Free(job);
    printf("Thread %ld: Finish the job\n", Pthread_self());
  }

  return NULL;
}

static int is_power_of_two(int n)
{
  return n && !(n & (n - 1));
}
