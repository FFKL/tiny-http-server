#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAXLINE 8192

int main(void)
{
  char *buf;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;
  /* Extract the two arguments */
  if ((buf = getenv("QUERY_STRING")) != NULL)
  {
    char *arg1p = strstr(buf, "num1=");
    char *arg2p = strstr(buf, "num2=");
    if (arg1p != NULL && arg2p != NULL)
    {
      strcpy(arg1, arg1p + 5);
      strcpy(arg2, arg2p + 5);
      n1 = atoi(arg1);
      n2 = atoi(arg2);
    }
  }
  /* Make the response body */
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  /* Generate the HTTP response */
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");

  if (strcasecmp(getenv("HTTP_METHOD"), "GET") == 0)
  {
    printf("%s", content);
  }

  fflush(stdout);
  exit(0);
}