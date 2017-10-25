TARGET=mydump    # the name of the executable

SRC=mydump.c mydump_data.c # list of source files

# we need to tell gcc where to find the include files of ribs2
CFLAGS+= -I ../../../include
# as well as include files in the local directory
CFLAGS+= -I .

# we need to tell the linker where to find ribs2 libraries
LDFLAGS+= -L ../../../lib -lribs2

include ../../../make/ribs.mk  # include ribs2 make system
