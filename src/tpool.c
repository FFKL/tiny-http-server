#include "../lib/concurrency.h"
#include "../lib/err.h"
#include "../lib/memory.h"

#include "tpool.h"

static job *tpool_pull_job(tpool *pool);

static job_queue *job_queue_create(int n);
static void job_queue_destroy(job_queue *queue);

static void thread_init(tpool *pool);
static void *thread_routine(tpool *pool);

/**
 * Binsem implementation repeats https://github.com/Pithikos/C-Thread-Pool
*/
static void binsem_init(binsem *sem, int value);
static void binsem_destroy(binsem *sem);
static void binsem_reset(binsem *sem);
static void binsem_post(binsem *sem);
static void binsem_post_all(binsem *sem);
static void binsem_wait(binsem *sem);

void tpool_init(tpool *pool, int threads_count, int queue_size)
{
  Pthread_mutex_init(&pool->lock, NULL);
  Pthread_cond_init(&pool->threads_wait, NULL);
  pool->threads_alive = threads_count;
  pool->threads_necessary = threads_count;
  pool->queue = job_queue_create(queue_size);
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
}

static void job_queue_destroy(job_queue *queue)
{
  // TODO: wait for jobs finishing and iterate over the job queue to free memory ??
  Free(queue);
}

void tpool_push_job(tpool *pool, routine routine, routine_arg arg)
{
  job_queue *queue = pool->queue;

  job *new_job = Malloc(sizeof(job));
  new_job->routine = routine;
  new_job->arg = arg;

  Pthread_mutex_lock(&pool->lock); // 🔒

  while (queue->filled == queue->n)
    Pthread_cond_wait(&pool->producers_wait, &pool->lock); // 💤
  queue->buf[(++queue->rear) % (queue->n)] = new_job;
  queue->filled += 1;
  Pthread_cond_signal(&pool->threads_wait); // ⚡

  Pthread_mutex_unlock(&pool->lock); // 🔓
}

static job *tpool_pull_job(tpool *pool)
{
  job_queue *queue = pool->queue;
  job *job = queue->buf[(++queue->front) % (queue->n)];

  queue->filled -= 1;

  return job;
}

static void thread_init(tpool *pool)
{
  pthread_t tid;
  Pthread_create(&tid, NULL, (void *(*)(void *))thread_routine, pool);
  Pthread_detach(tid);
}

static void *thread_routine(tpool *pool)
{
  Pthread_mutex_lock(&pool->lock); // 🔒
  pool->threads_alive += 1;
  Pthread_mutex_unlock(&pool->lock); // 🔓

  while (1)
  {
    Pthread_mutex_lock(&pool->lock); // 🔒

    while ((pool->queue->filled == 0) || (pool->threads_alive <= pool->threads_necessary))
      Pthread_cond_wait(&pool->threads_wait, &pool->lock); // 💤

    if (pool->threads_alive > pool->threads_necessary)
    {
      pool->threads_alive -= 1;
      Pthread_mutex_unlock(&pool->lock); // 🔓
      break;
    }

    job *job = tpool_pull_job(pool);
    Pthread_cond_signal(&pool->producers_wait); // ⚡

    Pthread_mutex_unlock(&pool->lock); // 🔓

    job->routine(job->arg);
    Free(job);
  }

  return NULL;
}

static void binsem_init(binsem *sem, int value)
{
  if (value < 0 || value > 1)
    app_error("binsim can be initialized with 1 or 0");

  Pthread_mutex_init(&sem->mutex, NULL);
  Pthread_cond_init(&sem->cond, NULL);
  sem->val = value;
}

static void binsem_destroy(binsem *sem)
{
  Pthread_mutex_destroy(&sem->mutex);
  Pthread_cond_destroy(&sem->cond);
}

static void binsem_reset(binsem *sem)
{
  binsem_init(sem, 0);
}

static void binsem_post(binsem *sem)
{
  pthread_mutex_lock(&sem->mutex);
  sem->val = 1;
  pthread_cond_signal(&sem->cond);
  pthread_mutex_unlock(&sem->mutex);
}

static void binsem_post_all(binsem *sem)
{
  pthread_mutex_lock(&sem->mutex);
  sem->val = 1;
  pthread_cond_broadcast(&sem->cond);
  pthread_mutex_unlock(&sem->mutex);
}

static void binsem_wait(binsem *sem)
{
  pthread_mutex_lock(&sem->mutex);
  while (sem->val != 1)
  {
    pthread_cond_wait(&sem->cond, &sem->mutex);
  }
  sem->val = 0;
  pthread_mutex_unlock(&sem->mutex);
}
