TARGET=test

SRC=test.c context.c
ASM=context_asm.S

CFLAGS+= -I ../include

include ../make/ribs.mk
