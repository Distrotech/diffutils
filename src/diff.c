/* GNU DIFF main routine.

   Copyright (C) 1988, 1989, 1992, 1993, 1994, 1996, 1998, 2001, 2002
   Free Software Foundation, Inc.

   This file is part of GNU DIFF.

   GNU DIFF is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU DIFF is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU DIFF; see the file COPYING.
   If not, write to the Free Software Foundation,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#define GDIFF_MAIN
#include "diff.h"
#include <dirname.h>
#include <error.h>
#include <exclude.h>
#include <fnmatch.h>
#include <freesoft.h>
#include <getopt.h>
#include <hard-locale.h>
#include <prepargs.h>
#include <quotesys.h>
#include <regex.h>
#include <setmode.h>
#include <signal.h>
#include <xalloc.h>

static char const authorship_msgid[] =
  N_("Written by Paul Eggert, Mike Haertel, David Hayes,\n\
Richard Stallman, and Len Tower.");

static char const copyright_string[] =
  "Copyright (C) 2002 Free Software Foundation, Inc.";

#ifndef GUTTER_WIDTH_MINIMUM
# define GUTTER_WIDTH_MINIMUM 3
#endif

struct regexp_list
{
  char *regexps;	/* chars representing disjunction of the regexps */
  size_t len;		/* chars used in `regexps' */
  size_t size;		/* size malloc'ed for `regexps'; 0 if not malloc'ed */
  bool multiple_regexps;/* Does `regexps' represent a disjunction?  */
  struct re_pattern_buffer *buf;
};

static int compare_files (struct comparison const *, char const *, char const *);
static void add_regexp (struct regexp_list *, char const *);
static void summarize_regexp_list (struct regexp_list *);
static void try_help (char const *, char const *) __attribute__((noreturn));
static void check_stdout (void);
static void usage (void);

/* If comparing two directories, compare their common subdirectories
   recursively.  */

static bool recursive;

/* For -F and -I: lists of regular expressions.  */
static struct regexp_list function_regexp_list;
static struct regexp_list ignore_regexp_list;

#if HAVE_SETMODE_DOS
/* Use binary input/output (--binary).  */
static bool binary;
#endif

/* If a file is new (appears in only one dir)
   include its entire contents (-N).
   Then `patch' would create the file with appropriate contents.  */
static bool new_file;

/* If a file is new (appears in only the second dir)
   include its entire contents (-P).
   Then `patch' would create the file with appropriate contents.  */
static bool unidirectional_new_file;

/* Report files compared that match (-s).
   Normally nothing is output when that happens.  */
static bool report_identical_files;


/* Return a string containing the command options with which diff was invoked.
   Spaces appear between what were separate ARGV-elements.
   There is a space at the beginning but none at the end.
   If there were no options, the result is an empty string.

   Arguments: OPTIONVEC, a vector containing separate ARGV-elements, and COUNT,
   the length of that vector.  */

static char *
option_list (char **optionvec, int count)
{
  int i;
  size_t size = 1;
  char *result;
  char *p;

  for (i = 0; i < count; i++)
    size += 1 + quote_system_arg ((char *) 0, optionvec[i]);

  p = result = xmalloc (size);

  for (i = 0; i < count; i++)
    {
      *p++ = ' ';
      p += quote_system_arg (p, optionvec[i]);
    }

  *p = 0;
  return result;
}


/* Return an option value suitable for add_exclude.  */

static int
exclude_options (void)
{
  return EXCLUDE_WILDCARDS | (ignore_file_name_case ? FNM_CASEFOLD : 0);
}

/* Values for long options that do not have single-letter equivalents.  */
enum
{
  BINARY_OPTION = CHAR_MAX + 1,
  FROM_FILE_OPTION,
  HELP_OPTION,
  HORIZON_LINES_OPTION,
  IGNORE_FILE_NAME_CASE_OPTION,
  INHIBIT_HUNK_MERGE_OPTION,
  LEFT_COLUMN_OPTION,
  LINE_FORMAT_OPTION,
  NO_IGNORE_FILE_NAME_CASE_OPTION,
  NORMAL_OPTION,
  SDIFF_MERGE_ASSIST_OPTION,
  STRIP_TRAILING_CR_OPTION,
  SUPPRESS_COMMON_LINES_OPTION,
  TO_FILE_OPTION,

  /* These options must be in sequence.  */
  UNCHANGED_LINE_FORMAT_OPTION,
  OLD_LINE_FORMAT_OPTION,
  NEW_LINE_FORMAT_OPTION,

  /* These options must be in sequence.  */
  UNCHANGED_GROUP_FORMAT_OPTION,
  OLD_GROUP_FORMAT_OPTION,
  NEW_GROUP_FORMAT_OPTION,
  CHANGED_GROUP_FORMAT_OPTION
};

