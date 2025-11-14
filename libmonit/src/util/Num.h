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


#ifndef NUM_INCLUDED
#define NUM_INCLUDED
#include <stdint.h>


/**
 * General purpose <b>Numeric</b> utility <b>class methods</b>.
 *
 * @author https://www.tildeslash.com/
 * @see https://mmonit.com/
 * @file
 */


/**
 * @brief Return the minimum of two values
 * 
 * Works with any numeric type (integral or floating-point). Uses compound statement 
 * expression to ensure type safety and avoid double evaluation.
 * 
 * @param a First value
 * @param b Second value
 * @return The smaller of the two values
 */
#define Num_min(a, b) ({ \
    __auto_type _a = (a); \
    __auto_type _b = (b); \
    _a < _b ? _a : _b; \
})


/**
 * @brief Return the maximum of two values
 * 
 * Works with any numeric type (integral or floating-point). Uses compound statement 
 * expression to ensure type safety and avoid double evaluation.
 * 
 * @param a First value
 * @param b Second value
 * @return The larger of the two values
 */
#define Num_max(a, b) ({ \
    __auto_type _a = (a); \
    __auto_type _b = (b); \
    _a > _b ? _a : _b; \
})


/**
 * @brief Calculate the delta between monotonically increasing unsigned counter readings
 * 
 * This type-generic macro correctly handles counter wrap-around for uint8_t, uint16_t, 
 * uint32_t, and uint64_t types. When current < previous, it assumes wrap-around and 
 * calculates the delta across the boundary.
 * 
 * Example: For uint8_t with previous=250, current=5, returns 11 (not -245) because 
 * the counter wrapped: 250→255(+5) then 0→5(+6) = 11 total
 * 
 * @param previous The previous counter reading (unsigned integer type)
 * @param current The current counter reading (same type as previous)
 * @return The delta, accounting for wrap-around (same type as input)
 */
#define Num_udelta(previous, current) _Generic((previous), \
    uint8_t:  ((current) < (previous) ? (UINT8_MAX - (previous)) + (current) + 1 : (current) - (previous)), \
    uint16_t: ((current) < (previous) ? (UINT16_MAX - (previous)) + (current) + 1 : (current) - (previous)), \
    uint32_t: ((current) < (previous) ? (UINT32_MAX - (previous)) + (current) + 1 : (current) - (previous)), \
    uint64_t: ((current) < (previous) ? (UINT64_MAX - (previous)) + (current) + 1 : (current) - (previous)) \
)

/**
 * @brief Calculate the absolute difference between two values
 * 
 * Returns the positive distance between two values regardless of order. Works with any 
 * numeric type (integral or floating-point). Unlike Num_udelta, this does NOT handle 
 * counter wrap-around.
 * 
 * Example: Num_delta(10, 7) returns 3, and Num_delta(7, 10) also returns 3
 * 
 * @param a First value
 * @param b Second value
 * @return The absolute difference |a - b|
 */
#define Num_delta(a, b) ({ \
    __auto_type _a = (a); \
    __auto_type _b = (b); \
    _a < _b ? _b - _a : _a - _b; \
})


#endif // !NUM_INCLUDED

