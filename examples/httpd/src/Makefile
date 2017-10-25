TARGET=httpd

SRC=httpd.c

CFLAGS+= -I ../../../include
LDFLAGS+= -L../../../lib

ifneq ($(wildcard /usr/include/openssl/ssl.h),)
RIBS2_SSL=1
LDFLAGS+= -lribs2_ssl -lssl -lcrypto
else
LDFLAGS+= -lribs2
endif

EXTRA_DEPS=../../../lib/libribs2.a

include ../../../make/ribs.mk
