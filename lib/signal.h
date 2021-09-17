#ifndef SIGNAL_H
#define SIGNAL_H

typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

#endif
