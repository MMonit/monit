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

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
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
#include "eventqueue.h"
#include "service.h"
#include "MMonit.h"

// libmonit
#include "io/File.h"
#include "system/Random.h"


/**
 * Implementation of the event queue.
 *
 * @file
 */


/* ------------------------------------------------------------- Definitions */


/*
 * Event structure version 4 (Monit < 6.1.0), as written to the event queue files:
 * identical to the structure version 5 except for the id, which was a long holding
 * the legacy event class bit value.
 */
#define EVENT_VERSION_4 4
struct EventV4_T {
        long               id;
        struct timeval     collected;
        struct Service_T  *source;
        Monitor_Mode       mode;
        Service_Type       type;
        State_Type         state;
        bool               state_changed;
        Handler_Type       flag;
        unsigned long long state_map;
        unsigned int       count;
        char              *message;
        EventAction_T      action;
        struct EventV4_T  *next;
};


/* ----------------------------------------------------------------- Private */


static bool _checkDirectory(const char *path) {
        if (mkdir(path, 0700) < 0 && errno != EEXIST) {
                Log_error("Cannot create the event queue directory '%s' -- %s\n", path, STRERROR);
                return false;
        }
        return true;
}


static bool _checkLimit(const char *path, int limit) {
        if (limit >= 0) {
                DIR *dir = opendir(path);
                if (! dir) {
                        Log_error("Cannot open the event queue directory '%s' -- %s\n", path, STRERROR);
                        return false;
                }
                int used = 0;
                struct dirent *de = NULL;
                while ((de = readdir(dir)) ) {
                        char buf[PATH_MAX];
                        snprintf(buf, sizeof(buf), "%s/%s", path, de->d_name);
                        if (File_isFile(buf) && ++used > limit) {
                                Log_error("Event queue is full\n");
                                closedir(dir);
                                return false;
                        }
                }
                closedir(dir);
        }
        return true;
}


static bool _write(FILE *file, const void *data, size_t size) {
        assert(file);
        /* write size */
        size_t rv = fwrite(&size, 1, sizeof(size_t), file);
        if (rv != sizeof(size_t)) {
                if (feof(file) || ferror(file))
                        Log_error("Queued event file: unable to write event size -- %s\n", feof(file) ? "end of file" : "stream error");
                else
                        Log_error("Queued event file: unable to write event size -- read returned %lu bytes\n", (unsigned long)rv);
                return false;
        }
        /* write data if any */
        if (size > 0) {
                if ((rv = fwrite(data, 1, size, file)) != size) {
                        if (feof(file) || ferror(file))
                                Log_error("Queued event file: unable to write event size -- %s\n", feof(file) ? "end of file" : "stream error");
                        else
                                Log_error("Queued event file: unable to write event size -- read returned %lu bytes\n", (unsigned long)rv);
                        return false;
                }
        }
        return true;
}


static void *_read(FILE *file, size_t *size) {
        assert(file);
        /* read size */
        size_t rv = fread(size, 1, sizeof(size_t), file);
        if (rv != sizeof(size_t)) {
                if (feof(file) || ferror(file))
                        Log_error("Queued event file: unable to read event size -- %s\n", feof(file) ? "end of file" : "stream error");
                else
                        Log_error("Queued event file: unable to read event size -- read returned %lu bytes\n", (unsigned long)rv);
                return NULL;
        }
        /* read data if any (allow 1MB at maximum to prevent enormous memory allocation) */
        void *data = NULL;
        if (*size > 0 && *size < 1048576) {
                data = CALLOC(1, *size + 1);
                if ((rv = fread(data, 1, *size, file)) != *size) {
                        FREE(data);
                        if (feof(file) || ferror(file))
                                Log_error("Queued event file: unable to read event data -- %s\n", feof(file) ? "end of file" : "stream error");
                        else
                                Log_error("Queued event file: unable to read event data -- read returned %lu bytes\n", (unsigned long)rv);
                        return NULL;
                }
        }
        return data;
}


