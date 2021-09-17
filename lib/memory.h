#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>
#include <sys/mman.h>

/* Memory mapping wrappers */
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
void Munmap(void *start, size_t length);

/* Dynamic storage allocation wrappers */
void *Malloc(size_t size);
void *Calloc(size_t nmemb, size_t size);
void Free(void *ptr);

#endif
