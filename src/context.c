/* Context-format output routines for GNU DIFF.

   Copyright (C) 1988, 1989, 1991, 1992, 1993, 1994, 1995, 1998, 2001,
   2002, 2004 Free Software Foundation, Inc.

   This file is part of GNU DIFF.

   GNU DIFF is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU DIFF is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "diff.h"
#include <inttostr.h>

#ifdef ST_MTIM_NSEC
# define TIMESPEC_NS(timespec) ((timespec).ST_MTIM_NSEC)
#else
# define TIMESPEC_NS(timespec) 0
#endif

size_t nstrftime (char *, size_t, char const *, struct tm const *, int, int);

static char const *find_function (char const * const *, lin, struct diffopt *);
static struct change *find_hunk (struct change *, struct diffopt const *opt);
static void mark_ignorable (struct change *, struct diffopt *);
static void pr_context_hunk (struct change *, struct diffopt *);
static void pr_unidiff_hunk (struct change *, struct diffopt *);

/* Print a label for a context diff, with a file name and date or a label.  */

static void
print_context_label (char const *mark,
		     struct file_data *inf,
		     char const *label,
		     struct diffopt const *opt)
{
  if (label)
    fprintf (opt->out, "%s %s\n", mark, label);
  else
    {
      char buf[MAX (INT_STRLEN_BOUND (int) + 32,
		    INT_STRLEN_BOUND (time_t) + 11)];
      struct tm const *tm = localtime (&inf->stat.st_mtime);
      int nsec = TIMESPEC_NS (inf->stat.st_mtim);
      if (! (tm && nstrftime (buf, sizeof buf, opt->time_format, tm, 0, nsec)))
	{
	  long int sec = inf->stat.st_mtime;
	  verify (info_preserved, sizeof inf->stat.st_mtime <= sizeof sec);
	  sprintf (buf, "%ld.%.9d", sec, nsec);
	}
      fprintf (opt->out, "%s %s\t%s\n", mark, inf->name, buf);
    }
}

/* Print a header for a context diff, with the file names and dates.  */

void
print_context_header (struct file_data inf[], bool unidiff,
		      struct diffopt const *opt)
{
  if (unidiff)
    {
      print_context_label ("---", &inf[0], opt->file_label[0], opt);
      print_context_label ("+++", &inf[1], opt->file_label[1], opt);
    }
  else
    {
      print_context_label ("***", &inf[0], opt->file_label[0], opt);
      print_context_label ("---", &inf[1], opt->file_label[1], opt);
    }
}

/* Print an edit script in context format.  */

void
print_context_script (struct change *script, bool unidiff,
		      struct diffopt *opt)
{
  if (opt->ignore_blank_lines || opt->ignore_regexp)
    mark_ignorable (script, opt);
  else
    {
      struct change *e;
      for (e = script; e; e = e->link)
	e->ignore = false;
    }

  opt->find_function.last_search = - opt->file[0].prefix_lines;
  opt->find_function.last_match = LIN_MAX;

  if (unidiff)
    print_script (script, find_hunk, pr_unidiff_hunk, opt);
  else
    print_script (script, find_hunk, pr_context_hunk, opt);
}

/* Print a pair of line numbers with a comma, translated for file FILE.
   If the second number is not greater, use the first in place of it.

   Args A and B are internal line numbers.
   We print the translated (real) line numbers.  */

static void
print_context_number_range (FILE *out, struct file_data const *file,
			    lin a, lin b)
{
  long int trans_a, trans_b;
  translate_range (file, a, b, &trans_a, &trans_b);

  /* We can have B <= A in the case of a range of no lines.
     In this case, we should print the line number before the range,
     which is B.

     POSIX 1003.1-2001 requires two line numbers separated by a comma
     even if the line numbers are the same.  However, this does not
     match existing practice and is surely an error in the
     specification.  */

  if (trans_b <= trans_a)
    fprintf (out, "%ld", trans_b);
  else
    fprintf (out, "%ld,%ld", trans_a, trans_b);
}

/* Print FUNCTION in a context header.  */
static void
print_context_function (FILE *out, char const *function)
{
  int i;
  putc (' ', out);
  for (i = 0; i < 40 && function[i] != '\n'; i++)
    continue;
  fwrite (function, sizeof (char), i, out);
}

/* Print a portion of an edit script in context format.
   HUNK is the beginning of the portion to be printed.
   The end is marked by a `link' that has been nulled out.
   OPT is the diff options.

   Prints out lines from both files, and precedes each
   line with the appropriate flag-character.  */

