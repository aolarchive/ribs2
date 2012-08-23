buildall:
	@$(MAKE) -s -f ribs2.mk
ifneq ($(wildcard /usr/include/mysql/mysql.h),)
	@$(MAKE) -s -f ribs2_mysql.mk
endif

include ../make/ribs.mk
