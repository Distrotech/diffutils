# Automakefile for GNU diffutils programs.

# Copyright (C) 2001-2002, 2006, 2009 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

bin_PROGRAMS = cmp diff diff3 sdiff

noinst_HEADERS = system.h

localedir = $(datadir)/locale

AM_CPPFLAGS = -I../lib -I$(top_srcdir)/lib

LDADD = ../lib/libdiffutils.a $(LIBINTL) $(LIBSIGSEGV)
diff_LDADD = $(LDADD) $(LIB_CLOCK_GETTIME)

cmp_SOURCES = cmp.c
diff3_SOURCES = diff3.c
sdiff_SOURCES = sdiff.c
diff_SOURCES = \
  analyze.c context.c diff.c diff.h dir.c ed.c ifdef.c io.c \
  normal.c side.c util.c

MOSTLYCLEANFILES = paths.h paths.ht

cmp.$(OBJEXT) diff3.$(OBJEXT) diff.$(OBJEXT) sdiff.$(OBJEXT): paths.h

gdiff = `echo diff|sed '$(transform)'`
BUILT_SOURCES = paths.h
paths.h:
	$(AM_V_GEN)(echo '#define DEFAULT_DIFF_PROGRAM "'$(gdiff)'"' && \
	  echo '#define LOCALEDIR "$(localedir)"') >$@t && mv $@t $@