static struct option const longopts[] =
{
  {"binary", 0, 0, BINARY_OPTION},
  {"brief", 0, 0, 'q'},
  {"changed-group-format", 1, 0, CHANGED_GROUP_FORMAT_OPTION},
  {"context", 2, 0, 'C'},
  {"ed", 0, 0, 'e'},
  {"exclude", 1, 0, 'x'},
  {"exclude-from", 1, 0, 'X'},
  {"expand-tabs", 0, 0, 't'},
  {"forward-ed", 0, 0, 'f'},
  {"from-file", 1, 0, FROM_FILE_OPTION},
  {"help", 0, 0, HELP_OPTION},
  {"horizon-lines", 1, 0, HORIZON_LINES_OPTION},
  {"ifdef", 1, 0, 'D'},
  {"ignore-all-space", 0, 0, 'w'},
  {"ignore-blank-lines", 0, 0, 'B'},
  {"ignore-case", 0, 0, 'i'},
  {"ignore-file-name-case", 0, 0, IGNORE_FILE_NAME_CASE_OPTION},
  {"ignore-matching-lines", 1, 0, 'I'},
  {"ignore-space-change", 0, 0, 'b'},
  {"ignore-tab-expansion", 0, 0, 'E'},
  {"inhibit-hunk-merge", 0, 0, INHIBIT_HUNK_MERGE_OPTION},
  {"initial-tab", 0, 0, 'T'},
  {"label", 1, 0, 'L'},
  {"left-column", 0, 0, LEFT_COLUMN_OPTION},
  {"line-format", 1, 0, LINE_FORMAT_OPTION},
  {"minimal", 0, 0, 'd'},
  {"new-file", 0, 0, 'N'},
  {"new-group-format", 1, 0, NEW_GROUP_FORMAT_OPTION},
  {"new-line-format", 1, 0, NEW_LINE_FORMAT_OPTION},
  {"no-ignore-file-name-case", 0, 0, NO_IGNORE_FILE_NAME_CASE_OPTION},
  {"normal", 0, 0, NORMAL_OPTION},
  {"old-group-format", 1, 0, OLD_GROUP_FORMAT_OPTION},
  {"old-line-format", 1, 0, OLD_LINE_FORMAT_OPTION},
  {"paginate", 0, 0, 'l'},
  {"rcs", 0, 0, 'n'},
  {"recursive", 0, 0, 'r'},
  {"report-identical-files", 0, 0, 's'},
  {"sdiff-merge-assist", 0, 0, SDIFF_MERGE_ASSIST_OPTION},
  {"show-c-function", 0, 0, 'p'},
  {"show-function-line", 1, 0, 'F'},
  {"side-by-side", 0, 0, 'y'},
  {"speed-large-files", 0, 0, 'H'},
  {"starting-file", 1, 0, 'S'},
  {"strip-trailing-cr", 0, 0, STRIP_TRAILING_CR_OPTION},
  {"suppress-common-lines", 0, 0, SUPPRESS_COMMON_LINES_OPTION},
  {"text", 0, 0, 'a'},
  {"to-file", 1, 0, TO_FILE_OPTION},
  {"unchanged-group-format", 1, 0, UNCHANGED_GROUP_FORMAT_OPTION},
  {"unchanged-line-format", 1, 0, UNCHANGED_LINE_FORMAT_OPTION},
  {"unidirectional-new-file", 0, 0, 'P'},
  {"unified", 2, 0, 'U'},
  {"version", 0, 0, 'v'},
  {"width", 1, 0, 'W'},
  {0, 0, 0, 0}
};

