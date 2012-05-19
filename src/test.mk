TARGET=test

SRC=test.c context.c acceptor.c epoll_worker.c
ASM=context_asm.S

CFLAGS+= -I ../include

include ../make/ribs.mk
