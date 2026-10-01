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

#include "config.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif


#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#include "monit.h"
#include "alert.h"
#include "event.h"
#include "service.h"
#include "state.h"
#include "eventqueue.h"
#include "MMonit.h"
#include "spawn.h"

/**
 * Implementation of the event interface.
 *
 * @file
 */


/* ------------------------------------------------------------- Definitions */

/*
 * Event table, indexed by Event_Type: the rows MUST be kept in the Event_Type order (Event_Table[id].id == id).
 */
const EventTable_T Event_Table[Event_Last + 1] = {
        {Event_Null,                 EventClass_Null,       "null",                 "No Event",                                "No Event",                              "No Event",                              "No Event",                                  State_None},
        {Event_Action,               EventClass_Action,     "action",               "Action done",                             "Action done",                           "Action done",                           "Action done",                               State_None},
        {Event_Checksum,             EventClass_Checksum,   "checksum",             "Checksum failed",                         "Checksum succeeded",                    "Checksum changed",                      "Checksum not changed",                      State_None},
        {Event_Connection,           EventClass_Connection, "connection",           "Connection failed",                       "Connection succeeded",                  "Connection changed",                    "Connection not changed",                    State_Changed},
        {Event_Content,              EventClass_Content,    "content",              "Content failed",                          "Content succeeded",                     "Content match",                         "Content doesn't match",                     State_Changed},
        {Event_Data,                 EventClass_Data,       "data",                 "Data access error",                       "Data access succeeded",                 "Data access changed",                   "Data access not changed",                   State_None},
        {Event_Exec,                 EventClass_Exec,       "exec",                 "Execution failed",                        "Execution succeeded",                   "Execution changed",                     "Execution not changed",                     State_None},
        {Event_Exist,                EventClass_Exist,      "exist",                "Does exist",                              "Exists not",                            "Existence changed",                     "Existence not changed",                     State_None},
        {Event_FsFlag,               EventClass_FsFlag,     "fsflags",              "Filesystem flags failed",                 "Filesystem flags succeeded",            "Filesystem flags changed",              "Filesystem flags not changed",              State_None},
        {Event_Gid,                  EventClass_Gid,        "gid",                  "GID failed",                              "GID succeeded",                         "GID changed",                           "GID not changed",                           State_None},
        {Event_Heartbeat,            EventClass_Heartbeat,  "heartbeat",            "Heartbeat failed",                        "Heartbeat succeeded",                   "Heartbeat changed",                     "Heartbeat not changed",                     State_None},
        {Event_Icmp,                 EventClass_Icmp,       "icmp",                 "ICMP failed",                             "ICMP succeeded",                        "ICMP changed",                          "ICMP not changed",                          State_None},
        {Event_Instance,             EventClass_Instance,   "instance",             "Monit instance failed",                   "Monit instance succeeded",              "Monit instance changed",                "Monit instance not changed",                State_None},
        {Event_Invalid,              EventClass_Invalid,    "invalid",              "Invalid type",                            "Type succeeded",                        "Type changed",                          "Type not changed",                          State_None},
        {Event_NonExist,             EventClass_NonExist,   "nonexist",             "Does not exist",                          "Exists",                                "Existence changed",                     "Existence not changed",                     State_None},
        {Event_Permission,           EventClass_Permission, "permission",           "Permission failed",                       "Permission succeeded",                  "Permission changed",                    "Permission not changed",                    State_None},
        {Event_Pid,                  EventClass_Pid,        "pid",                  "PID failed",                              "PID succeeded",                         "PID changed",                           "PID not changed",                           State_None},
        {Event_PPid,                 EventClass_PPid,       "ppid",                 "PPID failed",                             "PPID succeeded",                        "PPID changed",                          "PPID not changed",                          State_None},
        {Event_Size,                 EventClass_Size,       "size",                 "Size failed",                             "Size succeeded",                        "Size changed",                          "Size not changed",                          State_Changed},
        {Event_Timeout,              EventClass_Timeout,    "timeout",              "Timeout",                                 "Timeout recovery",                      "Timeout changed",                       "Timeout not changed",                       State_None},
        {Event_Uid,                  EventClass_Uid,        "uid",                  "UID failed",                              "UID succeeded",                         "UID changed",                           "UID not changed",                           State_None},
        {Event_Uptime,               EventClass_Uptime,     "uptime",               "Uptime failed",                           "Uptime succeeded",                      "Uptime changed",                        "Uptime not changed",                        State_None},
        {Event_ByteIn,               EventClass_ByteIn,     "bytein",               "Download bytes exceeded",                 "Download bytes ok",                     "Download bytes changed",                "Download bytes not changed",                State_None},
        {Event_ByteOut,              EventClass_ByteOut,    "byteout",              "Upload bytes exceeded",                   "Upload bytes ok",                       "Upload bytes changed",                  "Upload bytes not changed",                  State_None},
        {Event_PacketIn,             EventClass_PacketIn,   "packetin",             "Download packets exceeded",               "Download packets ok",                   "Download packets changed",              "Download packets not changed",              State_None},
        {Event_PacketOut,            EventClass_PacketOut,  "packetout",            "Upload packets exceeded",                 "Upload packets ok",                     "Upload packets changed",                "Upload packets not changed",                State_None},
        {Event_Cpu,                  EventClass_Resource,   "cpu",                  "CPU usage matched limit",                 "CPU usage ok",                          "CPU usage changed",                     "CPU usage not changed",                     State_None},
        {Event_CpuTotal,             EventClass_Resource,   "totalcpu",             "Total CPU usage matched limit",           "Total CPU usage ok",                    "Total CPU usage changed",               "Total CPU usage not changed",               State_None},
        {Event_Memory,               EventClass_Resource,   "memory",               "Memory usage matched limit",              "Memory usage ok",                       "Memory usage changed",                  "Memory usage not changed",                  State_None},
        {Event_MemoryTotal,          EventClass_Resource,   "totalmemory",          "Total memory usage matched limit",        "Total memory usage ok",                 "Total memory usage changed",            "Total memory usage not changed",            State_None},
        {Event_Swap,                 EventClass_Resource,   "swap",                 "Swap usage matched limit",                "Swap usage ok",                         "Swap usage changed",                    "Swap usage not changed",                    State_None},
        {Event_LoadAverage1m,        EventClass_Resource,   "loadavg1m",            "Load average (1min) matched limit",       "Load average (1min) ok",                "Load average (1min) changed",           "Load average (1min) not changed",           State_None},
        {Event_LoadAverage5m,        EventClass_Resource,   "loadavg5m",            "Load average (5min) matched limit",       "Load average (5min) ok",                "Load average (5min) changed",           "Load average (5min) not changed",           State_None},
        {Event_LoadAverage15m,       EventClass_Resource,   "loadavg15m",           "Load average (15min) matched limit",      "Load average (15min) ok",               "Load average (15min) changed",          "Load average (15min) not changed",          State_None},
        {Event_Threads,              EventClass_Resource,   "threads",              "Threads count matched limit",             "Threads count ok",                      "Threads count changed",                 "Threads count not changed",                 State_None},
        {Event_Children,             EventClass_Resource,   "children",             "Children count matched limit",            "Children count ok",                     "Children count changed",                "Children count not changed",                State_None},
        {Event_Filedescriptors,      EventClass_Resource,   "filedescriptors",      "Filedescriptors usage matched limit",     "Filedescriptors usage ok",              "Filedescriptors usage changed",         "Filedescriptors usage not changed",         State_None},
        {Event_FiledescriptorsTotal, EventClass_Resource,   "totalfiledescriptors", "Total filedescriptors usage matched limit", "Total filedescriptors usage ok",      "Total filedescriptors usage changed",   "Total filedescriptors usage not changed",   State_None},
        {Event_ReadBytes,            EventClass_Resource,   "readbytes",            "Read rate matched limit",                 "Read rate ok",                          "Read rate changed",                     "Read rate not changed",                     State_None},
        {Event_ReadOperations,       EventClass_Resource,   "readoperations",       "Read operations rate matched limit",      "Read operations rate ok",               "Read operations rate changed",          "Read operations rate not changed",          State_None},
        {Event_WriteBytes,           EventClass_Resource,   "writebytes",           "Write rate matched limit",                "Write rate ok",                         "Write rate changed",                    "Write rate not changed",                    State_None},
        {Event_WriteOperations,      EventClass_Resource,   "writeoperations",      "Write operations rate matched limit",     "Write operations rate ok",              "Write operations rate changed",         "Write operations rate not changed",         State_None},
        {Event_ServiceTime,          EventClass_Resource,   "servicetime",          "Service time matched limit",              "Service time ok",                       "Service time changed",                  "Service time not changed",                  State_None},
        {Event_Space,                EventClass_Resource,   "space",                "Space usage matched limit",               "Space usage ok",                        "Space usage changed",                   "Space usage not changed",                   State_None},
        {Event_Inode,                EventClass_Resource,   "inode",                "Inode usage matched limit",               "Inode usage ok",                        "Inode usage changed",                   "Inode usage not changed",                   State_None},
        {Event_Hardlink,             EventClass_Resource,   "hardlink",             "Hardlink failed",                         "Hardlink succeeded",                    "Hardlink changed",                      "Hardlink not changed",                      State_None},
        {Event_Pagein,               EventClass_Resource,   "pagein",               "Swap pagein matched limit",               "Swap pagein ok",                        "Swap pagein changed",                   "Swap pagein not changed",                   State_None},
        {Event_Pageout,              EventClass_Resource,   "pageout",              "Swap pageout matched limit",              "Swap pageout ok",                       "Swap pageout changed",                  "Swap pageout not changed",                  State_None},
        {Event_Timestamp,            EventClass_Timestamp,  "timestamp",            "Timestamp failed",                        "Timestamp succeeded",                   "Timestamp changed",                     "Timestamp not changed",                     State_Changed},
        {Event_TimestampAccess,      EventClass_Timestamp,  "atime",                "Access timestamp failed",                 "Access timestamp succeeded",            "Access timestamp changed",              "Access timestamp not changed",              State_Changed},
        {Event_TimestampChange,      EventClass_Timestamp,  "ctime",                "Change timestamp failed",                 "Change timestamp succeeded",            "Change timestamp changed",              "Change timestamp not changed",              State_Changed},
        {Event_TimestampModify,      EventClass_Timestamp,  "mtime",                "Modify timestamp failed",                 "Modify timestamp succeeded",            "Modify timestamp changed",              "Modify timestamp not changed",              State_Changed},
        {Event_Certificate,          EventClass_Timestamp,  "certificate",          "Certificate validity failed",             "Certificate validity succeeded",        "Certificate changed",                   "Certificate not changed",                   State_None},
        {Event_LinkStatus,           EventClass_Link,       "link",                 "Link down",                               "Link up",                               "Link changed",                          "Link not changed",                          State_None},
        {Event_LinkErrorsIn,         EventClass_Link,       "linkerrorsin",         "Download errors detected",                "Download errors ok",                    "Download errors changed",               "Download errors not changed",               State_None},
        {Event_LinkErrorsOut,        EventClass_Link,       "linkerrorsout",        "Upload errors detected",                  "Upload errors ok",                      "Upload errors changed",                 "Upload errors not changed",                 State_None},
        {Event_ResponseTime,         EventClass_Speed,      "responsetime",         "Response time failed",                    "Response time ok",                      "Response time changed",                 "Response time not changed",                 State_None},
        {Event_LinkSpeed,            EventClass_Speed,      "linkspeed",            "Link speed failed",                       "Link speed ok",                         "Link speed changed",                    "Link speed not changed",                    State_Changed},
        {Event_LinkDuplex,           EventClass_Speed,      "linkduplex",           "Link duplex failed",                      "Link duplex ok",                        "Link duplex changed",                   "Link duplex not changed",                   State_Changed},
        {Event_LinkSaturation,       EventClass_Saturation, "saturation",           "Saturation exceeded",                     "Saturation ok",                         "Saturation changed",                    "Saturation not changed",                    State_None},
        {Event_LinkSaturationIn,     EventClass_Saturation, "saturationin",         "Download saturation exceeded",            "Download saturation ok",                "Download saturation changed",           "Download saturation not changed",           State_None},
        {Event_LinkSaturationOut,    EventClass_Saturation, "saturationout",        "Upload saturation exceeded",              "Upload saturation ok",                  "Upload saturation changed",             "Upload saturation not changed",             State_None},
        {Event_Euid,                 EventClass_Uid,        "euid",                 "EUID failed",                             "EUID succeeded",                        "EUID changed",                          "EUID not changed",                          State_None},
        {Event_ProgramOutput,        EventClass_Content,    "programoutput",        "Program output failed",                   "Program output succeeded",              "Program output match",                  "Program output doesn't match",              State_None},
        {Event_Status,               EventClass_Status,     "status",               "Status failed",                           "Status succeeded",                      "Status changed",                        "Status not changed",                        State_None},
        {Event_Spawn,                EventClass_Status,     "spawn",                "Program start failed",                    "Program started",                       "Program start changed",                 "Program start not changed",                 State_None},
        {Event_Zombie,               EventClass_Data,       "zombie",               "Process is a zombie",                     "Zombie check succeeded",                "Zombie state changed",                  "Zombie state not changed",                  State_None},
        {Event_SecurityAttribute,    EventClass_Invalid,    "securityattribute",    "Security attribute failed",               "Security attribute succeeded",          "Security attribute changed",            "Security attribute not changed",            State_None}
};


