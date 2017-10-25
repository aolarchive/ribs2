# buildall : ribs2_
# clean: ribs2_clean

ribs2_libs=ribs2

#SSL
ifneq ($(RIBS2_SSL),0)
ifneq ($(wildcard /usr/include/openssl/ssl.h),)
ribs2_libs+=ribs2_ssl
endif
endif

# mysql
ifneq ($(wildcard /usr/include/mysql/mysql.h),)
ribs2_libs+=ribs2_mysql
endif

#zlib
ifneq ($(wildcard /usr/include/zlib.h),)
ribs2_libs+=ribs2_zlib
endif

ribs2_libs_clean=$(ribs2_libs:%=%_clean)

buildall : $(ribs2_libs)
clean: $(ribs2_libs_clean)

$(ribs2_libs):
	@echo "~~~~~~~~[$@]~~~~~~~~"
	@$(MAKE) -s -f "$@.mk"

$(ribs2_libs_clean):
	@echo "~~~~~~~~[$@]~~~~~~~~"
	@$(MAKE) -s -f "$(@:%_clean=%).mk" clean


