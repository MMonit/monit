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
#include <strings.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>
#include <regex.h>
#include <limits.h>


#include "NumberFormatException.h"
#include "system/System.h"
#include "Str.h"


/**
 * Implementation of the Str interface
 *
 * @author https://www.tildeslash.com/
 * @see https://mmonit.com/
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#ifndef HAVE_STRCASESTR
static char *strcasestr(const char *haystack, const char *needle) {
        if (!*needle)
                return (char *)haystack;
        for (; *haystack; haystack++) {
                const char *h = haystack;
                const char *n = needle;
                while (*h && *n && tolower((uchar_t)*h) == tolower((uchar_t)*n)) {
                        h++;
                        n++;
                }
                if (!*n)
                        return (char *)haystack;
        }
        return NULL;
}
#endif


/* -------------------------------------------------------- Public Methods */


char *Str_chomp(char *s) {
        if (s) {
                char *p = strpbrk(s, "\r\n");
                if (p)
                        *p = 0;
        }
        return s;
}


char *Str_trim(char *s) {
        if (STR_UNDEF(s))
                return s;
        
        unsigned char *start = (unsigned char *)s;
        unsigned char *end;
        
        while (isspace(*start)) start++;
        if (!*start) { *s = '\0'; return s; }
        
        for (end = start + strlen((const char *)start) - 1; isspace(*end); end--);
        
        size_t len = (size_t)(end - start) + 1;
        end[1] = '\0';
        
        if (start != (unsigned char *)s)
                memmove(s, start, len + 1);
        
        return s;
}


char *Str_rtrim(char *s) {
        if (STR_UNDEF(s))
                return s;
        
        unsigned char *end = (unsigned char *)s + strlen(s);
        
        while (end > (unsigned char *)s && isspace(*(end - 1))) end--;
        *end = '\0';
        
        return s;
}


char *Str_unquote(char *s) {
        if (STR_DEF(s)) {
                char *t = s;
                while (*t == '"' || *t == '\'' || isspace((uchar_t)*t))
                        t++;
                char *u = s;
                char *end = s;
                while (*t) {
                        *u = *t;
                        if (!(*t == '"' || *t == '\'' || isspace((uchar_t)*t)))
                                end = u + 1;
                        u++;
                        t++;
                }
                *end = '\0';
        }
        return s;
}


int Str_parseInt(const char *s) {
        int i;
        char *e;
        if (STR_UNDEF(s))
                THROW(NumberFormatException, "For input string null");
        errno = 0;
        i = (int)strtol(s, &e, 10);
        if (errno || (e == s))
                THROW(NumberFormatException, "For input string %s -- %s", s, System_getError(errno));
        return i;
}


long long Str_parseLLong(const char *s) {
        char *e;
        long long l;
        if (STR_UNDEF(s))
                THROW(NumberFormatException, "For input string null");
        errno = 0;
        l = strtoll(s, &e, 10);
        if (errno || (e == s))
                THROW(NumberFormatException, "For input string %s -- %s", s, System_getError(errno));
        return l;
}


double Str_parseDouble(const char *s) {
        char *e;
        double d;
        if (STR_UNDEF(s))
                THROW(NumberFormatException, "For input string null");
        errno = 0;
        d = strtod(s, &e);
        if (errno || (e == s))
                THROW(NumberFormatException, "For input string %s -- %s", s, System_getError(errno));
        return d;
}


char *Str_replaceChar(char *s, char o, char n) {
        if (s) {
                for (char *t = s; *t; t++)
                        if (*t == o)
                                *t = n;
        }
        return s;
}


bool Str_startsWith(const char *a, const char *b) {
        if (!STR_DEF(a) || !STR_DEF(b))
                return false;
        size_t b_len = strlen(b);
        return strncasecmp(a, b, b_len) == 0;
}


bool Str_endsWith(const char *a, const char *b) {
        if (!STR_DEF(a) || !STR_DEF(b))
                return false;
        size_t a_len = strlen(a);
        size_t b_len = strlen(b);
        if (a_len < b_len)
                return false;
        return strcasecmp(a + (a_len - b_len), b) == 0;
}


