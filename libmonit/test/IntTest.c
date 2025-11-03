#include "Config.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "Bootstrap.h"
#include "Int.h"


/**
 * Int.c unity tests
 */


int main(void) {

        Bootstrap(); // Need to initialize library

        printf("============> Start Int Tests\n\n");

        printf("=> Test1: counter delta: normal increments\n");
        {
                assert(Int_deltaUINT64(100, 150) == 50);
                assert(Int_deltaUINT64(0, 100) == 100);
                assert(Int_deltaUINT64(1000, 2000) == 1000);
        }
        printf("=> Test1: OK\n\n");

        printf("=> Test2: counter delta: no change\n");
        {
                assert(Int_deltaUINT64(0, 0) == 0);
                assert(Int_deltaUINT64(100, 100) == 0);
                assert(Int_deltaUINT64(UINT64_MAX, UINT64_MAX) == 0);
        }
        printf("=> Test2: OK\n\n");

        printf("=> Test3: counter delta: current less then previous (wrap)\n");
        {
                assert(Int_deltaUINT64(150, 100) == UINT64_MAX - 49);
                assert(Int_deltaUINT64(UINT64_MAX, 0) == 1);
                assert(Int_deltaUINT64(UINT64_MAX - 10, 5) == 16);
                assert(Int_deltaUINT64(UINT64_MAX - 100, UINT64_MAX - 50) == 50);
                assert(Int_deltaUINT64(UINT64_MAX - 1000, 1000) == 2001);
                assert(Int_deltaUINT64(100, 100) == 0);
                assert(Int_deltaUINT64(UINT64_MAX, UINT64_MAX - 1) == UINT64_MAX);
                assert(Int_deltaUINT64(UINT64_MAX - 5, 0) == 6);
                assert(Int_deltaUINT64(UINT64_MAX - 5, 1) == 7);
                assert(Int_deltaUINT64(UINT64_MAX - 5, 2) == 8);
                assert(Int_deltaUINT64(UINT64_MAX - 999, 999) == 1999);
                assert(Int_deltaUINT64(UINT64_MAX - 1, 1) == 3);
        }
        printf("=> Test3: OK\n\n");

        printf("=> Test4: counter delta: edge cases\n");
        {
                assert(Int_deltaUINT64(0, UINT64_MAX) == UINT64_MAX);
                assert(Int_deltaUINT64(1, 0) == UINT64_MAX);
        }
        printf("=> Test4: OK\n\n");

        printf("============> Int Tests: OK\n\n");
        return 0;
}