static void
pr_context_hunk (struct change *hunk, struct diffopt *opt)
{
  lin first0, last0, first1, last1, i;
  char const *prefix;
  char const *function;
  FILE *out;

  /* Determine range of line numbers involved in each file.  */

  enum changes changes = analyze_hunk (hunk, &first0, &last0, &first1, &last1,
				       opt);
  if (! changes)
    return;

  /* Include a context's width before and after.  */

  i = - opt->file[0].prefix_lines;
  first0 = MAX (first0 - opt->context, i);
  first1 = MAX (first1 - opt->context, i);
  if (last0 < opt->file[0].valid_lines - opt->context)
    last0 += opt->context;
  else
    last0 = opt->file[0].valid_lines - 1;
  if (last1 < opt->file[1].valid_lines - opt->context)
    last1 += opt->context;
  else
    last1 = opt->file[1].valid_lines - 1;

  /* If desired, find the preceding function definition line in file 0.  */
  function = 0;
  if (opt->function_regexp)
    function = find_function (opt->file[0].linbuf, first0, opt);

  begin_output (opt);
  out = opt->out;

  fprintf (out, "***************");

  if (function)
    print_context_function (out, function);

  fprintf (out, "\n*** ");
  print_context_number_range (opt->out, &opt->file[0], first0, last0);
  fprintf (out, " ****\n");

  if (changes & OLD)
    {
      struct change *next = hunk;

      for (i = first0; i <= last0; i++)
	{
	  /* Skip past changes that apply (in file 0)
	     only to lines before line I.  */

	  while (next && next->line0 + next->deleted <= i)
	    next = next->link;

	  /* Compute the marking for line I.  */

	  prefix = " ";
	  if (next && next->line0 <= i)
	    /* The change NEXT covers this line.
	       If lines were inserted here in file 1, this is "changed".
	       Otherwise it is "deleted".  */
	    prefix = (next->inserted > 0 ? "!" : "-");

	  print_1_line (prefix, &opt->file[0].linbuf[i], opt);
	}
    }

  fprintf (out, "--- ");
  print_context_number_range (opt->out, &opt->file[1], first1, last1);
  fprintf (out, " ----\n");

  if (changes & NEW)
    {
      struct change *next = hunk;

      for (i = first1; i <= last1; i++)
	{
	  /* Skip past changes that apply (in file 1)
	     only to lines before line I.  */

	  while (next && next->line1 + next->inserted <= i)
	    next = next->link;

	  /* Compute the marking for line I.  */

	  prefix = " ";
	  if (next && next->line1 <= i)
	    /* The change NEXT covers this line.
	       If lines were deleted here in file 0, this is "changed".
	       Otherwise it is "inserted".  */
	    prefix = (next->deleted > 0 ? "!" : "+");

	  print_1_line (prefix, &opt->file[1].linbuf[i], opt);
	}
    }
}

/* Print a pair of line numbers with a comma, translated for file FILE.
   If the second number is smaller, use the first in place of it.
   If the numbers are equal, print just one number.

   Args A and B are internal line numbers.
   We print the translated (real) line numbers.

   OPT is the diff options.  */

static void
print_unidiff_number_range (FILE *out, struct file_data const *file,
			    lin a, lin b)
{
  long int trans_a, trans_b;
  translate_range (file, a, b, &trans_a, &trans_b);

  /* We can have B < A in the case of a range of no lines.
     In this case, we print the line number before the range,
     which is B.  It would be more logical to print A, but
     'patch' expects B in order to detect diffs against empty files.  */
  if (trans_b <= trans_a)
    fprintf (out, trans_b < trans_a ? "%ld,0" : "%ld", trans_b);
  else
    fprintf (out, "%ld,%ld", trans_a, trans_b - trans_a + 1);
}

/* Print a portion of an edit script in unidiff format.
   HUNK is the beginning of the portion to be printed.
   The end is marked by a `link' that has been nulled out.

   Prints out lines from both files, and precedes each
   line with the appropriate flag-character.

   OPT is the diff options.  */

