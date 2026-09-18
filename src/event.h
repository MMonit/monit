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

#ifndef MONIT_EVENT_H
#define MONIT_EVENT_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>


/*
 * Event type definitions.
 */


/**
 * Legacy event classes. This is the original 31-bit event bitmask which is
 * frozen for backward compatibility: it is sent to M/Monit in the XML
 * <status>, <status_hint> and <event><id> elements.
 *
 * Never add values here, add a new Event_Type instead and map it to the closest class.
 */
typedef enum {
        EventClass_Null       = 0x0,
        EventClass_Checksum   = 0x1,
        EventClass_Resource   = 0x2,
        EventClass_Timeout    = 0x4,
        EventClass_Timestamp  = 0x8,
        EventClass_Size       = 0x10,
        EventClass_Connection = 0x20,
        EventClass_Permission = 0x40,
        EventClass_Uid        = 0x80,
        EventClass_Gid        = 0x100,
        EventClass_NonExist   = 0x200,
        EventClass_Invalid    = 0x400,
        EventClass_Data       = 0x800,
        EventClass_Exec       = 0x1000,
        EventClass_FsFlag     = 0x2000,
        EventClass_Icmp       = 0x4000,
        EventClass_Content    = 0x8000,
        EventClass_Instance   = 0x10000,
        EventClass_Action     = 0x20000,
        EventClass_Pid        = 0x40000,
        EventClass_PPid       = 0x80000,
        EventClass_Heartbeat  = 0x100000,
        EventClass_Status     = 0x200000,
        EventClass_Uptime     = 0x400000,
        EventClass_Link       = 0x800000,
        EventClass_Speed      = 0x1000000,
        EventClass_Saturation = 0x2000000,
        EventClass_ByteIn     = 0x4000000,
        EventClass_ByteOut    = 0x8000000,
        EventClass_PacketIn   = 0x10000000,
        EventClass_PacketOut  = 0x20000000,
        EventClass_Exist      = 0x40000000,
        EventClass_All        = 0x7FFFFFFF
} EventClass_T;


/**
 * Event types. Sequential ids, one per monitored metric or attribute. The
 * values are part of the M/Monit protocol (<errors>, the "type" attribute of <id>) and of the
 * event queue file format: append new types at the end with the next free
 * value, never reorder, renumber or reuse a value. Each type belongs to
 * exactly one EventClass_T (see Event_Table in event.c).
 */
typedef enum {
        Event_Null                 = 0,
        Event_Action               = 1,
        Event_Checksum             = 2,
        Event_Connection           = 3,
        Event_Content              = 4,
        Event_Data                 = 5,
        Event_Exec                 = 6,
        Event_Exist                = 7,
        Event_FsFlag               = 8,
        Event_Gid                  = 9,
        Event_Heartbeat            = 10,
        Event_Icmp                 = 11,
        Event_Instance             = 12,
        Event_Invalid              = 13,
        Event_NonExist             = 14,
        Event_Permission           = 15,
        Event_Pid                  = 16,
        Event_PPid                 = 17,
        Event_Size                 = 18,
        Event_Timeout              = 19,
        Event_Uid                  = 20,
        Event_Uptime               = 21,
        Event_ByteIn               = 22,
        Event_ByteOut              = 23,
        Event_PacketIn             = 24,
        Event_PacketOut            = 25,
        Event_Cpu                  = 26,
        Event_CpuTotal             = 27,
        Event_Memory               = 28,
        Event_MemoryTotal          = 29,
        Event_Swap                 = 30,
        Event_LoadAverage1m        = 31,
        Event_LoadAverage5m        = 32,
        Event_LoadAverage15m       = 33,
        Event_Threads              = 34,
        Event_Children             = 35,
        Event_Filedescriptors      = 36,
        Event_FiledescriptorsTotal = 37,
        Event_ReadBytes            = 38,
        Event_ReadOperations       = 39,
        Event_WriteBytes           = 40,
        Event_WriteOperations      = 41,
        Event_ServiceTime          = 42,
        Event_Space                = 43,
        Event_Inode                = 44,
        Event_Hardlink             = 45,
        Event_Pagein               = 46,
        Event_Pageout              = 47,
        Event_Timestamp            = 48,
        Event_TimestampAccess      = 49,
        Event_TimestampChange      = 50,
        Event_TimestampModify      = 51,
        Event_Certificate          = 52,
        Event_LinkStatus           = 53,
        Event_LinkErrorsIn         = 54,
        Event_LinkErrorsOut        = 55,
        Event_ResponseTime         = 56,
        Event_LinkSpeed            = 57,
        Event_LinkDuplex           = 58,
        Event_LinkSaturation       = 59,
        Event_LinkSaturationIn     = 60,
        Event_LinkSaturationOut    = 61,
        Event_Euid                 = 62,
        Event_ProgramOutput        = 63,
        Event_Status               = 64,
        Event_Spawn                = 65,
        Event_Zombie               = 66,
        Event_SecurityAttribute    = 67,
        Event_Last                 = Event_SecurityAttribute
} Event_Type;


