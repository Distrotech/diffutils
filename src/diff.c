/* GNU DIFF main routine.  */

static char const copyright_string[] =
  "Copyright 1988, 89, 92, 93, 94, 96, 1998 Free Software Foundation, Inc.";

/* This file is part of GNU DIFF.

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

static char const authorship_msgid[] =
  "Written by Paul Eggert, Mike Haertel, David Hayes,\n\
Richard Stallman, and Len Tower.";

#define GDIFF_MAIN
#include "diff.h"
#include <signal.h>
#include "getopt.h"
#include "fnmatch.h"
#include "quotesys.h"

#ifndef DEFAULT_WIDTH
#define DEFAULT_WIDTH 130
#endif

#ifndef GUTTER_WIDTH_MINIMUM
#define GUTTER_WIDTH_MINIMUM 3
#endif

struct regexp_list
{
  char *regexps;	/* chars representing disjunction of the regexps */
  size_t len;		/* chars used in `regexps' */
  size_t size;		/* size malloc'ed for `regexps'; 0 if not malloc'ed */
  int multiple_regexps;	/* Does `regexps' represent a disjunction?  */
  struct re_pattern_buffer *buf;
};

static char const *filetype PARAMS((struct stat const *));
static char *option_list PARAMS((char **, int));
static int add_exclude_file PARAMS((char const *));
static int compare_files PARAMS((struct comparison const *, char const *, char const *));
static int specify_format PARAMS((char **, char *));
static void add_exclude PARAMS((char const *));
static void add_regexp PARAMS((struct regexp_list *, char const *));
static void numeric_arg PARAMS((char const *, char const *, int *));
static void summarize_regexp_list PARAMS((struct regexp_list *));
static void specify_style PARAMS((enum output_style));
static void try_help PARAMS((char const *, char const *)) __attribute__((noreturn));
static void check_stdout PARAMS((void));
static void usage PARAMS((void));

/* Nonzero for -r: if comparing two directories,
   compare their common subdirectories recursively.  */

static int recursive;

/* For -F and -I: lists of regular expressions.  */
static struct regexp_list function_regexp_list;
static struct regexp_list ignore_regexp_list;

#if HAVE_SETMODE
/* Use binary input/output (--binary).  */
static int binary_flag;
#endif

/* If a file is new (appears in only one dir)
   include its entire contents (-N).
   Then `patch' would create the file with appropriate contents.  */
static int entire_new_file_flag;

/* If a file is new (appears in only the second dir)
   include its entire contents (-P).
   Then `patch' would create the file with appropriate contents.  */
static int unidirectional_new_file_flag;

/* Report files compared that match (-s).
   Normally nothing is output when that happens.  */
static int print_file_same_flag;


/* Return a string containing the command options with which diff was invoked.
   Spaces appear between what were separate ARGV-elements.
   There is a space at the beginning but none at the end.
   If there were no options, the result is an empty string.

   Arguments: OPTIONVEC, a vector containing separate ARGV-elements, and COUNT,
   the length of that vector.  */

static char *
option_list (optionvec, count)
     char **optionvec;  /* Was `vector', but that collides on Alliant.  */
     int count;
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

/* Handle a numeric argument of type ARGTYPE_MSGID by converting
   STR to a nonnegative integer, storing the result in *OUT.  */
static void
numeric_arg (argtype_msgid, str, out)
     char const *argtype_msgid;
     char const *str;
     int *out;
{
  int value = 0;
  char const *p = str;

  do
    {
      int v10 = value * 10;
      int digit = *p - '0';

      if (9 < (unsigned) digit)
	error (2, 0, _("%s `%s' is not a number"), _(argtype_msgid), str);

      if (v10 / 10 != value || v10 + digit < v10)
	error (2, 0, _("%s `%s' is too large"), _(argtype_msgid), str);

      value = v10 + digit;
    }
  while (*++p);

  if (0 <= *out && *out != value)
    error (2, 0, _("%s `%d' conflicts with `%d'"),
	   _(argtype_msgid), *out, value);

  *out = value;
}

/* Keep track of excluded file name patterns.  */

static char const **exclude;
static int exclude_alloc, exclude_count;

