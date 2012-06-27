#ifndef _LOGGER__H_
#define _LOGGER__H_

#include "ribs_defs.h"
#include <stdarg.h>

void logger_log(const char *format, ...);
void logger_log_at(const char *filename, unsigned int linenum, const char *format, ...);
void logger_error(const char *format, ...);
void logger_error_at(const char *filename, unsigned int linenum, const char *format, ...);
void logger_error_func_at(const char *filename, unsigned int linenum, const char *funcname, const char *format, ...);
void logger_perror(const char *format, ...);
void logger_perror_at(const char *filename, unsigned int linenum, const char *format, ...);
void logger_perror_func_at(const char *filename, unsigned int linenum, const char *funcname, const char *format, ...);
void logger_vlog(int fd, const char *format, const char *msg_class, va_list ap);
void logger_vlog_at(int fd, const char *filename, unsigned int linenum, const char *format, const char *msg_class, va_list ap);
void logger_vlog_func_at(int fd, const char *filename, unsigned int linenum, const char *funcname, const char *format, const char *msg_class, va_list ap);

#define LOGGER_WHERE __FILE__, __LINE__
#define LOGGER_INFO(format, ...) logger_log(format, ##__VA_ARGS__)
#define LOGGER_INFO_AT(format, ...) logger_log_at(LOGGER_WHERE, format, ##__VA_ARGS__)
#define LOGGER_ERROR(format, ...) logger_error_at(LOGGER_WHERE, format, ##__VA_ARGS__)
#define LOGGER_ERROR_FUNC(format, ...) logger_error_func_at(LOGGER_WHERE, __FUNCTION__, format, ##__VA_ARGS__)
#define LOGGER_ERROR_EXIT(ec, format, ...) { logger_error_at(LOGGER_WHERE, format, ##__VA_ARGS__); exit(ec); }
#define LOGGER_PERROR(format, ...) logger_perror_at(LOGGER_WHERE, format, ##__VA_ARGS__)
#define LOGGER_PERROR_FUNC(format, ...) logger_perror_func_at(LOGGER_WHERE, __FUNCTION__, format, ##__VA_ARGS__)
#define LOGGER_PERROR_EXIT(ec, format, ...) { logger_perror_at(LOGGER_WHERE, format, ##__VA_ARGS__); exit(ec); }

#endif // _LOGGER__H_
