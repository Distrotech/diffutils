# Configure template for GNU diffutils.
# Copyright 1994, 1995, 1998 Free Software Foundation, Inc.
# Process this file with autoconf to produce a configure script.

AC_INIT(diff.h)
AC_CONFIG_HEADER(config.h:config.hin)
AC_ARG_PROGRAM

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL

AC_PATH_PROG(PR_PROGRAM, pr, "")

# If available, prefer support for large files unless the user specified
# one of the CPPFLAGS, LDFLAGS, or LIBS variables.
AC_MSG_CHECKING(whether large file support needs explicit enabling)
ac_getconfs=''
ac_result=yes
ac_set=''
ac_shellvars='CPPFLAGS LDFLAGS LIBS'
for ac_shellvar in $ac_shellvars; do
  case $ac_shellvar in
  CPPFLAGS) ac_lfsvar=LFS_CFLAGS ;;
  *) ac_lfsvar=LFS_$ac_shellvar ;;
  esac
  eval test '"${'$ac_shellvar'+set}"' = set && ac_set=$ac_shellvar
  (getconf $ac_lfsvar) >/dev/null 2>&1 || { ac_result=no; break; }
  ac_getconf=`getconf $ac_lfsvar`
  ac_getconfs=$ac_getconfs$ac_getconf
  eval ac_test_$ac_shellvar=\$ac_getconf
done
case "$ac_result$ac_getconfs" in
yes) ac_result=no ;;
esac
case "$ac_result$ac_set" in
yes?*) ac_result="yes, but $ac_set is already set, so use its settings"
esac
AC_MSG_RESULT($ac_result)
case $ac_result in
yes)
  for ac_shellvar in $ac_shellvars; do
    eval $ac_shellvar=\$ac_test_$ac_shellvar
  done ;;
esac

AC_ISC_POSIX
AC_MINIX
AC_C_CONST

AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h fcntl.h libintl.h limits.h locale.h stdlib.h string.h sys/file.h time.h)
AC_CHECK_LIB(intl, gettext)
AC_HEADER_DIRENT
AC_HEADER_STAT
AC_HEADER_SYS_WAIT
AC_STRUCT_ST_BLKSIZE
AC_STRUCT_ST_RDEV
AC_TYPE_PID_T
AC_TYPE_SIGNAL

AC_MSG_CHECKING([for st_fstype string in struct stat])
AC_CACHE_VAL(diff_cv_st_fstype_string,
  [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/stat.h>], [struct stat s; s.st_fstype[0] = 'x';],
   diff_cv_st_fstype_string=yes,
   diff_cv_st_fstype_string=no)])
AC_MSG_RESULT($diff_cv_st_fstype_string)
if test $diff_cv_st_fstype_string = yes; then
  AC_DEFINE(HAVE_ST_FSTYPE_STRING)
fi

AC_ARG_WITH(mvfs-stat-bug,
  [  --with-mvfs-stat-bug    work around ClearCase MVFS stat bug (bug id ATRia04618)])
case $with_mvfs_stat_bug in
yes)
  AC_DEFINE(WITH_MVFS_STAT_BUG)
esac

AC_ARG_WITH(nfs-stat-bug,
  [  --with-nfs-stat-bug  work around rare NFS server bugs (see README)])
case $with_nfs_stat_bug in
yes)
  AC_DEFINE(WITH_NFS_STAT_BUG)
esac

# Check for NetBSD 1.0 bug, where memchr(..., 0) returns nonzero.
AC_MSG_CHECKING(for working memchr)
AC_CACHE_VAL(ac_cv_func_memchr,
[AC_TRY_RUN([#include <string.h>
main () { exit (memchr ("", 0, 0) != 0 || memchr ("", 1, 0) != 0); }],
  ac_cv_func_memchr=yes,
  ac_cv_func_memchr=no,
  AC_MSG_WARN([We are cross-compiling so we assume memchr does not work.])
  ac_cv_func_memchr=no)])dnl
AC_MSG_RESULT($ac_cv_func_memchr)
if test $ac_cv_func_memchr = yes; then
  AC_DEFINE(HAVE_MEMCHR)
fi

AC_CHECK_FUNCS(dup2 sigaction sigprocmask strchr strerror tmpnam)
AC_REPLACE_FUNCS(memchr waitpid)
AC_FUNC_ALLOCA
AC_FUNC_CLOSEDIR_VOID
dnl No need for AC_FUNC_MEMCMP, since memcmp is used only to test for equality.
AC_FUNC_VFORK
AC_FUNC_VPRINTF

AC_OUTPUT(Makefile, [date > stamp-h])