int
excluded_filename (f)
     char const *f;
{
  int i;
  for (i = 0;  i < exclude_count;  i++)
    if (fnmatch (exclude[i], f, 0) == 0)
      return 1;
  return 0;
}

static void
add_exclude (pattern)
     char const *pattern;
{
  if (exclude_alloc <= exclude_count)
    {
      exclude_alloc = exclude_alloc ? 2 * exclude_alloc : 64;
      exclude = (char const **) xrealloc (exclude,
					  exclude_alloc * sizeof (*exclude));
    }

  exclude[exclude_count++] = pattern;
}

static int
add_exclude_file (name)
     char const *name;
{
  struct file_data f;
  char *p, *q, *lim;

  f.name = optarg;
  f.desc = (strcmp (optarg, "-") == 0
	    ? STDIN_FILENO
	    : open (optarg, O_RDONLY, 0));
  if (f.desc < 0 || fstat (f.desc, &f.stat) != 0)
    return -1;

  sip (&f, 1);
  slurp (&f);

  for (p = f.buffer, lim = p + f.buffered_chars;  p < lim;  p = q)
    {
      q = (char *) memchr (p, '\n', lim - p);
      if (!q)
	q = lim;
      *q++ = 0;
      add_exclude (p);
    }

  return close (f.desc);
}

static struct option const longopts[] =
{
  {"ignore-blank-lines", 0, 0, 'B'},
  {"context", 2, 0, 'C'},
  {"ifdef", 1, 0, 'D'},
  {"show-function-line", 1, 0, 'F'},
  {"speed-large-files", 0, 0, 'H'},
  {"ignore-matching-lines", 1, 0, 'I'},
  {"label", 1, 0, 'L'},
  {"new-file", 0, 0, 'N'},
  {"unidirectional-new-file", 0, 0, 'P'},
  {"starting-file", 1, 0, 'S'},
  {"initial-tab", 0, 0, 'T'},
  {"width", 1, 0, 'W'},
  {"text", 0, 0, 'a'},
  {"ignore-space-change", 0, 0, 'b'},
  {"minimal", 0, 0, 'd'},
  {"ed", 0, 0, 'e'},
  {"forward-ed", 0, 0, 'f'},
  {"ignore-case", 0, 0, 'i'},
  {"paginate", 0, 0, 'l'},
  {"rcs", 0, 0, 'n'},
  {"show-c-function", 0, 0, 'p'},
  {"brief", 0, 0, 'q'},
  {"recursive", 0, 0, 'r'},
  {"report-identical-files", 0, 0, 's'},
  {"expand-tabs", 0, 0, 't'},
  {"version", 0, 0, 'v'},
  {"ignore-all-space", 0, 0, 'w'},
  {"exclude", 1, 0, 'x'},
  {"exclude-from", 1, 0, 'X'},
  {"side-by-side", 0, 0, 'y'},
  {"unified", 2, 0, 'U'},
  {"left-column", 0, 0, CHAR_MAX + 1},
  {"suppress-common-lines", 0, 0, CHAR_MAX + 2},
  {"sdiff-merge-assist", 0, 0, CHAR_MAX + 3},
  {"old-line-format", 1, 0, CHAR_MAX + 4},
  {"new-line-format", 1, 0, CHAR_MAX + 5},
  {"unchanged-line-format", 1, 0, CHAR_MAX + 6},
  {"line-format", 1, 0, CHAR_MAX + 7},
  {"old-group-format", 1, 0, CHAR_MAX + 8},
  {"new-group-format", 1, 0, CHAR_MAX + 9},
  {"unchanged-group-format", 1, 0, CHAR_MAX + 10},
  {"changed-group-format", 1, 0, CHAR_MAX + 11},
  {"horizon-lines", 1, 0, CHAR_MAX + 12},
  {"help", 0, 0, CHAR_MAX + 13},
  {"binary", 0, 0, CHAR_MAX + 14},
  {"from-file", 1, 0, CHAR_MAX + 15},
  {"to-file", 1, 0, CHAR_MAX + 16},
  {"inhibit-hunk-merge", 0, 0, CHAR_MAX + 17},
  {0, 0, 0, 0}
};

