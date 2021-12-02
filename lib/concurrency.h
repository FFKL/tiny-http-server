#ifndef CONCURRENCY_H
#define CONCURRENCY_H

#include <semaphore.h>
#include <pthread.h>

void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp, void *(*routine)(void *), void *argp);
void Pthread_cancel(pthread_t tid);
void Pthread_join(pthread_t tid, void **thread_return);
void Pthread_detach(pthread_t tid);
void Pthread_exit(void *retval);
pthread_t Pthread_self(void);
void Pthread_once(pthread_once_t *once_control, void (*init_function)());

void Sem_init(sem_t *sem, int pshared, unsigned int value);
void P(sem_t *sem);
void V(sem_t *sem);

#endif
