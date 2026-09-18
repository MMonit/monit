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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#include "monit.h"
#include "engine.h"

// libmonit
#include "io/File.h"

/**
 *  Utilities for managing files used by monit.
 *
 *  @file
 */


#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif


/* ------------------------------------------------------------------ Public */


void file_init(void) {
        char pidfile[STRLEN];
        char buf[STRLEN];
        /* Check if the pidfile was already set during configfile parsing */
        if (Run.files.pidfile == NULL) {
                /* Set the location of this programs pidfile */
                if (! getuid()) {
                        snprintf(pidfile, STRLEN, "%s/%s", MYPIDDIR, MYPIDFILE);
                } else {
                        snprintf(pidfile, STRLEN, "%s/.%s", Run.Env.home, MYPIDFILE);
                }
                Run.files.pidfile = Str_dup(pidfile);
        }
        /* Set the location of monit's id file */
        if (Run.files.id == NULL) {
                snprintf(buf, STRLEN, "%s/.%s", Run.Env.home, MYIDFILE);
                Run.files.id = Str_dup(buf);
        }
        file_monitId(Run.files.id);
        /* Set the location of monit's state file */
        if (Run.files.state == NULL) {
                snprintf(buf, STRLEN, "%s/.%s", Run.Env.home, MYSTATEFILE);
                Run.files.state = Str_dup(buf);
        }
}


void file_finalize(void) {
        Engine_cleanup();
        if (Run.files.pidfile_lock >= 0) {
                File_unlock(Run.files.pidfile_lock);
                Run.files.pidfile_lock = -1;
        }
        unlink(Run.files.pidfile);
}


char *file_findControlFile(void) {
        char *rcfile = CALLOC(sizeof(char), STRLEN + 1);
        snprintf(rcfile, STRLEN, "%s/.%s", Run.Env.home, MONITRC);
        if (File_exist(rcfile)) {
                return rcfile;
        }
        snprintf(rcfile, STRLEN, "/etc/%s", MONITRC);
        if (File_exist(rcfile)) {
                return rcfile;
        }
        snprintf(rcfile, STRLEN, "%s/%s", SYSCONFDIR, MONITRC);
        if (File_exist(rcfile)) {
                return rcfile;
        }
        snprintf(rcfile, STRLEN, "/usr/local/etc/%s", MONITRC);
        if (File_exist(rcfile)) {
                return rcfile;
        }
        if (File_exist(MONITRC)) {
                snprintf(rcfile, STRLEN, "%s/%s", Run.Env.cwd, MONITRC);
                return rcfile;
        }
        Log_error("Cannot find the Monit control file at ~/.%s, /etc/%s, %s/%s, /usr/local/etc/%s or at ./%s \n", MONITRC, MONITRC, SYSCONFDIR, MONITRC, MONITRC, MONITRC);
        exit(1);
}


bool file_createPidFile(void) {
        assert(Run.files.pidfile);
        /*
         * If we already hold a lock and the pidfile path hasn't changed,
         * there's nothing to do (happens during reinit with same config).
         */
        if (Run.files.pidfile_lock >= 0 && ! Run.files.pidfile_changed) {
                return true;
        }
        /*
         * If the pidfile path changed during reinit, release the old lock
         * before creating a new pidfile.
         */
        if (Run.files.pidfile_lock >= 0 && Run.files.pidfile_changed) {
                File_unlock(Run.files.pidfile_lock);
                Run.files.pidfile_lock = -1;
        }
        Run.files.pidfile_changed = false;
        /* Create the pidfile and write our PID */
        unlink(Run.files.pidfile);
        int fd = open(Run.files.pidfile, O_CREAT | O_EXCL | O_WRONLY | O_NOFOLLOW, 0644);
        if (fd < 0) {
                Log_error("Error opening pidfile '%s' for writing -- %s\n", Run.files.pidfile, STRERROR);
                return false;
        }
        FILE *F = fdopen(fd, "w");
        if (! F) {
                Log_error("Error opening pidfile '%s' for writing -- %s\n", Run.files.pidfile, STRERROR);
                close(fd);
                unlink(Run.files.pidfile);
                return false;
        }
        fprintf(F, "%d\n", (int)getpid());
        fclose(F);
        /* Acquire an exclusive lock on the pidfile */
        int lock = File_lock(Run.files.pidfile);
        if (lock < 0) {
                Log_error("Error acquiring lock on pidfile '%s' -- %s\n", Run.files.pidfile, STRERROR);
                unlink(Run.files.pidfile);
                return false;
        }
        Run.files.pidfile_lock = lock;
        return true;
}


