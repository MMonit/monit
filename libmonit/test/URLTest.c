#include "Config.h"

#include <stdio.h>
#include <ctype.h>
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

        printf("=> Test2: URL_escape and URL_ensureEscaped\n");
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

                assert(!URL_ensureEscaped(NULL));

                static const struct { const char *in; const char *out; } once[] = {
                        // An escape sequence is kept as it is written
                        {"/my%20page",                      "/my%20page"},
                        {"%41%42",                          "%41%42"},
                        {"%2f%2F%aB%Ab",                    "%2f%2F%aB%Ab"},
                        {"%00%7F%ff",                       "%00%7F%ff"},
                        {"%2520",                           "%2520"},
                        {"%252",                            "%252"},
                        {"/q?a+b&c=d;e:f@g!$'()*,-._~",     "/q?a+b&c=d;e:f@g!$'()*,-._~"},
                        // The rest is escaped, beside an escape sequence too
                        {"/my page?x=1%2B1",                "/my%20page?x=1%2B1"},
                        {"/items?filter[name]=J%C3%B8rgen", "/items?filter%5Bname%5D=J%C3%B8rgen"},
                        {"/Jørgen/j%c3%b8rgen",             "/J%C3%B8rgen/j%c3%b8rgen"},
                        {"%20 %20",                         "%20%20%20"},
                        {" %20 ",                           "%20%20%20"},
                        {"<>\"#{}|\\^[]`",                  "%3C%3E%22%23%7B%7D%7C%5C%5E%5B%5D%60"},
                        // A '%' that does not start an escape sequence
                        {"%",                               "%25"},
                        {"100%",                            "100%25"},
                        {"%4",                              "%254"},
                        {"%4G",                             "%254G"},
                        {"%G4",                             "%25G4"},
                        {"%%",                              "%25%25"},
                        {"%%%",                             "%25%25%25"},
                        {"%%41",                            "%25%41"},
                        {"%4%41",                           "%254%41"},
                        {"% 41",                            "%25%2041"},
                        {"50%zz%4",                         "50%25zz%254"},
                        // The characters next to 0-9, A-F and a-f are not hex digits
                        {"%/0%:0%@A%Ga%`a%ga",              "%25/0%25:0%25@A%25Ga%25%60a%25ga"},
                        {"",                                ""},
                };
                for (size_t i = 0; i < sizeof(once) / sizeof(once[0]); i++) {
                        char *escaped = URL_ensureEscaped(once[i].in);
                        char *again = URL_ensureEscaped(escaped);
                        printf("\t%-34s -> %s\n", once[i].in, escaped);
                        assert(escaped != once[i].in);
                        assert(Str_isByteEqual(escaped, once[i].out));
                        assert(Str_isByteEqual(again, escaped));
                        FREE(escaped);
                        FREE(again);
                }

                char *bytes = URL_ensureEscaped("\t\x01\x7f\xff");
                assert(Str_isByteEqual(bytes, "%09%01%7F%FF"));
                FREE(bytes);

                // Every string over {%, 4, f, G, space} up to six characters
                int strings = 0;
                for (int len = 1; len <= 6; len++) {
                        int combos = 1;
                        for (int k = 0; k < len; k++)
                                combos *= 5;
                        for (int m = 0; m < combos; m++) {
                                char in[7], decoded[7], redecoded[19];
                                int v = m, unsafe = 0, kept = 0;
                                for (int k = 0; k < len; k++) {
                                        in[k] = "%4fG "[v % 5];
                                        v /= 5;
                                }
                                in[len] = 0;
                                for (int k = 0; k < len; k++) {
                                        if (URL_isunsafe((unsigned char)in[k]))
                                                unsafe++;
                                        if (in[k] == '%' && isxdigit((unsigned char)in[k + 1]) && isxdigit((unsigned char)in[k + 2]))
                                                kept++;
                                }
                                // URL_escape() adds two characters for every unsafe
                                // character and decodes to the input itself
                                char *all = URL_escape(in);
                                assert(strlen(all) == (size_t)(len + 2 * unsafe));
                                Str_copy(redecoded, all, sizeof(redecoded));
                                assert(Str_isByteEqual(URL_unescape(redecoded), in));
                                // URL_ensureEscaped() adds them for all but the
                                // '%' of an escape sequence
                                char *escaped = URL_ensureEscaped(in);
                                assert(strlen(escaped) == (size_t)(len + 2 * (unsafe - kept)));
                                // Nothing unsafe is left and every '%' starts an
                                // escape sequence
                                for (char *s = escaped; *s; s++)
                                        assert(*s == '%' ? isxdigit((unsigned char)s[1]) && isxdigit((unsigned char)s[2]) : !URL_isunsafe((unsigned char)*s));
                                // It decodes to what the input decodes to
                                Str_copy(decoded, in, sizeof(decoded));
                                Str_copy(redecoded, escaped, sizeof(redecoded));
                                assert(Str_isByteEqual(URL_unescape(redecoded), URL_unescape(decoded)));
                                // Escaped again it is the same string
                                char *again = URL_ensureEscaped(escaped);
                                assert(Str_isByteEqual(again, escaped));
                                FREE(all);
                                FREE(escaped);
                                FREE(again);
                                strings++;
                        }
                }
                assert(strings == 19530);
                printf("\t%d strings over {%%, 4, f, G, space} escaped: OK\n", strings);
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
