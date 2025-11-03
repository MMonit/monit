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


/**
 * Implementation of the Int interface
 *
 * @author https://www.tildeslash.com/
 * @see https://mmonit.com/
 * @file
 */


/* -------------------------------------------------------- Public Methods */


uint64_t Int_deltaUINT64(uint64_t previous, uint64_t current) {
        if (current < previous) {
                // Counter wrapped
                return (UINT64_MAX - previous) + current + 1;
        }
        return current - previous;
}

