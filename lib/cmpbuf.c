/* Buffer primitives for comparison operations.
   Copyright 1993, 1995, 1997 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation, 
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "system.h"
#include "cmpbuf.h"

/* Read NCHARS bytes from descriptor FD into BUF.
   Return the number of characters successfully read.
   The number returned is always NCHARS unless end-of-file or error.  */

size_t
block_read (fd, buf, nchars)
     int fd;
     char *buf;
     size_t nchars;
{
  char *bp = buf;

  do
    {
      size_t nread = read (fd, bp, nchars);
      if (nread == -1)
	return -1;
      if (nread == 0)
	break;
      bp += nread;
      nchars -= nread;
    }
  while (nchars != 0);

  return bp - buf;
}

/* Least common multiple of two buffer sizes A and B.  */

size_t
buffer_lcm (a, b)
     size_t a, b;
{
  size_t m, n, r;

  /* Yield reasonable values if buffer sizes are zero.  */
  if (!a)
    return b ? b : 8 * 1024;
  if (!b)
    return a;

  /* n = gcd (a, b) */
  for (m = a, n = b;  (r = m % n) != 0;  m = n, n = r)
    continue;

  return a/n * b;
}
