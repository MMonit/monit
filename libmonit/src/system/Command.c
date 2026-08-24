/*
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
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


#include "Config.h"

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>

#ifdef HAVE_USERSEC_H
#include <usersec.h>
#endif

#include "Str.h"
#include "Dir.h"
#include "File.h"
#include "List.h"
#include "Array.h"
#include "system/Net.h"
#include "StringBuffer.h"

#include "system/System.h"
#include "system/Time.h"
#include "system/Command.h"


/**
 * Implementation of the Command and Process interfaces.
 *
 * @author https://tildeslash.com
 * @see https://mmonit.com
 * @file
 */



// MARK: - Definitions


#define T Command_T
struct T {
        uid_t uid;
        gid_t gid;
        List_T env;
        List_T args;
        mode_t umask;
        char *working_directory;
};

struct Process_T {
        pid_t pid;
        int status;
        bool isdetached;
        int ctrl_pipe[2];
        int stdin_pipe[2];
        int stdout_pipe[2];
        int stderr_pipe[2];
        InputStream_T in;
        InputStream_T err;
        OutputStream_T out;
};

struct _usergroups {
        gid_t gid;
        int ngroups;
        gid_t groups[NGROUPS_MAX];
};

struct _childspec {
        T C;
        char **args;
        char **env;
        char *home;
        int descriptors;
        struct _usergroups ug;
};

static Array_T processTable = NULL;

// Some POSIX systems does not define environ explicit
extern char **environ;

// Default umask for the sub-process
#define DEFAULT_UMASK 022


// MARK: - SIGCHLD handling

static inline void _setstatus(Process_T P, int status) {
        if (WIFEXITED(status))
                P->status = WEXITSTATUS(status);
        else if (WIFSIGNALED(status))
                P->status = WTERMSIG(status);
        else if (WIFSTOPPED(status))
                P->status = WSTOPSIG(status);
}


static void _childSignal(int how) {
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        pthread_sigmask(how, &mask, NULL);
}


// Signal handler for children exit. OS blocks SIGCHLD during this call
static void _handleChildren(__attribute__ ((unused)) int sig) {
        pid_t pid;
        int status;
        int save_errno = errno;

        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                Process_T found = Array_remove(processTable, pid);
                if (found) {
                        _setstatus(found, status);
                }
        }

        errno = save_errno;
}


static void __attribute__ ((constructor)) _constructor(void) {
        processTable = Array_new(20);

        struct sigaction act = {
                .sa_handler = _handleChildren,
                .sa_flags = SA_RESTART | SA_NODEFER
        };
        // Set up mask for blocking SIGCHLD during handler execution
        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, SIGCHLD);

        if (sigaction(SIGCHLD, &act, NULL)) {
                ERROR("Command: SIGCHLD handler failed: %s", System_lastError());
        }
}


static void __attribute__ ((destructor)) _destructor(void) {
        _childSignal(SIG_BLOCK);

        // No need to free the table entries - Process members are freed explicitly, just drop the table
        Array_free(&processTable);
}


// MARK: - Private methods

// Search the env list and return the pointer to the name (in the list)
// if found, otherwise NULL.
static inline char *_findEnv(T C, const char *name, size_t len) {
        assert(len > 0);
        for (_list_t p = C->env->head; p; p = p->next) {
                if ((strncmp(p->e, name, len) == 0))
                        if (((char*)p->e)[len] == '=') // Ensure that name is not just a sub-string
                                return p->e;
        }
        return NULL;
}


// Remove env identified by name
static inline void _removeEnv(T C, const char *name) {
        char *e = _findEnv(C, name, strlen(name));
        if (e) {
                List_remove(C->env, e);
                FREE(e);
        }
}


// Free each string in a list of strings
static void _freeElementsIn(List_T l) {
        while (List_length(l) > 0) {
                char *s = List_pop(l);
                FREE(s);
        }
}


// Build the Command args list. The list represent the array sent
// to execv and the List contains the following entries: args[0] is the
// path to the program, the rest are optional arguments to the program
static void _buildArgs(T C, const char *path, va_list ap) {
        List_append(C->args, Str_dup(path));
        va_list ap_copy;
        va_copy(ap_copy, ap);
        for (char *a = va_arg(ap_copy, char *); a; a = va_arg(ap_copy, char *))
                List_append(C->args, Str_dup(a));
        va_end(ap_copy);
}


