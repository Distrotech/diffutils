/* Functions specific to PC operating systems
   Copyright 1994, 1995, 1998 Free Software Foundation, Inc.

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

#include <string.h>

char *
filename_lastdirchar (char const *filename)
{
  char const *last = 0;

  for (;  *filename;  filename++)
    if (*filename == '/' || *filename == '\\' || (*filename == ':' && !last))
      last = filename;

  return (char *) last;
}

/* Place into QUOTED a quoted version of ARG suitable for `system'.
   Return the length of the resulting string (which is not null-terminated).
   If QUOTED is null, return the length without any side effects.  */

size_t
quote_system_arg (char *quoted, char const *arg)
{
  int needs_quoting = !*arg || strchr (arg, ' ') || strchr (arg, '\t');
  size_t backslashes = 0;
  size_t len = 0;

  if (needs_quoting)
    {
      if (quoted)
	quoted[len] = '"';
      len++;
    }

  for (;;)
    {
      char c = *arg++;
      switch (c)
	{
	case 0:
	  if (needs_quoting)
	    {
	      if (quoted)
		{
		  memset (quoted + len, '\\', backslashes);
		  quoted[len + backslashes] = '"';
		}
	      len += backslashes + 1;
	    }
	  return len;

	case '"':
	  backslashes++;
	  if (quoted)
	    memset (quoted + len, '\\', backslashes);
	  len += backslashes;
	  /* fall through */
	default:
	  backslashes = 0;
	  break;

	case '\\':
	  backslashes++;
	  break;
	}

      if (quoted)
	quoted[len] = c;
      len++;
    }
}
