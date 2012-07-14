buildall: 
	@$(MAKE) -s -f ribs2.mk
ifneq ($(wildcard /usr/include/mysql/mysql.h),)
	@$(MAKE) -s -f ribs2_mysql.mk
endif
	@$(MAKE) -s -f httpd.mk
	@$(MAKE) -s -f logplayer.mk


include ../make/ribs.mk