int
main (int argc, char **argv)
{
  int exit_status = 0;
  int c;
  int i;
  int prev = -1;
  int width = 130;
  bool show_c_function = 0;
  char const *from_file = 0;
  char const *to_file = 0;
  uintmax_t numval;
  char *numend;

  /* Do our initializations.  */
  xalloc_exit_failure = 2;
  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
  context = -1;
  horizon_lines = -1;
  function_regexp_list.buf = &function_regexp;
  ignore_regexp_list.buf = &ignore_regexp;
  re_set_syntax (RE_SYNTAX_GREP | RE_NO_POSIX_BACKTRACKING);
  excluded = new_exclude ();

  if (hard_locale (LC_TIME))
    time_format = "%Y-%m-%d %H:%M:%S.%N %z";
  else
    {
      /* See POSIX 1003.2-1992 section 4.17.6.1.4 for this format.
         POSIX 1003.1-2001 is unchanged here.  */
      time_format = "%a %b %e %T %Y";
    }

  /* Decode the options.  */

  while ((c = getopt_long (argc, argv,
			   "0123456789abBcC:dD:eEfF:hHiI:lL:nNpPqrsS:tTuU:vwW:x:X:y",
			   longopts, 0))
	 != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	  /* All digits combine in decimal to specify the context-size.  */
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '0':
	  if (context == -1)
	    context = 0;
	  /* If a context length has already been specified,
	     more digits allowed only if they follow right after the others.
	     Reject two separate runs of digits, or digits after -C.  */
	  else if (! ISDIGIT (prev))
	    fatal ("context length specified twice");

	  context = context * 10 + c - '0';
	  break;

	case 'a':
	  /* Treat all files as text files; never treat as binary.  */
	  text = 1;
	  break;

	case 'b':
	  if (ignore_white_space < IGNORE_SPACE_CHANGE)
	    ignore_white_space = IGNORE_SPACE_CHANGE;
	  break;

	case 'B':
	  /* Ignore changes affecting only blank lines.  */
	  ignore_blank_lines = 1;
	  break;

	case 'C':		/* +context[=lines] */
	case 'U':		/* +unified[=lines] */
	  if (optarg)
	    {
	      errno = 0;
	      context = numval = strtoumax (optarg, &numend, 10);
	      if (context < 0 || context != numval || errno || *numend)
		try_help ("invalid context length `%s'", optarg);
	    }
	  /* Fall through.  */
	case 'c':
	  /* Make context-style output.  */
	  output_style = c == 'U' ? OUTPUT_UNIFIED : OUTPUT_CONTEXT;
	  break;

	case 'd':
	  /* Don't discard lines.  This makes things slower (sometimes much
	     slower) but will find a guaranteed minimal set of changes.  */
	  minimal = 1;
	  break;

	case 'D':
	  /* Make merged #ifdef output.  */
	  output_style = OUTPUT_IFDEF;
	  {
	    static char const C_ifdef_group_formats[] =
	      "%%=%c#ifndef %s\n%%<#endif /* ! %s */\n%c#ifdef %s\n%%>#endif /* %s */\n%c#ifndef %s\n%%<#else /* %s */\n%%>#endif /* %s */\n";
	    char *b = xmalloc (sizeof C_ifdef_group_formats
			       + 7 * strlen (optarg) - 14 /* 7*"%s" */
			       - 8 /* 5*"%%" + 3*"%c" */);
	    sprintf (b, C_ifdef_group_formats,
		     0,
		     optarg, optarg, 0,
		     optarg, optarg, 0,
		     optarg, optarg, optarg);
	    for (i = 0; i < sizeof group_format / sizeof *group_format; i++)
	      {
		group_format[i] = b;
		b += strlen (b) + 1;
	      }
	  }
	  break;

	case 'e':
	  /* Make output that is a valid `ed' script.  */
	  output_style = OUTPUT_ED;
	  break;

	case 'E':
	  if (ignore_white_space < IGNORE_TAB_EXPANSION)
	    ignore_white_space = IGNORE_TAB_EXPANSION;
	  break;

	case 'f':
	  /* Make output that looks vaguely like an `ed' script
	     but has changes in the order they appear in the file.  */
	  output_style = OUTPUT_FORWARD_ED;
	  break;

	case 'F':
	  /* Show, for each set of changes, the previous line that
	     matches the specified regexp.  Currently affects only
	     context-style output.  */
	  add_regexp (&function_regexp_list, optarg);
	  break;

	case 'h':
	  /* Split the files into chunks of around 1500 lines
	     for faster processing.  Usually does not change the result.

	     This currently has no effect.  */
	  break;

	case 'H':
	  /* Turn on heuristics that speed processing of large files
	     with a small density of changes.  */
	  speed_large_files = 1;
	  break;

	case 'i':
	  /* Ignore changes in case.  */
	  ignore_case = 1;
	  break;

	case 'I':
	  /* Ignore changes affecting only lines that match the
	     specified regexp.  */
	  add_regexp (&ignore_regexp_list, optarg);
	  break;

	case 'l':
	  /* Pass the output through "pr" to paginate it.  */
	  if (!pr_program[0])
	    try_help ("pagination not supported on this host", 0);
	  paginate = 1;
#if !defined SIGCHLD && defined SIGCLD
# define SIGCHLD SIGCLD
#endif
#ifdef SIGCHLD
	  /* Pagination requires forking and waiting, and
	     System V fork+wait does not work if SIGCHLD is ignored.  */
	  signal (SIGCHLD, SIG_DFL);
#endif
	  break;

	case 'L':
	  /* Specify file labels for `-c' output headers.  */
	  if (!file_label[0])
	    file_label[0] = optarg;
	  else if (!file_label[1])
	    file_label[1] = optarg;
	  else
	    fatal ("too many file label options");
	  break;

	case 'n':
	  /* Output RCS-style diffs, like `-f' except that each command
	     specifies the number of lines affected.  */
	  output_style = OUTPUT_RCS;
	  break;

	case 'N':
	  /* When comparing directories, if a file appears only in one
	     directory, treat it as present but empty in the other.  */
	  new_file = 1;
	  break;

	case 'p':
	  /* Make context-style output and show name of last C function.  */
	  show_c_function = 1;
	  add_regexp (&function_regexp_list, "^[[:alpha:]$_]");
	  break;

	case 'P':
	  /* When comparing directories, if a file appears only in
	     the second directory of the two,
	     treat it as present but empty in the other.  */
	  unidirectional_new_file = 1;
	  break;

	case 'q':
	  brief = 1;
	  break;

	case 'r':
	  /* When comparing directories,
	     recursively compare any subdirectories found.  */
	  recursive = 1;
	  break;

	case 's':
	  /* Print a message if the files are the same.  */
	  report_identical_files = 1;
	  break;

	case 'S':
	  /* When comparing directories, start with the specified
	     file name.  This is used for resuming an aborted comparison.  */
	  starting_file = optarg;
	  break;

	case 't':
	  /* Expand tabs to spaces in the output so that it preserves
	     the alignment of the input files.  */
	  expand_tabs = 1;
	  break;

	case 'T':
	  /* Use a tab in the output, rather than a space, before the
	     text of an input line, so as to keep the proper alignment
	     in the input line without changing the characters in it.  */
	  initial_tab = 1;
	  break;

	case 'u':
	  /* Output the context diff in unidiff format.  */
	  output_style = OUTPUT_UNIFIED;
	  break;

	case 'v':
	  printf ("diff %s\n%s\n\n%s\n\n%s\n",
		  version_string, copyright_string,
		  _(free_software_msgid), _(authorship_msgid));
	  check_stdout ();
	  exit (0);

	case 'w':
	  ignore_white_space = IGNORE_ALL_SPACE;
	  break;

	case 'x':
	  add_exclude (excluded, optarg, exclude_options ());
	  break;

	case 'X':
	  if (add_exclude_file (add_exclude, excluded, optarg,
				exclude_options (), '\n'))
	    pfatal_with_name (optarg);
	  break;

	case 'y':
	  /* Use side-by-side (sdiff-style) columnar output.  */
	  output_style = OUTPUT_SDIFF;
	  break;

	case 'W':
	  /* Set the line width for OUTPUT_SDIFF.  */
	  errno = 0;
	  width = numval = strtoumax (optarg, &numend, 10);
	  if (width <= 0 || width != numval || errno || *numend)
	    try_help ("invalid width `%s'", optarg);
	  break;

	case BINARY_OPTION:
	  /* Use binary I/O when reading and writing data.
	     On POSIX hosts, this has no effect.  */
#if HAVE_SETMODE_DOS
	  binary = 1;
	  set_binary_mode (STDOUT_FILENO, 1);
#endif
	  break;

	case FROM_FILE_OPTION:
	  if (from_file)
	    fatal ("multiple `--from-file' options");
	  from_file = optarg;
	  break;

	case HELP_OPTION:
	  usage ();
	  check_stdout ();
	  exit (0);

	case HORIZON_LINES_OPTION:
	  errno = 0;
	  horizon_lines = numval = strtoumax (optarg, &numend, 10);
	  if (horizon_lines < 0 || horizon_lines != numval || errno || *numend)
	    try_help ("invalid horizon length `%s'", optarg);
	  break;

	case IGNORE_FILE_NAME_CASE_OPTION:
	  ignore_file_name_case = 1;
	  break;

	case INHIBIT_HUNK_MERGE_OPTION:
	  /* This option is obsolete, but accept it for backward
             compatibility.  */
	  break;

	case LEFT_COLUMN_OPTION:
	  left_column = 1;
	  break;

	case LINE_FORMAT_OPTION:
	  output_style = OUTPUT_IFDEF;
	  for (i = 0; i < sizeof line_format / sizeof *line_format; i++)
	    line_format[i] = optarg;
	  break;

	case NO_IGNORE_FILE_NAME_CASE_OPTION:
	  ignore_file_name_case = 0;
	  break;

	case NORMAL_OPTION:
	  output_style = OUTPUT_NORMAL;
	  break;

	case SDIFF_MERGE_ASSIST_OPTION:
	  output_style = OUTPUT_SDIFF;
	  sdiff_merge_assist = 1;
	  break;

	case STRIP_TRAILING_CR_OPTION:
	  strip_trailing_cr = 1;
	  break;

	case SUPPRESS_COMMON_LINES_OPTION:
	  suppress_common_lines = 1;
	  break;

	case TO_FILE_OPTION:
	  if (to_file)
	    fatal ("multiple `--to-file' options");
	  to_file = optarg;
	  break;

	case UNCHANGED_LINE_FORMAT_OPTION:
	case OLD_LINE_FORMAT_OPTION:
	case NEW_LINE_FORMAT_OPTION:
	  output_style = OUTPUT_IFDEF;
	  line_format[c - UNCHANGED_LINE_FORMAT_OPTION] = optarg;
	  break;

	case UNCHANGED_GROUP_FORMAT_OPTION:
	case OLD_GROUP_FORMAT_OPTION:
	case NEW_GROUP_FORMAT_OPTION:
	case CHANGED_GROUP_FORMAT_OPTION:
	  output_style = OUTPUT_IFDEF;
	  group_format[c - UNCHANGED_GROUP_FORMAT_OPTION] = optarg;
	  break;

	default:
	  try_help (0, 0);
	}
      prev = c;
    }

  {
    /*
     *	We maximize first the half line width, and then the gutter width,
     *	according to the following constraints:
     *	1.  Two half lines plus a gutter must fit in a line.
     *	2.  If the half line width is nonzero:
     *	    a.  The gutter width is at least GUTTER_WIDTH_MINIMUM.
     *	    b.  If tabs are not expanded to spaces,
     *		a half line plus a gutter is an integral number of tabs,
     *		so that tabs in the right column line up.
     */
    int t = expand_tabs ? 1 : TAB_WIDTH;
    int off = (width + t + GUTTER_WIDTH_MINIMUM) / (2*t)  *  t;
    sdiff_half_width = MAX (0, MIN (off - GUTTER_WIDTH_MINIMUM, width - off)),
    sdiff_column2_offset = sdiff_half_width ? off : width;
  }

  if (show_c_function && output_style != OUTPUT_UNIFIED)
    output_style = OUTPUT_CONTEXT;

  if (output_style == OUTPUT_UNSPECIFIED)
    output_style = OUTPUT_NORMAL;

  if (output_style != OUTPUT_CONTEXT && output_style != OUTPUT_UNIFIED)
    context = 0;
  else if (context == -1)
    /* Default amount of context for -c.  */
    context = 3;

  /* Make the horizon at least as large as the context, so that
     shift_boundaries has more freedom to shift the first and last hunks.  */
  if (horizon_lines < context)
    horizon_lines = context;

  summarize_regexp_list (&function_regexp_list);
  summarize_regexp_list (&ignore_regexp_list);

  if (output_style == OUTPUT_IFDEF)
    {
      for (i = 0; i < sizeof line_format / sizeof *line_format; i++)
	if (!line_format[i])
	  line_format[i] = "%l\n";
      if (!group_format[OLD])
	group_format[OLD]
	  = group_format[CHANGED] ? group_format[CHANGED] : "%<";
      if (!group_format[NEW])
	group_format[NEW]
	  = group_format[CHANGED] ? group_format[CHANGED] : "%>";
      if (!group_format[UNCHANGED])
	group_format[UNCHANGED] = "%=";
      if (!group_format[CHANGED])
	group_format[CHANGED] = concat (group_format[OLD],
					group_format[NEW], "");
    }

  no_diff_means_no_output =
    (output_style == OUTPUT_IFDEF ?
      (!*group_format[UNCHANGED]
       || (strcmp (group_format[UNCHANGED], "%=") == 0
	   && !*line_format[UNCHANGED]))
     : (output_style != OUTPUT_SDIFF) | suppress_common_lines);

  files_can_be_treated_as_binary =
    (brief
     & ~ (ignore_blank_lines | ignore_case | strip_trailing_cr
	  | (ignore_regexp_list.regexps || ignore_white_space)));

  switch_string = option_list (argv + 1, optind - 1);

  if (from_file)
    {
      for (;  optind < argc;  optind++)
	{
	  int status = compare_files ((struct comparison *) 0,
				      from_file, argv[optind]);
	  if (exit_status < status)
	    exit_status = status;
	}
    }

  if (to_file)
    {
      for (;  optind < argc;  optind++)
	{
	  int status = compare_files ((struct comparison *) 0,
				      argv[optind], to_file);
	  if (exit_status < status)
	    exit_status = status;
	}
    }

  if (!from_file && !to_file)
    {
      if (argc - optind != 2)
	{
	  if (argc - optind < 2)
	    try_help ("missing operand after `%s'", argv[argc - 1]);
	  else
	    try_help ("extra operand `%s'", argv[optind + 2]);
	}

      exit_status = compare_files ((struct comparison *) 0,
				   argv[optind], argv[optind + 1]);
    }

  /* Print any messages that were saved up for last.  */
  print_message_queue ();

  check_stdout ();
  exit (exit_status);
  return exit_status;
}

