ifndef OPTFLAGS
OPTFLAGS=-O3
endif
ifndef OBJ_SUB_DIR
OBJ_DIR=../obj
else
OBJ_DIR=../obj/$(OBJ_SUB_DIR)
endif

LDFLAGS+= -lrt -L../lib $(LIBS:%=-l%)
CFLAGS+= $(OPTFLAGS) -ggdb3 -W -Wall -Werror

OBJ=$(SRC:%.c=$(OBJ_DIR)/%.o) $(ASM:%.S=$(OBJ_DIR)/%.o) 
DEP=$(SRC:%.c=$(OBJ_DIR)/%.d)

DIRS=$(OBJ_DIR)/.dir ../bin/.dir ../lib/.dir

ifeq ($(TARGET:%.a=%).a,$(TARGET))
LIB_OBJ:=$(OBJ)
TARGET_FILE=../lib/lib$(TARGET)
else
TARGET_FILE=../bin/$(TARGET)
endif

all: $(TARGET_FILE)

$(DIRS):
	@echo "  (MKDIR) -p $(@:%/.dir=%)"
	@-mkdir -p $(@:%/.dir=%)
	@touch $@

$(OBJ_DIR)/%.o: %.c $(OBJ_DIR)/%.d
	@echo "  (C)    $*.c  [ -c $(CFLAGS) $*.c -o $(OBJ_DIR)/$*.o ]"
	@$(CC) -c $(CFLAGS) $*.c -o $(OBJ_DIR)/$*.o

$(OBJ_DIR)/%.o: %.S
	@echo "  (ASM)  $*.S  [ -c $(CFLAGS) $*.S -o $(OBJ_DIR)/$*.o ]"
	@$(CC) -c $(CFLAGS) $*.S -o $(OBJ_DIR)/$*.o

$(OBJ_DIR)/%.d: %.c
	@echo "  (DEP)  $*.c"
	@$(CC) -MM $(CFLAGS) $(INCLUDES) $*.c | sed -e 's|.*:|$(OBJ_DIR)/$*.o:|' > $@

$(OBJ): $(DIRS)

$(DEP): $(DIRS)

../lib/%: $(LIB_OBJ)
	@echo "  (AR)   $(@:../lib/%=%)  [ rcs $@ $^ ]"
	@ar rcs $@ $^

../bin/%: $(OBJ) $(LIBS:%=../lib/lib%.a)
	@echo "  (LD)   $(@:../bin/%=%)  [ -o $@ $(OBJ) $(LDFLAGS) ]"
	@$(CC) -o $@ $(OBJ) $(LDFLAGS)

$(DIRS:%=RM_%):
	@echo "  (RM)   $(@:RM_%/.dir=%)/*"
	@-rm -f $(@:RM_%/.dir=%)/*

clean: $(DIRS:%=RM_%)

etags:
	@echo "  (ETAGS)"
	@find . -name "*.[ch]" | cut -c 3- | xargs etags -I

ifneq ($(MAKECMDGOALS),clean)
-include $(DEP)
endif
