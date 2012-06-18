
_RIBS_INLINE_ void timeout_handler_add_fd_data(struct timeout_handler *timeout_handler, struct epoll_worker_fd_data *fd_data) {
    gettimeofday(&fd_data->timestamp, NULL);
    if (list_empty(&timeout_handler->timeout_chain)) {
        struct itimerspec when = {{0,0},{timeout_handler->timeout/1000,(timeout_handler->timeout%1000)*1000000L}};
        if (0 > timerfd_settime(timeout_handler->timeout_handler_ctx->fd, 0, &when, NULL))
            perror("timerfd_settime"), abort();
    }
    list_insert_tail(&timeout_handler->timeout_chain, &fd_data->timeout_chain);
}
