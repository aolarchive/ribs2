TARGET=test_mysql

SRC=test_mysql.c

RIBIFY=libmysqlclient.a libGeoIP.a

CFLAGS+= -I ../include -I../3rd_party/mysql_client/include
LDFLAGS+= -Wl,-static $(RIBIFY:%=../ribified/%) -Wl,-dy -lm -lpthread -ldl -lz
LIBS+=ribs2

include ../make/ribs.mk
