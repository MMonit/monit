#include "Config.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "Bootstrap.h"
#include "Str.h"


/**
 * Str.c unity tests
 */


int main(void) {

        Bootstrap(); // Need to initialize library

        printf("============> Start Str Tests\n\n");

        printf("=> Test1: copy\n");
        {
                char s3[STRLEN];
                printf("\tResult: %s\n", Str_copy(s3, "The abc house", 7));
                assert(Str_isEqual(s3, "The abc"));
                printf("\tTesting for NULL argument\n");
                assert(!Str_copy(NULL, NULL, 7));
                assert(Str_isEqual(Str_copy(s3, NULL, sizeof(s3)), ""));
        }
        printf("=> Test1: OK\n\n");

        printf("=> Test2: dup\n");
        {
                char *s4 = Str_dup("abc123");
                printf("\tResult: %s\n", s4);
                assert(Str_isEqual(s4, "abc123"));
                printf("\tTesting for NULL argument\n");
                assert(!Str_dup(NULL));
                FREE(s4);
        }
        printf("=> Test2: OK\n\n");

        printf("=> Test3: ndup\n");
        {
                char *s5 = Str_ndup("abc123", 3);
                printf("\tResult: %s\n", s5);
                assert(Str_isEqual(s5, "abc"));
                printf("\tTesting for NULL argument\n");
                assert(!Str_ndup(NULL, 3));
                FREE(s5);
        }
        printf("=> Test3: OK\n\n");

        printf("=> Test4: Str_cat & Str_vcat\n");
        {
                char *s6;
                s6 = Str_cat("%s://%s%s?%s", "https", "foo.bar",
                                   "/uri", "abc=123");
                printf("\tResult: %s\n", s6);
                assert(Str_isEqual(s6, "https://foo.bar/uri?abc=123"));
                FREE(s6);
                printf("\tTesting for NULL arguments\n");
                s6 = Str_cat(NULL);
                assert(s6==NULL);
                FREE(s6);
        }
        printf("=> Test4: OK\n\n");

        printf("=> Test5: chomp\n");
        {
                char s3[] = "abc\r\n123";
                printf("\tResult: %s\n", Str_chomp(s3));
                assert(Str_isEqual(s3, "abc"));
                printf("\tTesting for NULL argument\n");
                assert(!Str_chomp(NULL));
        }
        printf("=> Test5: OK\n\n");

        printf("=> Test6: trim\n");
        {
                char er[]  = "   ";
                char elr[] = "   ";
                char or[]  = " a ";
                char olr[] = " a ";
                char s4[]  = "  \t abc \r\n\t ";
                printf("\tResult: %s\n", Str_trim(s4));
                assert(Str_isEqual(s4, "abc"));
                printf("\tTesting for NULL argument\n");
                assert(!Str_trim(NULL));
                assert(Str_isEqual(Str_rtrim(er), ""));
                assert(Str_isEqual(Str_trim(elr), ""));
                assert(Str_isEqual(Str_rtrim(or), " a"));
                assert(Str_isEqual(Str_trim(olr), "a"));
                assert(Str_isEqual(Str_trim(olr), "a"));
        }
        printf("=> Test6: OK\n\n");

        printf("=> Test7: trim quotes\n");
        {
                char t1[] = "\"'abc'\"";
                char t2[] = "\"'abc";
                char t3[] = "abc'\"";
                char t4[] = "'\"";
                char t5[] = " \t abc def '\"  ";
                char t6[] = "\n \"ab\" cd\' ef \t g\r\n";
                char t7[] = "";
                char t8[] = "abc";
                char t9[] = " \t\n ";
                char t10[] = "'a'";
                printf("\tTesting balanced quotes\n");
                assert(Str_isEqual("abc", Str_unquote(t1)));
                printf("\tTesting leading quotes only\n");
                assert(Str_isEqual("abc", Str_unquote(t2)));
                printf("\tTesting trailing quotes only\n");
                assert(Str_isEqual("abc", Str_unquote(t3)));
                printf("\tTesting NULL argument\n");
                assert(!Str_unquote(NULL));
                printf("\tTesting quotes-only argument\n");
                assert(Str_isEqual("", Str_unquote(t4)));
                printf("\tTesting quotes and whitespace removal\n");
                assert(Str_isEqual("abc def", Str_unquote(t5)));
                printf("\tTesting inside quotes and whitespace preservation\n");
                assert(Str_isEqual("ab\" cd\' ef \t g", Str_unquote(t6)));
                printf("\tTesting empty string\n");
                assert(Str_isEqual("", Str_unquote(t7)));
                printf("\tTesting nothing to trim\n");
                assert(Str_isEqual("abc", Str_unquote(t8)));
                printf("\tTesting whitespace only\n");
                assert(Str_isEqual("", Str_unquote(t9)));
                printf("\tTesting single char quoted\n");
                assert(Str_isEqual("a", Str_unquote(t10)));
        }
        printf("=> Test7: OK\n\n");
        
        printf("=> Test8: parseInt, parseLLong, parseDouble\n");
        {
                char i[STRLEN] = "   -2812 bla";
                char ll[STRLEN] = "  2147483642 blabla";
                char d[STRLEN] = "  2.718281828 this is e";
                char de[STRLEN] = "1.495E+08 kilometer = An Astronomical Unit";
                char ie[STRLEN] = " 9999999999999999999999999999999999999999";
                char ie2[] = " 9999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999";
                printf("\tResult:\n");
                printf("\tParsed int = %d\n", Str_parseInt(i));
                assert(Str_parseInt(i)==-2812);
                TRY
                {
                        assert(Str_parseInt(NULL) == 0);
                        assert(false);
                }
                CATCH(NumberFormatException)
                {
                        printf("\tok\n");
                }
                END_TRY;
                TRY
                {
                        assert(Str_parseInt("") == 0);
                        assert(false);
                }
                CATCH(NumberFormatException)
                {
                        printf("\tok\n");
                }
                END_TRY;
                TRY
                {
                        Str_parseInt(ie);
                        assert(false);
                }
                CATCH(NumberFormatException)
                {
                        printf("\tok\n");
                }
                END_TRY;
                printf("\tParsed long long = %lld\n", Str_parseLLong(ll));
                assert(Str_parseLLong(ll)==2147483642);
                TRY
                {
                        assert(Str_parseLLong(NULL) == 0);
                        assert(false);
                }
                CATCH(NumberFormatException)
                {
                        printf("\tok\n");
                }
                END_TRY;
                TRY
                {
                        assert(Str_parseLLong("") == 0);
                        assert(false);
                }
                CATCH(NumberFormatException)
                {
                        printf("\tok\n");
                }
                END_TRY;
                TRY
                {
                        Str_parseLLong(ie2);
                        assert(false);
                }
                CATCH(NumberFormatException)
                {
                        printf("\tok\n");
                }
                END_TRY;
                printf("\tParsed double = %.9f\n", Str_parseDouble(d));
                assert(Str_parseDouble(d)==2.718281828);
                printf("\tParsed double exp = %.3e\n", Str_parseDouble(de));
                assert(Str_parseDouble(de)==1.495e+08);
                TRY
                {
                        assert(Str_parseDouble(NULL) == 0);
                        assert(false);
                }
                CATCH(NumberFormatException)
                {
                        printf("\tok\n");
                }
                END_TRY;
                TRY
                {
                        assert(Str_parseDouble("") == 0);
                        assert(false);
                }
                CATCH(NumberFormatException)
                {
                        printf("\tok\n");
                }
                END_TRY;
                TRY
                {
                        printf("%lf\n", Str_parseDouble(ie2));
                        assert(false);
                }
                CATCH(NumberFormatException)
                {
                        printf("\tok\n");
                }
                END_TRY;
        }
        printf("=> Test8: OK\n\n");

        printf("=> Test9: replace\n");
        {
                char s9[] = "abccba";
                printf("\tResult: %s\n", Str_replaceChar(s9, 'b', 'X'));
                assert(Str_isEqual(s9, "aXccXa"));
                printf("\tTesting for NULL argument\n");
                assert(!Str_replaceChar(NULL, 'b', 'X'));
        }
        printf("=> Test9: OK\n\n");

        printf("=> Test10: startsWith\n");
        {
                char *a = "mysql://localhost:3306/zild?user=root&password=swordfish";
                printf("\tResult: starts with mysql - %s\n", Str_startsWith(a, "mysql") ? "yes" : "no");
                assert(Str_startsWith(a, "mysql"));
                assert(!Str_startsWith(a, "sqlite"));
                assert(Str_startsWith("sqlite", "sqlite"));
                printf("\tTesting for NULL and NUL argument\n");
                assert(!Str_startsWith(a, NULL));
                assert(!Str_startsWith(a, ""));
                assert(!Str_startsWith(NULL, "mysql"));
                assert(!Str_startsWith("", NULL));
                assert(!Str_startsWith(NULL, NULL));
                assert(!Str_startsWith("", ""));
                assert(!Str_startsWith("/", "/WEB-INF"));
        }
        printf("=> Test10: OK\n\n");

        printf("=> Test11: endsWith\n");
        {
                char *a = "mysql://localhost:3306";
                printf("\tResult: ends with 3306 - %s\n", Str_endsWith(a, "3306") ? "yes" : "no");
                assert(Str_endsWith(a, "3306"));
                assert(!Str_endsWith(a, "sqlite"));
                assert(Str_endsWith("sqlite", "sqlite"));
                printf("\tTesting for NULL and NUL argument\n");
                assert(!Str_endsWith(a, NULL));
                assert(!Str_endsWith(a, "")); // a ends with 0
                assert(!Str_endsWith(NULL, "mysql"));
                assert(!Str_endsWith("", NULL));
                assert(!Str_endsWith(NULL, NULL));
                assert(!Str_endsWith("", ""));
                assert(!Str_endsWith("abc", "defabc"));
        }
        printf("=> Test11: OK\n\n");

        printf("=> Test12: isEqual\n");
        {
                char *a = "mysql://localhost:3306";
                printf("\tResult: is equal - %s\n", Str_isEqual(a, "mysql://localhost:3306") ? "yes" : "no");
                assert(Str_isEqual("sqlite", "sqlite"));
                printf("\tTesting for NULL and NUL argument\n");
                assert(!Str_isEqual(a, NULL));
                assert(!Str_isEqual(a, ""));
                assert(!Str_isEqual(NULL, "mysql"));
                assert(!Str_isEqual("", NULL));
                assert(!Str_isEqual(NULL, NULL));
                assert(Str_isEqual("", ""));
        }
        printf("=> Test12: OK\n\n");

        printf("=> Test13: trail\n");
        {
                char s[] = "This string will be trailed someplace";
                assert(Str_trunc(NULL, 100) == NULL);
                assert(Str_isEqual(Str_trunc("", 0), ""));
                assert(Str_isEqual(Str_trunc(s, (int)strlen(s)), "This string will be trailed someplace"));
                printf("\tResult: %s\n", Str_trunc(s, 30));
                assert(Str_isEqual(s, "This string will be trailed..."));
                printf("\tResult: %s\n", Str_trunc(s, 3));
                assert(Str_isEqual(s, "..."));
                printf("\tResult: %s\n", Str_trunc(s, 0));
                assert(Str_isEqual(s, ""));
        }
        printf("=> Test13: OK\n\n");

        printf("=> Test14: regular expression match\n");
        {
                char *phone_pattern = "^[-0-9+( )]{7,40}$";
                char *email_pattern = "^[^@ ]+@([-a-zA-Z0-9]+\\.)+[a-zA-Z]{2,}$";
                char *valid_phone1 = "+4797141255";
                char *valid_phone2 = "(47)-97-14-12-55";
                char *invalid_phone1 = "141255";
                char *invalid_phone2 = "(47)971412551234567890123456789012345678901234567890";
                char *invalid_phone3 = "";
                char *invalid_phone4 = "abc123";
                char *valid_email1 = "hauk@TILDESLASH.com";
                char *valid_email2 = "jan-henrik.haukeland@haukeland.co.uk";
                char *invalid_email1 = "hauktildeslash.com";
                char *invalid_email2 = "";
                char *invalid_email3 = "hauk@tildeslashcom";
                char *invalid_email4 = "hauk@æøåtildeslash.com";
                char *invalid_pattern = "^[[";
                // phone
                printf("\tResult: match(%s, %s)\n", phone_pattern, valid_phone1);
                assert(Str_match(phone_pattern, valid_phone1));
                printf("\tResult: match(%s, %s)\n", phone_pattern, valid_phone2);
                assert(Str_match(phone_pattern, valid_phone2));
                printf("\tResult: match(%s, %s)\n", phone_pattern, invalid_phone1);
                assert(! Str_match(phone_pattern, invalid_phone1));
                printf("\tResult: match(%s, %s)\n", phone_pattern, invalid_phone2);
                assert(! Str_match(phone_pattern, invalid_phone2));
                printf("\tResult: match(%s, %s)\n", phone_pattern, invalid_phone3);
                assert(! Str_match(phone_pattern, invalid_phone3));
                printf("\tResult: match(%s, %s)\n", phone_pattern, invalid_phone4);
                assert(! Str_match(phone_pattern, invalid_phone4));
                // email
                printf("\tResult: match(%s, %s)\n", email_pattern, valid_email1);
                assert(Str_match(email_pattern, valid_email1));
                printf("\tResult: match(%s, %s)\n", email_pattern, valid_email2);
                assert(Str_match(email_pattern, valid_email2));
                printf("\tResult: match(%s, %s)\n", email_pattern, invalid_email1);
                assert(! Str_match(email_pattern, invalid_email1));
                printf("\tResult: match(%s, %s)\n", email_pattern, invalid_email2);
                assert(! Str_match(email_pattern, invalid_email2));
                printf("\tResult: match(%s, %s)\n", email_pattern, invalid_email3);
                assert(! Str_match(email_pattern, invalid_email3));
                printf("\tResult: match(%s, %s)\n", email_pattern, invalid_email4);
                assert(! Str_match(email_pattern, invalid_email4));
                // invalid regex
                TRY
                {
                        Str_match(invalid_pattern, valid_email1);
                        assert(false);
                }
                CATCH(AssertException)
                {
                        printf("\tok\n");
                }
                END_TRY;
        }
        printf("=> Test14: OK\n\n");

        printf("=> Test15: substring\n");
        {
                assert(Str_sub("foo bar baz", "bar"));
                assert(!  Str_sub("foo bar baz", "barx"));
                assert(Str_isEqual(Str_sub("foo bar baz", "baz"), "baz"));
                assert(Str_sub("foo bar baz", "foo bar baz"));
                assert(Str_sub("a", "a"));
                assert(! Str_sub("a", "b"));
                assert(! Str_sub("", ""));
                assert(! Str_sub("foo", ""));
                assert(! Str_sub("abc", "abcdef"));
                assert(! Str_sub("foo", "foo bar"));
                assert(Str_isEqual(Str_sub("foo foo bar", "foo bar"), "foo bar"));
                assert(Str_sub("foo foo bar foo bar baz fuu", "foo bar baz"));
                assert(Str_isEqual(Str_sub("abcd abcc", "abcc"), "abcc"));
        }
        printf("=> Test15: OK\n\n");

        printf("=> Test16: Str_curtail\n");
        {
                char s[] = "<text>Hello World</text>";
                assert(Str_isByteEqual(Str_curtail(s, "</text>"), "<text>Hello World"));
                assert(Str_isByteEqual(Str_curtail(s, ">"), "<text"));
                assert(Str_isByteEqual(Str_curtail(s, "@"), "<text"));
                assert(Str_isByteEqual(NULL, "a") == false);
                assert(Str_isByteEqual("a", NULL) == false);
                assert(Str_isByteEqual(NULL, NULL) == false);
        }
        printf("=> Test16: OK\n\n");

        printf("=> Test17: Str_authcmp\n");
        {
                assert(!Str_authcmp(NULL,     NULL));
                assert(!Str_authcmp("abcdef", NULL));
                assert(!Str_authcmp(NULL,     "abcdef"));
                assert(!Str_authcmp("",     "abcdef"));
                assert(!Str_authcmp("abcdef",     ""));
                assert(Str_authcmp("abcdef", "abcdef"));
                assert(!Str_authcmp("abcdef", "ABCDEF"));
                char *a = "da091173a92116fc7b86990368647f99228cd0b5d993e93248b501e059674b7e";
                char *b = "9b594557f02a0084bcd10cbb160406618312ce6612aeb8da86e57b2929fa1465";
                assert(!Str_authcmp(a, b));
                assert(Str_authcmp(a, a));
        }
        printf("=> Test17: OK\n\n");

        printf("=> Test18: Str_cmp\n");
        {
                assert(Str_cmp("foo", "foo") == 0);
                assert(Str_cmp("foo", "FOO") != 0);
                assert(Str_cmp("foo", "bar") != 0);
        }
        printf("=> Test18: OK\n\n");

        printf("============> Str Tests: OK\n\n");
        return 0;
}