pid_t file_getPid(const char *pidfile) {
        assert(pidfile);
        if (! File_exist(pidfile)) {
                DEBUG("pidfile '%s' does not exist\n", pidfile);
                return -1;
        }
        if (! File_isFile(pidfile)) {
                DEBUG("pidfile '%s' is not a regular file\n", pidfile);
                return -1;
        }
        FILE *file = fopen(pidfile, "r");
        if (file == NULL) {
                DEBUG("Error opening the pidfile '%s' -- %s\n", pidfile, STRERROR);
                return -1;
        }
        pid_t pid = -1;
        if (fscanf(file, "%d", &pid) != 1) {
                DEBUG("Error reading pid from file '%s'\n", pidfile);
        }
        if (fclose(file))
                DEBUG("Error closing file '%s' -- %s\n", pidfile, STRERROR);
        return pid;
}


char *file_monitId(char *idfile) {
        assert(idfile);
        FILE *file = NULL;
        if (! File_exist(idfile)) {
                // Generate the unique id
                file = fopen(idfile, "w");
                if (! file) {
                        Log_error("Error opening the idfile '%s' -- %s\n", idfile, STRERROR);
                        return NULL;
                }
                fprintf(file, "%s", Util_getToken(Run.id));
                Log_info(" New Monit id: %s\n Stored in '%s'\n", Run.id, idfile);
        } else {
                if (! File_isFile(idfile)) {
                        Log_error("idfile '%s' is not a regular file\n", idfile);
                        return NULL;
                }
                if ((file = fopen(idfile,"r")) == (FILE *)NULL) {
                        Log_error("Error opening the idfile '%s' -- %s\n", idfile, STRERROR);
                        return NULL;
                }
                if (fscanf(file, "%64s", Run.id) != 1) {
                        Log_error("Error reading id from file '%s'\n", idfile);
                        if (fclose(file))
                                Log_error("Error closing file '%s' -- %s\n", idfile, STRERROR);
                        return NULL;
                }
        }
        fflush(file);
        fsync(fileno(file));
        if (fclose(file))
                Log_error("Error closing file '%s' -- %s\n", idfile, STRERROR);

        return Run.id;
}


bool file_checkStat(const char *filename, const char *description, mode_t permmask) {
        assert(filename);
        assert(description);
        errno = 0;
        struct stat buf;
        if (stat(filename, &buf) < 0) {
                Log_error("Cannot stat the %s '%s' -- %s\n", description, filename, STRERROR);
                return false;
        }
        if (! S_ISREG(buf.st_mode)) {
                Log_error("The %s '%s' is not a regular file.\n", description,  filename);
                return false;
        }
        if (buf.st_uid != geteuid())  {
                Log_error("The %s '%s' must be owned by you.\n", description, filename);
                return false;
        }
        if ((buf.st_mode & 0777) & ~permmask) {
                Log_error("The %s '%s' permission 0%o is wrong, maximum 0%o allowed\n", description, filename, buf.st_mode & 0777, permmask & 0777);
                return false;
        }
        return true;
}


bool file_readProc(char *buf, int buf_size, const char *name, int pid, int *bytes_read) {
        assert(buf);
        assert(name);

        char filename[STRLEN];
        if (pid < 0)
                snprintf(filename, sizeof(filename), "/proc/%s", name);
        else
                snprintf(filename, sizeof(filename), "/proc/%d/%s", pid, name);

        int fd = open(filename, O_RDONLY);
        if (fd < 0) {
                if (Run.debug >= 2)
                        DEBUG("Cannot open proc file '%s' -- %s\n", filename, STRERROR);
                return false;
        }

        bool rv = false;
        int bytes = (int)read(fd, buf, buf_size - 1);
        if (bytes >= 0) {
                if (bytes_read)
                        *bytes_read = bytes;
                buf[bytes] = 0;
                rv = true;
        } else {
                *buf = 0;
                DEBUG("Cannot read proc file '%s' -- %s\n", filename, STRERROR);
        }

        if (close(fd) < 0)
                Log_error("Failed to close proc file '%s' -- %s\n", filename, STRERROR);

        return rv;
}

