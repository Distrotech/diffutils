/* Functions specific to PC operating systems
   Copyright (C) 1994 Free Software Foundation, Inc.

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
along with GNU DIFF; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <stdio.h>
#include <string.h>

void
initialize_main (int *pargc, char ***pargv)
{
  _response (pargc, pargv);
  _wildcard (pargc, pargv);

  _emxload_env ("RCSLOAD");

  setvbuf (stdout, NULL, _IOFBF, BUFSIZ);
}

char *
filename_lastdirchar (char const *filename)
{
  char const *last = 0;

  for (;  *filename;  filename++)
    if (*filename == '/' || *filename == '\\' || (*filename == ':' && !last))
      last = filename;

  return (char *) last;
}

char *
system_quote_arg (char *q, char const *a)
{
  int needs_quoting = !*a || strchr (a, ' ') || strchr (a, '\t');
  size_t backslashes = 0;
  char c;

  if (needs_quoting)
    *q++ = '"';

  for (;;  *q++ = c)
    {
      c = *a++;
      switch (c)
	{
	case '\\':
	  backslashes++;
	  continue;

	case '"':
	  backslashes++;
	  memset (q, '\\', backslashes);
	  q += backslashes;
	  /* fall through */
	default:
	  backslashes = 0;
	  continue;

	case 0:
	  break;
	}
      break;
    }

  if (needs_quoting)
    {
      memset (q, '\\', backslashes);
      q += backslashes;
      *q++ = '"';
    }

  return q;
}
