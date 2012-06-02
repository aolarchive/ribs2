TARGET=ribs2.a

SRC=context.c epoll_worker.c ctx_pool.c http_server.c hashtable.c mime_types.c
ASM=context_asm.S

CFLAGS+= -I ../include

include ../make/ribs.mk
