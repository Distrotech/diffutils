/* OS/2 specific initialization */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
os2_initialize_main (int *pargc, char ***pargv)
{
  _response (pargc, pargv);
  _wildcard (pargc, pargv);

  _emxload_env ("RCSLOAD");

  setvbuf (stdout, NULL, _IOFBF, BUFSIZ);
}

char *
os2_filename_lastdirchar (char const *filename)
{
  char const *last = 0;
 
  for (;  *filename;  filename++)
    if (*filename == '/' || *filename == '\\' || (*filename == ':' && !last))
      last = filename;

  return (char *) last;
}
