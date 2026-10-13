/*
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 * Copyright (C) 1994,1995,1996,1997 by David R. Hanson.
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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "Str.h"
#include "system/System.h"
#include "Exception.h"


/**
 * Implementation of the Exception interface. This implementation
 * defines the Thread local Exception stack and all Exceptions used
 * in the system. New Exceptions should also be defined in this class.
 *
 * This implementation is a minor modification of the Except code found
 * in David R. Hanson's excellent book "C Interfaces and Implementations".
 * See http://www.cs.princeton.edu/software/cii/
 *
 * @author https://www.tildeslash.com/
 * @see https://mmonit.com/
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#define T Exception_T
/* Thread local Exception stack */
_Thread_local Exception_Frame *Exception_Stack;
/* System exceptions */
T IOException = {"IOException"};
T AssertException = {"AssertException"};
T MemoryException = {"MemoryException"};
T NumberFormatException = {"NumberFormatException"};
T ProtocolException = {"ProtocolException"};


/* ---------------------------------------------------------------- Public */


void Exception_throw(const T *e, const char *func, const char *file, int line, const char *message) {
        Exception_Frame *p = Exception_Stack;
        assert(e);
        if (p) {
                p->exception = e;
                p->func = func;
                p->file = file;
                p->line = line;
                if (message)
                        Str_copy(p->message, message, EXCEPTION_MESSAGE_LENGTH);
                Exception_Stack = p->prev;
                siglongjmp(p->env, Exception_thrown);
        } else if (message) {
                ABORT("%s: %s\n raised in %s at %s:%d\n", e->name, message, func ? func : "?", file ? file : "?", line);
        } else {
                ABORT("%s: 0x%p\n raised in %s at %s:%d\n", e->name, e, func ? func : "?", file ? file : "?", line);
        }
}


void Exception_vthrow(const T *e, const char *func, const char *file, int line, const char *cause, ...) {
        char message[EXCEPTION_MESSAGE_LENGTH];
        if (cause) {
                va_list ap;
                va_start(ap, cause);
                vsnprintf(message, EXCEPTION_MESSAGE_LENGTH, cause, ap);
                va_end(ap);
                Exception_throw(e, func, file, line, message);
        } else {
                Exception_throw(e, func, file, line, NULL);
        }
}

