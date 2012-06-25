TARGET=test_mysql

SRC=test_mysql.c

RIBIFY=libmysqlclient.a

CFLAGS+=-I ../include -I../3rd_party/mysql_client/include
LDFLAGS+=-L../ribified -pthread -lmysqlclient -lm -lz -ldl
LIBS+=ribs2

include ../make/ribs.mk
