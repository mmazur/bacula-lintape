/*
 * bacula.h -- main header file to include in all Bacula source
 *
 *   Version $Id$
 */

/*
   Copyright (C) 2000, 2001, 2002 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

 */

#ifndef _BACULA_H
#define _BACULA_H 1

#include "config.h"

#define _REENTRANT    1
#define _THREAD_SAFE  1
#define _POSIX_PTHREAD_SEMANTICS 1

/* System includes */
#include <stdarg.h>
#include <stdio.h>
#if STDC_HEADERS
#include <stdlib.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <errno.h>
#include <fcntl.h>

#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include "lib/getopt.h"
#endif

#include <string.h>
#include <strings.h>
#include <signal.h>
#include <ctype.h>
#include <syslog.h>
#if HAVE_LIMITS_H
#include <limits.h>
#endif
#include <pwd.h>
#include <time.h>
#include <netdb.h>
#include <sys/types.h>
#ifdef HAVE_SYS_BITYPES_H
#include <sys/bitypes.h>
#endif
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
     
/* Local Bacula includes. Be sure to put all the system
 *  includes before these.  
 */
#include "version.h"
#include "baconfig.h"
#include "bc_types.h"
#include "lib/lib.h"

#endif
