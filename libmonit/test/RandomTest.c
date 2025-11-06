#include "Config.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdarg.h>
#include <limits.h>

#include "Bootstrap.h"
#include "Str.h"
#include "system/Random.h"
#include "system/Time.h"

/**
 * Ranom.c unity tests.
 */


int main(void) {

        Bootstrap(); // Need to initialize library

        printf("============> Start Random Tests\n\n");
        {
                //
                printf("\tnumber:   %llu\n", Random_number());
                //
                printf("\t1  byte:  ");
                char buf0[1];
                assert(Random_bytes(buf0, sizeof(buf0)));
                for (size_t i = 0; i < sizeof(buf0); i++) {
                        printf("%x", buf0[i]);
                }
                printf("\n");
                //
                printf("\t4  bytes: ");
                char buf1[4];
                assert(Random_bytes(buf1, sizeof(buf1)));
                for (size_t i = 0; i < sizeof(buf1); i++) {
                        printf("%x", buf1[i]);
                }
                printf("\n");
                //
                printf("\t16 bytes: ");
                char buf2[16];
                assert(Random_bytes(buf2, sizeof(buf2)));
                for (size_t i = 0; i < sizeof(buf2); i++) {
                        printf("%x", buf2[i]);
                }
                printf("\n");
                //
                assert(Random_number() != Random_number());
                
                // Random_bytes quality check: verify we don't get all identical bytes
                char buf_quality[100];
                assert(Random_bytes(buf_quality, sizeof(buf_quality)));
                int all_same = 1;
                for (size_t i = 1; i < sizeof(buf_quality); ++i) {
                        if (buf_quality[i] != buf_quality[0]) {
                                all_same = 0;
                                break;
                        }
                }
                assert(!all_same); // Extremely unlikely all 100 bytes are identical
                printf("\tRandom_bytes quality check: OK\n");
                
                // Random_range tests
                unsigned long long n = Random_range(1, 100);
                printf("\tRandom number in range [1..100]: %llu\n", n);
                assert(n >= 1 && n <= 100);
                TRY {
                        Random_range(1, 0);
                        printf("\tTest failed: did not get exception for min > max\n");
                        exit(1);
                }
                ELSE
                // OK
                END_TRY;
                
                // Edge case: min == max
                assert(Random_range(0, 0) == 0);
                assert(Random_range(1, 1) == 1);
                assert(Random_range(ULLONG_MAX, ULLONG_MAX) == ULLONG_MAX);
                printf("\tmin == max: OK\n");
                
                // Edge case: full range (should complete quickly, not infinite loop)
                long long start_time = Time_monotonic().seconds;
                for (int i = 0; i < 100; ++i) {
                        n = Random_range(0, ULLONG_MAX);
                        assert(n <= ULLONG_MAX);
                }
                long long elapsed = Time_monotonic().seconds - start_time;
                assert(elapsed < 2); // Should be nearly instant
                printf("\tFull range [0..ULLONG_MAX]: OK (completed in %lld seconds)\n", elapsed);
                
                // Edge case: power-of-2 boundaries (common bug sources)
                n = Random_range(0, 255);
                assert(n <= 255);
                n = Random_range(0, 65535);
                assert(n <= 65535);
                n = Random_range(0, 0xFFFFFFFF);
                assert(n <= 0xFFFFFFFF);
                printf("\tPower-of-2 boundaries: OK\n");
                
                // Edge case: ranges with non-zero min
                n = Random_range(ULLONG_MAX - 100, ULLONG_MAX);
                assert(n >= ULLONG_MAX - 100 && n <= ULLONG_MAX);
                n = Random_range(1000000, 1000010);
                assert(n >= 1000000 && n <= 1000010);
                printf("\tNon-zero min ranges: OK\n");
                
                // Edge case: two-value ranges (ensure both values are possible)
                int found0 = 0, found1 = 0;
                for (int i = 0; i < 1000; ++i) {
                        n = Random_range(0, 1);
                        assert(n == 0 || n == 1);
                        if (n == 0) found0 = 1;
                        if (n == 1) found1 = 1;
                        if (found0 && found1) break;
                }
                assert(found0 && found1);
                found0 = found1 = 0;
                for (int i = 0; i < 1000; ++i) {
                        n = Random_range(ULLONG_MAX-1, ULLONG_MAX);
                        assert(n == ULLONG_MAX-1 || n == ULLONG_MAX);
                        if (n == ULLONG_MAX-1) found0 = 1;
                        if (n == ULLONG_MAX) found1 = 1;
                        if (found0 && found1) break;
                }
                assert(found0 && found1);
                printf("\tTwo-value ranges reachability: OK\n");
                
                // Edge case: medium-sized range (verify all values are reachable)
                int hits[5] = {};
                for (int i = 0; i < 10000; ++i) {
                        n = Random_range(10, 14);
                        assert(n >= 10 && n <= 14);
                        hits[n-10] = 1;
                }
                for (int i = 0; i < 5; ++i) {
                        assert(hits[i]); // All values in range must be reachable
                }
                printf("\tMedium range all values reachable: OK\n");
                
                // Verify we get different values (not stuck on one value)
                unsigned long long first = Random_range(0, 100);
                int different = 0;
                for (int i = 0; i < 100; ++i) {
                        if (Random_range(0, 100) != first) {
                                different = 1;
                                break;
                        }
                }
                assert(different); // Extremely unlikely to get same value 100 times in a row
                printf("\tVariability check: OK\n");
                
                // Basic uniformity check for small range
                int distribution[10] = {};
                for (int i = 0; i < 10000; ++i) {
                        n = Random_range(0, 9);
                        assert(n <= 9);
                        distribution[n]++;
                }
                // Each value should appear at least once and roughly 1000 times (±700 is generous)
                for (int i = 0; i < 10; ++i) {
                        assert(distribution[i] > 0 && distribution[i] > 300 && distribution[i] < 1700);
                }
                printf("\tBasic uniformity check: OK\n");
        }
        printf("============> Random Tests: OK\n\n");

        return 0;
}
