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

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "protocol.h"

static struct Protocol_T protocols[] = {
        {.name = "DEFAULT",         .check = check_default},
        {.name = "HTTP",            .check = check_http},
        {.name = "FTP",             .check = check_ftp},
        {.name = "SMTP",            .check = check_smtp},
        {.name = "POP",             .check = check_pop},
        {.name = "IMAP",            .check = check_imap},
        {.name = "NNTP",            .check = check_nntp},
        {.name = "SSH",             .check = check_ssh},
        {.name = "DWP",             .check = check_dwp},
        {.name = "LDAP2",           .check = check_ldap2},
        {.name = "LDAP3",           .check = check_ldap3},
        {.name = "RDATE",           .check = check_rdate},
        {.name = "RSYNC",           .check = check_rsync},
        {.name = "generic",         .check = check_generic},
        {.name = "APACHESTATUS",    .check = check_apache_status},
        {.name = "NTP3",            .check = check_ntp3},
        {.name = "MYSQL",           .check = check_mysql},
        {.name = "DNS",             .check = check_dns},
        {.name = "POSTFIX-POLICY",  .check = check_postfix_policy},
        {.name = "TNS",             .check = check_tns},
        {.name = "PGSQL",           .check = check_pgsql},
        {.name = "CLAMAV",          .check = check_clamav},
        {.name = "SIP",             .check = check_sip},
        {.name = "LMTP",            .check = check_lmtp},
        {.name = "GPS",             .check = check_gps},
        {.name = "RADIUS",          .check = check_radius},
        {.name = "MEMCACHE",        .check = check_memcache},
        {.name = "WEBSOCKET",       .check = check_websocket},
        {.name = "REDIS",           .check = check_redis},
        {.name = "MONGODB",         .check = check_mongodb},
        {.name = "SIEVE",           .check = check_sieve},
        {.name = "SPAMASSASSIN",    .check = check_spamassassin},
        {.name = "FAIL2BAN",        .check = check_fail2ban},
        {.name = "MQTT",            .check = check_mqtt}
};


/* ------------------------------------------------------------------ Public */


Protocol_T Protocol_get(Protocol_Type type) {
        if (type >= sizeof(protocols)/sizeof(protocols[0]))
                return &protocols[0];
        return &protocols[type];
}


