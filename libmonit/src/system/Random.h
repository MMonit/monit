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


#ifndef RANDOM_INCLUDED
#define RANDOM_INCLUDED


/**
 * Random routines
 *
 * @author https://www.tildeslash.com/
 * @see https://mmonit.com/
 * @file
 */


/**
 * @brief Fills the specified buffer with random bytes.
 *
 * Uses the strongest available random number generator on the system.
 * On platforms with cryptographically secure PRNGs, the data is suitable
 * for most security purposes. If only weak PRNGs are available, this may
 * not be true; see implementation notes.
 *
 * @param buf The pointer to the buffer to fill with random bytes.
 * @param nbytes The number of bytes to fill.
 * @return true if successful (buffer is filled with random data), false if an error occurs.
 */
bool Random_bytes(void *buf, size_t nbytes);


/**
 * @brief Returns a random unsigned 64-bit integer.
 *
 * The value is generated using the platform's strongest available random source.
 * No specific range is guaranteed except the full width of 64 bits.
 *
 * @return A random unsigned 64-bit integer.
 */
unsigned long long Random_number(void);


/**
 * @brief Returns a uniform random unsigned 64-bit integer in the range [min, max] (inclusive).
 *
 * @param min The lower bound of the range (inclusive).
 * @param max The upper bound of the range (inclusive).
 * If min == max, returns min.
 * @return A random number N such that min <= N <= max.
 * @exception AssertException If called with min > max
 */
unsigned long long Random_range(unsigned long long min, unsigned long long max);


#endif // !RANDOM_INCLUDED