static char **_buildChildEnvironment(T C, char *home) {
        if (List_length(C->env) == 0 && ! home)
                return NULL;
        int envc = 0;
        while (environ[envc])
                envc++;
        char **array = CALLOC(List_length(C->env) + envc + 2, sizeof *array); // +home +NULL
        int n = 0;
        // The resolved HOME takes precedence over any HOME set explicitly
        if (home)
                array[n++] = home;
        for (_list_t p = C->env->head; p; p = p->next) {
                if (home && strncmp(p->e, "HOME=", 5) == 0)
                        continue;
                array[n++] = p->e;
        }
        for (int i = 0; environ[i]; i++) {
                // Variable name length (before '='); guard against a malformed entry with no '='
                const char *eq = strchr(environ[i], '=');
                size_t len = eq ? (size_t)(eq - environ[i]) : 0;
                if (len > 0) {
                        if (home && strncmp(home, environ[i], len) == 0 && home[len] == '=')
                                continue;
                        if (_findEnv(C, environ[i], len))
                                continue;
                }
                array[n++] = environ[i];
        }
        array[n] = NULL;
        return array;
}


#ifndef HAVE_GETGROUPLIST
#ifdef AIX
static int getgrouplist(const char *name, int basegid, int *groups, int *ngroups) {
        int rv = -1;

        // Open the user database
        if (setuserdb(S_READ) != 0) {
                DEBUG("Cannot open user database -- %s\n", System_getError(errno));
                goto fail4;
        }

        // Get administrative domain for the user so we can lookup the group membership in the correct database (files, LDAP, etc).
        char *registry;
        if (getuserattr((char *)name, S_REGISTRY, &registry, SEC_CHAR) == 0 && setauthdb(registry, NULL) != 0) {
                DEBUG("Administrative domain switch to %s for user %s failed -- %s\n", registry, name, System_getError(errno));
                goto fail3;
        }

        // Get the list of groups for the named user
        char *groupList = getgrset(name);
        if (! groupList) {
                DEBUG("Cannot get groups for user %s\n", name);
                goto fail2;
        }

        // Add the base GID
        int count = 1;
        groups[0] = basegid;

        // Parse the comma separated list of groups
        char *lastGroup = NULL;
        for (char *currentGroup = strtok_r(groupList, ",", &lastGroup); currentGroup; currentGroup = strtok_r(NULL, ",", &lastGroup)) {
                gid_t gid = (gid_t)Str_parseInt(currentGroup);
                // Add the GID to the list (unless it's basegid, which we pushed to the beginning of groups list already)
                if (gid != basegid) {
                        if (count == *ngroups) {
                                // Maximum groups reached (error will be indicated by -1 return value, but we return as many groups as possible in the list)
                                goto fail1;
                        }
                        groups[count++] = gid;
                }
        }

        // Success
        rv = 0;
        *ngroups = count;

fail1:
        FREE(groupList);

fail2:
        // Restore the administrative domain
        setauthdb(NULL, NULL);

fail3:
        // Close the user database
        if (enduserdb() != 0) {
                DEBUG("Cannot close user database -- %s\n", System_getError(errno));
        }

fail4:
        return rv;
}
#else
#error "getgrouplist missing"
#endif
#endif


// Block all signals and make the current thread not cancellable
static struct _block {sigset_t sigmask; int threadstate;} _block(void) {
        sigset_t b;
        sigfillset(&b);
        struct _block block = {};
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &block.threadstate);
        pthread_sigmask(SIG_BLOCK, &b, &block.sigmask);
        return block;
}


// Un-Block signals and make the current thread cancellable again
static void _unblock(struct _block *block) {
        pthread_sigmask(SIG_SETMASK, &block->sigmask, NULL);
        pthread_setcancelstate(block->threadstate, 0);
}


// Reset all signals to default, except for SIGHUP and SIGPIPE which
// are set to SIG_IGN
static void _resetSignals(void) {
        sigset_t mask;
        sigemptyset(&mask);
        pthread_sigmask(SIG_SETMASK, &mask, 0);
        struct sigaction sa_default = {.sa_handler = SIG_DFL};
        struct sigaction sa_ignore = {.sa_handler = SIG_IGN};
        for (int i = 1; i < NSIG; ++i) {
                if (i == SIGKILL || i == SIGSTOP)
                        continue;
                if (i == SIGHUP || i == SIGPIPE)
                        sigaction(i, &sa_ignore, NULL);
                else
                        sigaction(i, &sa_default, NULL);
        }
}


