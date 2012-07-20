#include "epoll_worker.h"
#include <sys/epoll.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/signalfd.h>
#include "logger.h"

int ribs_epoll_fd = -1;
struct epoll_event last_epollev;
struct epoll_worker_fd_data *epoll_worker_fd_map;

static int my_pid = -1;

LIST_CREATE(epoll_worker_timeout_chain);

static void sigrtmin_to_context(void) {
    struct signalfd_siginfo siginfo;
    while(1) {
       int res = read(last_epollev.data.fd, &siginfo, sizeof(struct signalfd_siginfo));
       if (sizeof(struct signalfd_siginfo) != res || NULL == (void *)siginfo.ssi_ptr) {
           LOGGER_PERROR("sigrtmin_to_ctx got NULL or < 128 bytes: %d", res);
           yield();
       } else
           ribs_swapcurcontext((void *)siginfo.ssi_ptr);
    }
}

int epoll_worker_init(void) {
    my_pid = getpid();
    struct rlimit rlim;
    if (0 > getrlimit(RLIMIT_NOFILE, &rlim))
        return LOGGER_PERROR("getrlimit(RLIMIT_NOFILE)"), -1;
    epoll_worker_fd_map = calloc(rlim.rlim_cur, sizeof(struct epoll_worker_fd_data));
    ribs_epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (ribs_epoll_fd < 0)
        return LOGGER_PERROR("epoll_create1"), -1;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    sigaddset(&set, SIGRTMIN);
    if (-1 == sigprocmask(SIG_BLOCK, &set, NULL))
        return LOGGER_PERROR("sigprocmask"), -1;

    sigdelset(&set, SIGPIPE);
    int sfd = signalfd(-1, &set, SFD_NONBLOCK);
    if (0 > sfd)
        return LOGGER_PERROR("signalfd"), -1;

    struct epoll_event ev = { .events = EPOLLIN, .data.fd = sfd };
    if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, sfd, &ev))
        return LOGGER_PERROR("epoll_ctl"), -1;

    void *ctx=ribs_context_create(SMALL_STACK_SIZE, sigrtmin_to_context);
    if (NULL == ctx)
        return LOGGER_PERROR("ribs_context_create"), -1;

    epoll_worker_set_fd_ctx(sfd, ctx);
    return 0;
}

void epoll_worker_loop(void) {
    for (;;yield());
}

inline void yield(void) {
    while(0 >= epoll_wait(ribs_epoll_fd, &last_epollev, 1, -1));
    ribs_swapcurcontext(epoll_worker_get_last_context());
}

inline void courtesy_yield(void) {
    if (0 == epoll_wait(ribs_epoll_fd, &last_epollev, 1, 0))
        return;
    if (0 > sigqueue(my_pid, SIGRTMIN, (const union sigval)(void *)current_ctx))
        LOGGER_PERROR("sigqueue");
    ribs_swapcurcontext(epoll_worker_get_last_context());
}
