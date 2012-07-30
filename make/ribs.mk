# This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
# RIBS is an infrastructure for building great SaaS applications (but not
# limited to).

# Copyright (C) 2012 Adap.tv, Inc.

# RIBS is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, version 2.1 of the License.

# RIBS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public License
# along with RIBS.  If not, see <http://www.gnu.org/licenses/>.

ifndef OPTFLAGS
OPTFLAGS=-O3
endif
ifndef OBJ_SUB_DIR
OBJ_DIR=../obj
else
OBJ_DIR=../obj/$(OBJ_SUB_DIR)
endif

LDFLAGS+=-L../lib
CFLAGS+=$(OPTFLAGS) -ggdb3 -W -Wall -Werror
ASFLAGS+=-g

RIBIFYFLAGS+= \
--redefine-sym write=ribs_write \
--redefine-sym read=ribs_read \
--redefine-sym connect=ribs_connect \
--redefine-sym fcntl=ribs_fcntl \
--redefine-sym recvfrom=ribs_recvfrom \
--redefine-sym send=ribs_send \
--redefine-sym recv=ribs_recv \
--redefine-sym readv=ribs_readv \
--redefine-sym writev=ribs_writev \
--redefine-sym pipe2=ribs_pipe2 \
--redefine-sym pipe=ribs_pipe \
--redefine-sym nanosleep=ribs_nanosleep \
--redefine-sym usleep=ribs_usleep \
--redefine-sym sleep=ribs_sleep

ifdef UGLY_GETADDRINFO_WORKAROUND
LDFLAGS+=-lanl
RIBIFYFLAGS+=--redefine-sym getaddrinfo=ribs_getaddrinfo
CFLAGS+=-DUGLY_GETADDRINFO_WORKAROUND
endif

OBJ=$(SRC:%.c=$(OBJ_DIR)/%.o) $(ASM:%.S=$(OBJ_DIR)/%.o)
DEP=$(SRC:%.c=$(OBJ_DIR)/%.d)

DIRS=$(OBJ_DIR)/.dir ../bin/.dir ../lib/.dir
RIBIFY_DIR=../ribified/.dir
ALL_DIRS=$(DIRS) $(RIBIFY_DIR)

ifeq ($(TARGET:%.a=%).a,$(TARGET))
LIB_OBJ:=$(OBJ)
TARGET_FILE=../lib/lib$(TARGET)
else
TARGET_FILE=../bin/$(TARGET)
endif

all: $(TARGET_FILE)

$(ALL_DIRS):
	@echo "  (MKDIR)  -p $(@:%/.dir=%)"
	@-mkdir -p $(@:%/.dir=%)
	@touch $@

$(OBJ_DIR)/%.o: %.c $(OBJ_DIR)/%.d
	@echo "  (C)      $*.c  [ -c $(CFLAGS) $*.c -o $(OBJ_DIR)/$*.o ]"
	@$(CC) -c $(CFLAGS) $*.c -o $(OBJ_DIR)/$*.o

$(OBJ_DIR)/%.o: %.S
	@echo "  (ASM)    $*.S  [ -c $(CFLAGS) $*.S -o $(OBJ_DIR)/$*.o ]"
	@$(AS) $(ASFLAGS) $*.S -o $(OBJ_DIR)/$*.o

$(OBJ_DIR)/%.d: %.c
	@echo "  (DEP)    $*.c"
	@$(CC) -MM $(CFLAGS) $(INCLUDES) $*.c | sed -e 's|.*:|$(OBJ_DIR)/$*.o:|' > $@

$(OBJ): $(DIRS)

$(DEP): $(DIRS)

../lib/%: $(LIB_OBJ)
	@echo "  (AR)     $(@:../lib/%=%)  [ rcs $@ $^ ]"
	@$(AR) rcs $@ $^

.PRECIOUS: $(RIBIFY:%=../ribified/%)

../ribified/%: $(RIBIFY_DIR)
	@echo "  (RIBIFY) $(@:../ribified/%=%) [ $@ $(RIBIFYFLAGS) ]"
	@objcopy $(shell find /usr/lib -name $(@:../ribified/%=%)) $@ $(RIBIFYFLAGS)

../bin/%: $(OBJ) $(RIBIFY:%=../ribified/%)
	@echo "  (LD)     $(@:../bin/%=%)  [ -o $@ $(OBJ) $(LDFLAGS) ]"
	@$(CC) -o $@ $(OBJ) $(LDFLAGS)

$(ALL_DIRS:%=RM_%):
	@echo "  (RM)     $(@:RM_%/.dir=%)/*"
	@-$(RM) $(@:RM_%/.dir=%)/*

clean: $(ALL_DIRS:%=RM_%)

etags:
	@echo "  (ETAGS)"
	@find . -name "*.[ch]" | cut -c 3- | xargs etags -I

ifneq ($(MAKECMDGOALS),clean)
-include $(DEP)
endif
