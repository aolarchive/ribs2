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
