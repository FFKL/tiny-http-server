#include "http.h"
#include "../lib/memory.h"

#include <string.h>
#include <setjmp.h>
#include <unistd.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define SPACE " "
#define HTTP_PREFIX "HTTP/"
#define HEADERS_MEM_SIZE 4096

char *http_methods[] = {"GET", "HEAD", "POST"};
char *http_versions[] = {"1.0", "1.1"};

jmp_buf jmp_env;

static int uri(char **cursor, char *uri_out);
static int token(char **cursor, char *sample, char *out);
static int one_of_token(char **cursor, char *sample[], size_t size, char *out);
static int parsing_exception();

int http_message_init(http_message *msg)
{
  msg->headers_mem = Malloc(HEADERS_MEM_SIZE); 
  return 0;
}

int http_message_free(http_message *msg)
{
  Free(msg->headers_mem);
  return 0;
}

int parse_http_message(char *text, http_message *msg_out)
{
  char *text_cursor = text;
  int err_code = setjmp(jmp_env);
  if (err_code == 0)
  {
    one_of_token(&text_cursor, http_methods, ARRAY_SIZE(http_methods), msg_out->method);
    token(&text_cursor, SPACE, NULL);
    uri(&text_cursor, msg_out->uri);
    token(&text_cursor, SPACE, NULL);
    token(&text_cursor, HTTP_PREFIX, NULL);
    one_of_token(&text_cursor, http_versions, ARRAY_SIZE(http_versions), msg_out->version);
    return 0;
  }
  else
    return -1;
}

static int uri(char **cursor, char *uri_out)
{
  char buf[URI_SIZE];
  int i;
  for (i = 0; i < URI_SIZE; i++)
  {
    if ('!' <= (*cursor)[i] && (*cursor)[i] <= '~')
      buf[i] = (*cursor)[i];
    else
      break;
  }
  if (i <= 0 || i >= URI_SIZE)
    return parsing_exception();

  (*cursor) += i;
  strncpy(uri_out, buf, i);
  return 0;
}

static int _token(char **cursor, char *sample, char *out)
{
  size_t size = strlen(sample);
  if (strncmp(*cursor, sample, size) == 0)
  {
    if (out != NULL)
      strcpy(out, sample);
    (*cursor) += size;
    return 0;
  }
  return -1;
}

static int token(char **cursor, char *sample, char *out)
{
  int res = _token(cursor, sample, out);
  return res == 0 ? 0 : parsing_exception();
}

static int one_of_token(char **cursor, char *sample[], size_t size, char *out)
{
  for (int i = 0; i < size; i++)
  {
    if (_token(cursor, sample[i], out) == 0)
      return 0;
  }
  return parsing_exception();
}

static int parsing_exception()
{
  longjmp(jmp_env, -1);
  return -1;
}
