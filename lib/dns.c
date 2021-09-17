#include "dns.h"
#include "err.h"

#include <stddef.h>

/************************
 * DNS interface wrappers 
 ***********************/

/* $begin gethostbyname */
struct hostent *Gethostbyname(const char *name)
{
  struct hostent *p;

  if ((p = gethostbyname(name)) == NULL)
    dns_error("Gethostbyname error");
  return p;
}
/* $end gethostbyname */

struct hostent *Gethostbyaddr(const char *addr, int len, int type)
{
  struct hostent *p;

  if ((p = gethostbyaddr(addr, len, type)) == NULL)
    dns_error("Gethostbyaddr error");
  return p;
}
