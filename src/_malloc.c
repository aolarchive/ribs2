/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2013,2014 Adap.tv, Inc.

    RIBS is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 2.1 of the License.

    RIBS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with RIBS.  If not, see <http://www.gnu.org/licenses/>.
*/

_RIBS_INLINE_ void *ribs_malloc(size_t size) {
    return ribs_malloc2(current_ctx, size);
}

_RIBS_INLINE_ void *ribs_malloc2(struct ribs_context *ctx, size_t size) {
    if (0 == size) return NULL;
    return memalloc_alloc(&ctx->memalloc, size);
}

_RIBS_INLINE_ void *ribs_calloc(size_t nmemb, size_t size) {
    return ribs_calloc2(current_ctx, nmemb, size);
}

_RIBS_INLINE_ void *ribs_calloc2(struct ribs_context *ctx, size_t nmemb, size_t size) {
    size_t s = nmemb * size;
    void *mem = ribs_malloc2(ctx, s);
    memset(mem, 0, s);
    return mem;
}

_RIBS_INLINE_ void ribs_reset_malloc(void) {
    ribs_reset_malloc2(current_ctx);
}

_RIBS_INLINE_ void ribs_reset_malloc2(struct ribs_context *ctx) {
    memalloc_reset(&ctx->memalloc);
}

_RIBS_INLINE_ char *ribs_malloc_vsprintf(const char *format, va_list ap) {
    return ribs_malloc_vsprintf2(current_ctx, format, ap);
}

_RIBS_INLINE_ char *ribs_malloc_vsprintf2(struct ribs_context *ctx, const char *format, va_list ap) {
    return memalloc_vsprintf(&ctx->memalloc, format, ap);
}

_RIBS_INLINE_ char *ribs_malloc_sprintf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    char *str = ribs_malloc_vsprintf(format, ap);
    va_end(ap);
    return str;
}

_RIBS_INLINE_ char *ribs_malloc_sprintf2(struct ribs_context *ctx, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    char *str = ribs_malloc_vsprintf2(ctx, format, ap);
    va_end(ap);
    return str;
}

_RIBS_INLINE_ int ribs_malloc_strcat_vsprintf(char **buf, const char *format, va_list ap) {
    return ribs_malloc_strcat_vsprintf2(current_ctx, buf, format, ap);
}

_RIBS_INLINE_ int ribs_malloc_strcat_vsprintf2(struct ribs_context *ctx, char **buf, const char *format, va_list ap) {
    return memalloc_strcat_vsprintf(&ctx->memalloc, buf, format, ap);
}

_RIBS_INLINE_ int ribs_malloc_strcat_sprintf(char **buf, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int res = ribs_malloc_strcat_vsprintf(buf, format, ap);
    va_end(ap);
    return res;
}

_RIBS_INLINE_ int ribs_malloc_strcat_sprintf2(struct ribs_context *ctx, char **buf, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int res = ribs_malloc_strcat_vsprintf2(ctx, buf, format, ap);
    va_end(ap);
    return res;
}

_RIBS_INLINE_ void ribs_malloc_str_remove_last_if(char c) {
    ribs_malloc_str_remove_last_if2(current_ctx, c);
}

_RIBS_INLINE_ void ribs_malloc_str_remove_last_if2(struct ribs_context *ctx, char c) {
    memalloc_str_remove_last_if(&ctx->memalloc, c);
}

_RIBS_INLINE_ void *ribs_memdup(const void *s, size_t n) {
    return ribs_memdup2(current_ctx, s, n);
}

_RIBS_INLINE_ void *ribs_memdup2(struct ribs_context *ctx, const void *s, size_t n) {
    return memalloc_memcpy(&ctx->memalloc, s, n);
}

_RIBS_INLINE_ char *ribs_strdup(const char *s) {
    return ribs_strdup2(current_ctx, s);
}

_RIBS_INLINE_ char *ribs_strdup2(struct ribs_context *ctx, const char *s) {
    return memalloc_strcpy(&ctx->memalloc, s);
}

_RIBS_INLINE_ char *ribs_malloc_strftime(const char *format, const struct tm *tm) {
    return ribs_malloc_strftime2(current_ctx, format, tm);
}

_RIBS_INLINE_ char *ribs_malloc_strftime2(struct ribs_context *ctx, const char *format, const struct tm *tm) {
    return memalloc_strftime(&ctx->memalloc, format, tm);
}

_RIBS_INLINE_ size_t ribs_malloc_usage(void) {
    return ribs_malloc_usage2(current_ctx);
}

_RIBS_INLINE_ size_t ribs_malloc_usage2(struct ribs_context *ctx) {
    return memalloc_usage(&ctx->memalloc);
}
