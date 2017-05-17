TARGET=mydumper    # the name of the executable

SRC=mydumper.c     # list of source files

RIBIFY=libmysqlclient.a  # ribify this needed library

# we need to tell gcc where to find the include files of ribs2
CFLAGS+= -I ../../../include

# we need to tell the linker where to find ribs2 libraries
LDFLAGS+= -L ../../../lib -lribs2_mysql -lribs2

# as well as where to find the mysql libraries
LDFLAGS+=-L../ribified -pthread -lribs2_mysql -lmysqlclient \
                -lm -lz -ldl -L../../ribs2/lib -lribs2

# to statically link libmysqlclient, we need stdc++ for mysql >= 5.6
MYSQLVER_GTE_5_6=$(shell expr `mysql_config --version` \>= 5.6)
ifeq ($(MYSQLVER_GTE_5_6),1)
LDFLAGS+=-lstdc++
endif


include ../../../make/ribs.mk  # include ribs2 make system
