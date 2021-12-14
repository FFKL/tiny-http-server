#include "../lib/concurrency.h"
#include "../lib/err.h"
#include "../lib/memory.h"

#include "tpool.h"

static job_queue *job_queue_create(int n);
static void job_queue_destroy(job_queue *queue);

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
  Pthread_mutex_init(&pool->threads_count_mutex, NULL);
  Pthread_cond_init(&pool->threads_wait, NULL);
  pool->threads_alive = threads_count;
  pool->threads_necessary = threads_count;
  pool->queue = job_queue_create(queue_size);
}

void tpool_free(tpool *pool)
{
  Pthread_mutex_destroy(&pool->threads_count_mutex);
  Pthread_cond_destroy(&pool->threads_wait);
  job_queue_destroy(pool->queue);
}

static job_queue *job_queue_create(int n)
{
  job_queue *queue = Malloc(sizeof(job_queue));
  queue->buf = NULL;
  queue->n = n;
  queue->front = queue->rear = 0;
  Pthread_mutex_init(&queue->mutex, NULL);
  binsem_init(&queue->slots, 0);
  binsem_init(&queue->items, 0);
}

static void job_queue_destroy(job_queue *queue)
{
  // TODO: iterate over job queue to free memory
  pthread_mutex_destroy(&queue->mutex);
  binsem_destroy(&queue->slots);
  binsem_destroy(&queue->items);
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
