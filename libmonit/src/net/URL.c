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

#include <ctype.h>
#include <string.h>

#include "Str.h"
#include "URL.h"


/**
 * Implementation of the URL interface.
 *
 * @file
 */


// MARK: - Definitions


/* Unsafe URL characters: [00-1F, 7F-FF] <>\"#%}{|\\^[] ` */
static const uchar_t urlunsafe[256] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};


// MARK: - Private methods


static inline int _x2b(const char *x) {
        int b = ((x[0] >= 'A') ? ((x[0] & 0xdf) - 'A')+10 : (x[0] - '0'));
        b *= 16;
        b += (x[1] >= 'A' ? ((x[1] & 0xdf) - 'A')+10 : (x[1] - '0'));
        return b;
}


static inline char *_b2x(uchar_t b, char *x) {
        static const char _b2x_table[] = "0123456789ABCDEF";
        *x++ = '%';
        *x++ = _b2x_table[b >> 4];
        *x = _b2x_table[b & 0xf];
        return x;
}


static inline bool _isEscape(const char *s) {
        return s[0] == '%' && isxdigit((unsigned char)s[1]) && isxdigit((unsigned char)s[2]);
}


static bool _isUnsafe(const char *s) {
        return urlunsafe[(unsigned char)*s];
}


static bool _isUnescaped(const char *s) {
        return _isUnsafe(s) && !_isEscape(s);
}


static char *_escape(const char *url, bool (*mustEscape)(const char *)) {
        char *escaped = 0;
        if (url) {
                char *p;
                int i, n;
                for (n = i = 0; url[i]; i++)
                        if (mustEscape(url + i))
                                n += 2;
                p = escaped = ALLOC(i + n + 1);
                for (; *url; url++, p++) {
                        *p = *url;
                        if (mustEscape(url))
                                p = _b2x((uchar_t)*url, p);
                }
                *p = 0;
        }
        return escaped;
}


// MARK: - URL Public methods


char *URL_unescape(char *url) {
        if (STR_DEF(url)) {
                int x = 0, y = 0;
                for (; url[y]; x++, y++) {
                        if (url[y] == '+') {
                                url[x] = ' ';
                        } else if (_isEscape(url + y) && (url[y + 1] != '0' || url[y + 2] != '0')) {
                                url[x] = (char)_x2b(url + y + 1);
                                y += 2;
                        } else {
                                url[x] = url[y];
                        }
                }
                url[x] = 0;
        }
        return url;
}


char *URL_escape(const char *url) {
        return _escape(url, _isUnsafe);
}


char *URL_ensureEscaped(const char *url) {
        return _escape(url, _isUnescaped);
}


bool URL_isunsafe(unsigned char c) {
        return urlunsafe[c];
}


bool URL_isabsolute(const char *url) {
        if (STR_DEF(url)) {
                char* separator = strchr(url, ':');
                if (STR_DEF(separator)) {
                        return ((separator[1] == '/') && (separator[2] == '/'));
                }
        }
        return false;
}
