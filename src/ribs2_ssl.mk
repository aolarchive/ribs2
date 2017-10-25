TARGET=ribs2_ssl.a
CPPFLAGS+=-DRIBS2_SSL
OBJ_SUB_DIR=ssl
include ribs2_common.mk
SRC+=ribs_ssl.c
include ../make/ribs.mk
