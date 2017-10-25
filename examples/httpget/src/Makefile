TARGET=httpget

SRC=httpget.c

RIBS2_SSL=1

CFLAGS+= -I ../../../include
LDFLAGS+= -L../../../lib -lribs2_ssl -lssl -lcrypto

EXTRA_DEPS=../../../lib/libribs2.a

include ../../../make/ribs.mk
