#include "Config.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "Bootstrap.h"
#include "Str.h"
#include "system/Time.h"
#include "Thread.h"
#include "system/Net.h"
#include "File.h"

/**
 * Net.c unity tests.
 */


static void onSignal(__attribute__ ((unused)) int sig) {
}


// As Monit does at a stop request: a deadline that has passed
static void onStop(__attribute__ ((unused)) int sig) {
        Net_setDeadline(Time_stamp());
}


static void *doSignal(void *args) {
        for (int i = 0; i < 15; i++) {
                Time_usleep(20000);
                pthread_kill(*(Thread_T *)args, SIGALRM);
        }
        return NULL;
}


int main(__attribute__ ((unused)) int argc, __attribute__ ((unused)) char **argv) {

        Bootstrap(); // Need to initialize library

        printf("============> Start Net Tests\n\n");

        printf("=> Test1: a signal neither restarts Net_read's timeout nor shows in errno\n");
        {
                // SIGALRM every 20 ms for 300 ms. Restarted, the 200 ms wait
                // would end at 500 ms. EAGAIN tells the caller it timed out
                Thread_T T;
                int sv[2];
                char c;
                Thread_T self = Thread_self();
                struct sigaction act = {.sa_handler = onSignal}, old;
                sigemptyset(&act.sa_mask);
                assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);
                assert(Net_setNonBlocking(sv[0]));
                assert(sigaction(SIGALRM, &act, &old) == 0);
                Thread_create(T, doSignal, &self);
                long long start = Time_monotonic().milliseconds;
                assert(Net_read(sv[0], &c, 1, 200) == 0);
                assert(errno == EAGAIN || errno == EWOULDBLOCK);
                long long elapsed = Time_monotonic().milliseconds - start;
                assert(elapsed >= 190 && elapsed < 400);
                Thread_join(T);
                assert(sigaction(SIGALRM, &old, NULL) == 0);
                assert(Net_close(sv[0]) && Net_close(sv[1]));
        }
        printf("=> Test1: OK\n\n");

        printf("=> Test2: Net_setDeadline ends a wait at the deadline, also one a signal sets during the wait\n");
        {
                Thread_T T;
                int sv[2];
                char c;
                Thread_T self = Thread_self();
                struct sigaction act = {.sa_handler = onStop}, old;
                sigemptyset(&act.sa_mask);
                assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);
                assert(Net_setNonBlocking(sv[0]));
                assert(sigaction(SIGALRM, &act, &old) == 0);
                long long start = Time_monotonic().milliseconds;
                Net_setDeadline(Time_stamp() + 200);
                assert(Net_read(sv[0], &c, 1, 2000) == 0);
                assert(errno == EAGAIN || errno == EWOULDBLOCK);
                long long elapsed = Time_monotonic().milliseconds - start;
                assert(elapsed >= 190 && elapsed < 1000);
                // A later deadline does not extend the wait
                start = Time_monotonic().milliseconds;
                Net_setDeadline(Time_stamp() + 2000);
                assert(Net_read(sv[0], &c, 1, 200) == 0);
                elapsed = Time_monotonic().milliseconds - start;
                assert(elapsed >= 190 && elapsed < 1000);
                Net_setDeadline(0);
                Thread_create(T, doSignal, &self);
                start = Time_monotonic().milliseconds;
                assert(Net_read(sv[0], &c, 1, 2000) == 0);
                assert(errno == EAGAIN || errno == EWOULDBLOCK);
                assert(Time_monotonic().milliseconds - start < 1000);
                Thread_join(T);
                start = Time_monotonic().milliseconds;
                assert(Net_read(sv[0], &c, 1, 2000) == 0);
                assert(Time_monotonic().milliseconds - start < 1000);
                Net_setDeadline(0);
                assert(sigaction(SIGALRM, &old, NULL) == 0);
                assert(Net_close(sv[0]) && Net_close(sv[1]));
        }
        printf("=> Test2: OK\n\n");

        printf("============> Net Tests: OK\n\n");

        return 0;
}