static void
pr_unidiff_hunk (struct change *hunk, struct diffopt *opt)
{
  lin first0, last0, first1, last1;
  lin i, j, k;
  struct change *next;
  char const *function;
  FILE *out;

  /* Determine range of line numbers involved in each file.  */

  if (! analyze_hunk (hunk, &first0, &last0, &first1, &last1, opt))
    return;

  /* Include a context's width before and after.  */

  i = - opt->file[0].prefix_lines;
  first0 = MAX (first0 - opt->context, i);
  first1 = MAX (first1 - opt->context, i);
  if (last0 < opt->file[0].valid_lines - opt->context)
    last0 += opt->context;
  else
    last0 = opt->file[0].valid_lines - 1;
  if (last1 < opt->file[1].valid_lines - opt->context)
    last1 += opt->context;
  else
    last1 = opt->file[1].valid_lines - 1;

  /* If desired, find the preceding function definition line in file 0.  */
  function = 0;
  if (opt->function_regexp)
    function = find_function (opt->file[0].linbuf, first0, opt);

  begin_output (opt);
  out = opt->out;

  fprintf (out, "@@ -");
  print_unidiff_number_range (opt->out, &opt->file[0], first0, last0);
  fprintf (out, " +");
  print_unidiff_number_range (opt->out, &opt->file[1], first1, last1);
  fprintf (out, " @@");

  if (function)
    print_context_function (out, function);

  putc ('\n', out);

  next = hunk;
  i = first0;
  j = first1;

  while (i <= last0 || j <= last1)
    {

      /* If the line isn't a difference, output the context from file 0. */

      if (!next || i < next->line0)
	{
	  putc (opt->initial_tab ? '\t' : ' ', out);
	  print_1_line (0, &opt->file[0].linbuf[i++], opt);
	  j++;
	}
      else
	{
	  /* For each difference, first output the deleted part. */

	  k = next->deleted;
	  while (k--)
	    {
	      putc ('-', out);
	      if (opt->initial_tab)
		putc ('\t', out);
	      print_1_line (0, &opt->file[0].linbuf[i++], opt);
	    }

	  /* Then output the inserted part. */

	  k = next->inserted;
	  while (k--)
	    {
	      putc ('+', out);
	      if (opt->initial_tab)
		putc ('\t', out);
	      print_1_line (0, &opt->file[1].linbuf[j++], opt);
	    }

	  /* We're done with this hunk, so on to the next! */

	  next = next->link;
	}
    }
}

/* Scan a (forward-ordered) edit script for the first place that more than
   2*CONTEXT unchanged lines appear, and return a pointer
   to the `struct change' for the last change before those lines.  */

static struct change *
find_hunk (struct change *start, struct diffopt const *opt)
{
  struct change *prev;
  lin top0, top1;
  lin thresh;

  /* Threshold distance is 2 * CONTEXT + 1 between two non-ignorable
     changes, but only CONTEXT if one is ignorable.  Watch out for
     integer overflow, though.  */
  lin non_ignorable_threshold =
    (LIN_MAX - 1) / 2 < opt->context ? LIN_MAX : 2 * opt->context + 1;
  lin ignorable_threshold = opt->context;

  do
    {
      /* Compute number of first line in each file beyond this changed.  */
      top0 = start->line0 + start->deleted;
      top1 = start->line1 + start->inserted;
      prev = start;
      start = start->link;
      thresh = (prev->ignore || (start && start->ignore)
		? ignorable_threshold
		: non_ignorable_threshold);
      /* It is not supposed to matter which file we check in the end-test.
	 If it would matter, crash.  */
      if (start && start->line0 - top0 != start->line1 - top1)
	abort ();
    } while (start
	     /* Keep going if less than THRESH lines
		elapse before the affected line.  */
	     && start->line0 - top0 < thresh);

  return prev;
}

/* Set the `ignore' flag properly in each change in SCRIPT.
   It should be 1 if all the lines inserted or deleted in that change
   are ignorable lines.  */

static void
mark_ignorable (struct change *script, struct diffopt *opt)
{
  while (script)
    {
      struct change *next = script->link;
      lin first0, last0, first1, last1;

      /* Turn this change into a hunk: detach it from the others.  */
      script->link = NULL;

      /* Determine whether this change is ignorable.  */
      script->ignore = ! analyze_hunk (script,
				       &first0, &last0, &first1, &last1, opt);

      /* Reconnect the chain as before.  */
      script->link = next;

      /* Advance to the following change.  */
      script = next;
    }
}

/* Find the last function-header line in LINBUF prior to line number LINENUM.
   This is a line containing a match for the regexp in `opt->function_regexp'.
   OPT is the diff options.
   Return the address of the text, or 0 if no function-header is found.  */

static char const *
find_function (char const * const *linbuf, lin linenum,
	       struct diffopt *opt)
{
  lin i = linenum;
  lin last = opt->find_function.last_search;
  opt->find_function.last_search = i;

  while (last <= --i)
    {
      /* See if this line is what we want.  */
      char const *line = linbuf[i];
      size_t linelen = linbuf[i + 1] - line - 1;

      /* FIXME: re_search's size args should be size_t, not int.  */
      int len = MIN (linelen, INT_MAX);

      if (0 <= re_search (opt->function_regexp, line, len, 0, len, 0))
	{
	  opt->find_function.last_match = i;
	  return line;
	}
    }
  /* If we search back to where we started searching the previous time,
     find the line we found last time.  */
  if (opt->find_function.last_match != LIN_MAX)
    return linbuf[opt->find_function.last_match];

  return 0;
}
