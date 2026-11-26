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


#ifndef URL_INCLUDED
#define URL_INCLUDED


/**
 * <b>URL</b> class methods.
 *
 * @author https://tildeslash.com
 * @see https://mmonit.com
 * @file
 */


/**
 * Unescape a URL string. The `url` parameter is modified
 * by this method. This method converts '+' -> ' ' and hex-encoded %xx
 * substrings to the corresponding character. A '%' that is not followed
 * by two hex digits, and %00, are kept as they are.
 * @param url An escaped URL string
 * @return A pointer to the unescaped `url` string
 */
char *URL_unescape(char *url);


/**
 * Escape a URL string, converting unsafe characters to a hex (%%HEXHEX)
 * representation. The following URL unsafe characters are encoded:
 * ```
 * ><\"#%%{}|\\^ []`
 * ```
 * as well as characters in the interval 00-1F hex (0-31 decimal) and
 * in the interval 7F-FF (127-255 decimal).
 * *The caller must free the returned string.* If the `url`
 * parameter is NULL this method returns NULL; if it is the empty
 * string "", a *new* empty string is returned.
 * @param url A URL string
 * @return The escaped string
 */
char *URL_escape(const char *url);


/**
 * Escape a URL string that may be escaped already, in whole or in part.
 * Works as URL_escape(), except that a '%' followed by two hex digits
 * is an escape sequence and is copied as it is. Any other '%' becomes
 * %25. For instance, `/a b%20c%` gives `/a%20b%20c%25`. Escaping the
 * result again changes nothing.
 * *The caller must free the returned string.* If the `url`
 * parameter is NULL this method returns NULL; if it is the empty
 * string "", a *new* empty string is returned.
 * @param url A URL string
 * @return The escaped string
 */
char *URL_ensureEscaped(const char *url);


/**
 * Returns true if the given char is URL unsafe.
 * @param c An unsigned char
 * @return true if the char is in the set of unsafe URL characters
 */
bool URL_isunsafe(unsigned char c);


/**
 * Tests whether a URL string is absolute. The definition of an
 * absolute URL is a URL that starts with `<protocol>://`.
 * @param url The URL string to test
 * @return true if the URL is absolute, otherwise false
 */
bool URL_isabsolute(const char *url);


#endif