// Resolve the target user's gid, supplementary groups and home; getpwuid_r()/getgrouplist() are not async-signal-safe
static bool _getUserGroups(T C, struct _usergroups *ug, char **home) {
        long hint = sysconf(_SC_GETPW_R_SIZE_MAX);
        size_t bufsize = (hint > 0 && hint <= 1024 * 1024) ? (size_t)hint : 16384;
        char *buffer = ALLOC(bufsize);
        struct passwd pwd;
        struct passwd *result = NULL;
        int rv;
        while ((rv = getpwuid_r(C->uid, &pwd, buffer, bufsize, &result)) == ERANGE && bufsize < 1024 * 1024) {
                bufsize *= 2;
                RESIZE(buffer, bufsize);
        }
        if (rv != 0 || ! result) {
                FREE(buffer);
                errno = rv;
                return false;
        }
        if (home)
                *home = Str_cat("HOME=%s", result->pw_dir ? result->pw_dir : "");
        // Explicit gid, else the user's primary group, so we never inherit root's gid
        ug->gid = C->gid ? C->gid : result->pw_gid;
        ug->ngroups = NGROUPS_MAX;
        int gl = getgrouplist(result->pw_name, ug->gid,
#ifdef __APPLE__
                         (int *)ug->groups,
#else
                         ug->groups,
#endif
                         &ug->ngroups);
        FREE(buffer);
        result = NULL; // points into buffer freed above
        if (gl < 0) {
                if (home) {
                        FREE(*home);
                        *home = NULL;
                }
                return false;
        }
        return true;
}


// MARK: - Process_T Private methods


static void Process_closeCtrlPipe(Process_T P) {
        // Close ctrl pipe ends if not already closed
        if (P->ctrl_pipe[1] >= 0) {
                close(P->ctrl_pipe[1]);
                P->ctrl_pipe[1] = -1;
        }
        if (P->ctrl_pipe[0] >= 0) {
                close(P->ctrl_pipe[0]);
                P->ctrl_pipe[0] = -1;
        }
}


static inline void _closeFd(int *fd) {
        if (*fd >= 0) {
                close(*fd);
                *fd = -1;
        }
}


// Close all stdio pipe descriptors still open in this process, except ctrl_pipe which is used during child setup before calling exec.
// On the success path Process_setupParentPipes() has already closed the child ends, so those are no-ops here. On any Command_execute() failure path the
// child ends were never closed and must be closed too.
static void Process_closePipes(Process_T P) {
        _closeFd(&P->stdin_pipe[0]);  // child read end
        _closeFd(&P->stdin_pipe[1]);  // parent write end
        _closeFd(&P->stdout_pipe[0]); // parent read end
        _closeFd(&P->stdout_pipe[1]); // child write end
        _closeFd(&P->stderr_pipe[0]); // parent read end
        _closeFd(&P->stderr_pipe[1]); // child write end
}


// Setup a controller pipe to be used between parent and child to
// report any errors during the setup phase or if execve fails
static int Process_createCtrlPipe(Process_T P) {
        int status = 0;
        // Not all POSIX systems have pipe2(), like macOS,
        if (pipe(P->ctrl_pipe) < 0) {
                status = -errno;
                DEBUG("Process_createCtrlPipe: ctrl pipe(2) failed -- %s\n", System_lastError());
                return status;
        }
        for (int i = 0; i < 2; i++) {
                if (fcntl(P->ctrl_pipe[i], F_SETFD, FD_CLOEXEC) < 0) {
                        status = -errno;
                        DEBUG("Process_createCtrlPipe: ctrl fcntl(2) FD_CLOEXEC failed -- %s\n", System_lastError());
                        return status;
                }
        }
        return 0;
}


// Create pipes for communication between parent and child process
static int Process_createPipes(Process_T P) {
        int status = Process_createCtrlPipe(P);
        if (status < 0)
                return status;
        if (pipe(P->stdin_pipe) < 0 || pipe(P->stdout_pipe) < 0 || pipe(P->stderr_pipe) < 0) {
                status = -errno;
                DEBUG("Process_createPipes: pipe(2) failed -- %s\n", System_lastError());
                Process_closePipes(P);
                return status;
        }
        return 0;
}