/*
 * Convert a version 4 event read from the queue file to the current version.
 * The version 4 format holds the legacy event class only, so the specific
 * test is not known for classes shared by several tests (e.g. Resource).
 * The event id is set to the first event type of that class in the event
 * table (any member gives the right class) and the legacy flag is set, so
 * the event is described by the class ("Resource limit matched") and is
 * reported to M/Monit with the class only, exactly as the old Monit did.
 * @return A new event object or NULL if the data is invalid
 */
static Event_T _fromV4(const char *file_name, const void *data, size_t size) {
        if (size != sizeof(struct EventV4_T)) {
                Log_error("Aborting queued event %s -- invalid version 4 event size %lu\n", file_name, (unsigned long)size);
                return NULL;
        }
        const struct EventV4_T *v4 = data;
        Event_Type id = Event_Null;
        for (int i = 1; i <= Event_Last; i++) {
                if ((long)Event_Table[i].class == v4->id) {
                        id = i;
                        break;
                }
        }
        if (id == Event_Null) {
                Log_error("Aborting queued event %s -- invalid version 4 event id: %ld\n", file_name, v4->id);
                return NULL;
        }
        Event_T e;
        NEW(e);
        e->id = id;
        e->collected = v4->collected;
        e->mode = v4->mode;
        e->type = v4->type;
        e->state = v4->state;
        e->state_changed = v4->state_changed;
        e->flag = v4->flag;
        e->state_map = v4->state_map;
        e->count = v4->count;
        e->legacy = true;
        return e;
}


/**
 * Update the partially handled event in the global queue
 * @param E An event object
 * @param file_name File name
 */
static void _update(Event_T E, const char *file_name) {
        int version = EVENT_VERSION;
        Action_Type action = Event_action(E);
        bool rv;

        assert(E);
        assert(E->flag != Handler_Succeeded);

        if (! _checkDirectory(Run.eventlist_dir)) {
                Log_error("Aborting event - cannot access the event queue directory %s\n", Run.eventlist_dir);
                return;
        }

        DEBUG("Updating event in the queue file %s for later delivery\n", file_name);

        FILE *file = fopen(file_name, "w");
        if (! file) {
                Log_error("Aborting event - cannot open the event file %s -- %s\n", file_name, STRERROR);
                return;
        }

        /* write event structure version */
        if (! (rv = _write(file, &version, sizeof(int))))
                goto error;

        /* write event structure */
        if (! (rv = _write(file, E, sizeof(*E))))
                goto error;

        /* write source */
        if (! (rv = _write(file, E->source->name, strlen(E->source->name) + 1)))
                goto error;

        /* write message */
        if (! (rv = _write(file, E->message, E->message ? strlen(E->message) + 1 : 0)))
                goto error;

        /* write event action */
        if (! (rv = _write(file, &action, sizeof(Action_Type))))
                goto error;

error:
        fclose(file);
        if (! rv) {
                Log_error("Aborting event - unable to update event information in '%s'\n", file_name);
                if (unlink(file_name) < 0)
                        Log_error("Failed to remove event file '%s' -- %s\n", file_name, STRERROR);
        }
}


/* ------------------------------------------------------------------ Public */


/**
 * Add the partially handled event to the global queue
 * @param E An event object
 */