char *Str_sub(const char *a, const char *b) {
        if (!a || !STR_DEF(b))
                return NULL;
        return strcasestr(a, b);
}


bool Str_has(const char *charset, const char *s) {
        if (charset && s)
                return strpbrk(s, charset) != NULL;
        return false;
}


bool Str_isEqual(const char *a, const char *b) {
        if (a && b)
                return (strcasecmp(a, b) == 0);
        return false;
}


bool Str_isByteEqual(const char *a, const char *b) {
        if (a && b)
                return (__builtin_strcmp(a, b) == 0);
        return false;
}


char *Str_copy(char *dest, const char *src, int n) {
        if (src && dest && (n > 0)) {
                char *t = dest;
                while (*src && n--)
                        *t++ = *src++;
                *t = 0;
        } else if (dest)
                *dest = 0;
        return dest;
}


// We don't use strdup so we can report MemoryException on OOM
char *Str_dup(const char *s) {
        char *t = NULL;
        if (s) {
                size_t n = strlen(s) + 1;
                t = CALLOC(1, n);
                memcpy(t, s, n);
        }
        return t;
}


char *Str_ndup(const char *s, long n) {
        char *t = NULL;
        assert(n >= 0);
        if (s) {
                long l = (long)strlen(s);
                n = l < n ? l : n; // Use the actual length of s if shorter than n
                t = CALLOC(1, n + 1);
                memcpy(t, s, n);
                t[n] = 0;
        }
        return t;
}


char *Str_cat(const char *s, ...) {
        char *t = NULL;
        if (s) {
                va_list ap;
                va_start(ap, s);
                t = Str_vcat(s, ap);
                va_end(ap);
        }
        return t;
}


char *Str_vcat(const char *s, va_list ap) {
        char *t = NULL;
        if (s) {
                va_list ap_copy;
                va_copy(ap_copy, ap);
                int size = vsnprintf(t, 0, s, ap_copy) + 1;
                va_end(ap_copy);
                t = CALLOC(1, size);
                va_copy(ap_copy, ap);
                vsnprintf(t, size, s, ap_copy);
                va_end(ap_copy);
        }
        return t;
}


char *Str_trunc(char *s, int n) {
        assert(n >= 0);
        if (s) {
                size_t sl = strlen(s);
                if (sl > (size_t)n) {
                        if (n >= 3) {
                                memset(s + n - 3, '.', 3);
                        }
                        s[n] = 0;
                }
        }
        return s;
}


char *Str_curtail(char *s, const char *t) {
        if (s) {
                char *x = Str_sub(s, t);
                if (x) *x = 0;
        }
        return s;
}


bool Str_match(const char *pattern, const char *subject) {
        assert(pattern);
        if (STR_DEF(subject)) {
                regex_t regex = {0};
                int error = regcomp(&regex, pattern, REG_NOSUB|REG_EXTENDED);
                if (error) {
                        char e[STRLEN];
                        regerror(error, &regex, e, STRLEN);
                        regfree(&regex);
                        THROW(AssertException, "regular expression error -- %s", e);
                } else {
                        error = regexec(&regex, subject, 0, NULL, 0);
                        regfree(&regex);
                        return (error == 0);
                }
        }
        return false;
}


int Str_cmp(const void *x, const void *y) {
        return strcmp((const char *)x, (const char *)y);
}


bool Str_authcmp(const char *a, const char *b) {
        if (!a || !b)
                return false;
        size_t al = strlen(a);
        size_t bl = strlen(b);
        size_t length = al ^ ((al ^ bl) & -(al < bl)); // max(al, bl) branchless
        volatile unsigned int diff = (unsigned int)(al ^ bl);
        for (size_t i = 0; i < length; i++) {
                // Clamp index: when i >= len, use len (the null terminator position)
                size_t over_a = -(i >= al); // 0 or ~0
                size_t over_b = -(i >= bl);
                size_t ia = (i & ~over_a) | (al & over_a);
                size_t ib = (i & ~over_b) | (bl & over_b);
                diff |= (unsigned char)a[ia] ^ (unsigned char)b[ib];
        }
        return diff == 0;
}