// Setup stdio pipes in subprocess. We need not close pipes as the child
// process will exit if this fails
static bool Process_setupChildPipes(Process_T P) {
        close(P->stdin_pipe[1]);   // close write end
        if (P->stdin_pipe[0] != STDIN_FILENO) {
                if (dup2(P->stdin_pipe[0],  STDIN_FILENO) != STDIN_FILENO)
                        return false;
        }
        close(P->stdout_pipe[0]);  // close read end
        if (P->stdout_pipe[1] != STDOUT_FILENO) {
                if (dup2(P->stdout_pipe[1], STDOUT_FILENO) != STDOUT_FILENO)
                        return false;
        }
        close(P->stderr_pipe[0]);  // close read end
        if (P->stderr_pipe[1] != STDERR_FILENO) {
                if (dup2(P->stderr_pipe[1], STDERR_FILENO) != STDERR_FILENO)
                        return false;
        }
        return true;
}


// Setup stdio pipes in parent process for communication with the subprocess
static void Process_setupParentPipes(Process_T P) {
        if (P->stdin_pipe[0] >= 0) {
                close(P->stdin_pipe[0]);  // close read end
                P->stdin_pipe[0] = -1;
        }
        if (P->stdout_pipe[1] >= 0) {
                close(P->stdout_pipe[1]); // close write end
                P->stdout_pipe[1] = -1;
        }
        if (P->stderr_pipe[1] >= 0) {
                close(P->stderr_pipe[1]); // close write end
                P->stderr_pipe[1] = -1;
        }
        Net_setNonBlocking(P->stdin_pipe[1]);
        Net_setNonBlocking(P->stdout_pipe[0]);
        Net_setNonBlocking(P->stderr_pipe[0]);
}


// Release stdio streams
static void Process_closeStreams(Process_T P) {
        if (P->in) InputStream_free(&P->in);
        if (P->err) InputStream_free(&P->err);
        if (P->out) OutputStream_free(&P->out);
}


static Process_T Process_new(void) {
        Process_T P;
        NEW(P);
        P->pid = -1;
        P->status = -1;
        P->ctrl_pipe[0] = P->ctrl_pipe[1] = -1;
        P->stdin_pipe[0] = P->stdin_pipe[1] = -1;
        P->stdout_pipe[0] = P->stdout_pipe[1] = -1;
        P->stderr_pipe[0] = P->stderr_pipe[1] = -1;
        return P;
}


// MARK: - Process_T Public methods


void Process_free(Process_T *P) {
        assert(P && *P);

        _childSignal(SIG_BLOCK);
        if (Array_get(processTable , (*P)->pid) == (*P)) {
                Array_remove(processTable, (*P)->pid);
        }
        _childSignal(SIG_UNBLOCK);

        if (!(*P)->isdetached) {
                if (Process_isRunning(*P)) {
                        Process_kill(*P);
                        // Trust SIGCHLD handler to wait for P's process
                }
                Process_detach(*P);
        }
        FREE(*P);
}


// Close pipes and streams to the sub-process. Because we ignored SIGPIPE when
// creating the sub-process it should not recieve SIGPIPE if it tries to write
// to one of its (now broken) output pipes. A proper daemon process will also
// normally redirect stdio to /dev/null and instead write to a log file after
// its initial setup phase
void Process_detach(Process_T P) {
        assert(P);
        if (!P->isdetached) {
                P->isdetached = true;
                Process_closeStreams(P);
                Process_closePipes(P);
        }
}


bool Process_isdetached(Process_T P) {
        assert(P);
        return P->isdetached;
}


pid_t Process_pid(Process_T P) {
        assert(P);
        return P->pid;
}


int Process_waitFor(Process_T P) {
        assert(P);
        _childSignal(SIG_BLOCK);
        if (P->status < 0) {
                Process_T current = Array_get(processTable, P->pid);
                if (current == P) {
                        int r, status;
                        do
                                r = waitpid(P->pid, &status, 0);
                        while (r == -1 && errno == EINTR);

                        if (r == P->pid) {
                                _setstatus(P, status);
                                if (Array_get(processTable, P->pid) == P) {
                                        Array_remove(processTable, P->pid);
                                }
                        }
                } else if (current) {
                        // Another Process with same PID is in the array - don't touch it
                        DEBUG("Process_waitFor: Different Process with pid %d found in Array", P->pid);
                } else {
                        // P is not in the array - could have been handled by SIGCHLD already
                        DEBUG("Process_waitFor: Process with pid %d not found in Array", P->pid);
                }
        }
        _childSignal(SIG_UNBLOCK);
        return P->status;
}