/* Append to REGLIST the regexp PATTERN.  */

static void
add_regexp (struct regexp_list *reglist, char const *pattern)
{
  size_t patlen = strlen (pattern);
  char const *m = re_compile_pattern (pattern, patlen, reglist->buf);

  if (m != 0)
    error (0, 0, "%s: %s", pattern, m);
  else
    {
      char *regexps = reglist->regexps;
      size_t len = reglist->len;
      bool multiple_regexps = reglist->multiple_regexps = regexps != 0;
      size_t newlen = reglist->len = len + 2 * multiple_regexps + patlen;
      size_t size = reglist->size;

      if (size <= newlen)
	{
	  if (!size)
	    size = 1;

	  do size *= 2;
	  while (size <= newlen);

	  reglist->size = size;
	  reglist->regexps = regexps = xrealloc (regexps, size);
	}
      if (multiple_regexps)
	{
	  regexps[len++] = '\\';
	  regexps[len++] = '|';
	}
      memcpy (regexps + len, pattern, patlen + 1);
    }
}

/* Ensure that REGLIST represents the disjunction of its regexps.
   This is done here, rather than earlier, to avoid O(N^2) behavior.  */

static void
summarize_regexp_list (struct regexp_list *reglist)
{
  if (reglist->regexps)
    {
      /* At least one regexp was specified.  Allocate a fastmap for it.  */
      reglist->buf->fastmap = xmalloc (1 << CHAR_BIT);
      if (reglist->multiple_regexps)
	{
	  /* Compile the disjunction of the regexps.
	     (If just one regexp was specified, it is already compiled.)  */
	  char const *m = re_compile_pattern (reglist->regexps, reglist->len,
					      reglist->buf);
	  if (m != 0)
	    error (2, 0, "%s: %s", reglist->regexps, m);
	}
    }
}

