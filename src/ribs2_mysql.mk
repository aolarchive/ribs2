TARGET=ribs2_mysql.a

SRC=mysql_helper.c mysql_dumper.c

CFLAGS+= -I ../include

include ../make/ribs.mk
