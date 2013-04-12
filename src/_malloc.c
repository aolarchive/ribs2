_RIBS_INLINE_ void *ribs_malloc(size_t size) {
    if (0 == size) return NULL;
    return memalloc_alloc(&current_ctx->memalloc, size);
}

_RIBS_INLINE_ void *ribs_calloc(size_t nmemb, size_t size) {
    size_t s = nmemb * size;
    void *mem = ribs_malloc(s);
    memset(mem, 0, s);
    return mem;
}

_RIBS_INLINE_ void ribs_reset_malloc(void) {
    memalloc_reset(&current_ctx->memalloc);
}
