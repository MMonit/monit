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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#ifdef HAVE_ZLIB_H
#include <zlib.h>
#endif

#include "Str.h"
#include "StringBuffer.h"


/**
 * Implementation of the StringBuffer interface.
 *
 * @author https://www.tildeslash.com/
 * @see https://mmonit.com/
 * @file
 */


/* ------------------------------------------------------------ Definitions */


#define T StringBuffer_T
struct T {
        int used;
        int length;
        unsigned char *buffer;
        void *compressedBuffer;
};


/* ---------------------------------------------------------------- Private */


__attribute__((format (printf, 2, 0)))
static inline void _append(T S, const char *s, va_list ap) {
        va_list ap_copy;
        while (true) {
                va_copy(ap_copy, ap);
                int n = vsnprintf((char *)(S->buffer + S->used), S->length - S->used, s, ap_copy);
                va_end(ap_copy);
                if (n < 0) {
                        S->buffer[S->used] = 0;
                        break;
                }
                if ((S->used + n) < S->length) {
                        S->used += n;
                        break;
                }
                S->length += STRLEN + n;
                RESIZE(S->buffer, S->length);
        }
}


static inline T _ctor(int hint) {
        T S;
        NEW(S);
        S->used = 0;
        S->length = hint;
        S->buffer = CALLOC(1, hint);
        *S->buffer = 0;
        return S;
}


/* ----------------------------------------------------------------- Public */


T StringBuffer_new(const char *s) {
        return StringBuffer_append(_ctor(STRLEN), "%s", s);
}


T StringBuffer_create(int hint) {
        if (hint <= 0)
                THROW(AssertException, "Illegal hint value");
        return _ctor(hint);
}


void StringBuffer_free(T *S) {
        assert(S && *S);
        FREE((*S)->buffer);
        FREE((*S)->compressedBuffer);
        FREE(*S);
}


T StringBuffer_append(T S, const char *s, ...) {
        assert(S);
        if (STR_DEF(s)) {
                va_list ap;
                va_start(ap, s);
                _append(S, s, ap);
                va_end(ap);
        }
        return S;
}


T StringBuffer_vappend(T S, const char *s, va_list ap) {
        assert(S);
        if (STR_DEF(s))
                _append(S, s, ap);
        return S;
}


T StringBuffer_trim(T S) {
        assert(S);
        if (S->used == 0)
                return S;
        // Right trim
        uchar_t *end = S->buffer + S->used - 1;
        if (isspace(*end)) {
                while (end >= S->buffer && isspace(*end))
                        end--;
                S->used = (int)(end - S->buffer) + 1;
                S->buffer[S->used] = 0;
        }
        // Left trim
        if (S->used > 0 && isspace(*S->buffer)) {
                uchar_t *start = S->buffer + 1;
                while (isspace(*start)) start++;
                int shift = (int)(start - S->buffer);
                S->used -= shift;
                memmove(S->buffer, start, S->used + 1);
        }
        return S;
}


int StringBuffer_length(T S) {
        assert(S);
        return S->used;
}


T StringBuffer_clear(T S) {
        assert(S);
        S->used = 0;
        *S->buffer = 0;
        FREE(S->compressedBuffer);
        return S;
}


const char *StringBuffer_toString(T S) {
        assert(S);
        return (const char *)S->buffer;
}


const void *StringBuffer_toCompressed(T S, int level, size_t *length) {
        assert(S);
        assert(length);
        assert(level >= 0 && level <= 9);
#ifdef HAVE_LIBZ
        *length = 0;
        if (S->used > 0) {
                z_stream zstream = {};
                zstream.next_in = S->buffer;
                zstream.avail_in = S->used;
                int status = deflateInit2(&zstream, level, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
                if (status == Z_OK) {
                        int need = (int)deflateBound(&zstream, S->used);
                        RESIZE(S->compressedBuffer, need);
                        zstream.next_out = S->compressedBuffer;
                        zstream.avail_out = need;
                        status = deflate(&zstream, Z_FINISH);
                        deflateEnd(&zstream);
                        if (status == Z_STREAM_END) {
                                *length = need - zstream.avail_out;
                                return (const void *)S->compressedBuffer;
                        }
                }
                FREE(S->compressedBuffer);
                THROW(AssertException, "compression failed: %s", zError(status));
        }
#else
        THROW(AssertException, "compression not supported");
#endif
        return NULL;
}