int
main (argc, argv)
     int argc;
     char *argv[];
{
  int exit_status = 0;
  int c;
  int prev = -1;
  int width = -1;
  int show_c_function = 0;
  char const *old_file = 0;
  char const *new_file = 0;

  /* Do our initializations.  */
  initialize_main (&argc, &argv);
  setlocale (LC_ALL, "");
  program_name = argv[0];
  xmalloc_exit_failure = 2;
  output_style = OUTPUT_NORMAL;
  context = -1;
  horizon_lines = -1;
  function_regexp_list.buf = &function_regexp;
  ignore_regexp_list.buf = &ignore_regexp;

  /* Decode the options.  */

  while ((c = getopt_long (argc, argv,
			   "0123456789abBcC:dD:efF:hHiI:lL:nNpPqrsS:tTuU:vwW:x:X:y",
			   longopts, 0))
	 != -1)
    {
      switch (c)
	{
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
	  else if (prev < '0' || prev > '9')
	    fatal ("context length specified twice");

	  context = context * 10 + c - '0';
	  break;

	case 'a':
	  /* Treat all files as text files; never treat as binary.  */
	  always_text_flag = 1;
	  break;

	case 'b':
	  /* Ignore changes in amount of white space.  */
	  ignore_space_change_flag = 1;
	  ignore_some_changes = 1;
	  break;

	case 'B':
	  /* Ignore changes affecting only blank lines.  */
	  ignore_blank_lines_flag = 1;
	  ignore_some_changes = 1;
	  break;

	case 'C':		/* +context[=lines] */
	case 'U':		/* +unified[=lines] */
	  if (optarg)
	    numeric_arg ("context length", optarg, &context);
	  /* Fall through.  */
	case 'c':
	  /* Make context-style output.  */
	  specify_style (c == 'U' ? OUTPUT_UNIFIED : OUTPUT_CONTEXT);
	  break;

	case 'd':
	  /* Don't discard lines.  This makes things slower (sometimes much
	     slower) but will find a guaranteed minimal set of changes.  */
	  no_discards = 1;
	  break;

	case 'D':
	  /* Make merged #ifdef output.  */
	  specify_style (OUTPUT_IFDEF);
	  {
	    int i, err = 0;
	    static char const C_ifdef_group_formats[] =
	      "#ifndef %s\n%%<#endif /* ! %s */\n%c#ifdef %s\n%%>#endif /* %s */\n%c%%=%c#ifndef %s\n%%<#else /* %s */\n%%>#endif /* %s */\n";
	    char *b = xmalloc (sizeof (C_ifdef_group_formats)
			       + 7 * strlen (optarg) - 14 /* 7*"%s" */
			       - 8 /* 5*"%%" + 3*"%c" */);
	    sprintf (b, C_ifdef_group_formats,
		     optarg, optarg, 0,
		     optarg, optarg, 0, 0,
		     optarg, optarg, optarg);
	    for (i = 0; i < 4; i++)
	      {
		err |= specify_format (&group_format[i], b);
		b += strlen (b) + 1;
	      }
	    if (err)
	      error (0, 0, _("conflicting #ifdef format"));
	  }
	  break;

	case 'e':
	  /* Make output that is a valid `ed' script.  */
	  specify_style (OUTPUT_ED);
	  break;

	case 'f':
	  /* Make output that looks vaguely like an `ed' script
	     but has changes in the order they appear in the file.  */
	  specify_style (OUTPUT_FORWARD_ED);
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
	  heuristic = 1;
	  break;

	case 'i':
	  /* Ignore changes in case.  */
	  ignore_case_flag = 1;
	  ignore_some_changes = 1;
	  break;

	case 'I':
	  /* Ignore changes affecting only lines that match the
	     specified regexp.  */
	  add_regexp (&ignore_regexp_list, optarg);
	  ignore_some_changes = 1;
	  break;

	case 'l':
	  /* Pass the output through `pr' to paginate it.  */
	  if (!pr_program[0])
	    error (2, 0, _("pagination not supported on this host"));
	  paginate_flag = 1;
#if !defined (SIGCHLD) && defined (SIGCLD)
#define SIGCHLD SIGCLD
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
	  specify_style (OUTPUT_RCS);
	  break;

	case 'N':
	  /* When comparing directories, if a file appears only in one
	     directory, treat it as present but empty in the other.  */
	  entire_new_file_flag = 1;
	  break;

	case 'p':
	  /* Make context-style output and show name of last C function.  */
	  show_c_function = 1;
	  add_regexp (&function_regexp_list, "^[_a-zA-Z$]");
	  break;

	case 'P':
	  /* When comparing directories, if a file appears only in
	     the second directory of the two,
	     treat it as present but empty in the other.  */
	  unidirectional_new_file_flag = 1;
	  break;

	case 'q':
	  no_details_flag = 1;
	  break;

	case 'r':
	  /* When comparing directories,
	     recursively compare any subdirectories found.  */
	  recursive = 1;
	  break;

	case 's':
	  /* Print a message if the files are the same.  */
	  print_file_same_flag = 1;
	  break;

	case 'S':
	  /* When comparing directories, start with the specified
	     file name.  This is used for resuming an aborted comparison.  */
	  if (dir_start_file && strcmp (dir_start_file, optarg) != 0)
	    fatal ("conflicting starting file");
	  dir_start_file = optarg;
	  break;

	case 't':
	  /* Expand tabs to spaces in the output so that it preserves
	     the alignment of the input files.  */
	  tab_expand_flag = 1;
	  break;

	case 'T':
	  /* Use a tab in the output, rather than a space, before the
	     text of an input line, so as to keep the proper alignment
	     in the input line without changing the characters in it.  */
	  tab_align_flag = 1;
	  break;

	case 'u':
	  /* Output the context diff in unidiff format.  */
	  specify_style (OUTPUT_UNIFIED);
	  break;

	case 'v':
	  printf ("diff %s\n%s\n\n%s\n\n%s\n",
		  version_string, copyright_string,
		  _(free_software_msgid), _(authorship_msgid));
	  check_stdout ();
	  exit (0);

	case 'w':
	  /* Ignore horizontal white space when comparing lines.  */
	  ignore_all_space_flag = 1;
	  ignore_some_changes = 1;
	  break;

	case 'x':
	  add_exclude (optarg);
	  break;

	case 'X':
	  if (add_exclude_file (optarg) != 0)
	    pfatal_with_name (optarg);
	  break;

	case 'y':
	  /* Use side-by-side (sdiff-style) columnar output.  */
	  specify_style (OUTPUT_SDIFF);
	  break;

	case 'W':
	  /* Set the line width for OUTPUT_SDIFF.  */
	  numeric_arg ("column width", optarg, &width);
	  if (!width)
	    fatal ("column width is zero");
	  break;

	case CHAR_MAX + 1:
	  sdiff_left_only = 1;
	  break;

	case CHAR_MAX + 2:
	  sdiff_skip_common_lines = 1;
	  break;

	case CHAR_MAX + 3:
	  /* sdiff-style columns output.  */
	  specify_style (OUTPUT_SDIFF);
	  sdiff_help_sdiff = 1;
	  break;

	case CHAR_MAX + 4:
	case CHAR_MAX + 5:
	case CHAR_MAX + 6:
	  specify_style (OUTPUT_IFDEF);
	  if (specify_format (&line_format[c - (CHAR_MAX + 4)], optarg) != 0)
	    error (0, 0, _("conflicting line format"));
	  break;

	case CHAR_MAX + 7:
	  specify_style (OUTPUT_IFDEF);
	  {
	    int i, err = 0;
	    for (i = 0; i < sizeof (line_format) / sizeof (*line_format); i++)
	      err |= specify_format (&line_format[i], optarg);
	    if (err)
	      error (0, 0, _("conflicting line format"));
	  }
	  break;

	case CHAR_MAX + 8:
	case CHAR_MAX + 9:
	case CHAR_MAX + 10:
	case CHAR_MAX + 11:
	  specify_style (OUTPUT_IFDEF);
	  if (specify_format (&group_format[c - (CHAR_MAX + 8)], optarg) != 0)
	    error (0, 0, _("conflicting group format"));
	  break;

	case CHAR_MAX + 12:
	  numeric_arg ("horizon length", optarg, &horizon_lines);
	  break;

	case CHAR_MAX + 13:
	  usage ();
	  check_stdout ();
	  exit (0);

	case CHAR_MAX + 14:
	  /* Use binary I/O when reading and writing data.
	     On Posix hosts, this has no effect.  */
#if HAVE_SETMODE
	  binary_flag = 1;
	  setmode (STDOUT_FILENO, O_BINARY);
#endif
	  break;

	case CHAR_MAX + 15:
	  if (old_file)
	    fatal ("multiple `--from-file' options");
	  old_file = optarg;
	  break;

	case CHAR_MAX + 16:
	  if (new_file)
	    fatal ("multiple `--to-file' options");
	  new_file = optarg;
	  break;

	case CHAR_MAX + 17:
	  inhibit_hunk_merge = 1;
	  break;

	default:
	  try_help (0, 0);
	}
      prev = c;
    }

  if (width < 0)
    width = DEFAULT_WIDTH;

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
    int t = tab_expand_flag ? 1 : TAB_WIDTH;
    int off = (width + t + GUTTER_WIDTH_MINIMUM) / (2*t)  *  t;
    sdiff_half_width = max (0, min (off - GUTTER_WIDTH_MINIMUM, width - off)),
    sdiff_column2_offset = sdiff_half_width ? off : width;
  }

  if (show_c_function && output_style != OUTPUT_UNIFIED)
    specify_style (OUTPUT_CONTEXT);

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
      /* Format arrays are char *, not char const *,
	 because integer formats are temporarily modified.
	 But it is safe to assign a constant like "%=" to a format array,
	 since "%=" does not format any integers.  */
      int i;
      for (i = 0; i < sizeof (line_format) / sizeof (*line_format); i++)
	if (!line_format[i])
	  line_format[i] = (char *) "%l\n";
      if (!group_format[OLD])
	group_format[OLD]
	  = group_format[UNCHANGED] ? group_format[UNCHANGED] : (char *) "%<";
      if (!group_format[NEW])
	group_format[NEW]
	  = group_format[UNCHANGED] ? group_format[UNCHANGED] : (char *) "%>";
      if (!group_format[UNCHANGED])
	group_format[UNCHANGED] = (char *) "%=";
      if (!group_format[CHANGED])
	group_format[CHANGED] = concat (group_format[OLD],
					group_format[NEW], "");
    }

  no_diff_means_no_output =
    (output_style == OUTPUT_IFDEF ?
      (!*group_format[UNCHANGED]
       || (strcmp (group_format[UNCHANGED], "%=") == 0
	   && !*line_format[UNCHANGED]))
     : output_style == OUTPUT_SDIFF ? sdiff_skip_common_lines : 1);

  switch_string = option_list (argv + 1, optind - 1);

  if (old_file)
    {
      for (;  optind < argc;  optind++)
	{
	  int status = compare_files ((struct comparison *) 0,
				      old_file, argv[optind]);
	  if (exit_status < status)
	    exit_status = status;
	}
    }

  if (new_file)
    {
      for (;  optind < argc;  optind++)
	{
	  int status = compare_files ((struct comparison *) 0,
				      argv[optind], new_file);
	  if (exit_status < status)
	    exit_status = status;
	}
    }
  
  if (!old_file && !new_file)
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
add_regexp (reglist, pattern)
     struct regexp_list *reglist;
     char const *pattern;
{
  size_t patlen = strlen (pattern);
  char const *m = re_compile_pattern (pattern, patlen, reglist->buf);

  if (m != 0)
    error (0, 0, "%s: %s", pattern, m);
  else
    {
      char *regexps = reglist->regexps;
      size_t len = reglist->len;
      int multiple_regexps = reglist->multiple_regexps = regexps != 0;
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
summarize_regexp_list (reglist)
     struct regexp_list *reglist;
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
try_help (reason_msgid, operand)
     char const *reason_msgid;
     char const *operand;
{
  if (reason_msgid)
    error (0, 0, _(reason_msgid), operand);
  error (2, 0, _("Try `%s --help' for more information."), program_name);
  abort ();
}

static void
check_stdout ()
{
  if (ferror (stdout) || fclose (stdout) != 0)
    fatal ("write failed");
}

static char const * const option_help_msgid[] = {
  "FILES are `FILE1 FILE2' or `DIR1 DIR2' or `DIR FILE...' or `FILE... DIR';",
  "if --from-file or --to-file is given, there are no restrictions on FILES.",
  "If a FILE is `-', read standard input.",
  "",
  "-i  --ignore-case  Consider upper- and lower-case to be the same.",
  "-w  --ignore-all-space  Ignore all white space.",
  "-b  --ignore-space-change  Ignore changes in the amount of white space.",
  "-B  --ignore-blank-lines  Ignore changes whose lines are all blank.",
  "-I RE  --ignore-matching-lines=RE  Ignore changes whose lines all match RE.",
#if HAVE_SETMODE
  "--binary  Read and write data in binary mode.",
#else
  "--binary  Read and write data in binary mode (no effect on this platform).",
#endif
  "-a  --text  Treat all files as text.",
  "",
  "-c  -C NUM  --context[=NUM]  Output NUM (default 2) lines of copied context.\n\
-u  -U NUM  --unified[=NUM]  Output NUM (default 2) lines of unified context.\n\
  -NUM  Use NUM context lines.\n\
  -L LABEL  --label LABEL  Use LABEL instead of file name.\n\
  -p  --show-c-function  Show which C function each change is in.\n\
  -F RE  --show-function-line=RE  Show the most recent line matching RE.",
  "-q  --brief  Output only whether files differ.",
  "-e  --ed  Output an ed script.",
  "-n  --rcs  Output an RCS format diff.",
  "-y  --side-by-side  Output in two columns.\n\
  -W NUM  --width=NUM  Output at most NUM (default 130) characters per line.\n\
  --left-column  Output only the left column of common lines.\n\
  --suppress-common-lines  Do not output common lines.",
  "-D NAME  --ifdef=NAME  Output merged file to show `#ifdef NAME' diffs.",
  "--GTYPE-group-format=GFMT  Similar, but format GTYPE input groups with GFMT.",
  "--line-format=LFMT  Similar, but format all input lines with LFMT.",
  "--LTYPE-line-format=LFMT  Similar, but format LTYPE input lines with LFMT.",
  "  LTYPE is `old', `new', or `unchanged'.  GTYPE is LTYPE or `changed'.",
  "  GFMT may contain:\n\
    %<  lines from FILE1\n\
    %>  lines from FILE2\n\
    %=  lines common to FILE1 and FILE2\n\
    %[-][WIDTH][.[PREC]]{doxX}LETTER  printf-style spec for LETTER\n\
      LETTERs are as follows for new group, lower case for old group:\n\
        F  first line number\n\
        L  last line number\n\
        N  number of lines = L-F+1\n\
        E  F-1\n\
        M  L+1",
  "  LFMT may contain:\n\
    %L  contents of line\n\
    %l  contents of line, excluding any trailing newline\n\
    %[-][WIDTH][.[PREC]]{doxX}n  printf-style spec for input line number",
  "  Either GFMT or LFMT may contain:\n\
    %%  %\n\
    %c'C'  the single character C\n\
    %c'\\OOO'  the character with octal code OOO",
  "",
  "-l  --paginate  Pass the output through `pr' to paginate it.",
  "-t  --expand-tabs  Expand tabs to spaces in output.",
  "-T  --initial-tab  Make tabs line up by prepending a tab.",
  "",
  "-r  --recursive  Recursively compare any subdirectories found.",
  "-N  --new-file  Treat absent files as empty.",
  "-P  --unidirectional-new-file  Treat absent first files as empty.",
  "-s  --report-identical-files  Report when two files are the same.",
  "-x PAT  --exclude=PAT  Exclude files that match PAT.",
  "-X FILE  --exclude-from=FILE  Exclude files that match any pattern in FILE.",
  "-S FILE  --starting-file=FILE  Start with FILE when comparing directories.",
  "--from-file=FILE1  Compare FILE1 to all operands.  FILE1 can be a directory.",
  "--to-file=FILE2  Compare all operands to FILE2.  FILE2 can be a directory.",
  "",
  "--horizon-lines=NUM  Keep NUM lines of the common prefix and suffix.",
  "--inhibit-hunk-merge  Do not merge hunks.",
  "-d  --minimal  Try hard to find a smaller set of changes.",
  "-H  --speed-large-files  Assume large files and many scattered small changes.",
  "",
  "-v  --version  Output version info.",
  "--help  Output this help.",
  "",
  "Report bugs to <bug-gnu-utils@gnu.org>.",
  0
};

static void
usage ()
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
	      printf ("  %.*s", nl + 1 - msg, msg);
	      msg = nl + 1;
	    }

	  printf ("  %s\n" + 2 * (*msg != ' ' && *msg != '-'), msg);
	}
    }
}

