/* Read, sort and compare two directories.  Used for GNU DIFF.
   Copyright 1988, 89, 92, 93, 94, 95, 1998 Free Software Foundation, Inc.

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

/* Read the directory named by DIR and store into DIRDATA a sorted vector
   of filenames for its contents.  DIR->desc == -1 means this directory is
   known to be nonexistent, so set DIRDATA to an empty vector.
   Return -1 (setting errno) if error, 0 otherwise.  */

struct dirdata
{
  char const **names;	/* Sorted names of files in dir, 0-terminated.  */
  char *data;	/* Allocated storage for file names.  */
};

static int compare_names PARAMS((void const *, void const *));
static int dir_loop PARAMS((struct comparison const *, int));
static int dir_sort PARAMS((struct file_data const *, struct dirdata *));

static int
dir_sort (dir, dirdata)
     struct file_data const *dir;
     struct dirdata *dirdata;
{
  register struct dirent *next;
  register int i;

  /* Address of block containing the files that are described.  */
  char const **names;

  /* Number of files in directory.  */
  size_t nnames;

  /* Allocated and used storage for file name data.  */
  char *data;
  size_t data_alloc, data_used;

  dirdata->names = 0;
  dirdata->data = 0;
  nnames = 0;
  data = 0;

  if (dir->desc != -1)
    {
      /* Open the directory and check for errors.  */
      register DIR *reading = opendir (dir->name);
      if (!reading)
	return -1;

      /* Initialize the table of filenames.  */

      data_alloc = max (1, (size_t) dir->stat.st_size);
      data_used = 0;
      dirdata->data = data = xmalloc (data_alloc);

      /* Read the directory entries, and insert the subfiles
	 into the `data' table.  */

      while ((errno = 0, (next = readdir (reading)) != 0))
	{
	  char *d_name = next->d_name;
	  size_t d_size = NAMLEN (next) + 1;

	  /* Ignore the files `.' and `..' */
	  if (d_name[0] == '.'
	      && (d_name[1] == 0 || (d_name[1] == '.' && d_name[2] == 0)))
	    continue;

	  if (excluded_filename (d_name))
	    continue;

	  while (data_alloc < data_used + d_size)
	    dirdata->data = data = xrealloc (data, data_alloc *= 2);
	  memcpy (data + data_used, d_name, d_size);
	  data_used += d_size;
	  nnames++;
	}
      if (errno)
	{
	  int e = errno;
	  closedir (reading);
	  errno = e;
	  return -1;
	}
#if CLOSEDIR_VOID
      closedir (reading);
#else
      if (closedir (reading) != 0)
	return -1;
#endif
    }

  /* Create the `names' table from the `data' table.  */
  dirdata->names = names = (char const **) xmalloc (sizeof (char *)
						    * (nnames + 1));
  for (i = 0;  i < nnames;  i++)
    {
      names[i] = data;
      data += strlen (data) + 1;
    }
  names[nnames] = 0;

  /* Sort the table.  */
  qsort (names, nnames, sizeof (char *), compare_names);

  return 0;
}

/* Sort the files now in the table.  */

static int
compare_names (file1, file2)
     void const *file1, *file2;
{
  return filename_cmp (* (char const *const *) file1,
		       * (char const *const *) file2);
}

/* Compare the contents of two directories named in CMP.
   This is a top-level routine; it does everything necessary for diff
   on two directories.

   CMP->file[0].desc == -1 says directory CMP->file[0] doesn't exist,
   but pretend it is empty.  Likewise for CMP->file[1].

   HANDLE_FILE is a caller-provided subroutine called to handle each file.
   It gets three operands: CMP, name of file in dir 0, name of file in dir 1.
   These names are relative to the original working directory.

   For a file that appears in only one of the dirs, one of the name-args
   to HANDLE_FILE is zero.

   Returns the maximum of all the values returned by HANDLE_FILE,
   or 2 if trouble is encountered in opening files.  */

int
diff_dirs (cmp, handle_file)
     struct comparison const *cmp;
     int (*handle_file) PARAMS((struct comparison const *, char const *, char const *));
{
  struct dirdata dirdata[2];
  int val = 0;			/* Return value.  */
  int i;

  if ((cmp->file[0].desc == -1 || dir_loop (cmp, 0))
      && (cmp->file[1].desc == -1 || dir_loop (cmp, 1)))
    {
      error (0, 0, "%s: recursive directory loop",
	     cmp->file[cmp->file[0].desc == -1].name);
      return 2;
    }

  /* Get sorted contents of both dirs.  */
  for (i = 0; i < 2; i++)
    if (dir_sort (&cmp->file[i], &dirdata[i]) != 0)
      {
	perror_with_name (cmp->file[i].name);
	val = 2;
      }

  if (val == 0)
    {
      register char const * const *names0 = dirdata[0].names;
      register char const * const *names1 = dirdata[1].names;

      /* If `-S name' was given, and this is the topmost level of comparison,
	 ignore all file names less than the specified starting name.  */

      if (dir_start_file && ! cmp->parent)
	{
	  while (*names0 && filename_cmp (*names0, dir_start_file) < 0)
	    names0++;
	  while (*names1 && filename_cmp (*names1, dir_start_file) < 0)
	    names1++;
	}

      /* Loop while files remain in one or both dirs.  */
      while (*names0 || *names1)
	{
	  /* Compare next name in dir 0 with next name in dir 1.
	     At the end of a dir,
	     pretend the "next name" in that dir is very large.  */
	  int nameorder = (!*names0 ? 1 : !*names1 ? -1
			   : filename_cmp (*names0, *names1));
	  int v1 = (*handle_file) (cmp,
				   0 < nameorder ? 0 : *names0++,
				   nameorder < 0 ? 0 : *names1++);
	  if (v1 > val)
	    val = v1;
	}
    }

  for (i = 0; i < 2; i++)
    {
      if (dirdata[i].names)
	free (dirdata[i].names);
      if (dirdata[i].data)
	free (dirdata[i].data);
    }

  return val;
}

/* Return nonzero if CMP is looping recursively in argument I.  */

static int
dir_loop (cmp, i)
     struct comparison const *cmp;
     int i;
{
  struct comparison const *p = cmp;
  while ((p = p->parent))
    if (0 < same_file (&p->file[i].stat, &cmp->file[i].stat))
      return 1;
  return 0;
}
