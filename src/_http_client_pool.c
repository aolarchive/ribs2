_RIBS_INLINE_ struct http_client_context *http_client_get_last_context() {
    return (struct http_client_context *)epoll_worker_get_last_context()->reserved;
}

_RIBS_INLINE_ int http_client_send_request(struct http_client_context *cctx) {
    int fd = RIBS_RESERVED_TO_CONTEXT(cctx)->fd;
    int res = vmbuf_write(&cctx->request, fd);
    if (res < 0) {
        perror("write request");
        cctx->http_status_code = 500;
        cctx->persistent = 0;
        close(fd);
        TIMEOUT_HANDLER_REMOVE_FD_DATA(epoll_worker_fd_map + fd);
    }
    return res;
}
