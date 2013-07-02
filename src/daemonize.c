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
#include <sys/prctl.h>
#include <sys/wait.h>
#include <sys/signalfd.h>

#include "logger.h"

static const char *pidfile = NULL;
static int child_is_up_pipe[2] = { -1, -1 };
static int daemon_instance = 0;
static int num_instances = -1;
static pid_t *children_pids;
static pid_t logger_pid = -1;
static int sigfd = -1;
static struct ribs_context *sigfd_ctx = NULL;

#define SIG_CHLD_STACK_SIZE 128*1024

static int _logger_init(const char *filename) {
    int res = 0;
    if ('|' == *filename) {
        ++filename;
        int fds[2];
        if (0 > pipe2(fds, O_CLOEXEC))
            return LOGGER_PERROR("pipe"), -1;
        int pid = fork();
        if (0 > pid)
            return -1;
        if (0 == pid) {
            if (0 > dup2(fds[0], STDIN_FILENO))
                LOGGER_PERROR("dup2"), res = -1;
            close(fds[0]);
            close(fds[1]);
            if (0 > execl("/bin/sh", "/bin/sh", "-c", filename, NULL))
                LOGGER_PERROR("execl: %s", filename), res = -1;
        } else {
            logger_pid = pid;
            if (0 > dup2(fds[1], STDOUT_FILENO) ||
                0 > dup2(fds[1], STDERR_FILENO))
                LOGGER_PERROR("dup2"), res = -1;
            close(fds[0]);
            close(fds[1]);
        }
    } else {
        // open
        int fd = open(filename, O_CREAT | O_WRONLY | O_APPEND, 0644);
        if (fd < 0)
            return LOGGER_PERROR(filename), -1;
        if (0 > dup2(fd, STDOUT_FILENO) ||
            0 > dup2(fd, STDERR_FILENO))
            LOGGER_PERROR("dup2"), res = -1;
        close(fd);
    }
    return res;
}

static void _cleanup_pidfile(void) {
    if (pidfile) unlink(pidfile);
}

static int _set_pidfile(const char *filename) {
    if (pidfile)
        return LOGGER_ERROR("pidfile is already set"), -1;
    pidfile = filename;
    struct vmfile vmf_pid = VMFILE_INITIALIZER;
    if (0 > vmfile_init(&vmf_pid, pidfile, 4096))
        return -1;
    vmfile_sprintf(&vmf_pid, "%d", (int)getpid());
    vmfile_close(&vmf_pid);
    if (0 != atexit(_cleanup_pidfile))
        return -1;
    return 0;
}

#define CLD_ENUM_STR(x) case x: return #x
static const char *_get_exit_reason(siginfo_t *info) {
    switch(info->si_code) {
        CLD_ENUM_STR(CLD_EXITED);
        CLD_ENUM_STR(CLD_KILLED);
        CLD_ENUM_STR(CLD_DUMPED);
    }
    return "UNKNOWN";
}

static void _handle_sig_child(void) {
    struct signalfd_siginfo sfd_info;
    for (;;yield()) {
        for (;;) {
            ssize_t res = read(sigfd, &sfd_info, sizeof(struct signalfd_siginfo));
            if (0 > res) {
                if (errno != EAGAIN)
                    LOGGER_PERROR("read from signal fd");
                break;
            }
            if (sizeof(struct signalfd_siginfo) != res) {
                LOGGER_ERROR("failed to read from signal fd, incorrect size");
                continue;
            }
            siginfo_t info;
            memset(&info, 0, sizeof(info));
            for (;;) {
                info.si_pid = 0;
                if (0 > waitid(P_ALL, 0, &info, WEXITED | WNOHANG) && ECHILD != errno) {
                    LOGGER_PERROR("waitid");
                    break;
                }
                if (0 == info.si_pid)
                    break; /* no more events */
                if (logger_pid == (pid_t)info.si_pid) {
                    char fname[256];
                    snprintf(fname, sizeof(fname), "%s-%d.crash.log", program_invocation_short_name, getpid());
                    fname[sizeof(fname)-1] = 0;
                    int fd = creat(fname, 0644);
                    if (0 <= fd) {
                        dup2(fd, STDOUT_FILENO);
                        dup2(fd, STDERR_FILENO);
                        close(fd);
                        LOGGER_ERROR("logger [%d] terminated unexpectedly: %s, status=%d", info.si_pid, _get_exit_reason(&info), info.si_status);
                    }
                    epoll_worker_exit();
                }
                int i;
                for (i = 0; i < num_instances-1; ++i) {
                    /* check if it is our pid */
                    if (children_pids[i] == (pid_t)info.si_pid) {
                        children_pids[i] = 0; /* mark pid as handled */
                        LOGGER_ERROR("child process [%d] terminated unexpectedly: %s, status=%d", info.si_pid, _get_exit_reason(&info), info.si_status);
                        epoll_worker_exit();
                    }
                }
            }
        }

    }
}