/*
 * Descriptions of the legacy event classes which were shared by several tests in the monit < 6.1.0 event format (v4). Used for events converted from the
 * version 4 queue files, where the specific test is not known. Classes not listed here map one-to-one to an event type with the same descriptions.
 */
static const struct {
        EventClass_T class;
        const char *description_failed;
        const char *description_succeeded;
        const char *description_changed;
        const char *description_changednot;
} _legacyDescriptions[] = {
        {EventClass_Resource,   "Resource limit matched", "Resource limit succeeded", "Resource limit changed", "Resource limit not changed"},
        {EventClass_Timestamp,  "Timestamp failed",       "Timestamp succeeded",      "Timestamp changed",      "Timestamp not changed"},
        {EventClass_Link,       "Link down",              "Link up",                  "Link changed",           "Link not changed"},
        {EventClass_Speed,      "Speed failed",           "Speed ok",                 "Speed changed",          "Speed not changed"},
        {EventClass_Saturation, "Saturation exceeded",    "Saturation ok",            "Saturation changed",     "Saturation not changed"},
        {EventClass_Uid,        "UID failed",             "UID succeeded",            "UID changed",            "UID not changed"},
        {EventClass_Content,    "Content failed",         "Content succeeded",        "Content match",          "Content doesn't match"},
        {EventClass_Status,     "Status failed",          "Status succeeded",         "Status changed",         "Status not changed"},
        {EventClass_Data,       "Data access error",      "Data access succeeded",    "Data access changed",    "Data access not changed"},
        {EventClass_Invalid,    "Invalid type",           "Type succeeded",           "Type changed",           "Type not changed"},
        {EventClass_Null,       NULL,                     NULL,                       NULL,                     NULL}
};


