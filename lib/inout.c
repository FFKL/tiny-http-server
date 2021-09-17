#include "inout.h"
#include "err.h"

/********************************
 * Wrappers for Unix I/O routines
 ********************************/

int Open(const char *pathname, int flags, mode_t mode)
{
  int rc;

  if ((rc = open(pathname, flags, mode)) < 0)
    unix_error("Open error");
  return rc;
}

ssize_t Read(int fd, void *buf, size_t count)
{
  ssize_t rc;

  if ((rc = read(fd, buf, count)) < 0)
    unix_error("Read error");
  return rc;
}

ssize_t Write(int fd, const void *buf, size_t count)
{
  ssize_t rc;

  if ((rc = write(fd, buf, count)) < 0)
    unix_error("Write error");
  return rc;
}

off_t Lseek(int fildes, off_t offset, int whence)
{
  off_t rc;

  if ((rc = lseek(fildes, offset, whence)) < 0)
    unix_error("Lseek error");
  return rc;
}

void Close(int fd)
{
  int rc;

  if ((rc = close(fd)) < 0)
    unix_error("Close error");
}

int Select(int n, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout)
{
  int rc;

  if ((rc = select(n, readfds, writefds, exceptfds, timeout)) < 0)
    unix_error("Select error");
  return rc;
}

int Dup2(int fd1, int fd2)
{
  int rc;

  if ((rc = dup2(fd1, fd2)) < 0)
    unix_error("Dup2 error");
  return rc;
}

void Stat(const char *filename, struct stat *buf)
{
  if (stat(filename, buf) < 0)
    unix_error("Stat error");
}

void Fstat(int fd, struct stat *buf)
{
  if (fstat(fd, buf) < 0)
    unix_error("Fstat error");
}

/******************************************
 * Wrappers for the Standard I/O functions.
 ******************************************/
void Fclose(FILE *fp)
{
  if (fclose(fp) != 0)
    unix_error("Fclose error");
}

FILE *Fdopen(int fd, const char *type)
{
  FILE *fp;

  if ((fp = fdopen(fd, type)) == NULL)
    unix_error("Fdopen error");

  return fp;
}

char *Fgets(char *ptr, int n, FILE *stream)
{
  char *rptr;

  if (((rptr = fgets(ptr, n, stream)) == NULL) && ferror(stream))
    app_error("Fgets error");

  return rptr;
}

FILE *Fopen(const char *filename, const char *mode)
{
  FILE *fp;

  if ((fp = fopen(filename, mode)) == NULL)
    unix_error("Fopen error");

  return fp;
}

void Fputs(const char *ptr, FILE *stream)
{
  if (fputs(ptr, stream) == EOF)
    unix_error("Fputs error");
}

size_t Fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  size_t n;

  if (((n = fread(ptr, size, nmemb, stream)) < nmemb) && ferror(stream))
    unix_error("Fread error");
  return n;
}

void Fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  if (fwrite(ptr, size, nmemb, stream) < nmemb)
    unix_error("Fwrite error");
}
