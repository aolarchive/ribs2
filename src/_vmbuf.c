/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012,2013,2014 Adap.tv, Inc.

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
static inline struct vmbuf_header *vmbuf_get_header(struct vmbuf *vmbuf) {
    return (struct vmbuf_header *)((char *)vmbuf->mem - sizeof(struct vmbuf_header));
}

static inline void vmbuf_reset(struct vmbuf *vmbuf) {
    vmbuf_get_header(vmbuf)->wloc = vmbuf->rloc = 0;
}

static inline void vmbuf_wreset(struct vmbuf *vmbuf) {
    vmbuf_get_header(vmbuf)->wloc = 0;
}

static inline void vmbuf_rreset(struct vmbuf *vmbuf) {
    vmbuf->rloc = 0;
}

static inline void vmbuf_resetz(struct vmbuf *vmbuf) {
    vmbuf_reset(vmbuf);
    *vmbuf_str(vmbuf) = 0;
}

static inline void *vmbuf_mem(struct vmbuf *vmbuf) {
    return vmbuf->mem;
}

static inline void *vmbuf_mem2(struct vmbuf *vmbuf, size_t ofs) {
    return (char *)vmbuf->mem + ofs;
}

static inline size_t vmbuf_wavail(struct vmbuf *vmbuf) {
    struct vmbuf_header *header = vmbuf_get_header(vmbuf);
    return vmbuf->capacity - header->wloc;
}

static inline size_t vmbuf_ravail(struct vmbuf *vmbuf) {
    struct vmbuf_header *header = vmbuf_get_header(vmbuf);
    return header->wloc - vmbuf->rloc;
}

static inline size_t vmbuf_capacity(struct vmbuf *vmbuf) {
    return vmbuf->capacity;
}

static inline void *vmbuf_wloc2(struct vmbuf *vmbuf) {
    return (char *)vmbuf->mem + vmbuf_get_header(vmbuf)->wloc;
}

static inline void *vmbuf_rloc2(struct vmbuf *vmbuf) {
    return (char *)vmbuf->mem + vmbuf->rloc;
}

static inline void vmbuf_wseek_no_resize(struct vmbuf *vmbuf, size_t by) {
    struct vmbuf_header *header = vmbuf_get_header(vmbuf);
    header->wloc += by;
}

static inline int vmbuf_resize_if_full(struct vmbuf *vmbuf) {
    struct vmbuf_header *header = vmbuf_get_header(vmbuf);
    if (unlikely(header->wloc == vmbuf->capacity))
        return vmbuf_add_capacity(vmbuf);
    return 0;
}

static inline int vmbuf_resize_if_less(struct vmbuf *vmbuf, size_t desired_size) {
    size_t wa = vmbuf_wavail(vmbuf);
    if (desired_size <= wa)
        return 0;
    return vmbuf_resize_to(vmbuf, vmbuf_size(vmbuf) + desired_size);
}

static inline int vmbuf_resize_by(struct vmbuf *vmbuf, size_t by) {
    return vmbuf_resize_to(vmbuf, vmbuf->capacity + by);
}

static inline int vmbuf_wseek(struct vmbuf *vmbuf, size_t by) {
    vmbuf_wseek_no_resize(vmbuf, by);
    return vmbuf_resize_if_full(vmbuf);
}

static inline void vmbuf_rseek(struct vmbuf *vmbuf, size_t by) {
    vmbuf->rloc += by;
}

static inline int vmbuf_vsprintf(struct vmbuf *vmbuf, const char *format, va_list ap) {
    int n;
    for (;;) {
        size_t wav = vmbuf_wavail(vmbuf);
        va_list apc;
        va_copy(apc, ap);
        n = vsnprintf(vmbuf_wloc(vmbuf), wav, format, apc);
        va_end(apc);
        if (unlikely(0 > n))
            return -1;
        if (likely((unsigned int)n < wav))
            break;
        /* not enough space, resize */
        if (0 > vmbuf_add_capacity(vmbuf))
            return -1;
    }
    vmbuf_wseek(vmbuf, n);
    return 0;
}

static inline int vmbuf_sprintf(struct vmbuf *vmbuf, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int res = vmbuf_vsprintf(vmbuf, format, ap);
    va_end(ap);
    return res;
}

static inline int vmbuf_strftime(struct vmbuf *vmbuf, const char *format, const struct tm *tm) {
    size_t n;
    for (;;) {
        size_t wav = vmbuf_wavail(vmbuf);
        n = strftime(vmbuf_wloc(vmbuf), wav, format, tm);
        if (n > 0)
            break;
        // not enough space, resize
        if (0 > vmbuf_resize_by(vmbuf, 4096))
            return -1;
    }
    vmbuf_wseek(vmbuf, n);
    return 0;
}

static inline int vmbuf_strcpy(struct vmbuf *vmbuf, const char *src) {
    do {
        char *w = vmbuf_wloc(vmbuf);
        char *dst = w;
        char *last = w + vmbuf_wavail(vmbuf);
        for (; dst < last && *src; *dst++ = *src++); /* copy until end of string or end of buf */
        if (0 > vmbuf_wseek(vmbuf, dst - w)) /* this will trigger resize if needed */
            return -1; /* error */
    } while (*src);
    *(char *)vmbuf_wloc(vmbuf) = 0; /* trailing \0 */
    return 0;
}

static inline int vmbuf_replace_last_if(struct vmbuf *vmbuf, char s, char d) {
    char *loc = vmbuf_wloc(vmbuf);
    if (0 < vmbuf_get_header(vmbuf)->wloc && s == *--loc) {
        *loc = d;
        return 0;
    }
    return -1;
}

