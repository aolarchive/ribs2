ifndef OPTFLAGS
OPTFLAGS=-O3
endif
ifndef OBJ_SUB_DIR
OBJ_DIR=../obj
else
OBJ_DIR=../obj/$(OBJ_SUB_DIR)
endif

LDFLAGS+=-L../lib $(LIBS:%=-l%) 
CFLAGS+=$(OPTFLAGS) -ggdb3 -W -Wall -Werror

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
--redefine-sym getaddrinfo=ribs_getaddrinfo

OBJ=$(SRC:%.c=$(OBJ_DIR)/%.o) $(ASM:%.S=$(OBJ_DIR)/%.o) 
DEP=$(SRC:%.c=$(OBJ_DIR)/%.d)

DIRS=$(OBJ_DIR)/.dir ../bin/.dir ../lib/.dir ../ribified/.dir

ifeq ($(TARGET:%.a=%).a,$(TARGET))
LIB_OBJ:=$(OBJ)
TARGET_FILE=../lib/lib$(TARGET)
else
TARGET_FILE=../bin/$(TARGET)
endif

all: $(TARGET_FILE)

$(DIRS):
	@echo "  (MKDIR)  -p $(@:%/.dir=%)"
	@-mkdir -p $(@:%/.dir=%)
	@touch $@

$(OBJ_DIR)/%.o: %.c $(OBJ_DIR)/%.d
	@echo "  (C)      $*.c  [ -c $(CFLAGS) $*.c -o $(OBJ_DIR)/$*.o ]"
	@$(CC) -c $(CFLAGS) $*.c -o $(OBJ_DIR)/$*.o

$(OBJ_DIR)/%.o: %.S
	@echo "  (ASM)    $*.S  [ -c $(CFLAGS) $*.S -o $(OBJ_DIR)/$*.o ]"
	@$(CC) -c $(CFLAGS) $*.S -o $(OBJ_DIR)/$*.o

$(OBJ_DIR)/%.d: %.c
	@echo "  (DEP)    $*.c"
	@$(CC) -MM $(CFLAGS) $(INCLUDES) $*.c | sed -e 's|.*:|$(OBJ_DIR)/$*.o:|' > $@

$(OBJ): $(DIRS)

$(DEP): $(DIRS)

../lib/%: $(LIB_OBJ)
	@echo "  (AR)     $(@:../lib/%=%)  [ rcs $@ $^ ]"
	@ar rcs $@ $^

.PRECIOUS: $(RIBIFY:%=../ribified/%)

../ribified/%:
	@echo "  (RIBIFY) $(@:../ribified/%=%) [ $@ $(RIBIFYFLAGS) ]"
	@objcopy $(shell find /usr/lib -name $(@:../ribified/%=%)) $@ $(RIBIFYFLAGS)

../bin/%: $(OBJ) $(LIBS:%=../lib/lib%.a) $(RIBIFY:%=../ribified/%)
	@echo "  (LD)     $(@:../bin/%=%)  [ $(CC) -o $@ $(OBJ) $(LDFLAGS) ]"
	@echo "$(CC) -o $@ $(OBJ) $(LDFLAGS)"
	@$(CC) -o $@ $(OBJ) $(LDFLAGS)

$(DIRS:%=RM_%):
	@echo "  (RM)     $(@:RM_%/.dir=%)/*"
	@-rm -f $(@:RM_%/.dir=%)/*

clean: $(DIRS:%=RM_%)

etags:
	@echo "  (ETAGS)"
	@find . -name "*.[ch]" | cut -c 3- | xargs etags -I

ifneq ($(MAKECMDGOALS),clean)
-include $(DEP)
endif
