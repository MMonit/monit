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


#ifndef INT_INCLUDED
#define INT_INCLUDED


/**
 * General purpose <b>Int</b> utility <b>class methods</b>.
 *
 * @author https://www.tildeslash.com/
 * @see https://mmonit.com/
 * @file
 */


/**
 * Return the delta between the previous and current counter readings.
 * Handle the wrap-around of the counter.
 * @param previous The previous counter reading.
 * @param current The next counter reading.
 * @return delta
 */
uint64_t Int_deltaUINT64(uint64_t previous, uint64_t current);


#endif

