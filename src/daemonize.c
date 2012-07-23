/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012 Adap.tv, Inc.

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
#include "daemonize.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "logger.h"

int daemonize() {
    pid_t pid = fork();
    if (pid < 0) {
        LOGGER_PERROR("daemonize, fork");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        LOGGER_INFO("daemonize started (pid=%d)", pid);
        exit(EXIT_SUCCESS);
    }

    umask(0);

    if (0 > setsid()) {
        LOGGER_PERROR("daemonize, setsid");
        exit(EXIT_FAILURE);
    }

    int fdnull = open("/dev/null", O_RDWR);
    dup2(fdnull, STDIN_FILENO);
    dup2(fdnull, STDOUT_FILENO);
    dup2(fdnull, STDERR_FILENO);
    close(fdnull);

    pid = getpid();

    LOGGER_INFO("child process started (pid=%d)", pid);
    return 0;
}
