TARGET=test_mysql

SRC=test_mysql.c

RIBIFY= /usr/lib/x86_64-linux-gnu/libmysqlclient.a
# /usr/lib/libGeoIP.a

CFLAGS+= -I ../include -I../3rd_party/mysql_client/include
LDFLAGS+= -Wl,-static ../ribified/$(RIBIFIED) -Wl,-dy -lm -lpthread -ldl -lz
LIBS+=ribs2

include ../make/ribs.mk
