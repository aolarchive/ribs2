all:
	@echo "[ribs2] build"
	@$(MAKE) -C src -s
	@echo "[ribs2] success"
#	@make -C examples -s

clean:
	@echo "[ribs2] clean"
	@$(MAKE) -C src -s clean
#	@make -C examples -s clean
etags:
	@echo "etags"
	find . -regex ".*\.[cChH]\(pp\)?" -print | etags -
