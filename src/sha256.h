/*
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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

#ifndef SHA256_H
#define SHA256_H


#define SHA256_DIGEST_SIZE 32

typedef struct {
    unsigned int state[8];
    unsigned int count[2];
    unsigned char buffer[64];
} sha256_context_t;

void sha256_init(sha256_context_t *context);
void sha256_append(sha256_context_t *context, const unsigned char *data, const size_t len);
void sha256_finish(sha256_context_t *context, unsigned char digest[SHA256_DIGEST_SIZE]);


#endif
