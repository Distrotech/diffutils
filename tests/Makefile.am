TESTS = \
  basic \
  binary \
  help-version	\
  function-line-vs-leading-space \
  label-vs-func	\
  no-newline-at-eof \
  stdin

EXTRA_DIST = \
  $(TESTS) init.sh t-local.sh

# Note that the first lines are statements.  They ensure that environment
# variables that can perturb tests are unset or set to expected values.
# The rest are envvar settings that propagate build-related Makefile
# variables to test scripts.
TESTS_ENVIRONMENT =				\
  tmp__=$$TMPDIR; test -d "$$tmp__" || tmp__=.;	\
  TMPDIR=$$tmp__; export TMPDIR;		\
  exec 9>&2;					\
  shell_or_perl_() {				\
    if grep '^\#!/usr/bin/perl' "$$1" > /dev/null; then			\
      if $(PERL) -e 'use warnings' > /dev/null 2>&1; then		\
	grep '^\#!/usr/bin/perl -T' "$$1" > /dev/null && T_=T || T_=;	\
        $(PERL) -w$$T_ -I$(srcdir) -MCoreutils				\
	      -M"CuTmpdir qw($$f)" -- "$$1";	\
      else					\
	echo 1>&2 "$$tst: configure did not find a usable version of Perl," \
	  "so skipping this test";		\
	(exit 77);				\
      fi;					\
    else					\
      $(SHELL) "$$1";				\
    fi;						\
  };						\
  export					\
  VERSION='$(VERSION)'				\
  abs_top_builddir='$(abs_top_builddir)'	\
  abs_top_srcdir='$(abs_top_srcdir)'		\
  abs_srcdir='$(abs_srcdir)'			\
  built_programs="`$(built_programs)`"		\
  srcdir='$(srcdir)'				\
  top_srcdir='$(top_srcdir)'			\
  CC='$(CC)'					\
  MAKE=$(MAKE)					\
  PACKAGE_BUGREPORT='$(PACKAGE_BUGREPORT)'	\
  PACKAGE_VERSION=$(PACKAGE_VERSION)		\
  CONFIG_HEADER='$(abs_top_builddir)/lib/config.h' \
  ENABLE_DEVICE_MAPPER=$(ENABLE_DEVICE_MAPPER)	\
  PERL='$(PERL)'				\
  PREFERABLY_POSIX_SHELL='$(PREFERABLY_POSIX_SHELL)' \
  REPLACE_GETCWD=$(REPLACE_GETCWD)		\
  PATH='$(abs_top_builddir)/src$(PATH_SEPARATOR)'"$$PATH" \
  ; shell_or_perl_

built_programs =							\
  echo 'spy:;@echo $$(PROGRAMS)'					\
    | MAKEFLAGS= $(MAKE) -s -C $(builddir)/../src -f Makefile -f - spy	\
    | fmt -1 | sed 's,$(EXEEXT)$$,,' | sort -u

VERBOSE = yes
