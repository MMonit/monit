#include "Config.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "Bootstrap.h"
#include "Str.h"
#include "URL.h"


/**
 * URL.c unity tests
 */


int main(void) {

        Bootstrap(); // Need to initialize library

        printf("============> Start URL Tests\n\n");

        printf("=> Test1: URL_unescape\n");
        {
                char s1[] = "http://www.tildeslash.com/zild+rules/%20zild%5B%5D.doc";
                assert(Str_isEqual(URL_unescape(s1), "http://www.tildeslash.com/zild rules/ zild[].doc"));

                char s2[] = "";
                assert(Str_isEqual(URL_unescape(s2), ""));

                assert(!URL_unescape(NULL));

                static const struct { const char *in; const char *out; } kept[] = {
                        {"http://www.tildeslash.com/%",  "http://www.tildeslash.com/%"},
                        {"http://www.tildeslash.com/%0", "http://www.tildeslash.com/%0"},
                        {"secret%zzjunk",                "secret%zzjunk"},
                        {"100%",                         "100%"},
                        {"%4",                           "%4"},
                        {"%G0x",                         "%G0x"},
                        {"secret%00junk",                "secret%00junk"},
                        {"%%41",                         "%A"},
                        {"a%zz%41+b",                    "a%zzA b"},
                        {"%2525",                        "%25"},  // Decoded once
                        {"%c3%bc%C3%BC",                 "üü"},
                };
                for (size_t i = 0; i < sizeof(kept) / sizeof(kept[0]); i++) {
                        char s[STRLEN];
                        Str_copy(s, kept[i].in, sizeof(s));
                        printf("\t%-32s -> %s\n", kept[i].in, URL_unescape(s));
                        assert(Str_isEqual(s, kept[i].out));
                }
        }
        printf("=> Test1: OK\n\n");

        printf("=> Test2: URL_escape\n");
        {
                assert(!URL_escape(NULL));

                char s1[] = "http://www.tildeslash.com/<>#%{}|\\^~[] `";
                char *rs1 = URL_escape(s1);
                assert(Str_isEqual(rs1, "http://www.tildeslash.com/%3C%3E%23%25%7B%7D%7C%5C%5E~%5B%5D%20%60"));
                FREE(rs1);

                char *rs2 = URL_escape("");
                assert(rs2 && Str_isEqual(rs2, ""));
                FREE(rs2);

                char *rs3 = URL_escape("/a b?q=1&r=ü+%41");
                assert(Str_isEqual(rs3, "/a%20b?q=1&r=%C3%BC+%2541"));
                FREE(rs3);
        }
        printf("=> Test2: OK\n\n");

        printf("=> Test3: URL_isunsafe\n");
        {
                assert(URL_isunsafe(0));
                assert(!URL_isunsafe('a'));
                assert(!URL_isunsafe('Z'));
                assert(!URL_isunsafe('0'));
                assert(URL_isunsafe(' '));
                assert(URL_isunsafe('<'));
                assert(URL_isunsafe(254));
        }
        printf("=> Test3: OK\n\n");

        printf("=> Test4: URL_isabsolute\n");
        {
                assert(URL_isabsolute("mysql://localhost:3306"));
                assert(URL_isabsolute("http://example.com"));
                assert(!URL_isabsolute("sqlite"));
                assert(!URL_isabsolute("/path/to/file"));
                assert(!URL_isabsolute(NULL));
        }
        printf("=> Test4: OK\n\n");

        printf("============> URL Tests: OK\n\n");

        return 0;
}
