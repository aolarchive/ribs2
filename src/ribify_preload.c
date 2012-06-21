#include "ribify.c"
#include <stdio.h>
#include <dlfcn.h>

int (*___real_connect)(int sockfd, const struct sockaddr *addr, socklen_t addrlen) = NULL;
int (*___real_read)(int fd, void *buf, size_t count) = NULL;
int (*___real_write)(int fd, const void *buf, size_t count) = NULL;

void __init(void) {
    printf("it is happening!\n");
    ___real_connect = dlsym(RTLD_NEXT, "connect");
    ___real_read = dlsym(RTLD_NEXT, "read");
    ___real_write = dlsym(RTLD_NEXT, "write");
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return __wrap_connect(sockfd, addr, addrlen);
}

ssize_t read(int fd, void *buf, size_t count) {
    return __wrap_read(fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return __wrap_write(fd, buf, count);
}

int __real_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return ___real_connect(sockfd, addr, addrlen);
}

ssize_t __real_read(int fd, void *buf, size_t count) {
    return ___real_read(fd, buf, count);
}

ssize_t __real_write(int fd, const void *buf, size_t count) {
    return ___real_write(fd, buf, count);
}

