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
#include "epoll_worker.h"
#include "vmfile.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "logger.h"

static const char *pidfile = NULL;
static int child_is_up_pipe[2] = { -1, -1 };

int daemonize(void) {
    if (0 > pipe(child_is_up_pipe))
        return LOGGER_ERROR("failed to create pipe"), -1;

    pid_t pid = fork();
    if (pid < 0) {
        LOGGER_PERROR("daemonize, fork");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        LOGGER_INFO("daemonize started (pid=%d)", pid);
        close(child_is_up_pipe[1]); /* close the write side */
        /* wait for child to come up */
        uint8_t t;
        int res;
        LOGGER_INFO("waiting for the child process to start...");
        if (0 >= (res = read(child_is_up_pipe[0], &t, sizeof(t)))) {
            if (0 > res)
                LOGGER_PERROR("pipe");
            LOGGER_ERROR("child process failed to start");
            exit(EXIT_FAILURE);
        }
        LOGGER_INFO("child process started successfully");
        exit(EXIT_SUCCESS);
    }

    close(child_is_up_pipe[0]); /* close the read side */

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

void daemon_finalize(void) {
    if (0 > child_is_up_pipe[1])
        return;
    uint8_t t = 0;
    if (0 > write(child_is_up_pipe[1], &t, sizeof(t))) {
        LOGGER_PERROR("pipe");
        exit(EXIT_FAILURE);
    }
}

int ribs_logger_init(const char *filename) {
    if ('|' == *filename) {
        // popen
        ++filename;
        FILE *fp = popen(filename, "w");
        if (NULL != fp) {
            int fd = fileno(fp);
            if (0 > dup2(fd, STDOUT_FILENO) ||
                0 > dup2(fd, STDERR_FILENO))
                return perror("dup2"), -1;
            pclose(fp);
         } else
            return -1;
    } else {
        // open
        int fd = open(filename, O_CREAT | O_WRONLY | O_APPEND, 0644);
        if (fd < 0)
            return perror(filename), -1;
        if (0 > dup2(fd, STDOUT_FILENO) ||
            0 > dup2(fd, STDERR_FILENO))
            return perror("dup2"), -1;
        close(fd);
    }
    return 0;
}

static void signal_handler(int signum) {
    switch(signum) {
    case SIGINT:
    case SIGTERM:
        if (pidfile) unlink(pidfile);
        epoll_worker_exit();
        break;
    default:
        LOGGER_ERROR("unknown signal");
    }
}

int ribs_set_pidfile(const char *filename) {
    pidfile = filename;
    struct vmfile vmf_pid = VMFILE_INITIALIZER;
    if (0 > vmfile_init(&vmf_pid, pidfile, 4096))
        return -1;
    vmfile_sprintf(&vmf_pid, "%d", (int)getpid());
    vmfile_close(&vmf_pid);
    struct sigaction sa = {
        .sa_handler = signal_handler,
        .sa_flags = 0
    };
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    return 0;
}
