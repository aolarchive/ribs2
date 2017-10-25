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
/*
 * inline
 */
_RIBS_INLINE_ void TEMPLATE(VMBUF_T,make)(struct VMBUF_T *vmb) {
    memset(vmb, 0, sizeof(struct VMBUF_T));
}

_RIBS_INLINE_ void TEMPLATE(VMBUF_T,reset)(struct VMBUF_T *vmb) {
    vmb->read_loc = vmb->write_loc = 0;
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,free)(struct VMBUF_T *vmb) {
    TEMPLATE(VMBUF_T,reset)(vmb);
    if (NULL != vmb->buf && munmap(vmb->buf, vmb->capacity) < 0)
        return perror(STRINGIFY(VMBUF_T)"_free"), -1;
    vmb->buf = NULL;
    vmb->capacity = 0;
    return 0;
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,free_most)(struct VMBUF_T *vmb) {
    TEMPLATE(VMBUF_T,reset)(vmb);
    if (NULL != vmb->buf && vmb->capacity > RIBS_VM_PAGESIZE) {
        if (0 > munmap(vmb->buf + RIBS_VM_PAGESIZE, vmb->capacity - RIBS_VM_PAGESIZE))
            return perror("munmap, " STRINGIFY(VMBUF_T) "_free_most"), -1;
        vmb->capacity = RIBS_VM_PAGESIZE;
    }
    return 0;
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,resize_by)(struct VMBUF_T *vmb, size_t by) {
    return TEMPLATE(VMBUF_T,resize_to)(vmb, vmb->capacity + by);
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,resize_if_full)(struct VMBUF_T *vmb) {
    if (vmb->write_loc == vmb->capacity)
        return TEMPLATE(VMBUF_T,resize_to)(vmb, vmb->capacity << 1);
    return 0;
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,resize_if_less)(struct VMBUF_T *vmb, size_t desired_size) {
    size_t wa = TEMPLATE(VMBUF_T,wavail)(vmb);
    if (desired_size <= wa)
        return 0;
    return TEMPLATE(VMBUF_T,resize_no_check)(vmb, desired_size);
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,resize_no_check)(struct VMBUF_T *vmb, size_t n) {
    size_t new_capacity = vmb->capacity;
    do {
        new_capacity <<= 1;
    } while (new_capacity - vmb->write_loc <= n);
    return TEMPLATE(VMBUF_T,resize_to)(vmb, new_capacity);
}

_RIBS_INLINE_ size_t TEMPLATE(VMBUF_T,alloc)(struct VMBUF_T *vmb, size_t n) {
    size_t loc = vmb->write_loc;
    if (0 > TEMPLATE(VMBUF_T,resize_if_less)(vmb, n) || 0 > TEMPLATE(VMBUF_T,wseek)(vmb, n))
        return -1;
    return loc;
}

_RIBS_INLINE_ size_t TEMPLATE(VMBUF_T,alloczero)(struct VMBUF_T *vmb, size_t n) {
    size_t loc = TEMPLATE(VMBUF_T,alloc)(vmb, n);
    memset(TEMPLATE(VMBUF_T,data_ofs)(vmb, loc), 0, n);
    return loc;
}

_RIBS_INLINE_ size_t TEMPLATE(VMBUF_T,alloc_aligned)(struct VMBUF_T *vmb, size_t n) {
    vmb->write_loc += 7; // align
    vmb->write_loc &= ~7;
    return TEMPLATE(VMBUF_T,alloc)(vmb, n);
}

_RIBS_INLINE_ size_t TEMPLATE(VMBUF_T,num_elements)(struct VMBUF_T *vmb, size_t size_of_element) {
    return TEMPLATE(VMBUF_T,wlocpos)(vmb) / size_of_element;
};

_RIBS_INLINE_ char *TEMPLATE(VMBUF_T,data)(struct VMBUF_T *vmb) {
    return vmb->buf;
}

_RIBS_INLINE_ char *TEMPLATE(VMBUF_T,data_ofs)(struct VMBUF_T *vmb, size_t loc) {
    return vmb->buf + loc;
}