/**
 * A set of event types (used for alert filters). Sized from Event_Last, so
 * it grows automatically (by one word) when event types are appended. The
 * word is 16 bits wide to keep the set small (the set lives in every alert
 * recipient); the operations are single indexed loads, the width doesn't
 * affect their cost.
 */
typedef uint16_t EventSet_Word;
#define EventSet_WordBits (sizeof(EventSet_Word) * 8)
#define EventSet_Words ((Event_Last / EventSet_WordBits) + 1)
typedef struct EventSet_T {
        EventSet_Word bits[EventSet_Words];
} EventSet_T;


/** The event object (defined below, after monit.h, as it references the monit.h types) */
typedef struct Event_T *Event_T;


#include "monit.h"


/* Forward declarations (the types are defined in monit.h, which may be in the middle of its own parsing when this header is included from it) */
typedef struct Service_T *Service_T;
typedef struct EventAction_T *EventAction_T;


/**
 * The event structure version, written to the event queue files. History:
 *   4: Monit <  6.1.0: the event id is a long holding the legacy event class bit value
 *   5: Monit >= 6.1.0: the event id is the Event_Type, the legacy flag was added
 * EventQueue_process() can read the previous version (see eventqueue.c).
 */
#define EVENT_VERSION 5


/**
 * The event object. Warning: the structure is written to the event queue
 * files as is (see event.c), bump EVENT_VERSION when changing it.
 */
struct Event_T {
        Event_Type         id;                                  /**< The event identification */
        struct timeval     collected;                            /**< When the event occurred */
        struct Service_T  *source;                                          /**< Event source */
        Monitor_Mode       mode;                         /**< Monitoring mode for the service */
        Service_Type       type;                                  /**< Monitored service type */
        State_Type         state;                                             /**< Test state */
        bool               state_changed;                          /**< true if state changed */
        Handler_Type       flag;                                 /**< The handlers state flag */
        unsigned long long state_map;                      /**< Event bitmap for last cycles */
        unsigned int       count;                                         /**< The event rate */
        bool               legacy;   /**< true if converted from a version 4 queue file: the specific test is not known, the event is described by its legacy class and reported to M/Monit without the specific type (the "type" attribute of the <id> element) */
        char              *message;                /**< Optional message describing the event */
        EventAction_T      action;                       /**< Description of the event action */
        /** For internal use */
        struct Event_T    *next;                                     /**< next event in chain */
};


typedef struct myeventtable {
        Event_Type id;                     /**< Event type (equals the table index) */
        EventClass_T class;                /**< Legacy class (used for backward compatibility) */
        const char *name;                  /**< Short keyword, e.g. "cpu", "space", "atime" */
        const char *description_failed;
        const char *description_succeeded;
        const char *description_changed;
        const char *description_changednot;
        State_Type saveState;              /**< Bitmap of the event states that should trigger state file update */
} EventTable_T;


