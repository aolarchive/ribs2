TARGET=ribs2.a

SRC=context.c epoll_worker.c ctx_pool.c http_server.c hashtable.c mime_types.c http_client_pool.c timeout_handler.c ribify.c logger.c daemonize.c http_headers.c http_cookies.c file_mapper.c ds_var_field.c
ASM=context_asm.S

CFLAGS+= -I ../include

include ../make/ribs.mk
