# Automakefile for GNU diffutils library.

# Copyright (C) 2001 Free Software Foundation, Inc.

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.

## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
## 02111-1307, USA.

AUTOMAKE_OPTIONS = gnits

noinst_LIBRARIES = libdiffutils.a

EXTRA_DIST = alloca.c fnmatch.c inttostr.c malloc.c memchr.c mkstemp.c \
  realloc.c regex.c strcasecmp.c tempname.c waitpid.c
SUBDIRS = posix

noinst_HEADERS = \
  cmpbuf.h error.h exclude.h fnmatch.hin freesoft.h getopt.h \
  hard-locale.h inttostr.h prepargs.h quotesys.h \
  regex.hin unlocked-io.h xalloc.h

libdiffutils_a_SOURCES = \
  cmpbuf.c error.c exclude.c freesoft.c getopt.c getopt1.c \
  hard-locale.c imaxtostr.c offtostr.c prepargs.c quotesys.c \
  strftime.c umaxtostr.c xmalloc.c

INCLUDES = -I../intl

libdiffutils_a_LIBADD = @ALLOCA@ @LIBOBJS@
libdiffutils_a_DEPENDENCIES = $(libdiffutils_a_LIBADD)

DISTCLEANFILES = fnmatch.h fnmatch.hno regex.h regex.hno