/* ----------------------------------------------------------------- Private */


static bool _legacyDescription(EventClass_T class, const char **failed, const char **succeeded, const char **changed, const char **changednot) {
        for (int i = 0; _legacyDescriptions[i].class != EventClass_Null; i++) {
                if (_legacyDescriptions[i].class == class) {
                        *failed = _legacyDescriptions[i].description_failed;
                        *succeeded = _legacyDescriptions[i].description_succeeded;
                        *changed = _legacyDescriptions[i].description_changed;
                        *changednot = _legacyDescriptions[i].description_changednot;
                        return true;
                }
        }
        return false;
}


static void _saveState(Event_Type id, State_Type state) {
        if (Event_Table[id].saveState & state)
                State_dirty();
}


/* Restart the state map on a state transition so the next change requires a full window of cycles to accumulate. */
static void _resetStateMap(Event_T E, State_Type currentState) {
        E->state_map = (currentState == State_Failed) ? ~0ULL : 0ULL;
}


/**
 * Return true if the posted state S represents a state change for event E,
 * taking the configured occurrence watermark into account.
 */
static bool _checkState(Event_T E, State_Type S) {
        assert(E);

        /* Translate the posted state to a 0/1 (succeeded/failed) class */
        State_Type currentState = (S == State_Succeeded || S == State_ChangedNot) ? State_Succeeded : State_Failed;

        /* Only failed/changed state condition can change the initial state. While the event is still in the State_Init
         * phase (the error didn't accumulate enough cycles to pass the "for X cycles" watermark), a success just clears
         * the pending soft error and must not be reported as a state change. No failure was reported yet, so there is
         * nothing to recover from */
        if (currentState == State_Succeeded && E->state == State_Init)
                return false;

        /* Internal instance and action events are reported on every occurrence */
        if (E->id == Event_Instance || E->id == Event_Action) {
                _resetStateMap(E, currentState);
                return true;
        }

        /* The action may require multiple errors/successes before switching state.
         * Count how many of the last 'cycles' samples match the posted state. */
        Action_T action = (currentState == State_Succeeded) ? E->action->succeeded : E->action->failed;
        int matchingStateCounter = 0;
        for (int i = 0; i < action->cycles; i++) {
                if (((E->state_map >> i) & 0x1) == currentState)
                        matchingStateCounter++;
        }

        if (matchingStateCounter >= action->count && (S != E->state || S == State_Changed)) {
                _resetStateMap(E, currentState);
                return true;
        }

        return false;
}