int Process_exitStatus(Process_T P) {
        assert(P);
        return P->status; // Trust SIGCHLD handler to set status
}


bool Process_isRunning(Process_T P) {
        assert(P);
        return (P->pid > 0 && Process_exitStatus(P) < 0);
}


OutputStream_T Process_outputStream(Process_T P) {
        assert(P);
        if (P->isdetached)
                return NULL;
        if (! P->out)
                P->out = OutputStream_new(P->stdin_pipe[1]);
        return P->out;
}


InputStream_T Process_inputStream(Process_T P) {
        assert(P);
        if (P->isdetached)
                return NULL;
        if (! P->in)
                P->in = InputStream_new(P->stdout_pipe[0]);
        return P->in;
}


InputStream_T Process_errorStream(Process_T P) {
        assert(P);
        if (P->isdetached)
                return NULL;
        if (! P->err)
                P->err = InputStream_new(P->stderr_pipe[0]);
        return P->err;
}


bool Process_terminate(Process_T P) {
        assert(P);
        if (P->pid <= 0)
                return false;
        return (kill(P->pid, SIGTERM) == 0);
}


bool Process_kill(Process_T P) {
        assert(P);
        if (P->pid <= 0)
                return false;
        return (kill(P->pid, SIGKILL) == 0);
}


// MARK: - Public methods


T _Command_new(const char *path, ...) {
        T C;
        assert(path);
        if (! File_exist(path))
                THROW(AssertException, "File '%s' does not exist", path);
        NEW(C);
        C->env = List_new();
        C->args = List_new();
        C->umask = DEFAULT_UMASK;
        va_list ap;
        va_start(ap, path);
        _buildArgs(C, path, ap);
        va_end(ap);
        return C;
}


void Command_free(T *C) {
        assert(C && *C);
        _freeElementsIn((*C)->args);
        List_free(&(*C)->args);
        _freeElementsIn((*C)->env);
        List_free(&(*C)->env);
        FREE((*C)->working_directory);
        FREE(*C);
}


void Command_appendArgument(T C, const char *argument) {
        assert(C);
        if (argument)
                List_append(C->args, Str_dup(argument));
}


void Command_setUid(T C, uid_t uid) {
        assert(C);
        if (getuid() != 0)
                THROW(AssertException, "Only the super user can switch uid");
        C->uid = uid;
}


uid_t Command_uid(T C) {
        assert(C);
        return C->uid;
}


void Command_setGid(T C, gid_t gid) {
        assert(C);
        if (getuid() != 0)
                THROW(AssertException, "Only the super user can switch gid");
        C->gid = gid;
}


gid_t Command_gid(T C) {
        assert(C);
        return C->gid;
}


void Command_setUmask(T C, mode_t umask) {
        assert(C);
        C->umask = umask;
}


mode_t Command_umask(T C) {
        assert(C);
        return C->umask;
}


// Set the sub-process working directory. If NULL (the default) the sub-process
// will inherit the calling process's current directory
void Command_setDir(T C, const char *dir) {
        assert(C);
        if (dir) {
                if (! File_isDirectory(dir))
                        THROW(AssertException, "The new working directory '%s' is not a directory", dir);
                if (! File_isExecutable(dir))
                        THROW(AssertException, "The new working directory '%s' is not accessible", dir);
        }
        FREE(C->working_directory);
        C->working_directory = File_removeTrailingSeparator(Str_dup(dir));
}


const char *Command_dir(T C) {
        assert(C);
        return C->working_directory;
}


// Env variables are stored in the environment list as "name=value" strings
void Command_setEnv(Command_T C, const char *name, const char *value) {
        assert(C);
        assert(name);
        _removeEnv(C, name);
        List_append(C->env, Str_cat("%s=%s", name, value ? value : ""));
}


// Env variables are stored in the environment list as "name=value" strings
void Command_vSetEnv(T C, const char *name, const char *value, ...) {
        assert(C);
        assert(name);
        _removeEnv(C, name);
        char *t = NULL;
        if (STR_DEF(value)) {
                va_list ap;
                va_start(ap, value);
                t = Str_vcat(value, ap);
                va_end(ap);
        }
        List_append(C->env, Str_cat("%s=%s", name, t?t:""));
        FREE(t);
}


