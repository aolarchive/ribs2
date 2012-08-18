#ifndef _LHASHTABLE__H_
#define _LHASHTABLE__H_

#include "ribs_defs.h"
#include "logger.h"

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

#define LHASHTABLE_FLAG_FIN   0x01  /* clean shutdown */
#define LHASHTABLE_NUM_SUB_TABLES 256
#define LHASHTABLE_FREELIST_COUNT 8192 /* (max record size) / (record alignment) */

/*
 * main header
 */
struct lhashtable_header {
    char signature[8]; /* = "RIBS_LH\0" */
    uint16_t version;
    uint8_t flags;
    uint64_t write_loc; /* always aligned to 8 bytes */
    uint64_t capacity;
    uint64_t tables_offsets[LHASHTABLE_NUM_SUB_TABLES]; /* 256 * [4GB..32GB] = [1TB..8TB] max */
    uint16_t num_data_blocks; /* default 4096, max 32768 */
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
    union lhashtable_data_ofs freelist[LHASHTABLE_FREELIST_COUNT];
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
/*
 * main struct
 */
struct lhashtable {
    void *mem;
    int fd;
};

int lhashtable_init(struct lhashtable *lht, const char *filename);
int lhashtable_close(struct lhashtable *lht);
int lhashtable_insert(struct lhashtable *lht, const void *key, size_t key_len, const void *val, size_t val_len);
int lhashtable_insert_str(struct lhashtable *lht, const char *key, const char *val);
uint64_t lhashtable_lookup(struct lhashtable *lht, const void *key, size_t key_len);
int lhashtable_remove(struct lhashtable *lht, const void *key, size_t key_len);
void lhashtable_dump(struct lhashtable *lht);

static inline void *lhashtable_get_val(struct lhashtable *lht, uint64_t rec_ofs) {
    struct lhashtable_record *rec = lht->mem + rec_ofs;
    return rec->data + rec->key_len;
}

#endif // _LHASHTABLE__H_
