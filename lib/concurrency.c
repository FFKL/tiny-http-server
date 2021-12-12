#include "concurrency.h"
#include "err.h"

/************************************************
 * Wrappers for Pthreads thread control functions
 ************************************************/

void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp, void *(*routine)(void *), void *argp)
{
  int rc;

  if ((rc = pthread_create(tidp, attrp, routine, argp)) != 0)
    posix_error(rc, "Pthread_create error");
}

void Pthread_cancel(pthread_t tid)
{
  int rc;

  if ((rc = pthread_cancel(tid)) != 0)
    posix_error(rc, "Pthread_cancel error");
}

void Pthread_join(pthread_t tid, void **thread_return)
{
  int rc;

  if ((rc = pthread_join(tid, thread_return)) != 0)
    posix_error(rc, "Pthread_join error");
}

void Pthread_detach(pthread_t tid)
{
  int rc;

  if ((rc = pthread_detach(tid)) != 0)
    posix_error(rc, "Pthread_detach error");
}

void Pthread_exit(void *retval)
{
  pthread_exit(retval);
}

pthread_t Pthread_self(void)
{
  return pthread_self();
}

void Pthread_once(pthread_once_t *once_control, void (*init_function)())
{
  pthread_once(once_control, init_function);
}

/************************************************
 * Wrappers for Pthread_mutex fuctions
 ************************************************/

void Pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
  int rc = pthread_mutex_init(mutex, attr);
  if (rc != 0)
    posix_error(rc, "Pthread_mutex_init error");
}

void Pthread_mutex_destroy(pthread_mutex_t *mutex)
{
  int rc = pthread_mutex_destroy(mutex);
  if (rc != 0)
    posix_error(rc, "Pthread_mutex_destroy error");
}

void Pthread_mutex_lock(pthread_mutex_t *mutex)
{
  int rc = pthread_mutex_lock(mutex);
  if (rc != 0)
    posix_error(rc, "Pthread_mutex_lock error");
}

void Pthread_mutex_unlock(pthread_mutex_t *mutex)
{
  int rc = pthread_mutex_unlock(mutex);
  if (rc != 0)
    posix_error(rc, "Pthread_mutex_unlock error");
}

void Pthread_mutex_trylock(pthread_mutex_t *mutex)
{
  int rc = pthread_mutex_trylock(mutex);
  if (rc != 0)
    posix_error(rc, "Pthread_mutex_trylock error");
}

/*******************************
 * Wrappers for Posix semaphores
 *******************************/

void Sem_init(sem_t *sem, int pshared, unsigned int value)
{
  if (sem_init(sem, pshared, value) < 0)
    unix_error("Sem_init error");
}

void P(sem_t *sem)
{
  if (sem_wait(sem) < 0)
    unix_error("P error");
}

void V(sem_t *sem)
{
  if (sem_post(sem) < 0)
    unix_error("V error");
}
