EXAMPLES=httpd helloworld mydump httpget

all:
	@echo "[ribs2] build"
	@$(MAKE) -C src -s
	@echo "[ribs2] success"
	@$(MAKE) -s $(EXAMPLES:%=example_%)

example_%:
	@echo "[examples/$(@:example_%=%)] build"
	@$(MAKE) -C examples/$(@:example_%=%)/src -s
	@echo "[examples/$(@:example_%=%)] success"

clean: $(EXAMPLES:%=clean_example_%)
	@echo "[ribs2] clean"
	@$(MAKE) -C src -s clean

clean_example_%:
	@echo "[examples/$(@:clean_example_%=%)] clean"
	@$(MAKE) -C examples/$(@:clean_example_%=%)/src -s clean

etags:
	@echo "etags"
	find . -regex ".*\.[cChH]\(pp\)?" -print | etags -
