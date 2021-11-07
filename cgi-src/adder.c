#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

#define MAXLINE 8192

#define NUM1_ARG "num1="
#define NUM2_ARG "num2="

int parse_query(char *query, int *num1_out, int *num2_out)
{
  char arg1[MAXLINE], arg2[MAXLINE];

  char *arg1p = strstr(query, NUM1_ARG);
  char *arg2p = strstr(query, NUM2_ARG);

  if (arg1p != NULL && arg2p != NULL)
  {
    strcpy(arg1, arg1p + strlen(NUM1_ARG));
    strcpy(arg2, arg2p + strlen(NUM2_ARG));
    *num1_out = atoi(arg1);
    *num2_out = atoi(arg2);

    return 0;
  }

  return -1;
}

int is_get()
{
  return strcasecmp(getenv("HTTP_METHOD"), "GET") == 0;
}

int is_post()
{
  return strcasecmp(getenv("HTTP_METHOD"), "POST") == 0;
}

int main(void)
{
  char *buf;
  char content[MAXLINE];
  int n1 = 0, n2 = 0;

  if (is_get() && (buf = getenv("QUERY_STRING")) != NULL)
  {
    parse_query(buf, &n1, &n2);
  }
  else if (is_post())
  {
    char body[MAXLINE];
    int res = read(STDIN_FILENO, body, MAXLINE);
    printf("Read %d\n", res);
    if (res <= 0)
      exit(1);
    body[res] = '\0';
    parse_query(body, &n1, &n2);
  }

  /* Make the response body */
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  /* Generate the HTTP response */
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");

  if (strcasecmp(getenv("HTTP_METHOD"), "GET") == 0 || strcasecmp(getenv("HTTP_METHOD"), "POST") == 0)
  {
    printf("%s", content);
  }

  fflush(stdout);
  exit(0);
}