static void _handleAction(Event_T E, Action_T A) {
        assert(E);
        assert(A);

        E->flag = Handler_Succeeded;

        if (A->id != Action_Ignored) {
                // As PID 1, once the shutdown has set the stop deadline, events are only logged, except that M/Monit gets "Monit stopped"
                if (Run.stopDeadline) {
                        if (E->id == Event_Instance)
                                MMonit_send(E);
                        return;
                }
                /* Alert and mmonit event notification are common actions */
                E->flag |= MMonit_send(E);
                E->flag |= handle_alert(E);
                /* In the case that some subhandler failed, enqueue the event for partial reprocessing */
                if (E->flag != Handler_Succeeded) {
                        if (Run.eventlist_dir)
                                EventQueue_add(E);
                        else
                                Log_error("Aborting event\n");
                }
                // Once a stop is pending as PID 1, the services are only stopped, in order, by do_exit()
                if (shutdown_pending())
                        return;
                /* Action event is handled already. For Instance events we don't want actions like stop to be executed to prevent the disabling of system service monitoring */
                if (A->id == Action_Alert || E->id == Event_Instance) {
                        return;
                } else if (A->id == Action_Exec) {
                        if (E->state_changed || (E->state && A->repeat && E->count % A->repeat == 0)) {
                                Log_info("'%s' exec: '%s'\n", E->source->name, Util_commandDescription(A->exec, (char[STRLEN]){}));
                                char spawn_error[STRLEN] = {"?"};
                                if (spawn(&(struct spawn_args_t){
                                        .S = E->source,
                                        .cmd = A->exec,
                                        .E = E,
                                        .err = spawn_error,
                                        .errlen = STRLEN
                                }) < 0) {
                                        Log_error("'%s' exec failed -- '%s'\n", E->source->name, spawn_error);
                                }
                                return;
                        }
                } else {
                        if (E->source->actionratelist && (A->id == Action_Start || A->id == Action_Restart)) {
                                E->source->nstart++;
                                State_dirty();
                        }
                        if (E->source->mode == Monitor_Passive && (A->id == Action_Start || A->id == Action_Stop  || A->id == Action_Restart))
                                return;
                        // Warning: the control_service() on stop/unmonitor may free the list of events linked to this service, including this event => do not use event after this call:
                        control_service(E->source->name, A->id);
                }
        }
}


