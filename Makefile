all:
	@echo "[ribs] build"
	@make -C src -s
	@echo "[ribs] success"
#	@make -C examples -s

clean:
	@echo "[ribs] clean"
	@make -C src -s clean
#	@make -C examples -s clean
