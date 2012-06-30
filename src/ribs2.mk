TARGET=ribs2.a

MYSQL_HELPER=mysql_helper.c
SRC=context.c epoll_worker.c ctx_pool.c http_server.c hashtable.c mime_types.c http_client_pool.c timeout_handler.c ribify.c logger.c daemonize.c http_headers.c $(MYSQL_HELPER)
ASM=context_asm.S

CFLAGS+= -I ../include

include ../make/ribs.mk
