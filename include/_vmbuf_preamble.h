#ifdef VMBUF_T
#undef VMBUF_T
#endif

#ifdef VMBUF_T_EXTRA
#undef VMBUF_T_EXTRA
#endif


#ifndef VMBUF_ONE_TIME
#define VMBUF_ONE_TIME

#define PAGEMASK 4095
#define PAGESIZE 4096

_RIBS_INLINE_ off_t vmbuf_align(off_t off) {
    off += PAGEMASK;
    off &= ~PAGEMASK;
    return off;
}

#endif // VMBUF_ONE_TIME