// Returns the value part from a "name=value" environment string
const char *Command_env(T C, const char *name) {
        assert(C);
        assert(name);
        size_t len = strlen(name);
        char *e = _findEnv(C, name, len);
        if (e)
                return e + len + 1;
        return NULL;
}


List_T Command_command(T C) {
        assert(C);
        return C->args;
}


// MARK: - Execute


static int _createChildSpec(T C, struct _childspec *spec) {
        if (C->uid && ! _getUserGroups(C, &spec->ug, &spec->home))
                return errno ? errno : EINVAL;
        spec->C = C;
        spec->descriptors = System_descriptors(256);
        spec->args = (char**)List_toArray(C->args);
        spec->env = _buildChildEnvironment(C, spec->home);
        return 0;
}


static void _disposeChildSpec(struct _childspec *spec) {
        FREE(spec->args);
        FREE(spec->env);
        FREE(spec->home);
}


static void Process_exec(Process_T P, const struct _childspec *spec) {
        T C = spec->C;
        int status = 0;
        _resetSignals();
        errno = 0;
        if (C->working_directory) {
                if (! Dir_chdir(C->working_directory))
                        goto fail;
        }
        if (setsid() < 0)
                goto fail;
        if (!Process_setupChildPipes(P))
                goto fail;
        for (int i = 3; i < spec->descriptors; i++) {
                if (i != P->ctrl_pipe[1])
                        close(i);
        }
        if (C->uid || C->gid) {
                gid_t gid = C->gid;
                if (C->uid) {
                        gid = spec->ug.gid;
                        if (setgroups(spec->ug.ngroups, spec->ug.groups) < 0)
                                goto fail;
                } else {
                        if (setgroups(1, &gid) < 0)
                                goto fail;
                }
                if (setgid(gid) < 0)
                        goto fail;
                if (getgid() != gid) {
                        errno = EPERM;
                        goto fail;
                }
                if (C->uid) {
                        if (setuid(C->uid) < 0)
                                goto fail;
                        if (getuid() != C->uid) {
                                errno = EPERM;
                                goto fail;
                        }
                }
        }
        umask(C->umask);
        execve(spec->args[0], spec->args, spec->env ? spec->env : environ);
fail:
        status = errno;
        if (status != 0)
                while (write(P->ctrl_pipe[1], &status, sizeof status) < 0);
        _exit(127);
}


// If the child process succeeded in calling execve, status is 0
static void Process_ctrl(Process_T P, int *status) {
        close(P->ctrl_pipe[1]);
        P->ctrl_pipe[1] = -1;
        if (read(P->ctrl_pipe[0], status, sizeof *status) != sizeof *status) {
                *status = 0;
        } else {
                // The child reported a setup/exec error and we reap it here
                waitpid(P->pid, &(int){0}, 0);
                P->pid = -1;
        }
}


/*
 The Execute function. We do not use posix_spawn(2) because it's not well
 suited for creating long-running daemon processes. Although posix_spawn
 is more efficient, its limitations makes it problematic for our use.
 */
Process_T Command_execute(T C) {
        assert(C);
        struct _block block = _block();
        Process_T P = Process_new();
        struct _childspec spec = {.ug = {.ngroups = NGROUPS_MAX}};
        int status = Process_createPipes(P);
        if (status < 0) {
                status = -status;
                goto fail;
        }
        if ((status = _createChildSpec(C, &spec)) != 0)
                goto fail;
        if ((P->pid = fork()) < 0) {
                status = errno;
        } else if (P->pid == 0) {
                Process_exec(P, &spec);
        } else {
                Process_ctrl(P, &status);
        }
fail:
        _disposeChildSpec(&spec);
        Process_closeCtrlPipe(P);
        if (status != 0) {
                DEBUG("Command: failed -- %s\n", System_getError(status));
                Process_free(&P);
        } else {
                // Add Process to processTable indexed by PID. This array is used by the SIGCHLD
                // handler to find the Process object and update its status
                Process_T previous = Array_put(processTable, P->pid, P);
                if (previous)
                        ERROR("Command: process with duplicate PID %d found in internal array\n", P->pid);
                Process_setupParentPipes(P);
        }
        _unblock(&block);
        errno = status;
        return P;
}