static void signal_handler(int signum) {
    switch(signum) {
    case SIGINT:
        if (0 != daemon_instance) {
            LOGGER_INFO("ignoring SIGINT in child process");
            break;
        }
    case SIGTERM:
        epoll_worker_exit();
        break;
    default:
        LOGGER_ERROR("unknown signal");
    }
}

static int _set_signals(void) {
    struct sigaction sa = {
        .sa_handler = signal_handler,
        .sa_flags = 0
    };
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGCHLD);
    if (0 > sigprocmask(SIG_BLOCK, &sigset, NULL))
        return LOGGER_PERROR("sigprocmask"), -1;
    sigfd = signalfd(-1, &sigset, SFD_NONBLOCK | SFD_CLOEXEC);
    if (0 > sigfd)
        return LOGGER_PERROR("signalfd"), -1;
    if (NULL == (sigfd_ctx = ribs_context_create(SIG_CHLD_STACK_SIZE, 0, _handle_sig_child)))
        return -1;
    return 0;
}

static int _init_subprocesses(const char *pidfilename, int num_forks) {
    if (0 >= num_forks) {
        num_forks = sysconf(_SC_NPROCESSORS_CONF);
        if (0 > num_forks)
            exit(EXIT_FAILURE);
    }
    LOGGER_INFO("num forks = %d", num_forks);
    num_instances = num_forks;
    daemon_instance = 0;
    children_pids = calloc(num_forks - 1, sizeof(pid_t));
    int i;
    for (i = 1; i < num_forks; ++i) {
        LOGGER_INFO("starting sub-process %d", i);
        pid_t pid = fork();
        if (0 > pid) return LOGGER_PERROR("fork"), -1;
        if (0 == pid) {
            daemon_instance = i;
            if (0 > prctl(PR_SET_PDEATHSIG, SIGTERM))
                return LOGGER_PERROR("prctl"), -1;
            if (0 > _set_signals())
                return LOGGER_ERROR("failed to set signals"), -1;
            LOGGER_INFO("sub-process %d started", i);
            return 0;
        } else
            children_pids[i-1] = pid;
    }
    if (pidfilename && 0 > _set_pidfile(pidfilename))
        return LOGGER_ERROR("failed to set pidfile"), -1;
    if (0 > _set_signals())
        return LOGGER_ERROR("failed to set signals"), -1;
    return 0;
}

