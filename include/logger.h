/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012,2013,2014 Adap.tv, Inc.

    RIBS is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 2.1 of the License.

    RIBS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with RIBS.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _LOGGER__H_
#define _LOGGER__H_

#include "ribs_defs.h"
#include <stdarg.h>

void logger_log(const char *format, ...) __attribute__ ((format (gnu_printf, 1, 2)));
void logger_log_at(const char *filename, unsigned int linenum, const char *format, ...) __attribute__ ((format (gnu_printf, 3, 4)));
void logger_error(const char *format, ...) __attribute__ ((format (gnu_printf, 1, 2)));
void logger_error_at(const char *filename, unsigned int linenum, const char *format, ...) __attribute__ ((format (gnu_printf, 3, 4)));
void logger_error_func_at(const char *filename, unsigned int linenum, const char *funcname, const char *format, ...) __attribute__ ((format (gnu_printf, 4, 5)));
void logger_perror(const char *format, ...) __attribute__ ((format (gnu_printf, 1, 2)));
void logger_perror_at(const char *filename, unsigned int linenum, const char *format, ...) __attribute__ ((format (gnu_printf, 3, 4)));
void logger_perror_func_at(const char *filename, unsigned int linenum, const char *funcname, const char *format, ...) __attribute__ ((format (gnu_printf, 4, 5)));
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

#ifdef DEBUG
#define LOGGER_DEBUG(format, ...) logger_log(format, ##__VA_ARGS__)
#define LOGGER_DEBUG_AT(format, ...) logger_log_at(LOGGER_WHERE, format, ##__VA_ARGS__)
#else
#define LOGGER_DEBUG(format, ...)
#define LOGGER_DEBUG_AT(format, ...)
#endif // DEBUG

#endif // _LOGGER__H_