/** Event table indexed by Event_Type (Event_Table[Event_Cpu].id == Event_Cpu) */
extern const EventTable_T Event_Table[Event_Last + 1];


/* ---------------------------------------------------------------- EventSet */


/**
 * Add the event type to the set
 */
static inline void EventSet_set(EventSet_T *set, Event_Type id) {
        set->bits[id / EventSet_WordBits] |= (EventSet_Word)(1U << (id % EventSet_WordBits));
}


/**
 * Remove the event type from the set
 */
static inline void EventSet_clear(EventSet_T *set, Event_Type id) {
        set->bits[id / EventSet_WordBits] &= (EventSet_Word)~(1U << (id % EventSet_WordBits));
}


/**
 * @return true if the event type is a member of the set
 */
static inline bool EventSet_has(const EventSet_T *set, Event_Type id) {
        return (set->bits[id / EventSet_WordBits] >> (id % EventSet_WordBits)) & 1U;
}


/**
 * Add all event types to the set
 */
void EventSet_setAll(EventSet_T *set);


/**
 * Add all event types belonging to the given legacy class(es) to the set
 */
void EventSet_setClass(EventSet_T *set, EventClass_T class);


/**
 * Replace the set with its complement (all event types not in the set)
 */
void EventSet_negate(EventSet_T *set);


/**
 * @return true if the set contains no event type
 */
bool EventSet_isEmpty(const EventSet_T *set);


/**
 * @return true if the set contains every event type
 */
bool EventSet_isAll(const EventSet_T *set);


/** Buffer size for EventSet_describe(), large enough for the names of all event types */
#define EventSet_DescribeLength 2048


/**
 * Describe the set as a space separated list of event type names, e.g.
 * "checksum atime mtime ", or "All events" / "No events".
 * @param set The event set
 * @param buf Output buffer (see EventSet_DescribeLength)
 * @param len Size of the output buffer
 * @return buf
 */
char *EventSet_describe(const EventSet_T *set, char *buf, int len);


/* ------------------------------------------------------------------ Events */


/**
 * Look up an event type by its keyword (case insensitive), e.g. "cpu"
 * @param name The event type keyword
 * @return The event type or Event_Null if no event type has the given name
 */
Event_Type Event_byName(const char *name);


/**
 * This class implements the <b>event</b> processing machinery used by
 * monit. In monit an event is an object containing a Service_T
 * reference indicating the object where the event originated, an id
 * specifying the event type, a value representing up or down state
 * and an optional message describing why the event was fired.
 *
 * Clients may use the function Event_post() to post events to the
 * event handler for processing.
 *
 * @file
 */


/**
 * Post a new Event
 * @param service The Service the event belongs to
 * @param id The event identification
 * @param state The event state
 * @param action Description of the event action
 * @param s Optional message describing the event
 */
void Event_post(Service_T service, Event_Type id, State_Type state, EventAction_T action, const char *s, ...) __attribute__((format (printf, 5, 6)));


/**
 * Get a textual description of the event type in the event's state. For
 * instance if the event type is Event_Cpu in the failed state, the
 * description is "CPU usage matched limit", in the succeeded state it is
 * "CPU usage ok" and so on. Events converted from the version 4 queue
 * files (legacy flag) are described by their legacy class, for instance
 * "Resource limit matched".
 * @param E An event object
 * @return A string describing the event in clear text, or NULL if the
 * event state is invalid
 */
const char *Event_description(Event_T E);


/**
 * Get the action of the event in the event's state (the failed action for
 * the failed / changed states, the succeeded action for the succeeded /
 * not-changed states). In passive monitoring mode the start, stop and
 * restart actions are reported as alert.
 * @param E An event object
 * @return An action id
 */
Action_Type Event_action(Event_T E);


/**
 * Get a textual description of the event's action, for instance "restart"
 * if the event is in the failed state and the failed action is restart, or
 * "alert" for a recovery with the alert action.
 * @param E An event object
 * @return A string describing the action in clear text
 */
const char *Event_actionDescription(Event_T E);


#endif
