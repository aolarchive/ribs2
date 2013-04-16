/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2013 Adap.tv, Inc.

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
