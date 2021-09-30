#ifndef HTTP_H
#define HTTP_H

#include <stdio.h>

#define HTTP_METHOD_SIZE 8
#define URI_SIZE 300
#define VERSION_SIZE 4

typedef struct http_message
{
  char method[HTTP_METHOD_SIZE];
  char uri[URI_SIZE];
  char version[VERSION_SIZE];
} http_message;

int parse_http_message(char *text, http_message *msg_out);

#endif
