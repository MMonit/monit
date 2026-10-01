#include "Config.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/resource.h>

#include "Bootstrap.h"
#include "Thread.h"
#include "system/Time.h"

/**
 * Thread.c unity tests.
 */


static _Atomic(bool) done;


static void *work(__attribute__ ((unused)) void *args) {
        atomic_store(&done, true);
        return NULL;
}


// Wait until work() has run and its Atomic Thread is no longer active
static void waitEnded(AtomicThread_T *T) {
        while (! atomic_load(&done) || AtomicThread_isActive(T))
                Time_usleep(1000);
}


int main(__attribute__ ((unused)) int argc, __attribute__ ((unused)) char **argv) {

        Bootstrap(); // Need to initialize library

        printf("============> Start Thread Tests\n\n");

        printf("=> Test1: an Atomic Thread is joinable until it is joined, also after it has ended\n");
        {
                AtomicThread_T T;
                AtomicThread_init(&T);
                atomic_store(&done, false);
                AtomicThread_create(&T, work, NULL);
                // Active from its creation until it has ended
                assert(AtomicThread_isActive(&T) || atomic_load(&done));
                waitEnded(&T);
                // No longer active, but only the join releases its resources
                assert(AtomicThread_isJoinable(&T));
                AtomicThread_join(&T);
                assert(! AtomicThread_isJoinable(&T));
                AtomicThread_join(&T); // Does nothing
                // Joined, it can be created again, and a join right away waits for it
                atomic_store(&done, false);
                AtomicThread_create(&T, work, NULL);
                AtomicThread_join(&T);
                assert(atomic_load(&done));
                assert(! AtomicThread_isJoinable(&T));
                AtomicThread_destroy(&T);
        }
        printf("=> Test1: OK\n\n");

        printf("=> Test2: an Atomic Thread that has not been joined is neither created again nor destroyed\n");
        {
                AtomicThread_T T;
                AtomicThread_init(&T);
                atomic_store(&done, false);
                AtomicThread_create(&T, work, NULL);
                waitEnded(&T);
                TRY
                {
                        AtomicThread_create(&T, work, NULL);
                        printf("AtomicThread_create() accepted an Atomic Thread that was not joined\n");
                        exit(1);
                }
                CATCH (AssertException)
                END_TRY;
                TRY
                {
                        AtomicThread_createDetached(&T, work, NULL);
                        printf("AtomicThread_createDetached() accepted an Atomic Thread that was not joined\n");
                        exit(1);
                }
                CATCH (AssertException)
                END_TRY;
                TRY
                {
                        AtomicThread_destroy(&T);
                        printf("AtomicThread_destroy() accepted an Atomic Thread that was not joined\n");
                        exit(1);
                }
                CATCH (AssertException)
                END_TRY;
                AtomicThread_join(&T);
                AtomicThread_destroy(&T);
        }
        printf("=> Test2: OK\n\n");

        printf("=> Test3: a failed create leaves the Atomic Thread as it was\n");
        {
#ifdef __linux__
                if (geteuid() == 0) {
                        printf("\tskipped: root is not held to RLIMIT_NPROC\n");
                } else {
                        AtomicThread_T T;
                        AtomicThread_init(&T);
                        struct rlimit saved;
                        assert(getrlimit(RLIMIT_NPROC, &saved) == 0);
                        // This process already counts against a limit of one, so pthread_create() fails
                        assert(setrlimit(RLIMIT_NPROC, &(struct rlimit){.rlim_cur = 1, .rlim_max = saved.rlim_max}) == 0);
                        TRY
                        {
                                AtomicThread_create(&T, work, NULL);
                                printf("AtomicThread_create() did not fail under RLIMIT_NPROC\n");
                                exit(1);
                        }
                        CATCH (AssertException)
                        END_TRY;
                        assert(! AtomicThread_isActive(&T));
                        assert(! AtomicThread_isJoinable(&T));
                        TRY
                        {
                                AtomicThread_createDetached(&T, work, NULL);
                                printf("AtomicThread_createDetached() did not fail under RLIMIT_NPROC\n");
                                exit(1);
                        }
                        CATCH (AssertException)
                        END_TRY;
                        assert(! AtomicThread_isActive(&T));
                        assert(setrlimit(RLIMIT_NPROC, &saved) == 0);
                        atomic_store(&done, false);
                        AtomicThread_create(&T, work, NULL);
                        AtomicThread_join(&T);
                        assert(atomic_load(&done));
                        AtomicThread_destroy(&T);
                }
#else
                printf("\tskipped: RLIMIT_NPROC limits threads only on Linux\n");
#endif
        }
        printf("=> Test3: OK\n\n");

        printf("============> Thread Tests: OK\n\n");

        return 0;
}
