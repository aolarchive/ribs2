_RIBS_INLINE_ struct http_client_context *http_client_get_context(void) {
    return (struct http_client_context *)(current_ctx->reserved);
}

_RIBS_INLINE_ struct http_client_context *http_client_get_last_context() {
    return (struct http_client_context *)epoll_worker_get_last_context()->reserved;
}

_RIBS_INLINE_ int http_client_send_request(struct http_client_context *cctx) {
    int res = vmbuf_write(&cctx->request, RIBS_RESERVED_TO_CONTEXT(cctx)->fd);
    if (res < 0)
        perror("http_client write");
    return res;
}
