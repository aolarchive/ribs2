/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012 Adap.tv, Inc.

    RIBS is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 2.1 of the License.

    RIBS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with RIBS.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _RIBIFY__H_
#define _RIBIFY__H_

int ribs_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int ribs_fcntl(int fd, int cmd, ...);
ssize_t ribs_read(int fd, void *buf, size_t count);
ssize_t ribs_write(int fd, const void *buf, size_t count);
ssize_t ribs_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t ribs_send(int sockfd, const void *buf, size_t len, int flags);
ssize_t ribs_recv(int sockfd, void *buf, size_t len, int flags);
ssize_t ribs_readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t ribs_writev(int fd, const struct iovec *iov, int iovcnt);
int ribs_pipe2(int pipefd[2], int flags);
int ribs_pipe(int pipefd[2]);
int ribs_nanosleep(const struct timespec *req, struct timespec *rem);
unsigned int ribs_sleep(unsigned int seconds);
int ribs_usleep(useconds_t usec);

#ifdef UGLY_GETADDRINFO_WORKAROUND
/* In this ugly mode ribs_getaddrinfo is not blocking but getaddrinfo_a creates threads internally */

#include <netdb.h>
int ribs_getaddrinfo(const char *node, const char *service,
                     const struct addrinfo *hints,
                     struct addrinfo **results);
#else
/* In this mode ribs_getaddrinfo is blocking */
#define ribs_getaddrinfo getaddrinfo
#endif

#endif // _RIBIFY__H_

