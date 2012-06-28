all:
	@echo "[ribs2] build"
	@$(MAKE) -C src -s
	@echo "[ribs2] success"
	@echo "[tests] build"
	@$(MAKE) -C tests/src -s
	@echo "[tests] success"

clean:
	@echo "[ribs2] clean"
	@$(MAKE) -C src -s clean
	@echo "[tests] clean"
	@$(MAKE) -C tests/src -s clean
etags:
	@echo "etags"
	find . -regex ".*\.[cChH]\(pp\)?" -print | etags -
