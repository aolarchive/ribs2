#ifndef _RIBS_DEFS__H_
#define _RIBS_DEFS__H_

#define _GNU_SOURCE

#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>


#define _RIBS_INLINE_ static inline

#define likely(x)     __builtin_expect((x),1)
#define unlikely(x)   __builtin_expect((x),0)

int __real_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t __real_read(int fd, void *buf, size_t count);
ssize_t __real_write(int fd, const void *buf, size_t count);
int __real_fcntl (int fd, int cmd, ...);

#endif // _RIBS_DEFS__H_
