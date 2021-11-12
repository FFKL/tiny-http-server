#ifndef INOUT_H
#define INOUT_H

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/stat.h>

int Open(const char *pathname, int flags, mode_t mode);
ssize_t Read(int fd, void *buf, size_t count);
ssize_t Write(int fd, const void *buf, size_t count);
off_t Lseek(int fildes, off_t offset, int whence);
void Close(int fd);
int Select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int Dup(int oldfd);
int Dup2(int fd1, int fd2);
void Stat(const char *filename, struct stat *buf);
void Fstat(int fd, struct stat *buf);

void Fclose(FILE *fp);
FILE *Fdopen(int fd, const char *type);
char *Fgets(char *ptr, int n, FILE *stream);
FILE *Fopen(const char *filename, const char *mode);
void Fputs(const char *ptr, FILE *stream);
size_t Fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
void Fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

#endif
