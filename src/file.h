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


#ifndef MONIT_FILE_H
#define MONIT_FILE_H


/**
 *  Utilities used for managing files used by monit.
 *
 *  @file
 */


/**
 * Initialize the programs file variables
 */
void file_init(void);


/**
 * Finalize and remove temporary files
 */
void file_finalize(void);


/**
 * Search the system for the monit control file. Try first ~/.monitrc,
 * if that fails try /etc/monitrc, then /usr/local/etc/monitrc and
 * finally ./monitrc.  Exit the application if the control file was
 * not found.
 * @return The location of monits control file (monitrc)
 */
char *file_findControlFile(void);


/**
 * Create Monit's pidfile - Such a file is created when in daemon
 * mode.
 * @return true if the file was created, otherwise false.
 */
bool file_createPidFile(void);


/**
 * Read the pid from the given pidfile.
 * @param pidfile A pidfile with full path
 * @return The pid (>0) if successful, otherwise -1
 */
pid_t file_getPid(const char *pidfile);


/**
 * Open and read the id from the given idfile. If the idfile doesn't exist,
 * generate new id and store it in the id file.
 * @param idfile An idfile with full path
 * @return the id or NULL
 */
char *file_monitId(char *idfile);


/**
 * Security check for files. The files must have the same uid as the
 * REAL uid of this process, it must have permissions no greater than
 * "maxpermission" and it must not be a symbolic link.  We check these
 * conditions here.
 * @param filename The filename of the checked file
 * @param description The description of the checked file
 * @param permmask The permission mask for the file
 * @return true if the test succeeded otherwise false
 */
bool file_checkStat(const char *filename, const char *description, mode_t permmask);


/**
 * Reads an proc filesystem object
 * @param buf buffer to write to
 * @param buf_size size of buf
 * @param name name of proc object
 * @param pid number of the process or < 0 if main directory
 * @param bytes_read number of bytes read to buffer
 * @return true if succeeded otherwise false.
 */
bool file_readProc(char *buf, int buf_size, const char *name, int pid, int *bytes_read);


#endif
