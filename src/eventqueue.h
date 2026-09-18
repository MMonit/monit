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

#ifndef MONIT_EVENTQUEUE_H
#define MONIT_EVENTQUEUE_H

#include "monit.h"
#include "event.h"


/**
 * The event queue: events whose handlers (alert, M/Monit) failed are
 * stored in files in the event queue directory (see "set eventqueue") and
 * the failed handlers are retried each cycle until they succeed. The files
 * hold the event structure (see EVENT_VERSION in event.h), the event source
 * name, the message and the action. Files written by the previous event
 * structure version are supported as well.
 *
 * @file
 */


/**
 * Add the partially handled event to the queue
 * @param E An event object
 */
void EventQueue_add(Event_T E);


/**
 * Reprocess the partially handled events in the queue
 */
void EventQueue_process(void);


#endif
