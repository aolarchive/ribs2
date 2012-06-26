#include "vmbuf.h"
#include "context.h"
#include "ctx_pool.h"
#include "hashtable.h"
#include <sys/time.h>
#include "list.h"
#include "mime_types.h"
#include "ds.h"

struct ribs_context *ctx1, *ctx2;

void fiber1()
{
   int a = 5;
   printf("in fiber 1: %d\n", a);
   int i;
   for (i = 0; i < 10; ++i)
   {
      ribs_swapcurcontext(ctx2);
      printf("back in fiber 1: %d  %d\n", a, i);
   }

}

void fiber2()
{
   printf("in fiber 2\n");
   int i;
   for (i = 0; i < 10; ++i)
   {
      ribs_swapcurcontext(ctx1);
      printf("back in fiber 2: %d\n", i);
   }
}

#define REPORT_TIME()                                                   \
    gettimeofday(&end, NULL);                                           \
    timersub(&end, &start, &diff);                                      \
    printf("time: %llu\n", diff.tv_sec * 1000000ULL + diff.tv_usec);    \
    start = end;


#define NUM_KEYS 10000

struct my_struct {
    int x;
    struct list list;
    int y;
};

int main(void) {
    DS_FIELD(uint32_t) ds_my_field1 = DS_FIELD_INITIALIZER;
    int r;
    for (r = 0; r < 100; ++r) {
        if (0 > DS_FIELD_INIT(uint32_t, &ds_my_field1, "test.ds"))
            break;
        printf("via GET_VAL = %u\n", DS_FIELD_GET_VAL(&ds_my_field1, 1));
        uint32_t *my_field1 = DS_FIELD_BEGIN(&ds_my_field1);
        uint32_t *my_field1_end = DS_FIELD_END(&ds_my_field1);

        for (; my_field1 != my_field1_end; ++my_field1)
            printf("my_field = %u\n", *my_field1);
    }

    if (0 > mime_types_init())
        return printf("ERROR: mime types\n"), 1;

    const char *filenames[] = { "filename.jpg",
                                "filename.doc",
                                "filename.xml",
                                "filename.test",
                                "filename.bin",
                                "filename.epsf"
    };
    const char **filename;
    for (filename = filenames; filename != filenames + sizeof(filenames)/sizeof(filenames[0]); ++filename)
        printf("%s = %s\n", *filename, mime_types_by_filename(*filename));

    int x = 0;
    struct list head;
    list_init(&head);
    for (x = 0; x < 100; ++x) {
        struct my_struct *m = (struct my_struct *)malloc(sizeof(struct my_struct));
        m->x = x + 1000;
        m->y = x + 2000;
        list_insert_tail(&head, &m->list);
    }

    for (x = 0; x < 10; ++x)
        list_make_first(&head, list_tail(&head));

    while (!list_empty(&head)) {
        struct list *l = list_pop_head(&head);
        struct my_struct *m = LIST_ENTRY(l, struct my_struct, list);
        printf(" x = %d, y = %d\n", m->x, m->y);
    }

    struct timeval start, end, diff;
    struct hashtable ht;
    hashtable_init(&ht, NUM_KEYS * 2);
    gettimeofday(&start, NULL);
    int i, k, v;
    for (i = 0; i < NUM_KEYS; ++i) {
        k = i * 3;
        v = i + 10000;
        hashtable_insert(&ht, &k, sizeof(k), &v, sizeof(v));
    }
    REPORT_TIME();
    int j;
    for (j = 0; j < 10000; ++j) {
        for (i = 0; i < NUM_KEYS; ++i) {
            k = i * 3;
            uint32_t ofs = hashtable_lookup(&ht, &k, sizeof(k));
            if (!ofs) {
                printf("ERROR!!!\n");
                continue;
            }
            v = *(int *)hashtable_get_val(&ht, ofs);
            //printf("key = %d, val = %d\n", k, v);

        }
    }

    REPORT_TIME();

   const unsigned STACK_SIZE = 65536;

   ctx1 = ribs_context_create(STACK_SIZE, fiber1);
   ctx2 = ribs_context_create(STACK_SIZE, fiber2);

   printf("in main\n");

   ribs_swapcurcontext(ctx1);
   printf("back in main\n");

   return 0;
}