_RIBS_INLINE_ size_t TEMPLATE(VMBUF_T,wavail)(struct VMBUF_T *vmb) {
    return vmb->capacity - vmb->write_loc;
}

_RIBS_INLINE_ size_t TEMPLATE(VMBUF_T,ravail)(struct VMBUF_T *vmb) {
    return vmb->write_loc - vmb->read_loc;
}

_RIBS_INLINE_ char *TEMPLATE(VMBUF_T,wloc)(struct VMBUF_T *vmb) {
    return vmb->buf + vmb->write_loc;
}

_RIBS_INLINE_ char *TEMPLATE(VMBUF_T,rloc)(struct VMBUF_T *vmb) {
    return vmb->buf + vmb->read_loc;
}

_RIBS_INLINE_ size_t TEMPLATE(VMBUF_T,rlocpos)(struct VMBUF_T *vmb) {
    return vmb->read_loc;
}

_RIBS_INLINE_ size_t TEMPLATE(VMBUF_T,wlocpos)(struct VMBUF_T *vmb) {
    return vmb->write_loc;
}

_RIBS_INLINE_ void TEMPLATE(VMBUF_T,rlocset)(struct VMBUF_T *vmb, size_t ofs) {
    vmb->read_loc = ofs;
}

_RIBS_INLINE_ void TEMPLATE(VMBUF_T,wlocset)(struct VMBUF_T *vmb, size_t ofs) {
    vmb->write_loc = ofs;
}

_RIBS_INLINE_ void TEMPLATE(VMBUF_T,rrewind)(struct VMBUF_T *vmb, size_t by) {
    vmb->read_loc -= by;
}

_RIBS_INLINE_ void TEMPLATE(VMBUF_T,wrewind)(struct VMBUF_T *vmb, size_t by) {
    vmb->write_loc -= by;
}

_RIBS_INLINE_ size_t TEMPLATE(VMBUF_T,capacity)(struct VMBUF_T *vmb) {
    return vmb->capacity;
}

_RIBS_INLINE_ void TEMPLATE(VMBUF_T,unsafe_wseek)(struct VMBUF_T *vmb, size_t by) {
    vmb->write_loc += by;
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,wseek)(struct VMBUF_T *vmb, size_t by) {
    TEMPLATE(VMBUF_T,unsafe_wseek)(vmb, by);
    return TEMPLATE(VMBUF_T,resize_if_full)(vmb);
}

_RIBS_INLINE_ void TEMPLATE(VMBUF_T,rseek)(struct VMBUF_T *vmb, size_t by) {
    vmb->read_loc += by;
}

_RIBS_INLINE_ void TEMPLATE(VMBUF_T,rreset)(struct VMBUF_T *vmb) {
    vmb->read_loc = 0;
}

_RIBS_INLINE_ void TEMPLATE(VMBUF_T,wreset)(struct VMBUF_T *vmb) {
    vmb->write_loc = 0;
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,sprintf)(struct VMBUF_T *vmb, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int res = TEMPLATE(VMBUF_T,vsprintf)(vmb, format, ap);
    va_end(ap);
    return res;
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,vsprintf)(struct VMBUF_T *vmb, const char *format, va_list ap) {
    int n;
    for (;;) {
        size_t wav = TEMPLATE(VMBUF_T,wavail)(vmb);
        va_list apc;
        va_copy(apc, ap);
        n = vsnprintf(TEMPLATE(VMBUF_T,wloc)(vmb), wav, format, apc);
        va_end(apc);
        if (unlikely(0 > n))
            return -1;
        if (likely((unsigned int)n < wav))
            break;
        // not enough space, resize
        if (unlikely(0 > TEMPLATE(VMBUF_T,resize_no_check)(vmb, n)))
            return -1;
    }
    TEMPLATE(VMBUF_T,wseek)(vmb, n);
    return 0;
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,strcpy)(struct VMBUF_T *vmb, const char *src) {
    do {
        char *w = TEMPLATE(VMBUF_T,wloc)(vmb);
        char *dst = w;
        char *last = w + TEMPLATE(VMBUF_T,wavail)(vmb);
        for (; dst < last && *src; *dst++ = *src++); // copy until end of string or end of buf
        if (0 > TEMPLATE(VMBUF_T,wseek)(vmb, dst - w)) // this will trigger resize if needed
            return -1; // error
    } while (*src);
    *(TEMPLATE(VMBUF_T,wloc)(vmb)) = 0; // trailing \0
    return 0;
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,replace_last_if)(struct VMBUF_T *vmb, char s, char d)
{
    char *loc = TEMPLATE(VMBUF_T,wloc)(vmb);
    if (0 < vmb->write_loc && s == *--loc) {
        *loc = d;
        return 0;
    }
    return -1;
}

