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

_RIBS_INLINE_ char *ribs_malloc_vsprintf(const char *format, va_list ap) {
    return memalloc_vsprintf(&current_ctx->memalloc, format, ap);
}

_RIBS_INLINE_ char *ribs_malloc_sprintf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    char *str = ribs_malloc_vsprintf(format, ap);
    va_end(ap);
    return str;
}

_RIBS_INLINE_ void *ribs_memdup(const void *s, size_t n) {
    return memalloc_memcpy(&current_ctx->memalloc, s, n);
}

_RIBS_INLINE_ char *ribs_strdup(const char *s) {
    return memalloc_strcpy(&current_ctx->memalloc, s);
}

_RIBS_INLINE_ char *ribs_malloc_strftime(const char *format, const struct tm *tm) {
    return memalloc_strftime(&current_ctx->memalloc, format, tm);
}