static int
specify_format (var, value)
     char **var;
     char *value;
{
  int err = *var ? strcmp (*var, value) : 0;
  *var = value;
  return err;
}

static void
specify_style (style)
     enum output_style style;
{
  if (output_style != OUTPUT_NORMAL
      && output_style != style)
    error (0, 0, _("conflicting specifications of output style"));
  output_style = style;
}

static char const *
filetype (st)
     struct stat const *st;
{
  /* See Posix.2 section 4.17.6.1.1 and Table 5-1 for these formats.
     To keep diagnostics grammatical, the returned string must start
     with a consonant.  */

  if (S_ISREG (st->st_mode))
    {
      if (st->st_size == 0)
	return _("regular empty file");
      /* Posix.2 section 5.14.2 seems to suggest that we must read the file
	 and guess whether it's C, Fortran, etc., but this is somewhat useless
	 and doesn't reflect historical practice.  We're allowed to guess
	 wrong, so we don't bother to read the file.  */
      return _("regular file");
    }
  if (S_ISDIR (st->st_mode)) return _("directory");

  /* other Posix.1 file types */
#ifdef S_ISBLK
  if (S_ISBLK (st->st_mode)) return _("block special file");
#endif
#ifdef S_ISCHR
  if (S_ISCHR (st->st_mode)) return _("character special file");
#endif
#ifdef S_ISFIFO
  if (S_ISFIFO (st->st_mode)) return _("fifo");
#endif