static void _handleEvent(Service_T S, Event_T E) {
        assert(E);
        assert(E->action);
        assert(E->action->failed);
        assert(E->action->succeeded);

        bool internalEvent = (E->id == Event_Instance || E->id == Event_Action);
        bool lastSampleFailed = (E->state_map & 0x1);

        if (E->message) {
                if (internalEvent) {
                        // Internal "Instance" change and "Action" events are always logged at info level
                        Log_info("'%s' %s\n", S->name, E->message);
                } else if (E->state == State_Init || E->state == State_Succeeded || E->state == State_ChangedNot) {
                        if (lastSampleFailed) {
                                // Failure that hasn't reached the error threshold yet is a warning, otherwise a success
                                Log_warning("'%s' %s\n", S->name, E->message);
                        } else if (E->state_changed) {
                                // Failure -> Success transition (this flag is always false in the case of State_Init)
                                Log_info("'%s' %s\n", S->name, E->message);
                        } else {
                                // The service is OK and this is another Success event
                                DEBUG("'%s' %s\n", S->name, E->message);
                        }
                } else {
                        Log_error("'%s' %s\n", S->name, E->message);
                }
        }

        if (E->state == State_Failed || E->state == State_Changed || lastSampleFailed /* error during State_Init or State_Succeeded with not enough X in 'for X cycles' */) {
                if (! internalEvent) {
                        /* Record the error state of this event type on the service: failed or changed */
                        S->status[E->id] = (E->state == State_Changed) ? State_Changed : State_Failed;
                }
                if (E->state != State_Init && E->state != State_Succeeded) {
                        /* During the multi-error init phase, keep the error flag set but skip the action */
                        _handleAction(E, E->action->failed);
                }
        } else {
                /* Only clear the service error flag if no other event with the same id still has an active failure
                 * (multiple rules may share the same event type, e.g. resource usage events) */
                bool otherActive = false;
                for (Event_T o = S->eventlist; o; o = o->next) {
                        if (o != E && o->id == E->id && (o->state_map & 0x1)) {
                                otherActive = true;
                                break;
                        }
                }
                if (! otherActive)
                        S->status[E->id] = State_Succeeded;
                if (E->state != State_Init) {
                        _handleAction(E, E->action->succeeded);
                }
        }
}


