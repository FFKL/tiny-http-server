#include "err.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h>

/************************** 
 * Error-handling functions
 **************************/

void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

void posix_error(int code, char *msg) /* Posix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(code));
    exit(1);
}

void dns_error(char *msg) /* DNS-style error */
{
    fprintf(stderr, "%s: DNS error %d\n", msg, h_errno);
    exit(1);
}

void app_error(char *msg) /* Application error */
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}
