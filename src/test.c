#include "vmbuf.h"
#include "context.h"
#include "ctx_pool.h"

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


int main(void) {
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
