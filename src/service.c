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

#include "monit.h"
#include "event.h"
#include "state.h"
#include "service.h"


/**
 * Implementation of the service helpers.
 *
 * @file
 */


/* ----------------------------------------------------------------- Private */


static void _resetFilesystemFlags(FilesystemFlags_T flags) {
        flags->previous = flags->value[0];
        flags->current = flags->value[1];
        *(flags->current) = 0;
        *(flags->previous) = 0;
}


static void _resetIOStatistics(IOStatistics_T S) {
        Statistics_reset(&(S->operations));
        Statistics_reset(&(S->bytes));
}


/* ------------------------------------------------------------------ Public */


Service_T Service_get(const char *name) {
        assert(name);
        for (Service_T s = Service_List; s; s = s->next)
                if (IS(s->name, name))
                        return s;
        return NULL;
}


int Service_count(void) {
        int i = 0;
        Service_T s;
        for (s = Service_List; s; s = s->next)
                i += 1;
        return i;
}


bool Service_exists(const char *name) {
        assert(name);
        return Service_get(name) ? true : false;
}


bool Service_hasStatus(Service_T s) {
        return((s->monitor & Monitor_Yes) && s->status[Event_NonExist] == State_Succeeded && s->status[Event_Zombie] == State_Succeeded && s->status[Event_Data] == State_Succeeded);
}


bool Service_hasErrors(Service_T s) {
        assert(s);
        for (int i = 1; i <= Event_Last; i++)
                if (s->status[i] != State_Succeeded)
                        return true;
        return false;
}


bool Service_hasConfirmedErrors(Service_T s) {
        assert(s);
        for (Event_T e = s->eventlist; e; e = e->next)
                if (e->id != Event_Instance && e->id != Event_Action && (e->state == State_Failed || e->state == State_Changed))
                        return true;
        return false;
}


void Service_resetInfo(Service_T s) {
        switch (s->type) {
                case Service_Filesystem:
                        s->inf.filesystem->f_bsize = 0LL;
                        s->inf.filesystem->f_blocks = 0LL;
                        s->inf.filesystem->f_blocksfree = 0LL;
                        s->inf.filesystem->f_blocksfreetotal = 0LL;
                        s->inf.filesystem->f_blocksused = 0LL;
                        s->inf.filesystem->f_files = 0LL;
                        s->inf.filesystem->f_filesfree = 0LL;
                        s->inf.filesystem->f_filesused = 0LL;
                        s->inf.filesystem->inode_percent = 0.;
                        s->inf.filesystem->space_percent = 0.;
                        s->inf.filesystem->mode = -1;
                        s->inf.filesystem->uid = -1;
                        s->inf.filesystem->gid = -1;
                        _resetFilesystemFlags(&(s->inf.filesystem->flags));
                        _resetIOStatistics(&(s->inf.filesystem->read));
                        _resetIOStatistics(&(s->inf.filesystem->write));
                        Statistics_reset(&(s->inf.filesystem->time.read));
                        Statistics_reset(&(s->inf.filesystem->time.write));
                        Statistics_reset(&(s->inf.filesystem->time.wait));
                        Statistics_reset(&(s->inf.filesystem->time.run));
                        break;
                case Service_File:
                        s->inf.file->size  = -1;
                        s->inf.file->readpos = 0;
                        s->inf.file->inode = 0;
                        s->inf.file->inode_prev = 0;
                        s->inf.file->mode = -1;
                        s->inf.file->uid = -1;
                        s->inf.file->gid = -1;
                        s->inf.file->nlink = -1;
                        s->inf.file->timestamp.access = 0;
                        s->inf.file->timestamp.change = 0;
                        s->inf.file->timestamp.modify = 0;
                        *s->inf.file->cs_sum = 0;
                        break;
                case Service_Directory:
                        s->inf.directory->mode = -1;
                        s->inf.directory->uid = -1;
                        s->inf.directory->gid = -1;
                        s->inf.directory->nlink = -1;
                        s->inf.directory->timestamp.access = 0;
                        s->inf.directory->timestamp.change = 0;
                        s->inf.directory->timestamp.modify = 0;
                        break;
                case Service_Fifo:
                        s->inf.fifo->mode = -1;
                        s->inf.fifo->uid = -1;
                        s->inf.fifo->gid = -1;
                        s->inf.fifo->nlink = -1;
                        s->inf.fifo->timestamp.access = 0;
                        s->inf.fifo->timestamp.change = 0;
                        s->inf.fifo->timestamp.modify = 0;
                        break;
                case Service_Process:
                        s->inf.process->_pid = -1;
                        s->inf.process->_ppid = -1;
                        s->inf.process->pid = -1;
                        s->inf.process->ppid = -1;
                        s->inf.process->uid = -1;
                        s->inf.process->euid = -1;
                        s->inf.process->gid = -1;
                        s->inf.process->zombie = false;
                        s->inf.process->threads = -1;
                        s->inf.process->children = -1;
                        s->inf.process->mem = 0ULL;
                        s->inf.process->total_mem = 0ULL;
                        s->inf.process->mem_percent = -1.;
                        s->inf.process->total_mem_percent = -1.;
                        s->inf.process->cpu_percent = -1.;
                        s->inf.process->total_cpu_percent = -1.;
                        s->inf.process->uptime = -1;
                        s->inf.process->filedescriptors.open = -1LL;
                        s->inf.process->filedescriptors.openTotal = -1LL;
                        *(s->inf.process->secattr) = 0;
                        _resetIOStatistics(&(s->inf.process->read));
                        _resetIOStatistics(&(s->inf.process->write));
                        break;
                case Service_Net:
                        if (s->inf.net->stats)
                                Link_reset(s->inf.net->stats);
                        break;
                default:
                        break;
        }
}


void Service_monitorSet(Service_T s) {
        assert(s);
        if (s->monitor == Monitor_Not) {
                s->monitor = Monitor_Init;
                DEBUG("'%s' monitoring enabled\n", s->name);
                State_dirty();
        }
}


void Service_monitorUnset(Service_T s) {
        assert(s);
        if (s->monitor != Monitor_Not) {
                s->monitor = Monitor_Not;
                DEBUG("'%s' monitoring disabled\n", s->name);
        }
        s->nstart = 0;
        s->ncycle = 0;
        if (s->every.type == Every_SkipCycles)
                s->every.spec.cycle.counter = 0;
        memset(s->status, 0, sizeof(s->status));
        if (s->eventlist)
                gc_event(&s->eventlist);
        Service_resetInfo(s);
        State_dirty();
}


void Service_legacyStatus(Service_T s, EventClass_T *status, EventClass_T *hint) {
        assert(s);
        assert(status);
        assert(hint);
        EventClass_T failed = EventClass_Null;
        EventClass_T changed = EventClass_Null;
        for (int i = 1; i <= Event_Last; i++) {
                if (s->status[i] == State_Failed)
                        failed |= Event_Table[i].class;
                else if (s->status[i] == State_Changed)
                        changed |= Event_Table[i].class;
        }
        /* A class with both failed and changed members is reported as failed */
        *hint = changed & ~failed;
        *status = failed | changed;
}
