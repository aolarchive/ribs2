TARGET=ribs2_mysql.a

SRC=mysql_helper.c mysql_dumper.c mysql_common.c

CFLAGS+= -I ../include

include ../make/ribs.mk
