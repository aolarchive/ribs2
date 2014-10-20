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
#include "logger.h"
#include "vmbuf.h"
#include "context.h"
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

static const char *MC_INFO  = "INFO ";
static const char *MC_ERROR = "ERROR";

static struct vmbuf log_buf = VMBUF_INITIALIZER;

static void begin_log_line(const char *msg_class) {
    struct tm tm, *tmp;
    struct timeval tv;
    intmax_t usec;
    gettimeofday(&tv, NULL);
    tmp = localtime_r(&tv.tv_sec, &tm);
    usec = tv.tv_usec;
    vmbuf_init(&log_buf, 4096);
    vmbuf_strftime(&log_buf, "%Y-%m-%d %H:%M:%S", tmp);
    vmbuf_sprintf(&log_buf, ".%03jd.%03jd %d %p %s ", usec / 1000, usec % 1000, getpid(), current_ctx, msg_class);
}

static inline void end_log_line(int fd) {
    *vmbuf_wloc(&log_buf) = '\n';
    vmbuf_wseek(&log_buf, 1);
    vmbuf_write(&log_buf, fd);
}

void logger_log(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    logger_vlog(STDOUT_FILENO, format, MC_INFO, ap);
    va_end(ap);
}

void logger_log_at(const char *filename, unsigned int linenum, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    logger_vlog_at(STDOUT_FILENO, filename, linenum, format, MC_INFO, ap);
    va_end(ap);
}

void logger_error(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    logger_vlog(STDERR_FILENO, format, MC_ERROR, ap);
    va_end(ap);
}

void logger_error_at(const char *filename, unsigned int linenum, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    logger_vlog_at(STDERR_FILENO, filename, linenum, format, MC_ERROR, ap);
    va_end(ap);
}

void logger_error_func_at(const char *filename, unsigned int linenum, const char *funcname, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    logger_vlog_func_at(STDERR_FILENO, filename, linenum, funcname, format, MC_ERROR, ap);
    va_end(ap);
}

void logger_perror(const char *format, ...) {
    begin_log_line(MC_ERROR);
    va_list ap;
    va_start(ap, format);

    char tmp[512];
    vmbuf_vsprintf(&log_buf, format, ap);
    vmbuf_sprintf(&log_buf, " (%s)", strerror_r(errno, tmp, 512));
    va_end(ap);
    end_log_line(STDERR_FILENO);
}

void logger_perror_at(const char *filename, unsigned int linenum, const char *format, ...) {
    begin_log_line(MC_ERROR);
    va_list ap;
    va_start(ap, format);

    vmbuf_sprintf(&log_buf, "[%s:%u]: ", filename, linenum);
    vmbuf_vsprintf(&log_buf, format, ap);
    char tmp[512];
    vmbuf_sprintf(&log_buf, " (%s)", strerror_r(errno, tmp, 512));
    va_end(ap);
    end_log_line(STDERR_FILENO);
}

void logger_perror_func_at(const char *filename, unsigned int linenum, const char *funcname, const char *format, ...) {
    begin_log_line(MC_ERROR);
    va_list ap;
    va_start(ap, format);

    vmbuf_sprintf(&log_buf, "[%s:%u] %s(): ", filename, linenum, funcname);
    vmbuf_vsprintf(&log_buf, format, ap);
    char tmp[512];
    vmbuf_sprintf(&log_buf, " (%s)", strerror_r(errno, tmp, 512));
    va_end(ap);
    end_log_line(STDERR_FILENO);
}


void logger_vlog(int fd, const char *format, const char *msg_class, va_list ap) {
    begin_log_line(msg_class);
    vmbuf_vsprintf(&log_buf, format, ap);
    end_log_line(fd);
}

void logger_vlog_at(int fd, const char *filename, unsigned int linenum, const char *format, const char *msg_class, va_list ap) {
    begin_log_line(msg_class);
    vmbuf_sprintf(&log_buf, "[%s:%u]: ", filename, linenum);
    vmbuf_vsprintf(&log_buf, format, ap);
    end_log_line(fd);
}

void logger_vlog_func_at(int fd, const char *filename, unsigned int linenum, const char *funcname, const char *format, const char *msg_class, va_list ap) {
    begin_log_line(msg_class);
    vmbuf_sprintf(&log_buf, "[%s:%u] %s(): ", filename, linenum, funcname);
    vmbuf_vsprintf(&log_buf, format, ap);
    end_log_line(fd);
}
