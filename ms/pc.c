/* OS/2 specific initialization */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern usage();
extern char *program;
extern const char version_string[];

void os2_initialize_main(int *pargc, char ***pargv)
{
  if (*pargc == 1)
  {
    program = (*pargv)[0];
    printf ("\nGNU %s, version %s\n\n", program, version_string);
    usage();
    exit(0);
  }

  _response(pargc, pargv);
  _wildcard(pargc, pargv);

  _emxload_env("RCSLOAD");

  setvbuf(stdout, NULL, _IOFBF, BUFSIZ);
}

char *os2_filename_lastdirchar(const char *filename)
{
  char *p = strrchr (filename, '/');

  if (p == NULL)
    p = strrchr (filename, '\\');
  if (p == NULL)
    p = strrchr (filename, ':');

  return p;
}

