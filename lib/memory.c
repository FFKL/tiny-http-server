#include "memory.h"
#include "err.h"

/***************************************
 * Wrappers for memory mapping functions
 ***************************************/

void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
  void *ptr;

  if ((ptr = mmap(addr, len, prot, flags, fd, offset)) == ((void *)-1))
    unix_error("mmap error");
  return (ptr);
}

void Munmap(void *start, size_t length)
{
  if (munmap(start, length) < 0)
    unix_error("munmap error");
}

/***************************************************
 * Wrappers for dynamic storage allocation functions
 ***************************************************/

void *Malloc(size_t size)
{
  void *p;

  if ((p = malloc(size)) == NULL)
    unix_error("Malloc error");
  return p;
}

void *Realloc(void *ptr, size_t size)
{
  void *p;

  if ((p = realloc(ptr, size)) == NULL)
    unix_error("Realloc error");
  return p;
}

void *Calloc(size_t nmemb, size_t size)
{
  void *p;

  if ((p = calloc(nmemb, size)) == NULL)
    unix_error("Calloc error");
  return p;
}

void Free(void *ptr)
{
  free(ptr);
}
