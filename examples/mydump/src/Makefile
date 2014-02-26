all:
	@$(MAKE) -s -f mydump.mk
ifneq ($(wildcard /usr/include/mysql/mysql.h),)
	@$(MAKE) -s -f mydumper.mk
endif

clean:
	@$(MAKE) -s -f mydump.mk clean
	@$(MAKE) -s -f mydumper.mk clean
