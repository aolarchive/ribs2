TARGET=test_mysql

SRC=test_mysql.c

CFLAGS+= -I ../include -I../3rd_party/mysql_client/include
LDFLAGS+=-L ../3rd_party/mysql_client/lib -lmysqlclient -lm -lpthread
LIBS+=ribs2

include ../make/ribs.mk
