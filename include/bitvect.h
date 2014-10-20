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
#ifndef _BITVECT__H_
#define _BITVECT__H_

#include "ribs_defs.h"
#include "malloc.h"
#include "vmbuf.h"

#define BITVECT_INITIALIZER { NULL, 0 }
#define BITVECT_NUM_BITS_ALIGN 6 /* 64 bits */
#define BITVECT_NUM_BITS (1 << BITVECT_NUM_BITS_ALIGN)
#define BITVECT_MASK (BITVECT_NUM_BITS - 1)
#define BITVECT_ROUND_UP_TO_NEAREST_BLOCK_SIZE(x) ((x + BITVECT_MASK) & ~(BITVECT_MASK))
#define BITVECT_NUM_BITS_TO_NUM_BYTES(x) ((BITVECT_ROUND_UP_TO_NEAREST_BLOCK_SIZE(x)) >> 3)
#define BITVECT_NUM_BITS_TO_NUM_BLOCKS(x) ((BITVECT_ROUND_UP_TO_NEAREST_BLOCK_SIZE(x)) >> BITVECT_NUM_BITS_ALIGN)
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
    size_t size; //Count of 64 bit blocks
};

_RIBS_INLINE_ int bitvect_init(struct bitvect *bv, size_t size) {
    bv->data = ribs_malloc(BITVECT_NUM_BITS_TO_NUM_BYTES(size));
    memset(bv->data,0,BITVECT_NUM_BITS_TO_NUM_BYTES(size));
    bv->size = BITVECT_NUM_BITS_TO_NUM_BLOCKS(size);
    return 0;
}

_RIBS_INLINE_ int bitvect_init_vmbuf(struct bitvect *bv, size_t size, struct vmbuf *buf) {
    vmbuf_init(buf, 4096);
    size_t ofs = vmbuf_alloczero(buf, BITVECT_NUM_BITS_TO_NUM_BYTES(size));
    bv->data = vmbuf_data_ofs(buf, ofs);
    bv->size = BITVECT_NUM_BITS_TO_NUM_BLOCKS(size);
    return 0;
}

_RIBS_INLINE_ int bitvect_init_mem(struct bitvect *bv, size_t size, void *mem) {
    bv->data = mem;
    bv->size = BITVECT_NUM_BITS_TO_NUM_BLOCKS(size);
    return 0;
}

_RIBS_INLINE_ size_t bitvect_calc_mem_size(size_t size) {
    return BITVECT_NUM_BITS_TO_NUM_BYTES(size);
}

_RIBS_INLINE_ int bitvect_set(struct bitvect *bv, size_t i) {
    uint64_t *data = bv->data;
    _BITVECT_BIT_OP(i,|=,);
    return 0;
}

_RIBS_INLINE_ int bitvect_set_resize(struct bitvect *bv, size_t i) {
    size_t new_size_bytes = BITVECT_NUM_BITS_TO_NUM_BYTES(i);
    size_t old_size_bytes = bv->size << (BITVECT_NUM_BITS_ALIGN - 3);
    if (new_size_bytes > old_size_bytes) {
        void *new_data = ribs_malloc(new_size_bytes);
        memcpy(new_data, bv->data, old_size_bytes);
        memset(new_data + old_size_bytes, 0, new_size_bytes - old_size_bytes);
        bv->data = new_data;
        bv->size = BITVECT_NUM_BITS_TO_NUM_BLOCKS(i);
    }
    return bitvect_set(bv, i);
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

_RIBS_INLINE_ int bitvect_isset_safe(struct bitvect *bv, size_t i) {
    uint64_t *data = bv->data;
    return ((bv->size << BITVECT_NUM_BITS_ALIGN) > i) && (_BITVECT_BIT_OP(i,&,)) ? 1 : 0;
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
        if (unlikely(idx == idx_end)) break;
        size_t loc = *idx >> BITVECT_NUM_BITS_ALIGN;
        if (likely(current != loc)) {
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

_RIBS_INLINE_ size_t bitvect_count(struct bitvect *bv) {
    uint64_t *data = bv->data, *data_end = data + bv->size;
    size_t count = 0;
    for (; data != data_end; ++data) {
        count += __builtin_popcountll(*data);
    }

    return count;
}


#endif // _BITVECT__H_