void EventQueue_add(Event_T E) {
        assert(E);
        assert(E->flag != Handler_Succeeded);

        if (! _checkDirectory(Run.eventlist_dir)) {
                Log_error("Aborting event - cannot access the event queue directory %s\n", Run.eventlist_dir);
                return;
        }

        if (! _checkLimit(Run.eventlist_dir, Run.eventlist_slots)) {
                Log_error("Aborting event - queue over quota\n");
                return;
        }

        // Compose a random file name
        char file_name[PATH_MAX];
        int fd = -1;
        for (int attempt = 0; attempt < 100; attempt++) {
                snprintf(file_name, PATH_MAX, "%s/monitevent_%016llx", Run.eventlist_dir, Random_number());
                if ((fd = open(file_name, O_WRONLY | O_CREAT | O_EXCL, 0600)) >= 0)
                        break;
                if (errno != EEXIST) {
                        Log_error("Aborting event - cannot create event file %s -- %s\n", file_name, STRERROR);
                        return;
                }
        }
        if (fd < 0) {
                Log_error("Aborting event - cannot create a unique event file in %s\n", Run.eventlist_dir);
                return;
        }

        Log_info("Adding event to the queue file %s for later delivery\n", file_name);

        FILE *file = fdopen(fd, "w");
        if (! file) {
                Log_error("Aborting event - cannot create event file %s -- %s\n", file_name, STRERROR);
                close(fd);
                if (unlink(file_name) < 0)
                        Log_error("Failed to remove event file '%s' -- %s\n", file_name, STRERROR);
                return;
        }

        bool rv;

        /* write event structure version */
        int version = EVENT_VERSION;
        if (! (rv = _write(file, &version, sizeof(int))))
                goto error;

        /* write event structure */
        if (! (rv = _write(file, E, sizeof(*E))))
                goto error;

        /* write source */
        if (! (rv = _write(file, E->source->name, strlen(E->source->name) + 1)))
                goto error;

        /* write message */
        if (! (rv = _write(file, E->message, E->message ? strlen(E->message) + 1 : 0)))
                goto error;

        /* write event action */
        Action_Type action = Event_action(E);
        if (! (rv = _write(file, &action, sizeof(Action_Type))))
                goto error;

error:
        fclose(file);
        if (! rv) {
                Log_error("Aborting event - unable to save event information to %s\n",  file_name);
                if (unlink(file_name) < 0)
                        Log_error("Failed to remove event file '%s' -- %s\n", file_name, STRERROR);
        } else {
                if (! (Run.flags & Run_HandlerInit) && E->flag & Handler_Alert)
                        Run.handler_queue[Handler_Alert]++;
                if (! (Run.flags & Run_HandlerInit) && E->flag & Handler_Mmonit)
                        Run.handler_queue[Handler_Mmonit]++;
        }
}


/**
 * Reprocess the partially handled event queue
 */
