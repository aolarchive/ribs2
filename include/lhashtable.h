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
#ifndef _LHASHTABLE__H_
#define _LHASHTABLE__H_

#include "ribs_defs.h"
#include "logger.h"
#include "ilog2.h"
#include <sys/mman.h>
#include <string.h>

/* linear hashing */
/*

  o--------o----------o----------o . . . o------------o
  | HEADER |  TABLE 1 |  TABLE 2 |          TABLE 256 |
  o--------o----------o----------o . . . o------------o

  Each table:
                                    double every resize                        total = 4 giga buckets
  o-------------o . . -----------o . .------------o . .------------o . . . . . . ---------------------o
  | BUCKETS_OFS |      8 buckets |     8 buckets |     16 buckets  |               2147483648 buckets |
  o-------------o . . -----------o . .------------o . .------------o . . . . . . ---------------------o
  The above covers the extreme case of 4 giga records 8 bytes each.
  This is possible only if setting num_data_blocks to 32768

  Data mapping:
  Space allocation for records is done in chunks of 1MB, data is allocated in multiples of 8 bytes.
  To address a record, one 32 bits number is used as follows:
  17bits - ofs within a block in 8 bytes increments ( (1<<17) * 8 = 1MB)
  15bits - block number. The real offset of the block is mapped via data_start_ofs

  real address = data_start_ofs[block] + (ofs << 3)

*/

/*
 * misc config
 */
#define LHT_FLAG_FIN   0x01  /* clean shutdown */
#define LHT_NUM_SUB_TABLES 256
#define LHT_FREELIST_COUNT 8192 /* (max record size) / (record alignment) */
/*
 * memory management
 */
#define LHT_BLOCK_SIZE_BITS 20
#define LHT_BLOCK_SIZE (1<<LHT_BLOCK_SIZE_BITS)
#define LHT_ALLOC_ALIGN_BITS 3
#define LHT_ALLOC_ALIGN (1<<LHT_ALLOC_ALIGN_BITS)
/*
 * sub table
 */
#define LHT_SUB_TABLE_MIN_BITS 3
#define LHT_SUB_TABLE_INITIAL_SIZE (1<<LHT_SUB_TABLE_MIN_BITS)
#define LHT_SUB_TABLE_HASH(h) ((h ^ (h >> 8)) & (LHT_NUM_SUB_TABLES - 1)) /* TODO: maybe change later */
#define LHT_GET_SUB_TABLE() ((struct lhashtable_table *)(lht->mem + sub_table_ofs))
/*
 * misc helpers
 */
#define LHT_GET_HEADER() ((struct lhashtable_header *)lht->mem)
#define _LHT_ALIGN_P2(x,b) (x)+=(b)-1; (x)&=~((b)-1)
#define LHT_X_ALIGN(x) _LHT_ALIGN_P2(x, LHT_ALLOC_ALIGN)
#define LHT_N_ALIGN() LHT_X_ALIGN(n)

/*
 * main header
 */
struct lhashtable_header {
    char signature[8]; /* = "RIBS_LH\0" */
    uint16_t version;
    uint8_t flags;
    uint64_t write_loc; /* always aligned to 8 bytes */
    uint64_t capacity;
    uint64_t tables_offsets[LHT_NUM_SUB_TABLES]; /* 256 * [4GB..32GB] = [1TB..8TB] max */
    uint16_t num_data_blocks; /* default 4096, max 32768 */
    uint32_t size; /* number of keys in all sub-tables */
};

/*
 * ofs ==> record
 */
union lhashtable_data_ofs {
    struct {
        unsigned ofs:17;
        unsigned block:15; /* block 0 is special case */
    } bits;
    uint32_t u32;;
};

/*
 * hashtable (sub-table)
 */
struct lhashtable_table {
    uint64_t buckets_offsets[29]; /* (1<<29)*8 = upto 4 billion entries */
    uint32_t mask;
    uint32_t size;
    uint32_t next_alloc;
    uint16_t current_block;
    union lhashtable_data_ofs freelist[LHT_FREELIST_COUNT];
    uint64_t data_start_ofs[/* num_data_blocks */]; /* num_data_blocks * 1M blocks = 4GB..32GB */
};

/*
 * record containing the key and the value
 */
struct lhashtable_record {
    uint16_t key_len;
    uint16_t val_len;
    char data[];
};

/*
 * bucket
 */
struct lhashtable_bucket {
    uint32_t hashcode;
    union lhashtable_data_ofs data_ofs;
};

#define LHASHTABLE_INITIALIZER { NULL, -1 }
#define LHASHTABLE_INIT(lht) (lht) = ((struct lhashtable)LHASHTABLE_INITIALIZER)
/*
 * main struct
 */
struct lhashtable {
    void *mem;
    int fd;
};

int lhashtable_init(struct lhashtable *lht, const char *filename);
int lhashtable_close(struct lhashtable *lht);
uint64_t lhashtable_put(struct lhashtable *lht, const void *key, size_t key_len, const void *val, size_t val_len);
uint64_t lhashtable_put_key(struct lhashtable *lht, const void *key, size_t key_len, size_t val_len, int *is_inserted);
uint64_t lhashtable_get(struct lhashtable *lht, const void *key, size_t key_len);
int lhashtable_del(struct lhashtable *lht, const void *key, size_t key_len);
int lhashtable_foreach(struct lhashtable *lht, int (*callback)(uint64_t, void *), void *arg);
uint32_t lhashtable_size(struct lhashtable *lht);
/*
 * inline
 */
_RIBS_INLINE_ uint64_t lhashtable_put_str(struct lhashtable *lht, const char *key, const char *val);
_RIBS_INLINE_ const char *lhashtable_get_str(struct lhashtable *lht, const char *key);
_RIBS_INLINE_ int lhashtable_del_str(struct lhashtable *lht, const char *key);
_RIBS_INLINE_ void *lhashtable_get_key(struct lhashtable *lht, uint64_t rec_ofs);
_RIBS_INLINE_ size_t lhashtable_get_key_len(struct lhashtable *lht, uint64_t rec_ofs);
_RIBS_INLINE_ void *lhashtable_get_val(struct lhashtable *lht, uint64_t rec_ofs);
_RIBS_INLINE_ size_t lhashtable_get_val_len(struct lhashtable *lht, uint64_t rec_ofs);
_RIBS_INLINE_ uint64_t lhashtable_writeloc(struct lhashtable *lht);

#include "../src/_lhashtable.c"

#endif // _LHASHTABLE__H_
