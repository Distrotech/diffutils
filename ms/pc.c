/* OS/2 specific initialization */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern usage ();
extern char *program_name;
extern const char version_string [];

void
os2_initialize_main (int *pargc, char ***pargv)
{
  if (*pargc == 1)
  {
    program_name = (*pargv)[0];
    printf ("\nGNU %s, version %s\n\n", program_name, version_string);
    usage ();
    exit (0);
  }

  _response (pargc, pargv);
  _wildcard (pargc, pargv);

  _emxload_env ("RCSLOAD");

  setvbuf (stdout, NULL, _IOFBF, BUFSIZ);
}

char *
os2_filename_lastdirchar (const char *filename)
{
  char const *last = 0;

  for (;  *filename;  filename++)
    if (*filename == '/' || *filename == '\\' || (*filename == ':' && !last))
      last = filename;

  return (char *) last;
}

char *os2_quote_arg(char *buffer, const char *arg)
{
  *buffer++ = '"';

  while (*arg)
  {
    if (*arg == '"')
      *buffer++ = '\\';

    *buffer++ = *arg++;
  }

  *buffer++ = '"';
  *buffer = '\0';

  return buffer;
}
