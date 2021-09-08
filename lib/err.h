#ifndef ERR_H
#define ERR_H

void unix_error(char *msg);
void posix_error(int code, char *msg);
void dns_error(char *msg);
void app_error(char *msg);

#endif
