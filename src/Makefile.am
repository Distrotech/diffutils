# Automakefile for GNU diffutils programs.

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

bin_PROGRAMS = cmp diff diff3 sdiff

noinst_HEADERS = diff.h system.h

datadir = $(prefix)/@DATADIRNAME@
localedir = $(datadir)/locale

INCLUDES = -I.. -I$(srcdir) -I$(top_srcdir)/lib -I../intl -I$(top_srcdir)/intl
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

# Link libdiffutils.a after @INTLLIBS@, since @INTLLIBS@ might invoke
# rpl_realloc.  And link it before @INTTLIBS@, since it invokes i18n
# functions.
LDADD = ../lib/libdiffutils.a @INTLLIBS@ ../lib/libdiffutils.a

cmp_SOURCES = cmp.c version.c
diff3_SOURCES = diff3.c version.c
sdiff_SOURCES = sdiff.c version.c
diff_SOURCES = \
  analyze.c context.c diff.c dir.c ed.c ifdef.c io.c \
  normal.c side.c util.c version.c

MAINTAINERCLEANFILES = $(srcdir)/version.c

$(srcdir)/version.c: $(top_srcdir)/configure.ac
	(echo '#include <config.h>' && \
	 echo 'char const version_string[] = "(GNU $(PACKAGE)) $(VERSION)";') \
		>$@
