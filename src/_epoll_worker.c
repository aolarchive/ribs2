/* Inline */

_RIBS_INLINE_ void epoll_worker_ignore_events() {
    epoll_worker_fd_map[current_ctx->fd].ctx = &main_ctx;
}

_RIBS_INLINE_ void epoll_worker_resume_events() {
    epoll_worker_fd_map[current_ctx->fd].ctx = current_ctx;
}
