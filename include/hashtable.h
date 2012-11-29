/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012,2013 Adap.tv, Inc.

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
#ifndef _HASHTABLE__H_
#define _HASHTABLE__H_

#include "ribs_defs.h"
#include "vmbuf.h"
#include "vmfile.h"
#include "template.h"
#include "hash_funcs.h"
#include "ilog2.h"

#define _TEMPLATE_HTBL_FUNC_HELPER(S,T,F) S ## T ##_## F
#define TEMPLATE_HTBL_FUNC(S,T,F) _TEMPLATE_HTBL_FUNC_HELPER(S,T,F)

#define _TEMPLATE_HTBL_HELPER(S,T) S ## T
#define TEMPLATE_HTBL(S,T) _TEMPLATE_HTBL_HELPER(S,T)

#define HASHTABLE_INITIAL_SIZE_BITS 5
#define HASHTABLE_INITIAL_SIZE (1<<HASHTABLE_INITIAL_SIZE_BITS)

struct hashtable {
    struct vmbuf data;
    uint32_t mask;
    uint32_t size;
};

struct hashtablefile_header {
    uint32_t size;
    uint32_t mask;
};

struct hashtablefile {
    struct vmfile data;
};

int hashtable_init(struct hashtable *ht, uint32_t initial_size);
int hashtablefile_init_create(struct hashtablefile *ht, const char *file_name, uint32_t initial_size);
int hashtablefile_finalize(struct hashtablefile *ht);

#ifdef T
#undef T
#endif

#ifdef TS
#undef TS
#endif

#define T
#define TS vmbuf
#define HT_SIZE ht->size
#define HT_MASK ht->mask
#define HT_SLOT ((uint32_t *) TEMPLATE(TS, data)(&ht->data))
#include "_hashtable.h"
#undef HT_SIZE
#undef HT_MASK
#undef HT_SLOT
#undef TS
#undef T

#define T file
#define TS vmfile
#define HT_SIZE (((struct hashtablefile_header *) TEMPLATE(TS, data)(&ht->data))->size)
#define HT_MASK (((struct hashtablefile_header *) TEMPLATE(TS, data)(&ht->data))->mask)
#define HT_SLOT ((uint32_t *) TEMPLATE(TS, data_ofs)(&ht->data, sizeof(struct hashtablefile_header)))
#include "_hashtable.h"
#undef HT_SIZE
#undef HT_MASK
#undef HT_SLOT
#undef TS
#undef T

#endif // _HASHTABLE__H_
