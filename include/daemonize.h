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
#ifndef _DAEMONIZE__H_
#define _DAEMONIZE__H_

#include "ribs_defs.h"
#include <sys/types.h>
#include <sys/wait.h>

int ribs_server_init(int daemonize, const char *pidfilename, const char *logfilename, int num_forks);
int ribs_server_signal_children(int sig);
void ribs_server_start(void);
int ribs_get_daemon_instance(void);
int daemonize(void);
void daemon_finalize(void);
int ribs_logger_init(const char *filename);
int ribs_set_signals(void);
int ribs_set_pidfile(const char *filename);
int ribs_queue_waitpid(pid_t pid);
const siginfo_t *ribs_last_siginfo(void);
const siginfo_t *ribs_fork_and_wait(void);

#endif // _DAEMONIZE__H_
