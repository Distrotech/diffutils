# Automakefile for GNU diffutils man pages

# Copyright (C) 2002, 2009-2010 Free Software Foundation, Inc.

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

dist_man1_MANS = cmp.1 diff.1 diff3.1 sdiff.1
MAINTAINERCLEANFILES = $(dist_man1_MANS)

S = $(top_srcdir)/src
cmp.1: $S/cmp.c
diff.1: $S/diff.c
diff3.1: $S/diff3.c
sdiff.1: $S/sdiff.c

# Depend on configure.ac to get version number changes.
$(dist_man1_MANS): $(top_srcdir)/configure.ac
	$(AM_V_GEN)base=`expr $@ : '\(.*\).1'` && \
	  (echo '[NAME]' && sed 's@/\* *@@; s/-/\\-/; q' $S/$$base.c) | \
	  $(HELP2MAN) -i - -S '$(PACKAGE) $(VERSION)' ../src/$$base | \
	  sed 's/^\.B info .*/.B info diff/' >$@
