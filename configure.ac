# Configure template for GNU Diffutils.

# Copyright (C) 1994, 1995, 1998, 2001, 2002, 2004, 2006 Free Software
# Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

AC_PREREQ(2.59)

AC_INIT([GNU diffutils], [2.8.7-cvs], [bug-gnu-utils@gnu.org])
AC_CONFIG_SRCDIR([src/diff.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([gnits])
AC_CONFIG_HEADER([config.h:config.hin])

gl_USE_SYSTEM_EXTENSIONS
AC_PROG_AWK
AC_PROG_CC
AM_PROG_CC_STDC
AM_MISSING_PROG([HELP2MAN], [help2man])
AC_PROG_RANLIB

AC_SYS_LARGEFILE

AC_C_INLINE
AC_C_VARARRAYS

AC_DEFINE([DEFAULT_EDITOR_PROGRAM], ["ed"],
  [Name of editor program, unless overridden.])

AC_PATH_PROG([PR_PROGRAM], [pr], [""])
AC_DEFINE_UNQUOTED([PR_PROGRAM], ["$PR_PROGRAM"], [Name of "pr" program.])

AC_CHECK_HEADERS_ONCE([fcntl.h locale.h sys/file.h unistd.h])
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_TYPE_PID_T

AC_DEFUN([DIFFUTILS_PREREQUISITES], [
  AC_REQUIRE([gl_AC_TYPE_INTMAX_T])
  AC_REQUIRE([gl_AC_TYPE_UINTMAX_T])
])
DIFFUTILS_PREREQUISITES

AC_CHECK_FUNCS_ONCE([dup2 sigaction sigprocmask strcasecoll stricoll])
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK

GNULIB_AUTOCONF_SNIPPET
AM_GNU_GETTEXT([external], [need-ngettext])
AM_GNU_GETTEXT_VERSION([0.14.5])
XGETTEXT="AWK='$AWK' \$(SHELL) \$(top_srcdir)/exgettext $XGETTEXT"

AC_CONFIG_FILES([Makefile doc/Makefile \
		 lib/Makefile man/Makefile \
		 ms/Makefile po/Makefile.in src/Makefile])
AC_OUTPUT
