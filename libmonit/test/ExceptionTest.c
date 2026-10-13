#include "Config.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "Bootstrap.h"
#include "Str.h"
#include "Thread.h"
#include "Exception.h"
#include "AssertException.h"


/**
 * Exception unity tests
 */

#define THREADS 200

Exception_T A = {"AException"};
Exception_T B = {"BException"};
Exception_T C = {"CException"};
Exception_T D = {"DException"};

static void throwA(void) {
        THROW(A, NULL);
}

static void throwB(void) {
        THROW(B, NULL);
}

static void throwC(void) {
        THROW(C, "A cause");
}

static void throwD(void) {
        THROW(D, "A cause");
}

static void indirectA(void) {
        throwA();
}

/* Throw and catch exceptions and check that we got the expected exception.
 * If the exception stack is corrupt somehow this should be detected
 */
static void *thread(__attribute__ ((unused)) void *args) {
        TRY
                THROW(A, "A cause");
                assert(false); // Should not be reached
        CATCH(A)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(B, "A cause");
                assert(false); // Should not be reached
        CATCH(B)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(D, NULL);
                assert(false); // Should not be reached
        CATCH(D)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(C, NULL);
                assert(false); // Should not be reached
        CATCH(C)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(A, NULL);
                assert(false); // Should not be reached
        CATCH(A)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                throwA();
                throwB();
                throwC();
                throwD();
        CATCH(A)
                // Ok
        CATCH(B)
                assert(false); // Should not be reached
        CATCH(C)
                assert(false); // Should not be reached
        CATCH(D)
                assert(false); // Should not be reached
        END_TRY;
        TRY
                indirectA();
        CATCH(A)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(B, NULL);
                assert(false); // Should not be reached
        CATCH(B)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(B, NULL);
                assert(false); // Should not be reached
        CATCH(B)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(B, NULL);
                assert(false); // Should not be reached
        CATCH(B)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(B, "A cause %s", "and another cause");
                assert(false); // Should not be reached
        CATCH(B)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(A, NULL);
                assert(false); // Should not be reached
        CATCH(A)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(B, NULL);
                assert(false); // Should not be reached
        CATCH(B)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(D, NULL);
                assert(false); // Should not be reached
        CATCH(D)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                RETURN (NULL);
                assert(false); // Should not be reached
        CATCH(A)
                assert(false); // Should not be reached
        END_TRY;
        return NULL;
}

static Mutex_T stepMutex = PTHREAD_MUTEX_INITIALIZER;
static Sem_T stepSem = PTHREAD_COND_INITIALIZER;
static int step = 0;
static bool caughtB = false;

static void waitFor(int n) {
        LOCK(stepMutex)
        {
                while (step < n)
                        Sem_wait(stepSem, stepMutex);
        }
        END_LOCK;
}

static void advanceTo(int n) {
        LOCK(stepMutex)
        {
                step = n;
                Sem_broadcast(stepSem);
        }
        END_LOCK;
}

// A leaves its TRY while B is inside its own, then B throws
static void *threadA(__attribute__ ((unused)) void *args) {
        TRY
                advanceTo(1);
                waitFor(2);
        END_TRY;
        advanceTo(3);
        return NULL;
}

static void *threadB(__attribute__ ((unused)) void *args) {
        waitFor(1);
        TRY
                advanceTo(2);
                waitFor(3);
                THROW(B, "Thrown in thread B");
        CATCH(B)
                caughtB = true;
        END_TRY;
        return NULL;
}

static int fails(void) {
        THROW(A, "A cause");
        return 0;
}

static int returnThrows(void) {
        TRY
                RETURN fails();
        CATCH(A)
                return -1;
        END_TRY;
        return 0;
}

static bool finallyRan = false;

static int returnThrowsFinally(void) {
        TRY
                RETURN fails();
        FINALLY
                finallyRan = true;
        END_TRY;
        return 0;
}

static int returnNested(void) {
        TRY
                TRY
                        return 7;
                END_TRY;
        END_TRY;
        return 0;
}

static int breakOut(void) {
        volatile int n = 0;
        for (int i = 0; i < 3; i++) {
                TRY
                        n++;
                        break;
                END_TRY;
        }
        return n;
}


