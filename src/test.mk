TARGET=test

SRC=test.c context.c acceptor.c epoll_worker.c ctx_pool.c
ASM=context_asm.S

CFLAGS+= -I ../include

include ../make/ribs.mk