void EventQueue_process(void) {
        /* return in the case that the eventqueue is not enabled or empty */
        if (! Run.eventlist_dir || (! (Run.flags & Run_HandlerInit) && ! Run.handler_queue[Handler_Alert] && ! Run.handler_queue[Handler_Mmonit]))
                return;

        DIR *dir = opendir(Run.eventlist_dir);
        if (! dir) {
                if (errno != ENOENT)
                        Log_error("Cannot open the directory %s -- %s\n", Run.eventlist_dir, STRERROR);
                return;
        }

        struct dirent *de = readdir(dir);
        if (de)
                DEBUG("Processing postponed events queue\n");

        Action_T a;
        NEW(a);

        EventAction_T ea;
        NEW(ea);

        while (de) {
                int handlers_passed = 0;

                /* In the case that all handlers failed, skip the further processing in this cycle. Alert handler is currently defined anytime (either explicitly or localhost by default) */
                if ( (Run.mmonits && FLAG(Run.handler_flag, Handler_Mmonit) && FLAG(Run.handler_flag, Handler_Alert)) || FLAG(Run.handler_flag, Handler_Alert))
                        break;

                char file_name[PATH_MAX];
                snprintf(file_name, sizeof(file_name), "%s/%s", Run.eventlist_dir, de->d_name);

                if (File_isFile(file_name)) {
                        DEBUG("Processing queued event '%s'\n", file_name);

                        FILE *file = fopen(file_name, "r");
                        if (! file) {
                                Log_error("Queued event processing failed - cannot open the file '%s' -- %s\n", file_name, STRERROR);
                                goto error1;
                        }

                        size_t size;

                        /* read event structure version */
                        int *version = _read(file, &size);
                        if (! version) {
                                DEBUG("Skipping file '%s' - not event queue data formatted\n", file_name);
                                goto error2;
                        }
                        if (size != sizeof(int)) {
                                Log_error("Aborting queued event %s - invalid size %lu\n", file_name, (unsigned long)size);
                                goto error3;
                        }
                        if (*version != EVENT_VERSION && *version != EVENT_VERSION_4) {
                                Log_error("Aborting queued event %s - incompatible data format version %d\n", file_name, *version);
                                goto error3;
                        }

                        /* read event structure */
                        Event_T e = _read(file, &size);
                        if (! e)
                                goto error3;
                        if (*version == EVENT_VERSION_4) {
                                /* Event queued by a previous Monit version: convert to the current structure */
                                DEBUG("Converting queued event %s from the data format version %d\n", file_name, *version);
                                Event_T converted = _fromV4(file_name, e, size);
                                FREE(e);
                                if (! converted)
                                        goto error3;
                                e = converted;
                        } else if (size != sizeof(*e)) {
                                goto error4;
                        }
                        e->source = NULL;
                        e->message = NULL;
                        e->action = NULL;
                        e->next = NULL;

                        /* validate the event id */
                        if (e->id <= Event_Null || e->id > Event_Last) {
                                Log_error("Aborting queued event %s -- invalid event id: %d\n", file_name, (int)e->id);
                                goto error4;
                        }

                        /* read source */
                        char *service = _read(file, &size);
                        if (! service)
                                goto error4;
                        if (! (e->source = Service_get(service))) {
                                Log_error("Aborting queued event '%s' - service %s not found in monit configuration\n", file_name, service);
                                FREE(service);
                                goto error4;
                        }
                        FREE(service);

                        /* read message */
                        if (! (e->message = _read(file, &size)))
                                goto error4;

                        /* read event action */
                        Action_Type *action = _read(file, &size);
                        if (! action)
                                goto error5;
                        if (size != sizeof(Action_Type))
                                goto error6;
                        if ((int)*action < Action_Ignored || (int)*action > Action_Monitor) {
                                Log_error("Aborting queued event %s -- invalid action id: %d\n", file_name, (int)*action);
                                goto error6;
                        }
                        a->id = *action;
                        switch (e->state) {
                                case State_Succeeded:
                                case State_ChangedNot:
                                        ea->succeeded = a;
                                        break;
                                case State_Failed:
                                case State_Changed:
                                case State_Init:
                                        ea->failed = a;
                                        break;
                                default:
                                        Log_error("Aborting queue event %s -- invalid state: %d\n", file_name, e->state);
                                        goto error6;
                        }
                        e->action = ea;

                        /* Retry all remaining handlers */

                        /* alert */
                        if (e->flag & Handler_Alert) {
                                if (Run.flags & Run_HandlerInit)
                                        Run.handler_queue[Handler_Alert]++;
                                if ((Run.handler_flag & Handler_Alert) != Handler_Alert) {
                                        if (handle_alert(e) != Handler_Alert) {
                                                e->flag &= ~Handler_Alert;
                                                Run.handler_queue[Handler_Alert]--;
                                                handlers_passed++;
                                        } else {
                                                Log_error("Alert handler failed, retry scheduled for next cycle\n");
                                                Run.handler_flag |= Handler_Alert;
                                        }
                                }
                        }

                        /* mmonit */
                        if (e->flag & Handler_Mmonit) {
                                if (Run.flags & Run_HandlerInit)
                                        Run.handler_queue[Handler_Mmonit]++;
                                if ((Run.handler_flag & Handler_Mmonit) != Handler_Mmonit) {
                                        if (MMonit_send(e) != Handler_Mmonit) {
                                                e->flag &= ~Handler_Mmonit;
                                                Run.handler_queue[Handler_Mmonit]--;
                                                handlers_passed++;
                                        } else {
                                                Log_error("M/Monit handler failed, retry scheduled for next cycle\n");
                                                Run.handler_flag |= Handler_Mmonit;
                                        }
                                }
                        }

                        /* If no error persists, remove it from the queue */
                        if (e->flag == Handler_Succeeded) {
                                DEBUG("Removing queued event %s\n", file_name);
                                if (unlink(file_name) < 0)
                                        Log_error("Failed to remove queued event file '%s' -- %s\n", file_name, STRERROR);
                        } else if (handlers_passed > 0) {
                                DEBUG("Updating queued event %s (some handlers passed)\n", file_name);
                                _update(e, file_name);
                        }

                error6:
                        FREE(action);
                error5:
                        FREE(e->message);
                error4:
                        FREE(e);
                error3:
                        FREE(version);
                error2:
                        fclose(file);
                }
        error1:
                de = readdir(dir);
        }
        Run.flags &= ~Run_HandlerInit;
        closedir(dir);
        FREE(a);
        FREE(ea);
}

