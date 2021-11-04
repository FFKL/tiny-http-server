/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 * GET method to serve static and dynamic content.
 */
#include "../lib/rio.h"
#include "../lib/inout.h"
#include "../lib/clserv.h"
#include "../lib/process.h"
#include "../lib/memory.h"
#include "../lib/socket.h"
#include "../lib/err.h"
#include "./http.h"

#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

#define MAXLINE 8192 /* max text line length */
#define MAXBUF 8192  /* max I/O buffer size */
#define MAXFILETYPE 100

#define GET_METHOD "GET"
#define HEAD_METHOD "HEAD"
#define POST_METHOD "POST"

extern char **environ; /* defined by libc */

void doit(int fd);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(char *method, int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, http_message *msg);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

void reap_child_process(int sig)
{
  pid_t pid;
  while ((pid = wait(NULL)) > 0)
    printf("Handler reaped child. PID=%d\n", (int)pid);
  if (errno != ECHILD)
    unix_error("Waitpid error in sigchid handler");
}

int main(int argc, char **argv)
{
  int listenfd, connfd, port, clientlen;
  struct sockaddr_in clientaddr;

  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  port = atoi(argv[1]);

  struct sigaction act = {
      .sa_flags = SA_RESTART,
      .sa_handler = reap_child_process};
  if (sigaction(SIGCHLD, &act, NULL) != 0)
  {
    unix_error("SIGCHILD error");
  }

  listenfd = Open_listenfd(port);
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *)&clientlen);
    doit(connfd);
    Close(connfd);
  }
}

void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);

  http_message msg;
  http_message_init(&msg);
  // TODO: handle parse code
  parse_http_message(rio.rio_buf, &msg);
  printf("%s %s %s\n", msg.method, msg.version, msg.uri);

  if (strcasecmp(msg.method, GET_METHOD) && strcasecmp(msg.method, HEAD_METHOD) && strcasecmp(msg.method, POST_METHOD))
  {
    clienterror(fd, msg.method, "501", "Not Implemented", "Tiny does not implement this method");
    return;
  }

  /* Parse URI from GET request */
  is_static = parse_uri(msg.uri, filename, cgiargs);
  if (stat(filename, &sbuf) < 0)
  {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn’t find this file");
    return;
  }

  if (is_static)
  { /* Serve static content */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn’t read the file");
      return;
    }
    serve_static(msg.method, fd, filename, sbuf.st_size);
  }
  else
  { /* Serve dynamic content */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn’t run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs, &msg);
  }
  http_message_free(&msg);
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  body[0] = '\0';
  strcat(body, "<html><title>Tiny Error</title>");
  strcat(body, "<body bgcolor=\"ffffff\">\r\n");
  sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
  strcat(body, buf);
  sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
  strcat(body, buf);
  strcat(body, "<hr><em>The Tiny Web server</em>\r\n");

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if (!strstr(uri, "cgi-bin"))
  { /* Static content */
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri) - 1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  else
  { /* Dynamic content */
    ptr = index(uri, '?');
    if (ptr)
    {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

void send_headers(int fd, char *filename, int filesize)
{
  char filetype[MAXFILETYPE], buf[MAXBUF], headers[MAXBUF];

  get_filetype(filename, filetype);
  headers[0] = '\0';
  strcat(headers, "HTTP/1.0 200 OK\r\n");
  strcat(headers, "Server: Tiny Web Server\r\n");
  sprintf(buf, "Content-length: %d\r\n", filesize);
  strcat(headers, buf);
  sprintf(buf, "Content-type: %s\r\n\r\n", filetype);
  strcat(headers, buf);
  Rio_writen(fd, headers, strlen(headers));
}

void send_response_body(int fd, char *filename, int filesize)
{
  int srcfd = Open(filename, O_RDONLY, 0);
  char *srcp = Malloc(filesize);
  ssize_t read_size = Rio_readn(srcfd, srcp, filesize);
  if (read_size != (ssize_t)filesize)
    app_error("Unexpected Error! File not read properly");
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  Free(srcp);
}

void serve_static(char *method, int fd, char *filename, int filesize)
{
  send_headers(fd, filename, filesize);
  if (strcasecmp(method, GET_METHOD) == 0)
  {
    send_response_body(fd, filename, filesize);
  }
}

/*
* get_filetype - derive file type from file name
*/
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mpg"))
    strcpy(filetype, "video/mpeg");
  else
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs, http_message *msg)
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  int pipe_fd[2];
  pipe(pipe_fd);

  if (Fork() == 0)
  { /* child */
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);
    setenv("HTTP_METHOD", msg->method, 1);
    Dup2(fd, STDOUT_FILENO); /* Redirect stdout to client */
    Close(pipe_fd[1]);
    Dup2(pipe_fd[0], STDIN_FILENO);
    Execve(filename, emptylist, environ); /* Run CGI program */
  }
  else
  {
    Close(pipe_fd[0]);
    Write(pipe_fd[1], msg->body, strlen(msg->body));
    Close(pipe_fd[1]);
  }
}
