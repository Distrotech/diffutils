/* #ifdef-format output routines for GNU DIFF.
   Copyright (C) 1989, 91, 92 Free Software Foundation, Inc.

This file is part of GNU DIFF.

GNU DIFF is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY.  No author or distributor
accepts responsibility to anyone for the consequences of using it
or for whether it serves any particular purpose or works at all,
unless he says so in writing.  Refer to the GNU DIFF General Public
License for full details.

Everyone is granted permission to copy, modify and redistribute
GNU DIFF, but only under the conditions described in the
GNU DIFF General Public License.   A copy of this license is
supposed to have been given to you along with GNU DIFF so you
can know your rights and responsibilities.  It should be in a
file named COPYING.  Among other things, the copyright notice
and this notice must be preserved on all copies.  */


#include "diff.h"

static void print_ifdef_hunk ();
static void print_ifdef_lines ();
struct change *find_change ();

static int next_line;

/* Print the edit-script SCRIPT as a merged #ifdef file.  */

void
print_ifdef_script (script)
     struct change *script;
{
  next_line = - files[0].prefix_lines;
  print_script (script, find_change, print_ifdef_hunk);
  if (next_line < files[0].valid_lines)
    {
      begin_output ();
      print_ifdef_lines (&files[0], next_line, files[0].valid_lines);
    }
}

/* Print a hunk of an ifdef diff.
   This is a contiguous portion of a complete edit script,
   describing changes in consecutive lines.  */

static void
print_ifdef_hunk (hunk)
     struct change *hunk;
{
  register FILE *out;
  int first0, last0, first1, last1, deletes, inserts;
  register char c;
  const char *format;

  /* Determine range of line numbers involved in each file.  */
  analyze_hunk (hunk, &first0, &last0, &first1, &last1, &deletes, &inserts);
  if (inserts)
    format = deletes ? ifnelse_format : ifdef_format;
  else if (deletes)
    format = ifndef_format;
  else
    return;

  begin_output ();

  /* Print out lines up to this change.  */
  print_ifdef_lines (&files[0], next_line, first0);

  next_line = last0 + 1;
  out = outfile;

  while ((c = *format++) != 0)
    {
      if (c == '%')
	switch ((c = *format++))
	  {
	  case 0:
	    return;

	  default:
	    continue;

	  case '<':
	    /* Print out stuff deleted from first file.  */
	    print_ifdef_lines (&files[0], first0, last0 + 1);
	    continue;

	  case '>':
	    /* Print out stuff inserted from second file.  */
	    print_ifdef_lines (&files[1], first1, last1 + 1);
	    continue;

	  case 'n':
	    c = '\n';
	    break;

	  case '%':
	    break;
	  }
      putc (c, out);
  }
}

/* Print lines of CURRENT starting with FROM and continuing up to UPTO.  */
static void
print_ifdef_lines (current, from, upto)
     struct file_data *current;
     int from, upto;
{
  const char **linbuf = current->linbuf;

  if (tab_expand_flag)
    while (from < upto)
      print_1_line ("", &linbuf[from++]);
  else
    fwrite (linbuf[from], sizeof (char), linbuf[upto] - linbuf[from], outfile);
}
