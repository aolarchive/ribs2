/* Inline */

_RIBS_INLINE_ void epoll_worker_ignore_events() {
    epoll_worker_fd_map[current_ctx->fd].ctx = &main_ctx;
}

_RIBS_INLINE_ void epoll_worker_resume_events() {
    epoll_worker_fd_map[current_ctx->fd].ctx = current_ctx;
}

_RIBS_INLINE_ struct ribs_context *epoll_worker_get_last_context() {
    return epoll_worker_fd_map[last_epollev.data.fd].ctx;
}

_RIBS_INLINE_ void epoll_worker_set_fd_ctx(int fd, struct ribs_context* ctx) {
    epoll_worker_fd_map[fd].ctx = ctx;
}

_RIBS_INLINE_ void epoll_worker_set_last_fd(int fd) {
    last_epollev.data.fd = fd;
}
