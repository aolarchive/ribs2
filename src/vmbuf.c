#include "vmbuf.h"

inline off_t vmbuf_align(off_t off) {
    off += PAGEMASK;
    off &= ~PAGEMASK;
    return off;
}

/* internal only */
inline int vmbuf_init_mem(struct vmbuf *vmb, size_t initial_size) {
    if (NULL == vmb->buf) {
        initial_size = vmbuf_align(initial_size);
        vmb->buf = (char *)mmap(NULL, initial_size, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (MAP_FAILED == vmb->buf) {
            perror("mmap, vmbuf_init");
            vmb->buf = NULL;
            return -1;
        }
        vmb->capacity = initial_size;
    } else if (vmb->capacity < initial_size) {
        return vmbuf_resize_to(vmb, initial_size);
    }
    return 0;
}

inline int vmbuf_init(struct vmbuf *vmb, size_t initial_size) {
    if (vmbuf_init_mem(vmb, initial_size) < 0)
        return -1;
    vmbuf_reset(vmb);
    return 0;
}

inline void vmbuf_make(struct vmbuf *vmb) {
   memset(vmb, 0, sizeof(struct vmbuf));
}

inline void vmbuf_reset(struct vmbuf *vmb) {
   vmb->read_loc = vmb->write_loc = 0;
}

inline int vmbuf_free(struct vmbuf *vmb) {
    vmbuf_reset(vmb);
    if (NULL != vmb->buf && munmap(vmb->buf, vmb->capacity) < 0)
        return perror("vmbuf_free"), -1;
    vmb->buf = NULL;
    vmb->capacity = 0;
    return 0;
}

inline int vmbuf_free_most(struct vmbuf *vmb) {
    vmbuf_reset(vmb);
    if (NULL != vmb->buf && vmb->capacity > PAGESIZE) {
        if (0 > munmap(vmb->buf + PAGESIZE, vmb->capacity - PAGESIZE))
            return perror("munmap, vmbuf_free_most"), -1;
        vmb->capacity = PAGESIZE;
    }
    return 0;
}

inline int vmbuf_resize_by(struct vmbuf *vmb, size_t by) {
    return vmbuf_resize_to(vmb, vmb->capacity + by);
}

inline int vmbuf_resize_to(struct vmbuf *vmb, size_t new_capacity) {
    new_capacity = vmbuf_align(new_capacity);
    char *newaddr = (char *)mremap(vmb->buf, vmb->capacity, new_capacity, MREMAP_MAYMOVE);
    if ((void *)-1 == newaddr)
        return perror("mremap, vmbuf_resize_to"), -1;
    // success
    vmb->buf = newaddr;
    vmb->capacity = new_capacity;
    return 0;
}

inline int vmbuf_resize_if_full(struct vmbuf *vmb) {
    if (vmb->write_loc == vmb->capacity)
        return vmbuf_resize_to(vmb, vmb->capacity << 1);
    return 0;
}

inline int vmbuf_resize_if_less(struct vmbuf *vmb, size_t desired_size) {
    size_t wa = vmbuf_wavail(vmb);
    if (desired_size <= wa)
        return 0;
    return vmbuf_resize_no_check(vmb, desired_size);
}

inline int vmbuf_resize_no_check(struct vmbuf *vmb, size_t n) {
    size_t new_capacity = vmb->capacity;
    do {
        new_capacity <<= 1;
    } while (new_capacity - vmb->write_loc <= n);
    return vmbuf_resize_to(vmb, new_capacity);
}

inline size_t vmbuf_alloc(struct vmbuf *vmb, size_t n) {
    size_t loc = vmb->write_loc;
    if (0 > vmbuf_resize_if_less(vmb, n) || 0 > vmbuf_wseek(vmb, n))
        return -1;
    return loc;
}

inline size_t vmbuf_alloczero(struct vmbuf *vmb, size_t n) {
    size_t loc = vmbuf_alloc(vmb, n);
    memset(vmbuf_data_ofs(vmb, loc), 0, n);
    return loc;
}

inline size_t vmbuf_alloc_aligned(struct vmbuf *vmb, size_t n) {
    vmb->write_loc += 7; // align
    vmb->write_loc &= ~7;
    return vmbuf_alloc(vmb, n);
}

inline size_t vmbuf_num_elements(struct vmbuf *vmb, size_t size_of_element) {
    return vmbuf_wlocpos(vmb) / size_of_element;
};

inline char *vmbuf_data(struct vmbuf *vmb) {
   return vmb->buf;
}

inline char *vmbuf_data_ofs(struct vmbuf *vmb, size_t loc) {
   return vmb->buf + loc;
}

inline size_t vmbuf_wavail(struct vmbuf *vmb) {
   return vmb->capacity - vmb->write_loc;
}

inline size_t vmbuf_ravail(struct vmbuf *vmb) {
   return vmb->write_loc - vmb->read_loc;
}

inline char *vmbuf_wloc(struct vmbuf *vmb) {
   return vmb->buf + vmb->write_loc;
}

inline char *vmbuf_rloc(struct vmbuf *vmb) {
   return vmb->buf + vmb->read_loc;
}

inline size_t vmbuf_rlocpos(struct vmbuf *vmb) {
   return vmb->read_loc;
}

inline size_t vmbuf_wlocpos(struct vmbuf *vmb) {
   return vmb->write_loc;
}

inline void vmbuf_rlocset(struct vmbuf *vmb, size_t ofs) {
   vmb->read_loc = ofs;
}

inline void vmbuf_wlocset(struct vmbuf *vmb, size_t ofs) {
   vmb->write_loc = ofs;
}

inline void vmbuf_rrewind(struct vmbuf *vmb, size_t by) {
   vmb->read_loc -= by;
}

inline void vmbuf_wrewind(struct vmbuf *vmb, size_t by) {
   vmb->write_loc -= by;
}

inline size_t vmbuf_capacity(struct vmbuf *vmb) {
   return vmb->capacity;
}

inline void vmbuf_unsafe_wseek(struct vmbuf *vmb, size_t by) {
   vmb->write_loc += by;
}

inline int vmbuf_wseek(struct vmbuf *vmb, size_t by) {
   vmbuf_unsafe_wseek(vmb, by); return vmbuf_resize_if_full(vmb);
}

inline void vmbuf_rseek(struct vmbuf *vmb, size_t by) {
   vmb->read_loc += by;
}

inline void vmbuf_rreset(struct vmbuf *vmb) {
   vmb->read_loc = 0;
}

inline void vmbuf_wreset(struct vmbuf *vmb) {
   vmb->write_loc = 0;
}

inline int vmbuf_sprintf(struct vmbuf *vmb, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int res = vmbuf_vsprintf(vmb, format, ap);
    va_end(ap);
    return res;
}

inline int vmbuf_vsprintf(struct vmbuf *vmb, const char *format, va_list ap) {
    size_t n;
    for (;;) {
        size_t wav = vmbuf_wavail(vmb);
        va_list apc;
        va_copy(apc, ap);
        n = vsnprintf(vmbuf_wloc(vmb), wav, format, apc);
        va_end(apc);
        if (n < wav)
            break;
        // not enough space, resize
        if (0 > vmbuf_resize_no_check(vmb, n))
            return -1;
    }
    vmbuf_wseek(vmb, n);
    return 0;
}

inline int vmbuf_strcpy(struct vmbuf *vmb, const char *src) {
    do {
        char *w = vmbuf_wloc(vmb);
        char *dst = w;
        char *last = w + vmbuf_wavail(vmb);
        for (; dst < last && *src; *dst++ = *src++); // copy until end of string or end of buf
        if (0 > vmbuf_wseek(vmb, dst - w)) // this will trigger resize if needed
            return -1; // error
    } while (*src);
    *(vmbuf_wloc(vmb)) = 0; // trailing \0
    return 0;
}

inline void vmbuf_remove_last_if(struct vmbuf *vmb, char c) {
    char *loc = vmbuf_wloc(vmb);
    --loc;
    if (vmb->write_loc > 0 && *loc == c) {
        --vmb->write_loc;
        *loc = 0;
    }
}

inline int vmbuf_read(struct vmbuf *vmb, int fd) {
    ssize_t res;
    while (0 < (res = read(fd, vmbuf_wloc(vmb), vmbuf_wavail(vmb)))) {
        if (0 > vmbuf_wseek(vmb, res))
            return -1;
    }
    if (res < 0)
        return (EAGAIN == errno ? 1 : -1);
    return 0; // remote side closed connection
}

inline int vmbuf_write(struct vmbuf *vmb, int fd) {
    ssize_t res;
    size_t rav;
    while ((rav = vmbuf_ravail(vmb)) > 0) {
        res = write(fd, vmbuf_rloc(vmb), vmbuf_ravail(vmb));
        if (res < 0)
            return (EAGAIN == errno ? 0 : -1);
        else if (res > 0)
            vmbuf_rseek(vmb, res);
        else
            return errno = ENODATA, -1; // error, can not be zero
    }
    return 1; // reached the end
}

inline int vmbuf_memcpy(struct vmbuf *vmb, const void *src, size_t n) {
    vmbuf_resize_if_less(vmb, n);
    memcpy(vmbuf_wloc(vmb), src, n);
    return vmbuf_wseek(vmb, n);
}

inline void vmbuf_memset(struct vmbuf *vmb, int c, size_t n) {
   memset(vmbuf_data(vmb), c, n);
}

inline int vmbuf_strftime(struct vmbuf *vmb, const char *format, const struct tm *tm) {
    size_t n;
    for (;;) {
        size_t wav = vmbuf_wavail(vmb);
        n = strftime(vmbuf_wloc(vmb), wav, format, tm);
        if (n > 0)
            break;
        // not enough space, resize
        if (0 > vmbuf_resize_by(vmb, PAGESIZE))
            return -1;
    }
    vmbuf_wseek(vmb, n);
    return 0;
}