#if defined(__clang__) && defined(__clang_major__) && __clang_major__ >= 12
__attribute__((no_sanitize("unsigned-integer-overflow", "unsigned-shift-base")))
#elif defined(__clang__) && defined(__clang_major__) && __clang_major__ >= 4
__attribute__((no_sanitize("unsigned-integer-overflow")))
#endif
static unsigned long long left_shift(unsigned long long v) {
        return v << 1;
}


/* ------------------------------------------------------------------ Public */


const char *Event_description(Event_T E) {
        assert(E);
        const char *failed, *succeeded, *changed, *changednot;
        if (E->legacy && _legacyDescription(Event_Table[E->id].class, &failed, &succeeded, &changed, &changednot)) {
                /* Event converted from the version 4 queue file: the specific test is not known, use the description of the legacy class */
        } else {
                const EventTable_T *et = &Event_Table[E->id];
                failed = et->description_failed;
                succeeded = et->description_succeeded;
                changed = et->description_changed;
                changednot = et->description_changednot;
        }
        switch (E->state) {
                case State_Succeeded:
                        return succeeded;
                case State_Failed:
                case State_Init:
                        return failed;
                case State_Changed:
                        return changed;
                case State_ChangedNot:
                        return changednot;
                default:
                        return NULL;
        }
}


/**
 * Post a new Event
 * @param service The Service the event belongs to
 * @param id The event identification
 * @param state The event state
 * @param action Description of the event action
 * @param s Optional message describing the event
 */
void Event_post(Service_T service, Event_Type id, State_Type state, EventAction_T action, const char *s, ...) {
        assert(service);
        assert(action);
        assert(s);
        assert(id > Event_Null && id <= Event_Last);
        assert(state == State_Failed || state == State_Succeeded || state == State_Changed || state == State_ChangedNot);

        // A check that a stop request cut short has no result to report or act on
        if (shutdown_pending())
                return;

        _saveState(id, state);

        va_list ap;
        va_start(ap, s);
        char *message = Str_vcat(s, ap);
        va_end(ap);

        Event_T e = service->eventlist;
        while (e) {
                if (e->action == action && e->id == id) {
                        gettimeofday(&e->collected, NULL);

                        /* Shift the existing event flags to the left and set the first bit based on actual state */
                        e->state_map = left_shift(e->state_map);
                        e->state_map |= ((state == State_Succeeded || state == State_ChangedNot) ? 0 : 1);

                        /* Update the message */
                        FREE(e->message);
                        e->message = message;
                        break;
                }
                e = e->next;
        }
        if (! e) {
                /* Only first failed/changed event can initialize the queue for given event type, thus succeeded events are ignored until first error. */
                if (state == State_Succeeded || state == State_ChangedNot) {
                        DEBUG("'%s' %s\n", service->name, message);
                        FREE(message);
                        return;
                }
                /* Initialize the event. The mandatory information is cloned so the event is as standalone as possible and may be saved
                 * to the queue without the dependency on the original service, thus persistent and manageable across monit restarts */
                NEW(e);
                e->id = id;
                gettimeofday(&e->collected, NULL);
                e->source = service;
                e->mode = service->mode;
                e->type = service->type;
                e->state = State_Init;
                e->state_map = 1;
                e->action = action;
                e->message = message;
                e->next = service->eventlist;
                service->eventlist = e;
        }
        e->state_changed = _checkState(e, state);
        /* In the case that the state changed, update it and reset the counter */
        if (e->state_changed) {
                e->state = state;
                e->count = 1;
        } else {
                e->count++;
        }
        _handleEvent(service, e);
}


