#include "Config.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "Bootstrap.h"
#include "Num.h"


/**
 * Int.c unity tests
 */


int main(void) {

        Bootstrap(); // Need to initialize library

        printf("============> Start Num Tests\n\n");

        printf("=> Test1: Num_min\n");
        {
                // Integer comparisons
                assert(Num_min(5, 10) == 5);
                assert(Num_min(10, 5) == 5);
                assert(Num_min(0, 0) == 0);
                assert(Num_min(-5, 5) == -5);
                assert(Num_min(-10, -5) == -10);
                assert(Num_min(100, 100) == 100);
                
                // Edge cases with different integer types
                assert(Num_min(INT32_MAX, INT32_MAX - 1) == INT32_MAX - 1);
                assert(Num_min(INT32_MIN, INT32_MIN + 1) == INT32_MIN);
                assert(Num_min(0, INT32_MAX) == 0);
                assert(Num_min(INT32_MIN, 0) == INT32_MIN);
                
                // Unsigned integers
                assert(Num_min(0U, 100U) == 0U);
                assert(Num_min(100U, 50U) == 50U);
                assert(Num_min(UINT32_MAX, UINT32_MAX - 1) == UINT32_MAX - 1);
                
                // Floating-point
                assert(Num_min(3.14, 2.71) == 2.71);
                assert(Num_min(1.5, 1.5) == 1.5);
                assert(Num_min(-1.5, 1.5) == -1.5);
                assert(Num_min(0.0, -0.1) == -0.1);
        }
        printf("=> Test1: OK\n\n");

        printf("=> Test2: Num_max\n");
        {
                // Integer comparisons
                assert(Num_max(5, 10) == 10);
                assert(Num_max(10, 5) == 10);
                assert(Num_max(0, 0) == 0);
                assert(Num_max(-5, 5) == 5);
                assert(Num_max(-10, -5) == -5);
                assert(Num_max(100, 100) == 100);
                
                // Edge cases with different integer types
                assert(Num_max(INT32_MAX, INT32_MAX - 1) == INT32_MAX);
                assert(Num_max(INT32_MIN, INT32_MIN + 1) == INT32_MIN + 1);
                assert(Num_max(0, INT32_MAX) == INT32_MAX);
                assert(Num_max(INT32_MIN, 0) == 0);
                
                // Unsigned integers
                assert(Num_max(0U, 100U) == 100U);
                assert(Num_max(100U, 50U) == 100U);
                assert(Num_max(UINT32_MAX, UINT32_MAX - 1) == UINT32_MAX);
                
                // Floating-point
                assert(Num_max(3.14, 2.71) == 3.14);
                assert(Num_max(1.5, 1.5) == 1.5);
                assert(Num_max(-1.5, 1.5) == 1.5);
                assert(Num_max(0.0, -0.1) == 0.0);
        }
        printf("=> Test2: OK\n\n");

        printf("=> Test3: Num_delta\n");
        {
                // Basic integer differences
                assert(Num_delta(10, 7) == 3);
                assert(Num_delta(7, 10) == 3);
                assert(Num_delta(100, 100) == 0);
                assert(Num_delta(0, 0) == 0);
                
                // Signed integers
                assert(Num_delta(5, -5) == 10);
                assert(Num_delta(-5, 5) == 10);
                assert(Num_delta(-10, -3) == 7);
                assert(Num_delta(-3, -10) == 7);
                
                // Large values
                assert(Num_delta(0, 1000) == 1000);
                assert(Num_delta(1000, 0) == 1000);
                assert(Num_delta(INT32_MAX, 0) == INT32_MAX);
                assert(Num_delta(0, INT32_MAX) == INT32_MAX);
                
                // Unsigned integers
                assert(Num_delta(100U, 50U) == 50U);
                assert(Num_delta(50U, 100U) == 50U);
                assert(Num_delta(UINT32_MAX, 0U) == UINT32_MAX);
                
                // Floating-point
                assert(Num_delta(3.14, 2.71) > 0.42 && Num_delta(3.14, 2.71) < 0.44);
                assert(Num_delta(2.71, 3.14) > 0.42 && Num_delta(2.71, 3.14) < 0.44);
                assert(Num_delta(1.5, 1.5) == 0.0);
                assert(Num_delta(-1.5, 1.5) == 3.0);
                assert(Num_delta(1.5, -1.5) == 3.0);
        }
        printf("=> Test3: OK\n\n");

        printf("=> Test4: Num_udelta\n");
        {
                // uint64_t: counter delta: normal increments
                assert(Num_udelta(100ULL, 150ULL) == 50);
                assert(Num_udelta(0ULL, 100ULL) == 100);
                assert(Num_udelta(1000ULL, 2000ULL) == 1000);
                
                // uint64_t: counter delta: no change
                assert(Num_udelta(0ULL, 0ULL) == 0);
                assert(Num_udelta(100ULL, 100ULL) == 0);
                assert(Num_udelta(UINT64_MAX, UINT64_MAX) == 0);
                
                // uint64_t: counter delta: current less then previous (wrap)
                assert(Num_udelta(150ULL, 100ULL) == UINT64_MAX - 49);
                assert(Num_udelta(UINT64_MAX, 0ULL) == 1);
                assert(Num_udelta(UINT64_MAX - 10ULL, 5ULL) == 16);
                assert(Num_udelta(UINT64_MAX - 100ULL, UINT64_MAX - 50ULL) == 50);
                assert(Num_udelta(UINT64_MAX - 1000, 1000) == 2001);
                assert(Num_udelta(100ULL, 100ULL) == 0);
                assert(Num_udelta(UINT64_MAX, UINT64_MAX - 1) == UINT64_MAX);
                assert(Num_udelta(UINT64_MAX - 5, 0) == 6);
                assert(Num_udelta(UINT64_MAX - 5, 1) == 7);
                assert(Num_udelta(UINT64_MAX - 5, 2) == 8);
                assert(Num_udelta(UINT64_MAX - 999, 999) == 1999);
                assert(Num_udelta(UINT64_MAX - 1, 1) == 3);
                
                // uint64_t: counter delta: edge cases
                assert(Num_udelta(0ULL, UINT64_MAX) == UINT64_MAX);
                assert(Num_udelta(1ULL, 0ULL) == UINT64_MAX);
                
                // uint64_t: additional tests
                uint64_t u64_prev = UINT64_MAX - 500;
                uint64_t u64_curr = 500;
                assert(Num_udelta(u64_prev, u64_curr) == 1001);
                
                // uint8_t tests
                uint8_t u8_prev = 250;
                uint8_t u8_curr = 5;
                assert(Num_udelta(u8_prev, u8_curr) == 11);
                assert(Num_udelta((uint8_t)0, (uint8_t)100) == 100);
                assert(Num_udelta((uint8_t)100, (uint8_t)100) == 0);
                assert(Num_udelta((uint8_t)UINT8_MAX, (uint8_t)0) == 1);
                
                // uint16_t tests
                uint16_t u16_prev = 65530;
                uint16_t u16_curr = 10;
                assert(Num_udelta(u16_prev, u16_curr) == 16);
                assert(Num_udelta((uint16_t)0, (uint16_t)1000) == 1000);
                assert(Num_udelta((uint16_t)1000, (uint16_t)1000) == 0);
                assert(Num_udelta((uint16_t)UINT16_MAX, (uint16_t)0) == 1);
                
                // uint32_t tests
                uint32_t u32_prev = UINT32_MAX - 100;
                uint32_t u32_curr = 50;
                assert(Num_udelta(u32_prev, u32_curr) == 151);
                assert(Num_udelta((uint32_t)0, (uint32_t)10000) == 10000);
                assert(Num_udelta((uint32_t)10000, (uint32_t)10000) == 0);
                assert(Num_udelta((uint32_t)UINT32_MAX, (uint32_t)0) == 1);
        }
        printf("=> Test4: OK\n\n");

        printf("============> Num Tests: OK\n\n");
        return 0;
}

