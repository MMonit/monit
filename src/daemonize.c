/*
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU Affero General Public License in all respects
 * for all of the code used other than OpenSSL.
 */


#include "config.h"

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "monit.h"
#include "file.h"

// libmonit
#include "io/File.h"


/**
 *  Transform this program into a daemon and provide methods for
 *  managing the daemon.
 *
 *  @file
 */


/* ------------------------------------------------------------------ Public */


/**
 * Transform a program into a daemon. Inspired by code from Stephen
 * A. Rago's book, Unix System V Network Programming.
 */
void daemonize(void) {
        pid_t pid;
        if ((pid = fork ()) < 0) {
                Log_error("Cannot fork a new process\n");
                exit (1);
        } else if (pid != 0) {
                _exit(0);
        }
        // Become a session leader to lose our controlling terminal
        setsid();
        if ((pid = fork ()) < 0) {
                Log_error("Cannot fork a new process\n");
                exit (1);
        } else if (pid != 0) {
                _exit(0);
        }
        // Change current directory to the root so that other file systems can be unmounted while we're running
        if (chdir("/") < 0) {
                Log_error("Cannot chdir to / -- %s\n", STRERROR);
                exit(1);
        }
        // Redirect standard descriptors to /dev/null. Other descriptors should be closed in env.c
        Util_redirectStdFds();
}


/**
 * Send signal to a daemon process
 * @param sig Signal to send daemon to
 * @return true if signal was send, otherwise false
 */
bool kill_daemon(int sig) {
        pid_t pid;
        if ((pid = exist_daemon()) > 0) {
                if (kill(pid, sig) < 0) {
                        Log_error("Cannot signal the monit daemon process -- %s\n", STRERROR);
                        return false;
                }
        } else {
                Log_info("Monit daemon is not running\n");
                return true;
        }
        if (sig == SIGTERM) {
                fprintf(stdout, "Monit daemon with pid [%d] killed\n", (int)pid);
                fflush(stdout);
        }
        return true;
}


/**
 * Check if a Monit daemon is already running by testing the pidfile lock.
 * @return The daemon's pid if running, otherwise 0
 */
pid_t exist_daemon(void) {
        pid_t pid = file_getPid(Run.files.pidfile);
        if (pid <= 0)
                return 0;
        int locked = File_isLocked(Run.files.pidfile);
        if (locked == 1)
                return pid;
        /*
         * Fallback for pre-6.0.0: pidfile exists but unlocked.
         *
         * TODO: Remove this legacy fallback in a future Monit release
         */
        if (pid != getpid() && kill(pid, 0) == 0)
                return pid;
        return 0;
}
