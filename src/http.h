#ifndef HTTP_H
#define HTTP_H

#include <stdio.h>

#define HTTP_METHOD_SIZE 8
#define URI_SIZE 300
#define VERSION_SIZE 4
#define HEADERS_SIZE 40

#define TEXT_END -2
#define NO_DATA -3
#define TOO_LONG_HEADERS -4
#define TOO_LONG_HEADER -5
#define CRLF_HAPPENED -6

typedef struct http_header
{
  char *name;
  char *value;
} http_header;

typedef struct http_message
{
  char method[HTTP_METHOD_SIZE];
  char uri[URI_SIZE];
  char version[VERSION_SIZE];
  http_header headers[HEADERS_SIZE];
  char *headers_mem;
  char *body;
} http_message;

typedef struct http_text
{
  int fd;
  int unread;
  int filled;
  char *bufptr;
  char *buf;
} http_text;

ssize_t http_text_init(int fd, http_text *text);
ssize_t http_text_free(http_text *text);
ssize_t next_head_line(http_text *text, char *buf, size_t size);
ssize_t http_consume(http_text *text);

int parse_http_message(char *text, http_message *msg_out);
int http_message_init(http_message *msg);
int http_message_free(http_message *msg);
http_header *http_get_header(http_message *msg, char *name);

#endif
