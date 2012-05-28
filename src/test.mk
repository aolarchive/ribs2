TARGET=test

SRC=test.c context.c epoll_worker.c ctx_pool.c hashtable.c vmbuf.c
ASM=context_asm.S

CFLAGS+= -I ../include

include ../make/ribs.mk