static void
try_help (char const *reason_msgid, char const *operand)
{
  if (reason_msgid)
    error (0, 0, _(reason_msgid), operand);
  error (2, 0, _("Try `%s --help' for more information."), program_name);
  abort ();
}

static void
check_stdout (void)
{
  if (ferror (stdout))
    fatal ("write failed");
  else if (fclose (stdout) != 0)
    pfatal_with_name (_("standard output"));
}

static char const * const option_help_msgid[] = {
  N_("FILES are `FILE1 FILE2' or `DIR1 DIR2' or `DIR FILE...' or `FILE... DIR';"),
  N_("if --from-file or --to-file is given, there are no restrictions on FILES."),
  N_("If a FILE is `-', read standard input."),
  "",
  N_("-i  --ignore-case  Ignore case differences in file contents."),
  N_("--ignore-file-name-case  Ignore case when comparing file names."),
  N_("--no-ignore-file-name-case  Consider case when comparing file names."),
  N_("-E  --ignore-tab-expansion  Ignore changes due to tab expansion."),
  N_("-b  --ignore-space-change  Ignore changes in the amount of white space."),
  N_("-w  --ignore-all-space  Ignore all white space."),
  N_("-B  --ignore-blank-lines  Ignore changes whose lines are all blank."),
  N_("-I RE  --ignore-matching-lines=RE  Ignore changes whose lines all match RE."),
  N_("--strip-trailing-cr  Strip trailing carriage return on input."),
#if HAVE_SETMODE_DOS
  N_("--binary  Read and write data in binary mode."),
#endif
  N_("-a  --text  Treat all files as text."),
  "",
  N_("-c  -C NUM  --context[=NUM]  Output NUM (default 3) lines of copied context.\n\
-u  -U NUM  --unified[=NUM]  Output NUM (default 3) lines of unified context.\n\
  -NUM  Use NUM context lines.\n\
  -L LABEL  --label LABEL  Use LABEL instead of file name.\n\
  -p  --show-c-function  Show which C function each change is in.\n\
  -F RE  --show-function-line=RE  Show the most recent line matching RE."),
  N_("-q  --brief  Output only whether files differ."),
  N_("-e  --ed  Output an ed script."),
  N_("--normal  Output a normal diff."),
  N_("-n  --rcs  Output an RCS format diff."),
  N_("-y  --side-by-side  Output in two columns.\n\
  -W NUM  --width=NUM  Output at most NUM (default 130) print columns.\n\
  --left-column  Output only the left column of common lines.\n\
  --suppress-common-lines  Do not output common lines."),
  N_("-D NAME  --ifdef=NAME  Output merged file to show `#ifdef NAME' diffs."),
  N_("--GTYPE-group-format=GFMT  Similar, but format GTYPE input groups with GFMT."),
  N_("--line-format=LFMT  Similar, but format all input lines with LFMT."),
  N_("--LTYPE-line-format=LFMT  Similar, but format LTYPE input lines with LFMT."),
  N_("  LTYPE is `old', `new', or `unchanged'.  GTYPE is LTYPE or `changed'."),
  N_("  GFMT may contain:\n\
    %<  lines from FILE1\n\
    %>  lines from FILE2\n\
    %=  lines common to FILE1 and FILE2\n\
    %[-][WIDTH][.[PREC]]{doxX}LETTER  printf-style spec for LETTER\n\
      LETTERs are as follows for new group, lower case for old group:\n\
        F  first line number\n\
        L  last line number\n\
        N  number of lines = L-F+1\n\
        E  F-1\n\
        M  L+1"),
  N_("  LFMT may contain:\n\
    %L  contents of line\n\
    %l  contents of line, excluding any trailing newline\n\
    %[-][WIDTH][.[PREC]]{doxX}n  printf-style spec for input line number"),
  N_("  Either GFMT or LFMT may contain:\n\
    %%  %\n\
    %c'C'  the single character C\n\
    %c'\\OOO'  the character with octal code OOO"),
  "",
  N_("-l  --paginate  Pass the output through `pr' to paginate it."),
  N_("-t  --expand-tabs  Expand tabs to spaces in output."),
  N_("-T  --initial-tab  Make tabs line up by prepending a tab."),
  "",
  N_("-r  --recursive  Recursively compare any subdirectories found."),
  N_("-N  --new-file  Treat absent files as empty."),
  N_("-P  --unidirectional-new-file  Treat absent first files as empty."),
  N_("-s  --report-identical-files  Report when two files are the same."),
  N_("-x PAT  --exclude=PAT  Exclude files that match PAT."),
  N_("-X FILE  --exclude-from=FILE  Exclude files that match any pattern in FILE."),
  N_("-S FILE  --starting-file=FILE  Start with FILE when comparing directories."),
  N_("--from-file=FILE1  Compare FILE1 to all operands.  FILE1 can be a directory."),
  N_("--to-file=FILE2  Compare all operands to FILE2.  FILE2 can be a directory."),
  "",
  N_("--horizon-lines=NUM  Keep NUM lines of the common prefix and suffix."),
  N_("--inhibit-hunk-merge  Do not merge hunks."),
  N_("-d  --minimal  Try hard to find a smaller set of changes."),
  N_("-H  --speed-large-files  Assume large files and many scattered small changes."),
  "",
  N_("-v  --version  Output version info."),
  N_("--help  Output this help."),
  "",
  N_("Report bugs to <bug-gnu-utils@gnu.org>."),
  0
};

