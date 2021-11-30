#include "http.h"
#include "../lib/inout.h"
#include "../lib/memory.h"
#include "../lib/err.h"

#include <string.h>
#include <setjmp.h>
#include <unistd.h>
#include <errno.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define SPACE " "
#define CRLF "\r\n"
#define HTTP_PREFIX "HTTP/"
#define HEADERS_MEM_SIZE 4096
#define BODY_MEM_SIZE 4096

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

// TODO: check memory boundaries after confirming the module interface.
// Now memory buffers can be exceeded.
int http_message_init(http_message *msg)
{
  msg->headers_mem = Malloc(HEADERS_MEM_SIZE);
  msg->body = Malloc(BODY_MEM_SIZE);
  return 0;
}

int http_message_free(http_message *msg)
{
  Free(msg->headers_mem);
  Free(msg->body);
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

    if (strcmp("POST", msg_out->method) == 0)
    {
      http_header *header = http_get_header(msg_out, "Content-Length");
      if (header != NULL)
      {
        int content_length = atoi(header->value);
        if (content_length <= BODY_MEM_SIZE)
        {
          strncpy(msg_out->body, parser_mem.text_cursor, content_length);
        }
        else
          return -1;
      }
      else
        return -2;
    }

    return 0;
  }
  else
    return -1;
}

http_header *http_get_header(http_message *msg, char *name)
{
  http_header *next_head = msg->headers;
  while (next_head->name != NULL)
  {
    if (strcasecmp(name, next_head->name) == 0)
    {
      return next_head;
    }
    next_head++;
  }
  return NULL;
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
  uri_out[i] = '\0';
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

ssize_t http_text_init(int fd, http_text *text)
{
  text->fd = Dup(fd);
  text->unread = 0;
  text->filled = 0;
  int flags = fcntl(text->fd, F_GETFL);
  if (flags < 0)
    unix_error("Fcntl error");

  int res = fcntl(text->fd, F_SETFL, flags | O_NONBLOCK);
  if (res < 0)
    unix_error("Fcntl error");

  text->buf = Malloc(HEADERS_MEM_SIZE);
  text->bufptr = text->buf;
  return 0;
}

ssize_t http_text_free(http_text *text)
{
  Close(text->fd);
  Free(text->buf);
  return 0;
}

static ssize_t find_line(char *buf, size_t size)
{
  for (int i = 0; i < size; i++)
  {
    if (buf[i] == '\r' && ((i + 1) < size) && buf[i + 1] == '\n')
      return i + 2;
  }
  return 0;
}

ssize_t next_head_line(http_text *text, char *buf, size_t size)
{
  ssize_t n;
  ssize_t line_size;
  while (text->filled != HEADERS_MEM_SIZE)
  {
    line_size = find_line(text->bufptr, text->unread);
    if (line_size > size)
      return TOO_LONG_HEADER;
    if (line_size > 0)
    {
      strncpy(buf, text->bufptr, line_size);
      text->unread -= line_size;
      text->bufptr += line_size;
      return line_size;
    }
    n = read(text->fd, text->bufptr + text->unread, HEADERS_MEM_SIZE - text->filled);
    if (n < 0)
    {
      if (errno == EINTR)
        continue;
      if (errno == EAGAIN)
        return NO_DATA;
      return -1;
    }
    else if (n == 0)
    {
      return TEXT_END;
    }
    else
    {

      text->filled += n;
      text->unread += n;
    }
  }
  return TOO_LONG_HEADERS;
}

ssize_t http_consume(http_text *text)
{
  ssize_t n;
  ssize_t line_size;
  while (text->filled != HEADERS_MEM_SIZE)
  {
    line_size = find_line(text->bufptr, text->unread);
    if (line_size == 2 && (strncmp(text->bufptr, CRLF, line_size) == 0))
      return CRLF_HAPPENED;
    n = read(text->fd, text->bufptr + text->unread, HEADERS_MEM_SIZE - text->filled);
    if (n < 0)
    {
      if (errno == EINTR)
        continue;
      if (errno == EAGAIN)
        return NO_DATA;
      return -1;
    }
    else if (n == 0)
    {
      return TEXT_END;
    }
    else
    {
      text->filled += n;
      text->unread += n;
    }
  }
  return TOO_LONG_HEADERS;
}