static inline void vmbuf_remove_last_if(struct vmbuf *vmbuf, char c) {
    if (0 == vmbuf_replace_last_if(vmbuf, c, 0))
        --vmbuf_get_header(vmbuf)->wloc;
}

static inline int vmbuf_chrcpy(struct vmbuf *vmbuf, char c) {
    *(char *)vmbuf_wloc(vmbuf) = c;
    return vmbuf_wseek(vmbuf, 1);
}

static inline int vmbuf_memcpy(struct vmbuf *vmbuf, const void *src, size_t n) {
    vmbuf_resize_if_less(vmbuf, n);
    memcpy(vmbuf_wloc(vmbuf), src, n);
    return vmbuf_wseek(vmbuf, n);
}

static inline void vmbuf_memset(struct vmbuf *vmbuf, int c, size_t n) {
    if (vmbuf_capacity(vmbuf) < n)
        vmbuf_resize_if_less(vmbuf, n);
    memset(vmbuf_mem(vmbuf), c, n);
}

static inline size_t vmbuf_alloc(struct vmbuf *vmbuf, size_t n) {
    struct vmbuf_header *header = vmbuf_get_header(vmbuf);
    size_t loc = header->wloc;
    if (0 > vmbuf_resize_if_less(vmbuf, n) || 0 > vmbuf_wseek(vmbuf, n))
        return -1;
    return loc;
}

static inline size_t vmbuf_alloczero(struct vmbuf *vmbuf, size_t n) {
    size_t loc = vmbuf_alloc(vmbuf, n);
    memset(vmbuf_mem2(vmbuf, loc), 0, n);
    return loc;
}

static inline size_t vmbuf_alloc_aligned(struct vmbuf *vmbuf, size_t n) {
    struct vmbuf_header *header = vmbuf_get_header(vmbuf);
    header->wloc += 7; /* align */
    header->wloc &= ~((size_t)7);
    return vmbuf_alloc(vmbuf, n);
}

static inline void *vmbuf_allocptr(struct vmbuf *vmbuf, size_t n) {
    return vmbuf_mem2(vmbuf, vmbuf_alloc_aligned(vmbuf, n));
}

static inline char *vmbuf_str(struct vmbuf *vmbuf) {
    return (char *)vmbuf_mem(vmbuf);
}

static inline char *vmbuf_str2(struct vmbuf *vmbuf, size_t ofs) {
    return (char *)vmbuf_mem2(vmbuf, ofs);
}

static inline size_t vmbuf_wlocpos(struct vmbuf *vmbuf) {
    return vmbuf_get_header(vmbuf)->wloc;
}

static inline size_t vmbuf_rlocpos(struct vmbuf *vmbuf) {
    return vmbuf->rloc;
}

static inline size_t vmbuf_size(struct vmbuf *vmbuf) {
    return vmbuf_wlocpos(vmbuf);
}

static inline size_t vmbuf_num_elements(struct vmbuf *vmbuf, size_t size_of_element) {
    return vmbuf_size(vmbuf) / size_of_element;
}

static inline int vmbuf_read(struct vmbuf *vmbuf, int fd) {
    ssize_t res;
    ssize_t wavail;
    while (0 < (res = read(fd, vmbuf_wloc(vmbuf), wavail = vmbuf_wavail(vmbuf)))) {
        if (0 > vmbuf_wseek(vmbuf, res))
            return -1;
        if (res < wavail)
            return errno=0, 1;
    }
    if (res < 0)
        return (EAGAIN == errno ? 1 : -1);
    return 0; // remote side closed connection
}

static inline int vmbuf_write(struct vmbuf *vmbuf, int fd) {
    ssize_t res;
    size_t rav;
    while ((rav = vmbuf_ravail(vmbuf)) > 0) {
        res = write(fd, vmbuf_rloc(vmbuf), rav);
        if (res < 0)
            return (EAGAIN == errno ? 0 : -1);
        else if (res > 0)
            vmbuf_rseek(vmbuf, res);
        else
            return errno = ENODATA, -1; // error, can not be zero
    }
    return 1; // reached the end
}

static inline void vmbuf_wlocset(struct vmbuf *vmbuf, size_t ofs) {
    struct vmbuf_header *header = vmbuf_get_header(vmbuf);
    header->wloc = ofs;
}

static inline void vmbuf_rlocset(struct vmbuf *vmbuf, size_t ofs) {
    vmbuf->rloc = ofs;
}

static inline void vmbuf_wrewind(struct vmbuf *vmbuf, size_t by) {
    struct vmbuf_header *header = vmbuf_get_header(vmbuf);
    header->wloc -= by;
}

static inline void vmbuf_rrewind(struct vmbuf *vmbuf, size_t by) {
    vmbuf->rloc -= by;
}

static inline void vmbuf_swap(struct vmbuf *vmbuf1, struct vmbuf *vmbuf2) {
    struct vmbuf tmpbuf = *vmbuf1;
    *vmbuf1 = *vmbuf2;
    *vmbuf2 = tmpbuf;
}

static inline char *vmbuf_data(struct vmbuf *vmbuf) {
    return (char *)vmbuf_mem(vmbuf);
}

static inline char *vmbuf_data_ofs(struct vmbuf *vmbuf, size_t ofs) {
    return (char *)vmbuf_mem2(vmbuf, ofs);
}

static inline char *vmbuf_wloc(struct vmbuf *vmbuf) {
    return (char *)vmbuf_wloc2(vmbuf);
}

static inline char *vmbuf_rloc(struct vmbuf *vmbuf) {
    return (char *)vmbuf_rloc2(vmbuf);
}
