#ifndef _RIBIFY__H_
#define _RIBIFY__H_

#include "ribs_defs.h"
#include <sys/socket.h>

int ribs_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen, int timeout);
ssize_t ribs_read(int fd, void *buf, size_t count);
ssize_t ribs_write(int fd, const void *buf, size_t count);

#endif // _RIBIFY__H_
