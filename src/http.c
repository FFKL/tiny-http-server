#include "http.h"
#include "../lib/memory.h"

#include <string.h>
#include <setjmp.h>
#include <unistd.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define SPACE " "
#define CRLF "\r\n"
#define HTTP_PREFIX "HTTP/"
#define HEADERS_MEM_SIZE 4096

char *http_methods[] = {"GET", "HEAD", "POST"};
char *http_versions[] = {"1.0", "1.1"};

jmp_buf jmp_env;

typedef struct message_mem
{
  char *headers_start;
  char *headers_end;
  char *headers_cursor;
  char *text;
  char *text_cursor;
} message_mem;

static int uri(char **cursor, char *uri_out);
static int header(message_mem *mem, http_header *header_out);
static int token(char **cursor, char *sample, char *out);
static int _token(char **cursor, char *sample, char *out);
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
  message_mem parser_mem = {
      .text = text,
      .text_cursor = text,
      .headers_start = msg_out->headers_mem,
      .headers_end = (msg_out->headers_mem + HEADERS_MEM_SIZE),
      .headers_cursor = msg_out->headers_mem};

  int err_code = setjmp(jmp_env);
  if (err_code == 0)
  {
    one_of_token(&parser_mem.text_cursor, http_methods, ARRAY_SIZE(http_methods), msg_out->method);
    token(&parser_mem.text_cursor, SPACE, NULL);
    uri(&parser_mem.text_cursor, msg_out->uri);
    token(&parser_mem.text_cursor, SPACE, NULL);
    token(&parser_mem.text_cursor, HTTP_PREFIX, NULL);
    one_of_token(&parser_mem.text_cursor, http_versions, ARRAY_SIZE(http_versions), msg_out->version);
    token(&parser_mem.text_cursor, CRLF, NULL);

    int header_idx = 0;
    while (_token(&parser_mem.text_cursor, CRLF, NULL) != 0)
    {
      header(&parser_mem, &msg_out->headers[header_idx]);
      header_idx++;
    }
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

static int header_name(message_mem *mem, http_header *header_out)
{
  char *header_start = mem->headers_cursor;
  while (mem->headers_cursor != mem->headers_end)
  {
    if ('!' <= (*mem->text_cursor) && (*mem->text_cursor) <= '~' && (*mem->text_cursor) != ':')
      (*mem->headers_cursor) = (*mem->text_cursor);
    else
      break;
    mem->text_cursor++;
    mem->headers_cursor++;
  }
  if (header_start == mem->headers_cursor)
    return parsing_exception();
  if (mem->headers_cursor == mem->headers_end)
    return parsing_exception();
  *mem->headers_cursor = '\0';
  mem->headers_cursor++;
  header_out->name = header_start;
  return 0;
}

static int header_value(message_mem *mem, http_header *header_out)
{

  char *header_start = mem->headers_cursor;
  while (mem->headers_cursor != mem->headers_end)
  {
    if (' ' <= (*mem->text_cursor) && (*mem->text_cursor) <= '~')
      (*mem->headers_cursor) = (*mem->text_cursor);
    else
      break;
    mem->text_cursor++;
    mem->headers_cursor++;
  }
  if (header_start == mem->headers_cursor)
    return parsing_exception();
  if (mem->headers_cursor == mem->headers_end)
    return parsing_exception();
  *mem->headers_cursor = '\0';
  mem->headers_cursor++;
  header_out->value = header_start;
  return 0;
}

static int header(message_mem *mem, http_header *header_out)
{
  header_name(mem, header_out);
  token(&mem->text_cursor, ": ", NULL);
  header_value(mem, header_out);
  token(&mem->text_cursor, CRLF, NULL);
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