static void
usage (void)
{
  char const * const *p;

  printf (_("Usage: %s [OPTION]... FILES\n"), program_name);

  for (p = option_help_msgid;  *p;  p++)
    {
      if (!**p)
	putchar ('\n');
      else
	{
	  char const *msg = _(*p);
	  char const *nl;
	  while ((nl = strchr (msg, '\n')))
	    {
	      int msglen = nl + 1 - msg;
	      printf ("  %.*s", msglen, msg);
	      msg = nl + 1;
	    }

	  printf ("  %s\n" + 2 * (*msg != ' ' && *msg != '-'), msg);
	}
    }
}

static char const *
filetype (struct stat const *st)
{
  /* See POSIX 1003.2-1992 section 4.17.6.1.1 and Table 5-1 for these formats.
     POSIX 1003.1-2001 is unchanged here.

     To keep diagnostics grammatical in English, the returned string
     must start with a consonant.  */

  if (S_ISREG (st->st_mode))
    {
      bool executable = (st->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0;
	  
      if (st->st_size == 0)
	return (executable
		? _("regular empty executable file")
		: _("regular empty file"));
      /* POSIX 1003.2-1992 section 5.14.2 seems to suggest that we
	 must read the file and guess whether it's C, Fortran, etc.,
	 but this is somewhat useless and doesn't reflect historical
	 practice.  POSIX 1003.1-2001 is unchanged here.  We're
	 allowed to guess wrong, so don't bother to read the file.  */
      return (executable
	      ? _("regular executable file")
	      : _("regular file"));
    }
  if (S_ISDIR (st->st_mode)) return _("directory");

  /* other POSIX file types */
#ifdef S_ISBLK
  if (S_ISBLK (st->st_mode)) return _("block special file");
#endif
#ifdef S_ISCHR
  if (S_ISCHR (st->st_mode)) return _("character special file");
#endif
#ifdef S_ISFIFO
  if (S_ISFIFO (st->st_mode)) return _("fifo");
#endif
#ifdef S_TYPEISMQ
  if (S_TYPEISMQ (st)) return _("message queue");
#endif
#ifdef S_TYPEISSEM
  if (S_TYPEISSEM (st)) return _("semaphore");
#endif
#ifdef S_TYPEISSHM
  if (S_TYPEISSHM (st)) return _("shared memory object");
#endif

  /* other popular file types */
  /* S_ISLNK is impossible with `fstat' and `stat'.  */
#ifdef S_ISSOCK
  if (S_ISSOCK (st->st_mode)) return _("socket");
#endif

  return _("weird file");
}

/* Set the last-modified time of *ST to be the current time.  */

static void
set_mtime_to_now (struct stat *st)
{
#ifdef ST_MTIM_NSEC

# if HAVE_CLOCK_GETTIME && defined CLOCK_REALTIME
  if (clock_gettime (CLOCK_REALTIME, &st->st_mtim) == 0)
    return;
# endif

# if HAVE_GETTIMEOFDAY
  {
    struct timeval timeval;
    if (gettimeofday (&timeval, NULL) == 0)
      {
	st->st_mtime = timeval.tv_sec;
	st->st_mtim.ST_MTIM_NSEC = timeval.tv_usec * 1000;
	return;
      }
  }
# endif

#endif /* ST_MTIM_NSEC */

  time (&st->st_mtime);
}

/* Compare two files (or dirs) with parent comparison PARENT
   and names NAME0 and NAME1.
   (If PARENT is 0, then the first name is just NAME0, etc.)
   This is self-contained; it opens the files and closes them.

   Value is 0 if files are the same, 1 if different,
   2 if there is a problem opening them.  */

static int
compare_files (struct comparison const *parent,
	       char const *name0,
	       char const *name1)
{
  struct comparison cmp;
#define DIR_P(f) (S_ISDIR (cmp.file[f].stat.st_mode) != 0)
  register int f;
  int status = 0;
  bool same_files;
  char *free0, *free1;

  /* If this is directory comparison, perhaps we have a file
     that exists only in one of the directories.
     If so, just print a message to that effect.  */

  if (! ((name0 && name1)
	 || (unidirectional_new_file && name1)
	 || new_file))
    {
      char const *name = name0 == 0 ? name1 : name0;
      char const *dir = parent->file[name0 == 0].name;

      /* See POSIX 1003.2-1992 section 4.17.6.1.1 for this format.
	 POSIX 1003.1-2001 is unchanged here.  */
      message ("Only in %s: %s\n", dir, name);

      /* Return 1 so that diff_dirs will return 1 ("some files differ").  */
      return 1;
    }

  memset (cmp.file, 0, sizeof cmp.file);
  cmp.parent = parent;

  /* cmp.file[f].desc markers */
#define NONEXISTENT (-1) /* nonexistent file */
#define UNOPENED (-2) /* unopened file (e.g. directory) */
#define ERRNO_ENCODE(errno) (-3 - (errno)) /* encoded errno value */

#define ERRNO_DECODE(desc) (-3 - (desc)) /* inverse of ERRNO_ENCODE */

  cmp.file[0].desc = name0 == 0 ? NONEXISTENT : UNOPENED;
  cmp.file[1].desc = name1 == 0 ? NONEXISTENT : UNOPENED;

  /* Now record the full name of each file, including nonexistent ones.  */

  if (name0 == 0)
    name0 = name1;
  if (name1 == 0)
    name1 = name0;

  if (!parent)
    {
      free0 = 0;
      free1 = 0;
      cmp.file[0].name = name0;
      cmp.file[1].name = name1;
    }
  else
    {
      cmp.file[0].name = free0
	= dir_file_pathname (parent->file[0].name, name0);
      cmp.file[1].name = free1
	= dir_file_pathname (parent->file[1].name, name1);
    }

  /* Stat the files.  */

  for (f = 0; f < 2; f++)
    {
      if (cmp.file[f].desc != NONEXISTENT)
	{
	  if (f && file_name_cmp (cmp.file[f].name, cmp.file[0].name) == 0)
	    {
	      cmp.file[f].desc = cmp.file[0].desc;
	      cmp.file[f].stat = cmp.file[0].stat;
	    }
	  else if (strcmp (cmp.file[f].name, "-") == 0)
	    {
	      cmp.file[f].desc = STDIN_FILENO;
	      if (fstat (STDIN_FILENO, &cmp.file[f].stat) != 0)
		cmp.file[f].desc = ERRNO_ENCODE (errno);
	      else
		{
		  if (S_ISREG (cmp.file[f].stat.st_mode))
		    {
		      off_t pos = lseek (STDIN_FILENO, (off_t) 0, SEEK_CUR);
		      if (pos < 0)
			cmp.file[f].desc = ERRNO_ENCODE (errno);
		      else
			cmp.file[f].stat.st_size =
			  MAX (0, cmp.file[f].stat.st_size - pos);
		    }

		  /* POSIX 1003.2-1992 4.17.6.1.4 requires current
		     time for stdin.  POSIX 1003.1-2001 is unchanged
		     here.  */
		  set_mtime_to_now (&cmp.file[f].stat);
		}
	    }
	  else if (stat (cmp.file[f].name, &cmp.file[f].stat) != 0)
	    cmp.file[f].desc = ERRNO_ENCODE (errno);
	}
    }

  /* Mark files as nonexistent at the top level as needed for -N and -P.  */
  if (! parent)
    {
      if ((new_file | unidirectional_new_file)
	  && cmp.file[0].desc == ERRNO_ENCODE (ENOENT)
	  && cmp.file[1].desc == UNOPENED)
	cmp.file[0].desc = NONEXISTENT;

      if (new_file
	  && cmp.file[0].desc == UNOPENED
	  && cmp.file[1].desc == ERRNO_ENCODE (ENOENT))
	cmp.file[1].desc = NONEXISTENT;
    }

  for (f = 0; f < 2; f++)
    if (cmp.file[f].desc == NONEXISTENT)
      cmp.file[f].stat.st_mode = cmp.file[1 - f].stat.st_mode;

  for (f = 0; f < 2; f++)
    {
      int e = ERRNO_DECODE (cmp.file[f].desc);
      if (0 <= e)
	{
	  errno = e;
	  perror_with_name (cmp.file[f].name);
	  status = 2;
	}
    }

  if (status == 0 && ! parent && DIR_P (0) != DIR_P (1))
    {
      /* If one is a directory, and it was specified in the command line,
	 use the file in that dir with the other file's basename.  */

      int fnm_arg = DIR_P (0);
      int dir_arg = 1 - fnm_arg;
      char const *fnm = cmp.file[fnm_arg].name;
      char const *dir = cmp.file[dir_arg].name;
      char const *filename = cmp.file[dir_arg].name = free0
	= dir_file_pathname (dir, base_name (fnm));

      if (strcmp (fnm, "-") == 0)
	fatal ("cannot compare `-' to a directory");

      if (stat (filename, &cmp.file[dir_arg].stat) != 0)
	{
	  perror_with_name (filename);
	  status = 2;
	}
    }

  if (status != 0)
    {
      /* One of the files should exist but does not.  */
    }
  else if ((same_files
	    = (cmp.file[0].desc != NONEXISTENT
	       && cmp.file[1].desc != NONEXISTENT
	       && 0 < same_file (&cmp.file[0].stat, &cmp.file[1].stat)
	       && same_file_attributes (&cmp.file[0].stat,
					&cmp.file[1].stat)))
	   && no_diff_means_no_output)
    {
      /* The two named files are actually the same physical file.
	 We know they are identical without actually reading them.  */
    }
  else if (DIR_P (0) & DIR_P (1))
    {
      if (output_style == OUTPUT_IFDEF)
	fatal ("-D option not supported with directories");

      /* If both are directories, compare the files in them.  */

      if (parent && !recursive)
	{
	  /* But don't compare dir contents one level down
	     unless -r was specified.
	     See POSIX 1003.2-1992 section 4.17.6.1.1 for this format.
	     POSIX 1003.1-2001 is unchanged here.  */
	  message ("Common subdirectories: %s and %s\n",
		   cmp.file[0].name, cmp.file[1].name);
	}
      else
	status = diff_dirs (&cmp, compare_files);
    }
  else if ((DIR_P (0) | DIR_P (1))
	   || (parent
	       && (! S_ISREG (cmp.file[0].stat.st_mode)
		   || ! S_ISREG (cmp.file[1].stat.st_mode))))
    {
      if (cmp.file[0].desc == NONEXISTENT || cmp.file[1].desc == NONEXISTENT)
	{
	  /* We have a subdirectory that exists only in one directory.  */

	  if ((DIR_P (0) | DIR_P (1))
	      && recursive
	      && (new_file
		  || (unidirectional_new_file
		      && cmp.file[0].desc == NONEXISTENT)))
	    status = diff_dirs (&cmp, compare_files);
	  else
	    {
	      char const *dir
		= parent->file[cmp.file[0].desc == NONEXISTENT].name;

	      /* See POSIX 1003.2-1992 section 4.17.6.1.1 for this format.
		 POSIX 1003.1-2001 is unchanged here.  */
	      message ("Only in %s: %s\n", dir, name0);

	      status = 1;
	    }
	}
      else
	{
	  /* We have two files that are not to be compared.  */

	  /* See POSIX 1003.2-1992 section 4.17.6.1.1 for this format.
	     POSIX 1003.1-2001 is unchanged here.  */
	  message5 ("File %s is a %s while file %s is a %s\n",
		    file_label[0] ? file_label[0] : cmp.file[0].name,
		    filetype (&cmp.file[0].stat),
		    file_label[1] ? file_label[1] : cmp.file[1].name,
		    filetype (&cmp.file[1].stat));

	  /* This is a difference.  */
	  status = 1;
	}
    }
  else if (files_can_be_treated_as_binary
	   && cmp.file[0].stat.st_size != cmp.file[1].stat.st_size
	   && (cmp.file[0].desc == NONEXISTENT
	       || S_ISREG (cmp.file[0].stat.st_mode))
	   && (cmp.file[1].desc == NONEXISTENT
	       || S_ISREG (cmp.file[1].stat.st_mode)))
    {
      message ("Files %s and %s differ\n",
	       file_label[0] ? file_label[0] : cmp.file[0].name,
	       file_label[1] ? file_label[1] : cmp.file[1].name);
      status = 1;
    }
  else
    {
      /* Both exist and neither is a directory.  */

      /* Open the files and record their descriptors.  */

      if (cmp.file[0].desc == UNOPENED)
	if ((cmp.file[0].desc = open (cmp.file[0].name, O_RDONLY, 0)) < 0)
	  {
	    perror_with_name (cmp.file[0].name);
	    status = 2;
	  }
      if (cmp.file[1].desc == UNOPENED)
	{
	  if (same_files)
	    cmp.file[1].desc = cmp.file[0].desc;
	  else if ((cmp.file[1].desc = open (cmp.file[1].name, O_RDONLY, 0))
		   < 0)
	    {
	      perror_with_name (cmp.file[1].name);
	      status = 2;
	    }
	}

#if HAVE_SETMODE_DOS
      if (binary)
	for (f = 0; f < 2; f++)
	  if (0 <= cmp.file[f].desc)
	    set_binary_mode (cmp.file[f].desc, 1);
#endif

      /* Compare the files, if no error was found.  */

      if (status == 0)
	status = diff_2_files (&cmp);

      /* Close the file descriptors.  */

      if (0 <= cmp.file[0].desc && close (cmp.file[0].desc) != 0)
	{
	  perror_with_name (cmp.file[0].name);
	  status = 2;
	}
      if (0 <= cmp.file[1].desc && cmp.file[0].desc != cmp.file[1].desc
	  && close (cmp.file[1].desc) != 0)
	{
	  perror_with_name (cmp.file[1].name);
	  status = 2;
	}
    }

  /* Now the comparison has been done, if no error prevented it,
     and STATUS is the value this function will return.  */

  if (status == 0)
    {
      if (report_identical_files && !DIR_P (0))
	message ("Files %s and %s are identical\n",
		 file_label[0] ? file_label[0] : cmp.file[0].name,
		 file_label[1] ? file_label[1] : cmp.file[1].name);
    }
  else
    {
      /* Flush stdout so that the user sees differences immediately.
	 This can hurt performance, unfortunately.  */
      if (fflush (stdout) != 0)
	pfatal_with_name (_("standard output"));
    }

  if (free0)
    free (free0);
  if (free1)
    free (free1);

  return status;
}
