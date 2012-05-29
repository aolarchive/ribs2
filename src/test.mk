TARGET=test

SRC=test.c

CFLAGS+= -I ../include
LIBS+=ribs2

include ../make/ribs.mk
