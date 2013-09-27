/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012 Adap.tv, Inc.

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
#ifndef _BITVECT__H_
#define _BITVECT__H_

#include "ribs_defs.h"
#include "malloc.h"
#include "vmbuf.h"

#define BITVECT_INITIALIZER { NULL, 0 }
#define BITVECT_NUM_BITS_ALIGN 6 /* 64 bits */
#define BITVECT_NUM_BITS (1 << BITVECT_NUM_BITS_ALIGN)
#define BITVECT_NUM_BYTES (BITVECT_NUM_BITS >> 3)
#define BITVECT_MASK (BITVECT_NUM_BITS - 1)
#define BITVECT_ROUNDUP_BYTES(x) (((x + BITVECT_MASK) & ~(BITVECT_MASK)) >> 3)
#define _BITVECT_BIT_OP(i,op,op2) data[i >> BITVECT_NUM_BITS_ALIGN] op (op2(1ULL << ((i) & (BITVECT_NUM_BITS - 1))))
#define _BITVECT_COMBINE(bv,bv_other,op)                                \
    if (bv->size != bv_other->size)                                     \
        return -1;                                                      \
    uint64_t *data = bv->data, *data_end = data + bv->size;             \
    uint64_t *data_other = bv_other->data;                              \
    for (; data != data_end; ++data, ++data_other) {                    \
        *data op *data_other;                                           \
    }                                                                   \

struct bitvect {
    void *data;
    size_t size;
};

_RIBS_INLINE_ int bitvect_init(struct bitvect *bv, size_t size) {
    bv->data = ribs_malloc(BITVECT_ROUNDUP_BYTES(size));
    bv->size = BITVECT_ROUNDUP_BYTES(size) >> (BITVECT_NUM_BITS_ALIGN - 3);
    return 0;
}

_RIBS_INLINE_ int bitvect_init_vmbuf(struct bitvect *bv, size_t size, struct vmbuf *buf) {
    vmbuf_init(buf, 4096);
    size_t ofs = vmbuf_alloczero(buf, BITVECT_ROUNDUP_BYTES(size));
    bv->data = vmbuf_data_ofs(buf, ofs);
    bv->size = BITVECT_ROUNDUP_BYTES(size) >> (BITVECT_NUM_BITS_ALIGN - 3);
    return 0;
}

_RIBS_INLINE_ int bitvect_init_mem(struct bitvect *bv, size_t size, void *mem) {
    bv->data = mem;
    bv->size = BITVECT_ROUNDUP_BYTES(size) >> (BITVECT_NUM_BITS_ALIGN - 3);
    return 0;
}

_RIBS_INLINE_ size_t bitvect_calc_mem_size(size_t size) {
    return BITVECT_ROUNDUP_BYTES(size);
}

_RIBS_INLINE_ int bitvect_set(struct bitvect *bv, size_t i) {
    uint64_t *data = bv->data;
    _BITVECT_BIT_OP(i,|=,);
    return 0;
}

_RIBS_INLINE_ int bitvect_reset(struct bitvect *bv, size_t i) {
    uint64_t *data = bv->data;
    _BITVECT_BIT_OP(i,&=,~);
    return 0;
}

_RIBS_INLINE_ int bitvect_isset(struct bitvect *bv, size_t i) {
    uint64_t *data = bv->data;
    return _BITVECT_BIT_OP(i,&,) ? 1 : 0;
}

/* non-optimized version */
/*
 _RIBS_INLINE_ int bitvect_from_index(struct bitvect *bv, uint32_t *index, size_t size) {
    uint64_t *data = bv->data;
    uint32_t *idx = index, *idx_end = index + size;
    for (; idx != idx_end; ++idx) {
        _BITVECT_BIT_OP(*idx,|=,);
    }
    return 0;
}
*/

_RIBS_INLINE_ int bitvect_from_index(struct bitvect *bv, uint32_t *index, size_t size) {
    if (0 == size) return -1;
    uint64_t *data = bv->data;
    uint32_t *idx = index, *idx_end = index + size;
    size_t current = *idx >> BITVECT_NUM_BITS_ALIGN;
    size_t a = 0;
    for (;;) {
        a |= 1ULL << (*idx & (BITVECT_NUM_BITS - 1));
        ++idx;
        if (idx == idx_end) break;
        size_t loc = *idx >> BITVECT_NUM_BITS_ALIGN;
        if (current != loc) {
            data[current] |= a;
            a = 0;
            current = loc;
        }
    }
    data[current] |= a;
    return 0;
}

_RIBS_INLINE_ int bitvect_combine_and(struct bitvect *bv, struct bitvect *bv_other) {
    _BITVECT_COMBINE(bv,bv_other,&=);
    return 0;
}

_RIBS_INLINE_ int bitvect_combine_or(struct bitvect *bv, struct bitvect *bv_other) {
    _BITVECT_COMBINE(bv,bv_other,|=);
    return 0;
}

_RIBS_INLINE_ int bitvect_to_index(struct bitvect *bv, struct vmbuf *output) {
    uint64_t *data = bv->data, *data_end = data + bv->size;
    size_t count = 0;
    for (; data != data_end; ++data, count += BITVECT_NUM_BITS) {
        uint64_t v = *data;
        uint8_t bit = 0;
        while (v) {
            if (v & 0x1) {
                *(uint32_t *)vmbuf_wloc(output) = count + bit;
                vmbuf_wseek(output, sizeof(uint32_t));
            }
            v >>= 1;
            ++bit;
        }
    }
    return 0;
}

_RIBS_INLINE_ int bitvect_dump(struct bitvect *bv) {
    uint64_t *data = bv->data, *data_end = data + bv->size;
    size_t count = 0;
    for (; data != data_end; ++data, count += BITVECT_NUM_BITS) {
        uint64_t v = *data;
        uint8_t bit = 0;
        while (v) {
            if (v & 0x1)
                printf("%zu\n", count + bit);
            v >>= 1;
            ++bit;
        }
    }
    return 0;
}


#endif // _BITVECT__H_
