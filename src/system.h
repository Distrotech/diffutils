/* System dependent declarations.
   Copyright (C) 1988, 1989, 1992 Free Software Foundation, Inc.

This file is part of GNU DIFF.

GNU DIFF is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU DIFF is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU DIFF; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <sys/types.h>
#include <sys/stat.h>

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined (USG) || defined (_POSIX_VERSION)
#include <time.h>
#include <fcntl.h>
#else
#include <sys/time.h>
#include <sys/file.h>
#endif

#ifndef HAVE_DUP2
#define dup2(f,t)	(close (t),  fcntl (f,F_DUPFD,t))
#endif

#ifndef O_RDONLY
#define O_RDONLY 0
#endif

#if !defined (USG) || defined (_POSIX_VERSION)
#include <sys/wait.h>
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#undef WIFEXITED		/* Avoid 4.3BSD incompatibility with Posix.  */
#endif
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#if defined (DIRENT) || defined (_POSIX_VERSION)
#include <dirent.h>
#ifdef direct
#undef direct
#endif
#define direct dirent
#else /* not (DIRENT or _POSIX_VERSION) */
#ifdef USG
#ifdef SYSNDIR
#include <sys/ndir.h>
#else /* !SYSNDIR */
#include <ndir.h>
#endif /* SYSNDIR */
#else /* !USG */
#include <sys/dir.h>
#endif /* USG */
#endif /* DIRENT or _POSIX_VERSION */

#ifdef HAVE_VFORK_H
#include <vfork.h>
#endif

#if defined (USG) || defined (STDC_HEADERS)
#include <string.h>
#define index	strchr
#define rindex	strrchr
#define bcopy(s,d,n)	memcpy (d,s,n)
#define bcmp(s1,s2,n)	memcmp (s1,s2,n)
#define bzero(s,n)	memset (s,0,n)
#else
#include <strings.h>
#endif

#if defined (STDC_HEADERS)
#include <stdlib.h>
#include <limits.h>
#else
char *getenv ();
char *malloc ();
char *realloc ();
#if __STDC__ || __GNUC__
#include "limits.h"
#else
#define INT_MAX 2147483647
#define CHAR_BIT 8
#endif
#endif

#include <errno.h>
#ifndef STDC_HEADERS
extern int errno;
#endif

#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif
#define TRUE		1
#define	FALSE		0

#if !__STDC__
#define const
#define volatile
#endif

#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))
