/* emx configuration */

#include <pc/config.h>

#define initialize_main(pargc, pargv) \
  { \
    _response (pargc, pargv); \
    _wildcard (pargc, pargv); \
    _emxload_env ("RCSLOAD"); \
    setvbuf (stdout, NULL, _IOFBF, BUFSIZ); \
  }

#define pid_t int

#define STAT_BLOCKSIZE(s) (64 * 1024)
