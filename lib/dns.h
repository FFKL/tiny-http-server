#ifndef DNS_H
#define DNS_H

#include <netdb.h>

struct hostent *Gethostbyname(const char *name);
struct hostent *Gethostbyaddr(const char *addr, int len, int type);

#endif
