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
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <stdarg.h>
#include <sys/uio.h>
#include <sys/stat.h>
#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

#include "system/Net.h"
#include "system/System.h"
#include "system/Time.h"
#include "util/Num.h"


/**
 * Implementation of the Net Facade for Unix Systems.
 *
 * @author https://www.tildeslash.com/
 * @see https://mmonit.com/
 * @file
 */


/* --------------------------------------------------------------- Private */


static _Atomic uint32_t _deadline; // A Time_stamp(), or 0. Set from signal handlers and read by every thread


// Milliseconds left until the deadline, or until the one from
// Net_setDeadline() if that is earlier
static int _timeLeft(long long deadline) {
        long long now = Time_monotonic().milliseconds;
        uint32_t bound = _deadline;
        if (bound)
                deadline = Num_min(deadline, now + (int32_t)(bound - Time_stamp()));
        return (int)Num_clamp(deadline - now, 0, INT_MAX);
}


// poll() takes an int, and waits for ever on a negative timeout. A signal
// does not restart the wait: poll() again for the time left
static int _poll(struct pollfd *fd, time_t milliseconds) {
        int r, error = errno;
        long long deadline = Time_monotonic().milliseconds + Num_clamp(milliseconds, 0, INT_MAX);
        while ((r = poll(fd, 1, _timeLeft(deadline))) == -1 && errno == EINTR)
                errno = error; // Callers read errno after a timeout
        return r;
}


/* ---------------------------------------------------------------- Public */


bool Net_setNonBlocking(int socket) {
        return (fcntl(socket, F_SETFL, fcntl(socket, F_GETFL, 0) | O_NONBLOCK) != -1);
}


bool Net_setBlocking(int socket) {
        return (fcntl(socket, F_SETFL, fcntl(socket, F_GETFL, 0) & ~O_NONBLOCK) != -1);
}


bool Net_canRead(int socket, time_t milliseconds) {
        struct pollfd fds[1];
        fds[0].fd = socket;
        fds[0].events = POLLIN;
        return (_poll(fds, milliseconds) > 0);
}


bool Net_canWrite(int socket, time_t milliseconds) {
        struct pollfd fds[1];
        fds[0].fd = socket;
        fds[0].events = POLLOUT;
        return (_poll(fds, milliseconds) > 0);
}


void Net_setDeadline(uint32_t stamp) {
        _deadline = stamp;
}


ssize_t Net_read(int socket, void *buffer, size_t size, time_t timeout) {
	ssize_t n = 0;
        if (size > 0) {
                do {
                        n = read(socket, buffer, size);
                } while (n == -1 && errno == EINTR);
                if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                        if ((timeout == 0) || (Net_canRead(socket, timeout) == false)) {
                                return 0;
                        }
                        do {
                                n = read(socket, buffer, size);
                        } while (n == -1 && errno == EINTR);
                }
        }
	return n;
}


ssize_t Net_write(int socket, const void *buffer, size_t size, time_t timeout) {
	ssize_t n = 0;
        if (size > 0) {
                do {
                        n = write(socket, buffer, size);
                } while (n == -1 && errno == EINTR);
                if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                        if ((timeout == 0) || (Net_canWrite(socket, timeout) == false)) {
                                return 0;
                        }
                        do {
                                n = write(socket, buffer, size);
                        } while (n == -1 && errno == EINTR);
                }
        }
	return n;
}


bool Net_shutdown(int socket, int how) {
        return (shutdown(socket, how) == 0);
}


bool Net_close(int socket) {
	int r = 0;
        do {
                r = close(socket);
        } while (r == -1 && errno == EINTR);
	return (r == 0);
}


bool Net_abort(int socket) {
   	int r;
        struct linger linger = {1, 0};
        if (setsockopt(socket, SOL_SOCKET, SO_LINGER, &linger, sizeof linger) < 0) {
                ERROR("Net: setsockopt failed -- %s\n", System_lastError());
        }
        do {
                r = close(socket);
        } while (r == -1 && errno == EINTR);
	return (r == 0);
}

