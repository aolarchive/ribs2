TARGET=test_mysql

SRC=test_mysql.c

RIBIFY=libmysqlclient.a

CFLAGS+=-I ../include -I../3rd_party/mysql_client/include
LDFLAGS+=../ribified/$(RIBIFY) ../lib/libribs2.a -lm -lpthread -lz -lanl -ldl
# LIBS+=ribs2

include ../make/ribs.mk
