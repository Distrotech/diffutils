# Configure template for GNU diffutils.
# Copyright (C) 1994, 1995, 1998, 2001 Free Software Foundation, Inc.
# Process this file with autoconf to produce a configure script.

AC_INIT(src/diff.c)
AM_CONFIG_HEADER([config.h:config.hin])
AC_PREREQ(2.52)
AM_INIT_AUTOMAKE(diffutils, 2.7.4)
ALL_LINGUAS="da de en_GB eo es fr gl id pl ru sv tr"

AC_PROG_AWK
AC_PROG_CC
AC_ISC_POSIX
AM_PROG_CC_STDC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_RANLIB

AH_VERBATIM([_GNU_SOURCE],
  [/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif])
AC_DEFINE([__EXTENSIONS__], 1, [Enable Solaris extensions.])
AC_SYS_LARGEFILE

AC_C_CONST
AC_C_INLINE
AC_C_VARARRAYS

AC_DEFINE(DEFAULT_DIFF_PROGRAM, "diff",
  [Name of "diff" program, unless overridden.])

AC_DEFINE(DEFAULT_EDITOR_PROGRAM, "ed",
  [Name of editor program, unless overridden.])

AC_PATH_PROG(PR_PROGRAM, pr, "")
AC_DEFINE(PR_PROGRAM, "$PR_PROGRAM", [Name of "pr" program.])

AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h libintl.h limits.h locale.h \
  stdbool.h stdlib.h string.h sys/file.h time.h unistd.h)
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_CHECK_TYPES([ptrdiff_t, uintmax_t])
AM_GNU_GETTEXT
XGETTEXT="AWK='$AWK' \$(SHELL) \$(top_srcdir)/exgettext $XGETTEXT"
AC_HEADER_DIRENT
AC_HEADER_STAT
AC_HEADER_SYS_WAIT
AC_STRUCT_ST_MTIM_NSEC
AC_TYPE_PID_T
AC_TYPE_SIGNAL

AC_CHECK_FUNCS(diraccess dup2 gettimeofday \
  sigaction sigprocmask strchr strerror tmpnam)
AC_REPLACE_FUNCS(memchr mkstemp strcasecmp waitpid)
if test $ac_cv_func_mkstemp != yes; then
  LIBOBJS="$LIBOBJS tempname.$ac_objext"
fi
AC_FUNC_ALLOCA
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_FUNC_VPRINTF
jm_FUNC_GLIBC_UNLOCKED_IO
jm_FUNC_GNU_STRFTIME
# No need for AC_FUNC_MEMCMP, since memcmp is used only to test for equality.
jm_FUNC_MALLOC
jm_FUNC_REALLOC
jm_PREREQ_ERROR
jm_PREREQ_QUOTEARG
jm_PREREQ_REGEX
jm_PREREQ_TEMPNAME
jm_AC_PREREQ_XSTRTOUMAX

# Check for clock_gettime and its library.  Solaris puts it in -lrt or
# -lposix4, but we don't want to link that library unless we have to.
diff_saved_libs=$LIBS
AC_SEARCH_LIBS(clock_gettime, [rt posix4])
case $ac_cv_search_clock_gettime in
no | 'none required')
  LIB_CLOCK_GETTIME=;;
*)
  LIB_CLOCK_GETTIME=$ac_cv_search_clock_gettime;;
esac
AC_SUBST([LIB_CLOCK_GETTIME])
AC_CHECK_FUNCS(clock_gettime)
LIBS=$diff_saved_libs

# fnmatch
AC_FUNC_FNMATCH
if test $ac_cv_func_fnmatch_works = yes; then
  rm -f lib/fnmatch.h
else
  AC_LIBOBJ(fnmatch)
  AC_CONFIG_LINKS(lib/fnmatch.h:lib/fnmatch.hin)
fi

# regex
jm_INCLUDED_REGEX([lib/regex.c])
if test "$jm_with_regex" = yes; then
  AC_CONFIG_LINKS(lib/regex.h:lib/regex.hin)
else
  rm -f lib/regex.h
fi

AC_CONFIG_FILES([Makefile doc/Makefile intl/Makefile \
		 lib/Makefile lib/posix/Makefile \
		 m4/Makefile po/Makefile.in src/Makefile])
AC_CONFIG_COMMANDS([default], [date > stamp-h])
AC_OUTPUT
