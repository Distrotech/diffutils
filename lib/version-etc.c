/* Utility to help print --version output in a consistent format.
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by Jim Meyering. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include "version-etc.h"

#include <gettext.h>
#define _(text) gettext (text)

/* Display the --version information the standard way.

   If COMMAND_NAME is NULL, the PACKAGE is asumed to be the name of
   the program.  The formats are therefore:

   PACKAGE VERSION

   or

   COMMAND_NAME (PACKAGE) VERSION.  */
void
version_etc (char const *command_name, char const *authorship_msgid)
{
  char const *package = PACKAGE_NAME;
  char const *version = PACKAGE_VERSION;

  /* TRANSLATORS: Please translate "(C)" to the C-in-a-circle symbol
     (U+00A9, COPYRIGHT SIGN) if possible, as this has some minor
     technical advantages in international copyright law.  If the
     copyright symbol is not available, please leave it as "(C)".  */
  char const *copyright_sign = _("(C)");

  if (command_name)
    printf ("%s (%s)", command_name, package);
  else
    printf ("%s", package);

  /* Do not translate the English word "Copyright", since it has
     special status in international copyright law.  Also, do not
     translate the name of the copyright holder, as common practice
     seems to be to leave names untranslated.  */
  printf (" %s\nCopyright %s 2002 Free Software Foundation, Inc.\n\n%s\n",
	  version, copyright_sign,
	   _("\
This program comes with NO WARRANTY, to the extent permitted by law.\n\
You may redistribute copies of this program\n\
under the terms of the GNU General Public License.\n\
For more information about these matters, see the files named COPYING."));

  if (authorship_msgid)
    printf ("\n%s\n", _(authorship_msgid));
}
