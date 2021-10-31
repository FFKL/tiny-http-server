#ifndef HTTP_H
#define HTTP_H

#include <stdio.h>

#define HTTP_METHOD_SIZE 8
#define URI_SIZE 300
#define VERSION_SIZE 4
#define HEADERS_SIZE 40

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

int parse_http_message(char *text, http_message *msg_out);
int http_message_init(http_message *msg);
int http_message_free(http_message *msg);
http_header *http_get_header(http_message *msg, char *name);

#endif
