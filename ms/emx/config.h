/* emx configuration */

#include <pc/config.h>
#include <sys/emxload.h>

#define initialize_main(pargc, pargv) \
  { \
    _response (pargc, pargv); \
    _wildcard (pargc, pargv); \
    _emxload_env ("RCSLOAD"); \
    setvbuf (stdout, NULL, _IOFBF, BUFSIZ); \
  }

/* diffutils doesn't need re_comp,
   but the shared library that diffutils builds
   is used by other (old-fashioned) programs that need re_comp.  */
#define _REGEX_RE_COMP

#define STAT_BLOCKSIZE(s) 65536