  /* other Posix.1b file types */
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

/* Compare two files (or dirs) with parent comparsion PARENT
   and names NAME0 and NAME1.
   (If PARENT is 0, then the first name is just NAME0, etc.)
   This is self-contained; it opens the files and closes them.

   Value is 0 if files are the same, 1 if different,
   2 if there is a problem opening them.  */

static int
compare_files (parent, name0, name1)
     struct comparison const *parent;
     char const *name0, *name1;
{
  struct comparison cmp;
# define DIR_P(i) (S_ISDIR (cmp.file[i].stat.st_mode) != 0)
  register int i;
  int status = 0;
  int same_files;
  char *free0, *free1;

  /* If this is directory comparison, perhaps we have a file
     that exists only in one of the directories.
     If so, just print a message to that effect.  */

  if (! ((name0 != 0 && name1 != 0)
	 || (unidirectional_new_file_flag && name1 != 0)
	 || entire_new_file_flag))
    {
      char const *name = name0 == 0 ? name1 : name0;
      char const *dir = parent->file[name0 == 0].name;
      /* See Posix.2 section 4.17.6.1.1 for this format.  */
      message ("Only in %s: %s\n", dir, name);
      /* Return 1 so that diff_dirs will return 1 ("some files differ").  */
      return 1;
    }

  bzero (cmp.file, sizeof (cmp.file));
  cmp.parent = parent;

  /* cmp.file[i].desc markers */
# define NONEXISTENT (-1) /* nonexistent file */
# define UNOPENED (-2) /* unopened file (e.g. directory) */
# define ERRNO_ENCODE(errno) (-3 - (errno)) /* encoded errno value */

# define ERRNO_DECODE(desc) (-3 - (desc)) /* inverse of ERRNO_ENCODE */

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

  for (i = 0; i <= 1; i++)
    {
      if (cmp.file[i].desc != NONEXISTENT)
	{
	  if (i && filename_cmp (cmp.file[i].name, cmp.file[0].name) == 0)
	    {
	      cmp.file[i].desc = cmp.file[0].desc;
	      cmp.file[i].stat = cmp.file[0].stat;
	    }
	  else if (strcmp (cmp.file[i].name, "-") == 0)
	    {
	      cmp.file[i].desc = STDIN_FILENO;
	      if (fstat (STDIN_FILENO, &cmp.file[i].stat) != 0)
		cmp.file[i].desc = ERRNO_ENCODE (errno);
	      else if (S_ISREG (cmp.file[i].stat.st_mode))
		{
		  off_t pos = lseek (STDIN_FILENO, (off_t) 0, SEEK_CUR);
		  if (pos == -1)
		    cmp.file[i].desc = ERRNO_ENCODE (errno);
		  else
		    {
		      if (pos <= cmp.file[i].stat.st_size)
			cmp.file[i].stat.st_size -= pos;
		      else
			cmp.file[i].stat.st_size = 0;
		      /* Posix.2 4.17.6.1.4 requires current time for stdin.  */
		      time (&cmp.file[i].stat.st_mtime);
		    }
		}
	    }
	  else if (stat (cmp.file[i].name, &cmp.file[i].stat) != 0)
	    cmp.file[i].desc = ERRNO_ENCODE (errno);
	}
    }

  /* Mark files as nonexistent at the top level as needed for -N and -P.  */
  if (! parent)
    {
      if ((entire_new_file_flag | unidirectional_new_file_flag)
	  && cmp.file[0].desc == ERRNO_ENCODE (ENOENT)
	  && cmp.file[1].desc == UNOPENED)
	cmp.file[0].desc = NONEXISTENT;

      if (entire_new_file_flag
	  && cmp.file[0].desc == UNOPENED
	  && cmp.file[1].desc == ERRNO_ENCODE (ENOENT))
	cmp.file[1].desc = NONEXISTENT;
    }

  for (i = 0; i <= 1; i++)
    if (cmp.file[i].desc == NONEXISTENT)
      cmp.file[i].stat.st_mode = cmp.file[1 - i].stat.st_mode;

  for (i = 0; i <= 1; i++)
    {
      int e = ERRNO_DECODE (cmp.file[i].desc);
      if (0 <= e)
	{
	  errno = e;
	  perror_with_name (cmp.file[i].name);
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
      char const *p = filename_lastdirchar (fnm);
      char const *filename = cmp.file[dir_arg].name = free0
	= dir_file_pathname (dir, p ? p + 1 : fnm);

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
	       && 0 < same_file (&cmp.file[0].stat, &cmp.file[1].stat)))
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
	     See Posix.2 section 4.17.6.1.1 for this format.  */
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
	      && (entire_new_file_flag
		  || (unidirectional_new_file_flag
		      && cmp.file[0].desc == NONEXISTENT)))
	    status = diff_dirs (&cmp, compare_files);
	  else
	    {
	      char const *dir
		= parent->file[cmp.file[0].desc == NONEXISTENT].name;
	      /* See Posix.2 section 4.17.6.1.1 for this format.  */
	      message ("Only in %s: %s\n", dir, name0);
	      status = 1;
	    }
	}
      else
	{
	  /* We have two files that are not to be compared.  */

	  /* See Posix.2 section 4.17.6.1.1 for this format.  */
	  message5 ("File %s is a %s while file %s is a %s\n",
		    file_label[0] ? file_label[0] : cmp.file[0].name,
		    filetype (&cmp.file[0].stat),
		    file_label[1] ? file_label[1] : cmp.file[1].name,
		    filetype (&cmp.file[1].stat));

	  /* This is a difference.  */
	  status = 1;
	}
    }
  else if ((no_details_flag & ~ignore_some_changes)
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

#if HAVE_SETMODE
      if (binary_flag)
	for (i = 0; i <= 1; i++)
	  if (0 <= cmp.file[i].desc)
	    setmode (cmp.file[i].desc, O_BINARY);
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
      if (print_file_same_flag && !DIR_P (0))
	message ("Files %s and %s are identical\n",
		 file_label[0] ? file_label[0] : cmp.file[0].name,
		 file_label[1] ? file_label[1] : cmp.file[1].name);
    }
  else
    {
      /* Flush stdout so that the user sees differences immediately.
	 This can hurt performance, unfortunately.  */
      if (fflush (stdout) != 0)
	pfatal_with_name (_("write failed"));
    }

  if (free0)
    free (free0);
  if (free1)
    free (free1);

  return status;
}