static int ribs_server_init_daemon(const char *pidfilename, const char *logfilename, int num_forks) {
    if (0 > pipe2(child_is_up_pipe, O_CLOEXEC))
        return LOGGER_ERROR("failed to create pipe"), -1;

    pid_t pid = fork();
    if (pid < 0)
        return LOGGER_PERROR("ribs_server_init_daemon, fork"), -1;

    if (pid > 0) {
        close(child_is_up_pipe[1]); /* close the write side */
        /* wait for child to come up */
        uint8_t t;
        int res;
        LOGGER_INFO("waiting for the child process [%d] to start...", pid);
        if (0 >= (res = read(child_is_up_pipe[0], &t, sizeof(t)))) {
            if (0 > res)
                LOGGER_PERROR("pipe");
            LOGGER_ERROR("child process failed to start");
            return -1;
        }
        LOGGER_INFO("child process started successfully");
        exit(EXIT_SUCCESS);
    }
    close(child_is_up_pipe[0]); /* close the read side */

    umask(0);

    if (0 > setsid())
        return LOGGER_PERROR("daemonize, setsid"), -1;

    int fdnull = open("/dev/null", O_RDWR);
    if (0 > fdnull)
        return LOGGER_PERROR("/dev/null"), -1;

    dup2(fdnull, STDIN_FILENO);
    if (logfilename) {
        if (0 > _logger_init(logfilename))
            return -1;
    } else {
        dup2(fdnull, STDOUT_FILENO);
        dup2(fdnull, STDERR_FILENO);
    }
    close(fdnull);
    LOGGER_INFO("child process started");
    return _init_subprocesses(pidfilename, num_forks);
}

int ribs_server_init(int daemonize, const char *pidfilename, const char *logfilename, int num_forks) {
    if (daemonize)
        return ribs_server_init_daemon(pidfilename, logfilename, num_forks);
    /* if (logfilename && 0 > _logger_init(logfilename)) */
    /*     exit(EXIT_FAILURE); */
    return _init_subprocesses(pidfilename, num_forks);
}

int ribs_server_signal_children(int sig) {
    if (0 >= num_instances || 0 != daemon_instance)
        return 0;
    int i, res = 0;
    for (i = 0; i < num_instances-1; ++i) {
        if (0 < children_pids[i] && 0 > kill(children_pids[i], sig)) {
            LOGGER_PERROR("kill [%d] %d", sig, children_pids[i]);
            res = -1;
        }
    }
    return res;
}

void ribs_server_start(void) {
    daemon_finalize();
    if (0 <= sigfd && 0 > ribs_epoll_add(sigfd, EPOLLIN, sigfd_ctx))
        return LOGGER_ERROR("ribs_epoll_add: sigfd");
    epoll_worker_loop();
    if (0 >= num_instances || 0 != daemon_instance)
        return;
    LOGGER_INFO("sending SIGTERM to sub-processes");
    ribs_server_signal_children(SIGTERM);
    LOGGER_INFO("waiting for sub-processes to exit");
    int i, status;
    for (i = 0; i < num_instances-1; ++i) {
        if (0 < children_pids[i] && 0 > waitpid(children_pids[i], &status, 0))
            LOGGER_PERROR("waitpid %d", children_pids[i]);
    }
    LOGGER_INFO("sub-processes terminated");
}

int ribs_get_daemon_instance(void) {
    return daemon_instance;
}

int daemonize(void) {
    if (0 > pipe2(child_is_up_pipe, O_CLOEXEC))
        return LOGGER_ERROR("failed to create pipe"), -1;

    pid_t pid = fork();
    if (pid < 0) {
        LOGGER_PERROR("daemonize, fork");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        close(child_is_up_pipe[1]); /* close the write side */
        /* wait for child to come up */
        uint8_t t;
        int res;
        LOGGER_INFO("waiting for the child process [%d] to start...", pid);
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
    if (daemon_instance > 0)
        return;
    if (0 > child_is_up_pipe[1])
        return;
    uint8_t t = 0;
    if (0 > write(child_is_up_pipe[1], &t, sizeof(t))) {
        LOGGER_PERROR("pipe");
        exit(EXIT_FAILURE);
    }
    close(child_is_up_pipe[1]);
}

int ribs_logger_init(const char *filename) {
    return _logger_init(filename);
}

int ribs_set_signals(void) {
    return _set_signals();
}

int ribs_set_pidfile(const char *filename) {
    if (0 > _set_pidfile(filename))
        return -1;
    return _set_signals();
}
