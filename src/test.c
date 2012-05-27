#include "vmbuf.h"
#include "context.h"
#include "ctx_pool.h"
#include "hashtable.h"
#include <sys/time.h>

struct ribs_context ctx_main, ctx1, ctx2;

void fiber1()
{
   int a = 5;
   printf("in fiber 1: %d\n", a);
   int i;
   for (i = 0; i < 10; ++i)
   {
      ribs_swapcontext(&ctx2, &ctx1);
      printf("back in fiber 1: %d  %d\n", a, i);
   }

}

void cleanup_func1(void) {
   printf("in cleanup_func1\n");
}

void fiber2()
{
   printf("in fiber 2\n");
   int i;
   for (i = 0; i < 10; ++i)
   {
      ribs_swapcontext(&ctx1, &ctx2);
      printf("back in fiber 2: %d\n", i);
   }
}

#define REPORT_TIME()                                                   \
    gettimeofday(&end, NULL);                                           \
    timersub(&end, &start, &diff);                                      \
    printf("time: %llu\n", diff.tv_sec * 1000000ULL + diff.tv_usec);    \
    start = end;


#define NUM_KEYS 10000

int main(void) {

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
    return 0;

   const unsigned STACK_SIZE = 65536;
   char stk1[STACK_SIZE];
   char stk2[STACK_SIZE];

   ribs_makecontext(&ctx1, &ctx_main, stk1 + STACK_SIZE, fiber1, cleanup_func1);
   ribs_makecontext(&ctx2, &ctx_main, stk2 + STACK_SIZE, fiber2, NULL);
   printf("in main\n");

   ribs_swapcontext(&ctx1, &ctx_main);
   printf("back in main\n");

   return 0;
}
