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

#ifndef MONIT_SERVICE_H
#define MONIT_SERVICE_H

#include "monit.h"


/**
 * Service helpers: lookup in the service list, monitoring state, error
 * state and the reset of the collected service data.
 *
 * @file
 */


/**
 * @param name A service name as stated in the config file
 * @return The named service or NULL if not found
 */
Service_T Service_get(const char *name);


/**
 * @param name A service name as stated in the config file
 * @return true if the service name exists in the service list, otherwise false
 */
bool Service_exists(const char *name);


/**
 * @return The number of services managed by monit (the length of the service list)
 */
int Service_count(void);


/**
 * Are the service status data available? (the service is monitored and
 * the monitored object exists and can be read)
 * @param s The service to test
 * @return true if available otherwise false
 */
bool Service_hasStatus(Service_T s);


/**
 * Does the service have errors?
 * @param s The service to test
 * @return true if any event type of the service is in the failed or changed state, otherwise false
 */
bool Service_hasErrors(Service_T s);


/**
 * Reset the collected service data (the service specific information structure)
 * @param s A Service_T object
 */
void Service_resetInfo(Service_T s);


/**
 * Enable the service monitoring in the case that it was disabled
 * @param s A Service_T object
 */
void Service_monitorSet(Service_T s);


/**
 * Disable the service monitoring in the case that it is enabled. The
 * service errors, events, counters and the collected data are reset.
 * @param s A Service_T object
 */
void Service_monitorUnset(Service_T s);


/**
 * Derive the legacy M/Monit status bitmaps from the per-type service state.
 * The legacy class is reported as "changed" only if every failing member of
 * the class is in the changed state, otherwise it is reported as "failed".
 * @param s A service
 * @param status Output: bitmap of legacy classes with an error (State_Failed or State_Changed)
 * @param hint Output: subset of status which is in the State_Changed state
 */
void Service_legacyStatus(Service_T s, EventClass_T *status, EventClass_T *hint);


#endif