int main(void) {
        Bootstrap(); // Need to initialize library
        printf("============> Start Exception Tests\n\n");

        printf("=> Test1: TRY-CATCH\n");
        {
                TRY
                        THROW(A, "A cause");
                        assert(false); // Should not be reached
                CATCH(A)
                        printf("\tResult: Ok\n");
                END_TRY;
        }
        printf("=> Test1: OK\n\n");

        printf("=> Test2: TRY-CATCH indirect throw\n");
        {
                TRY
                        indirectA();
                        assert(false); // Should not be reached
                CATCH(A)
                        printf("\tResult: Ok\n");
                END_TRY;
        }
        printf("=> Test2: OK\n\n");

        printf("=> Test3: TRY-ELSE\n");
        {
                TRY
                        THROW(B, NULL);
                        assert(false); // Should not be reached
                ELSE
                        printf("\tResult: Ok\n");
                END_TRY;
        }
        printf("=> Test3: OK\n\n");

        printf("=> Test4: TRY-CATCH-ELSE\n");
        {
                TRY
                        throwB();
                        assert(false); // Should not be reached
                CATCH(A)
                        assert(false); // Should not be reached
                ELSE
                        printf("\tResult: Ok\n");
                END_TRY;
        }
        printf("=> Test4: OK\n\n");

        printf("=> Test5: TRY-FINALLY\n");
        {
                volatile int i = 0;
                TRY
                        i++;
                FINALLY
                        printf("\tResult: ok\n");
                        assert(i==1);
                END_TRY;
        }
        printf("=> Test5: OK\n\n");

        printf("=> Test6: TRY-CATCH-FINALLY\n");
        {
                volatile int i = 0;
                TRY
                        i++;
                        THROW(C, "A cause");
                CATCH(C)
                        i++;
                FINALLY
                        printf("\tResult: ok\n");
                        assert(i == 2);
                END_TRY;
        }
        printf("=> Test6: OK\n\n");

        printf("=> Test7: CATCH NumberFormatException\n");
        {
                TRY
                        Str_parseInt("not a number");
                        assert(false); // Should not be reached
                CATCH(NumberFormatException)
                        printf("\tResult: ok got NumberFormatException\n");
                END_TRY;
        }
        printf("=> Test7: OK\n\n");

        printf("=> Test8: CATCH AssertException\n");
        {
                TRY
                        assert(false); // Throws AssertException
                        printf("Test8 failed\n");
                        exit(1);
                CATCH(AssertException)
                        printf("\tResult: ok got AssertException\n");
                END_TRY;
        }
        printf("=> Test8: OK\n\n");

        printf("=> Test9: Nested TRY-CATCH\n");
        {
                TRY
                        TRY
                                THROW(B, "A cause");
                                THROW(A, "Another cause");
                                assert(false); // Should not be reached
                        CATCH(A)
                                assert(false); // Should not be reached
                        END_TRY;
                CATCH(B)
                        printf("\tResult: ok\n");
                END_TRY;
        }
        printf("=> Test9: OK\n\n");

        printf("=> Test10: RETHROW\n");
        {
                TRY
                        TRY
                                THROW(A, "A cause %s", "and a cause");
                                assert(false); // Should not be reached
                        CATCH(A)
                                RETHROW;
                        END_TRY;
                CATCH(A)
                        printf("\tResult: ok got Exception\n");
                END_TRY;
        }
        printf("=> Test10: OK\n\n");

        printf("=> Test11: No exception thrown\n");
        {
                TRY
                        __attribute__ ((unused)) int i = 0; i++;
                ELSE
                        assert(false); // Should not be reached
                END_TRY;
        }
        printf("=> Test11: OK\n\n");

        printf("=> Test12: Exception message append\n");
        {
                TRY
                        TRY
                                THROW(AssertException, "Original reason");
                                exit(0); // Should not be reached
                        CATCH(AssertException)
                                THROW(AssertException, "%s - Appended reason", Exception_frame.message);
                        END_TRY;
                CATCH(AssertException)
                        printf("\tResult: Got '%s' with reason: %s\n", AssertException.name, Exception_frame.message);
                        assert(Str_startsWith(Exception_frame.message, "Original"));
                END_TRY;
        }
        printf("=> Test12: OK\n\n");


        printf("=> Test13: FINALLY rethrows exception\n");
        {
                TRY
                {
                        TRY
                        {
                                THROW(AssertException, "A reason");
                                exit(0); // Should not be reached
                        }
                        FINALLY
                        END_TRY;
                        exit(0); // Should not be reached
                }
                CATCH(AssertException)
                {
                        printf("\tResult: Got '%s' thrown from FINALLY\n", AssertException.name);
                }
                END_TRY;
        }
        printf("=> Test13: OK\n\n");


        printf("=> Test14: Test thread-safeness\n");
        {
                int i;
                Thread_T threads[THREADS];
                for (i = 0; i < THREADS; i++) {
                        Thread_create(threads[i], thread, NULL);
                }
                for (i = 0; i<THREADS; i++)
                        Thread_join(threads[i]);
        }
        printf("=> Test14: OK\n\n");

        printf("=> Test15: 2nd level FINALLY rethrows exception from nested 1st level TRY with '%%n' characters in vararg string argument\n");
        {
                TRY
                {
                        TRY
                        {
                                TRY
                                {
                                        // Trigger exception
                                        printf("\t1st level TRY: throw exception with string argument 'hello%%nelly'\n");
                                        THROW(AssertException, "1st level TRY: %s", "hello%nelly");
                                }
                                END_TRY;
                        }
                        FINALLY
                        {
                                printf("\t2nd level TRY: FINALLY will rethrow the 1st level exception '%s'\n", Exception_frame.message);
                                // The FINALLY will RETHROW the exception, which leads to Exception_throw()
                        }
                        END_TRY;
                }
                ELSE
                {
                        // Catch the exception
                        printf("\t3nd level TRY: catch the 1st level exception '%s'\n", Exception_frame.message);
                        assert(Str_isEqual(Exception_frame.message, "1st level TRY: hello%nelly"));
                }
                END_TRY;
        }
        printf("=> Test15: OK\n\n");

        printf("=> Test16: 2nd level FINALLY rethrows exception from nested 1st level TRY with '%%s' characters in vararg string argument\n");
        {
                TRY
                {
                        TRY
                        {
                                TRY
                                {
                                        // Trigger exception
                                        printf("\t1st level TRY: throw exception with string argument 'hello%%sally'\n");
                                        THROW(AssertException, "1st level TRY: %s", "hello%sally");
                                }
                                END_TRY;
                        }
                        FINALLY
                        {
                                printf("\t2nd level TRY: FINALLY will rethrow the 1st level exception '%s'\n", Exception_frame.message);
                                // The FINALLY will RETHROW the exception, which leads to Exception_throw()
                        }
                        END_TRY;
                }
                ELSE
                {
                        // Catch the exception
                        printf("\t3nd level TRY: catch the 1st level exception '%s'\n", Exception_frame.message);
                        assert(Str_isEqual(Exception_frame.message, "1st level TRY: hello%sally"));
                }
                END_TRY;
        }
        printf("=> Test16: OK\n\n");

        printf("=> Test17: Frames of two threads that do not nest\n");
        {
                Thread_T a, b;
                Thread_create(a, threadA, NULL);
                Thread_create(b, threadB, NULL);
                Thread_join(a);
                Thread_join(b);
                assert(caughtB);
        }
        printf("=> Test17: OK\n\n");

        printf("=> Test18: A throw in RETURN's expression reaches its own CATCH\n");
        {
                assert(returnThrows() == -1);
        }
        printf("=> Test18: OK\n\n");

        printf("=> Test19: A throw in RETURN's expression reaches its own FINALLY, then the caller\n");
        {
                TRY
                        returnThrowsFinally();
                        assert(false); // Should not be reached
                CATCH(A)
                        assert(finallyRan);
                END_TRY;
        }
        printf("=> Test19: OK\n\n");

        printf("=> Test20: return from nested TRY statements pops both frames\n");
        {
                volatile bool popped = false;
                TRY
                        assert(returnNested() == 7);
                        popped = Exception_Stack == &Exception_frame;
                END_TRY;
                assert(popped);
        }
        printf("=> Test20: OK\n\n");

        printf("=> Test21: break leaves the TRY statement, not the loop, and pops its frame\n");
        {
                volatile bool popped = false;
                TRY
                        assert(breakOut() == 3);
                        popped = Exception_Stack == &Exception_frame;
                END_TRY;
                assert(popped);
        }
        printf("=> Test21: OK\n\n");

        printf("=> Test22: A throw out of a nested TRY leaves the outer frame popped\n");
        {
                // Fails if the jump ran the inner frame's cleanup on its way out
                TRY
                        TRY
                                THROW(A, "A cause");
                        END_TRY;
                CATCH(A)
                        assert(Exception_Stack == Exception_frame.prev);
                END_TRY;
        }
        printf("=> Test22: OK\n\n");

        printf("============> Exception Tests: OK\n\n");

        return 0;
}

