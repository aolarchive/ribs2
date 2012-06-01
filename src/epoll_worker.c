#include "epoll_worker.h"
#include <sys/epoll.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <signal.h>

int ribs_epoll_fd = -1;
struct epoll_event last_epollev;
struct epoll_worker_fd_data *epoll_worker_fd_map;

LIST_CREATE(epoll_worker_timeout_chain);

int epoll_worker_init(void) {
    struct rlimit rlim;
    if (0 > getrlimit(RLIMIT_NOFILE, &rlim))
        return perror("getrlimit(RLIMIT_NOFILE)"), -1;
    epoll_worker_fd_map = calloc(rlim.rlim_cur, sizeof(struct epoll_worker_fd_data));
    ribs_epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (ribs_epoll_fd < 0)
        return perror("epoll_create1"), -1;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    if (-1 == sigprocmask(SIG_BLOCK, &set, NULL))
        return perror("sigprocmask"), -1;
    return 0;
}

void epoll_worker_loop(void) {
    current_ctx = &main_ctx;
    for (;;yield());
}

void yield(void) {
   while(0 >= epoll_wait(ribs_epoll_fd, &last_epollev, 1, -1));
   ribs_swapcurcontext(epoll_worker_fd_map[last_epollev.data.fd].ctx);
}