_RIBS_INLINE_ void TEMPLATE(VMBUF_T,remove_last_if)(struct VMBUF_T *vmb, char c) {
    if (!TEMPLATE(VMBUF_T,replace_last_if)(vmb, c, 0))
        --vmb->write_loc;
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,read)(struct VMBUF_T *vmb, int fd) {
    ssize_t res;
    ssize_t wavail;
    while (0 < (res = read(fd, TEMPLATE(VMBUF_T,wloc)(vmb), wavail = TEMPLATE(VMBUF_T,wavail)(vmb)))) {
        if (0 > TEMPLATE(VMBUF_T,wseek)(vmb, res))
            return -1;
        if (res < wavail)
            return errno=0, 1;
    }
    if (res < 0)
        return (EAGAIN == errno ? 1 : -1);
    return 0; // remote side closed connection
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,write)(struct VMBUF_T *vmb, int fd) {
    ssize_t res;
    size_t rav;
    while ((rav = TEMPLATE(VMBUF_T,ravail)(vmb)) > 0) {
        res = write(fd, TEMPLATE(VMBUF_T,rloc)(vmb), rav);
        if (res < 0)
            return (EAGAIN == errno ? 0 : -1);
        else if (res > 0)
            TEMPLATE(VMBUF_T,rseek)(vmb, res);
        else
            return errno = ENODATA, -1; // error, can not be zero
    }
    return 1; // reached the end
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,memcpy)(struct VMBUF_T *vmb, const void *src, size_t n) {
    TEMPLATE(VMBUF_T,resize_if_less)(vmb, n);
    memcpy(TEMPLATE(VMBUF_T,wloc)(vmb), src, n);
    return TEMPLATE(VMBUF_T,wseek)(vmb, n);
}

_RIBS_INLINE_ void TEMPLATE(VMBUF_T,memset)(struct VMBUF_T *vmb, int c, size_t n) {
    memset(TEMPLATE(VMBUF_T,data)(vmb), c, n);
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,strftime)(struct VMBUF_T *vmb, const char *format, const struct tm *tm) {
    size_t n;
    for (;;) {
        size_t wav = TEMPLATE(VMBUF_T,wavail)(vmb);
        n = strftime(TEMPLATE(VMBUF_T,wloc)(vmb), wav, format, tm);
        if (n > 0)
            break;
        // not enough space, resize
        if (0 > TEMPLATE(VMBUF_T,resize_by)(vmb, RIBS_VM_PAGESIZE))
            return -1;
    }
    TEMPLATE(VMBUF_T,wseek)(vmb, n);
    return 0;
}

_RIBS_INLINE_ void *TEMPLATE(VMBUF_T,allocptr)(struct VMBUF_T *vmb, size_t n) {
    return TEMPLATE(VMBUF_T,data_ofs)(vmb, TEMPLATE(VMBUF_T,alloc)(vmb, n));
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,chrcpy)(struct VMBUF_T *vmb, char c) {
    *(TEMPLATE(VMBUF_T,wloc)(vmb)) = c;
    return TEMPLATE(VMBUF_T,wseek)(vmb, 1);
}

_RIBS_INLINE_ void TEMPLATE(VMBUF_T,swap)(struct VMBUF_T *vmb1, struct VMBUF_T *vmb2) {
    struct VMBUF_T tmpbuf = *vmb1;
    *vmb1 = *vmb2;
    *vmb2 = tmpbuf;
}
