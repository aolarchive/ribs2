TARGET=test

SRC=test.c

CFLAGS+= -I ../include
LDFLAGS+=-lribs2

include ../make/ribs.mk
