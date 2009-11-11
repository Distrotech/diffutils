# Customize maint.mk                           -*- makefile -*-
# Copyright (C) 2003-2009 Free Software Foundation, Inc.

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

# Use alpha.gnu.org for alpha and beta releases.
# Use ftp.gnu.org for major releases.
gnu_ftp_host-alpha = alpha.gnu.org
gnu_ftp_host-beta = alpha.gnu.org
gnu_ftp_host-major = ftp.gnu.org
gnu_rel_host = $(gnu_ftp_host-$(RELEASE_TYPE))

# Used in maint.mk's web-manual rule
manual_title = Comparing and Merging Files

url_dir_list = \
  ftp://$(gnu_rel_host)/gnu/$(PACKAGE)

# The GnuPG ID of the key used to sign the tarballs.
gpg_key_ID = B9AB9A16

# Tests not to run as part of "make distcheck".
local-checks-to-skip =		\
  sc_cast_of_x_alloc_return_value	\
  sc_error_message_period	\
  sc_error_message_uppercase	\
  sc_file_system		\
  sc_m4_quote_check		\
  sc_po_check			\
  sc_program_name		\
  sc_prohibit_HAVE_MBRTOWC	\
  sc_prohibit_cvs_keyword	\
  sc_prohibit_strcmp		\
  sc_require_config_h		\
  sc_require_config_h_first	\
  sc_space_tab			\
  sc_the_the			\
  sc_unmarked_diagnostics

# Tools used to bootstrap this package, used for "announcement".
bootstrap-tools = autoconf,automake,gnulib

# Now that we have better tests, make this the default.
export VERBOSE = yes

old_NEWS_hash = d41d8cd98f00b204e9800998ecf8427e