/**
 * Get a textual description of actual event type.
 * @param E An event object
 * @return A string describing the event type in clear text. If the
 * event type is not found NULL is returned.
 */


Event_Type Event_byName(const char *name) {
        if (name) {
                for (int i = 1; i <= Event_Last; i++)
                        if (Str_isEqual(Event_Table[i].name, name))
                                return Event_Table[i].id;
        }
        return Event_Null;
}


/* ---------------------------------------------------------------- EventSet */


void EventSet_setAll(EventSet_T *set) {
        assert(set);
        for (int i = 1; i <= Event_Last; i++)
                EventSet_set(set, i);
}


void EventSet_setClass(EventSet_T *set, EventClass_T class) {
        assert(set);
        for (int i = 1; i <= Event_Last; i++)
                if (Event_Table[i].class & class)
                        EventSet_set(set, i);
}


void EventSet_negate(EventSet_T *set) {
        assert(set);
        for (int i = 1; i <= Event_Last; i++) {
                if (EventSet_has(set, i))
                        EventSet_clear(set, i);
                else
                        EventSet_set(set, i);
        }
}


bool EventSet_isEmpty(const EventSet_T *set) {
        assert(set);
        for (int i = 1; i <= Event_Last; i++)
                if (EventSet_has(set, i))
                        return false;
        return true;
}


bool EventSet_isAll(const EventSet_T *set) {
        assert(set);
        for (int i = 1; i <= Event_Last; i++)
                if (! EventSet_has(set, i))
                        return false;
        return true;
}


char *EventSet_describe(const EventSet_T *set, char *buf, int len) {
        assert(set);
        assert(buf);
        assert(len > 0);
        *buf = 0;
        if (EventSet_isEmpty(set)) {
                snprintf(buf, len, "No events");
        } else if (EventSet_isAll(set)) {
                snprintf(buf, len, "All events");
        } else {
                char *p = buf;
                for (int i = 1; i <= Event_Last; i++) {
                        if (EventSet_has(set, i)) {
                                int n = snprintf(p, len - (p - buf), "%s ", Event_Table[i].name);
                                if (n < 0 || n >= len - (p - buf))
                                        break;
                                p += n;
                        }
                }
        }
        return buf;
}


/**
 * Get the action of the event in the event's state
 * @param E An event object
 * @return An action id
 */
Action_Type Event_action(Event_T E) {
        assert(E);
        Action_T A = NULL;
        switch (E->state) {
                case State_Succeeded:
                case State_ChangedNot:
                        A = E->action->succeeded;
                        break;
                case State_Failed:
                case State_Changed:
                case State_Init:
                        A = E->action->failed;
                        break;
                default:
                        Log_error("Invalid event state: %d\n", E->state);
                        return Action_Ignored;
        }
        if (! A)
                return Action_Ignored;
        /* In the case of passive mode we replace the description of start, stop or restart action for alert action, because these actions are passive in this mode */
        return (E->mode == Monitor_Passive && ((A->id == Action_Start) || (A->id == Action_Stop) || (A->id == Action_Restart))) ? Action_Alert : A->id;
}


/**
 * Get a textual description of the event's action
 * @param E An event object
 * @return A string describing the action in clear text
 */
const char *Event_actionDescription(Event_T E) {
        assert(E);
        return Action_Names[Event_action(E)];
}


