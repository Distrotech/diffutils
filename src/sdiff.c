/* SDIFF -- interactive merge front end to diff */

static char const copyright_string[] =
  "Copyright 1992, 93, 94, 95, 96, 1997 Free Software Foundation, Inc.";

/* This file is part of GNU DIFF.

   GNU DIFF is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU DIFF is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation, 
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

static char const authorship_msgid[] =
  "Written by Thomas Lord.";

#include "system.h"
#include <stdio.h>
#include <signal.h>
#include "getopt.h"
#include "quotearg.h"

/* Size of chunks read from files which must be parsed into lines.  */
#define SDIFF_BUFSIZE ((size_t) 65536)

extern char const free_software_msgid[];
extern char const version_string[];
char *program_name;

static char const *editor_program = DEFAULT_EDITOR_PROGRAM;
static char const **diffargv;
static char const *not_found;

static char *tmpname;
static int volatile tmpmade;

#if HAVE_FORK
static pid_t volatile diffpid;
#endif

struct line_filter;

static FILE *ck_fopen PARAMS((char const *, char const *));
static RETSIGTYPE catchsig PARAMS((int));
static char const *expand_name PARAMS((char *, int, char const *));
static int edit PARAMS((struct line_filter *, char const *, int, int, struct line_filter *, char const *, int, int, FILE*));
static int interact PARAMS((struct line_filter *, struct line_filter *, char const *, struct line_filter *, char const *, FILE*));
static int lf_snarf PARAMS((struct line_filter *, char *, size_t));
static int skip_white PARAMS((void));
static size_t ck_fread PARAMS((char *, size_t, FILE *));
static size_t lf_refill PARAMS((struct line_filter *));
static void checksigs PARAMS((void));
static void ck_fclose PARAMS((FILE *));
static void ck_fflush PARAMS((FILE *));
static void ck_fwrite PARAMS((char const *, size_t, FILE *));
static void cleanup PARAMS((void));
static void diffarg PARAMS((char const *));
static void execdiff PARAMS((void));
static void exiterr PARAMS((void)) __attribute__((noreturn));
static void fatal PARAMS((char const *)) __attribute__((noreturn));
static void flush_line PARAMS((void));
static void give_help PARAMS((void));
static void lf_copy PARAMS((struct line_filter *, int, FILE *));
static void lf_init PARAMS((struct line_filter *, FILE *));
static void lf_skip PARAMS((struct line_filter *, int));
static void perror_fatal PARAMS((char const *)) __attribute__((noreturn));
static void trapsigs PARAMS((void));
static void try_help PARAMS((char const *, char const *)) __attribute__((noreturn));
static void untrapsig PARAMS((int));
static void usage PARAMS((void));

#define NUM_SIGS (sizeof (sigs) / sizeof (*sigs))
static int const sigs[] = {
#ifdef SIGHUP
       SIGHUP,
#endif
#ifdef SIGQUIT
       SIGQUIT,
#endif
#ifdef SIGTERM
       SIGTERM,
#endif
#ifdef SIGXCPU
       SIGXCPU,
#endif
#ifdef SIGXFSZ
       SIGXFSZ,
#endif
       SIGINT,
       SIGPIPE
};
#if HAVE_SIGACTION
  /* Prefer `sigaction' if it is available, since `signal' can lose signals.  */
  static struct sigaction initial_action[NUM_SIGS];
# define initial_handler(i) (initial_action[i].sa_handler)
  static void signal_handler PARAMS((int, RETSIGTYPE (*) PARAMS((int))));
#else
  static RETSIGTYPE (*initial_action[NUM_SIGS]) ();
# define initial_handler(i) (initial_action[i])
# define signal_handler(sig, handler) signal (sig, handler)
#endif

#if !HAVE_SIGPROCMASK
#define sigset_t int
#define sigemptyset(s) (*(s) = 0)
#ifndef sigmask
#define sigmask(sig) (1 << ((sig) - 1))
#endif
#define sigaddset(s, sig) (*(s) |= sigmask (sig))
#ifndef SIG_BLOCK
#define SIG_BLOCK 0
#endif
#ifndef SIG_SETMASK
#define SIG_SETMASK (!SIG_BLOCK)
#endif
#define sigprocmask(how, n, o) \
  ((how) == SIG_BLOCK \
   ? *(o) = sigblock (*(n)) \
   : sigsetmask (*(n)))
#endif

/* this lossage until the gnu libc conquers the universe */
#if HAVE_TMPNAM
#define private_tempnam() tmpnam ((char *) 0)
#else
#ifndef PVT_tmpdir
#define PVT_tmpdir "/tmp"
#endif
#ifndef TMPDIR_ENV
#define TMPDIR_ENV "TMPDIR"
#endif
static char *private_tempnam PARAMS((void));
static int exists PARAMS((char const *));
#endif
static int diraccess PARAMS((char const *));

#if __STDC__ && (HAVE_VPRINTF || HAVE_DOPRNT)
void error (int, int, char const *, ...) __attribute__((format (printf, 3, 4)));
#else
void error ();
#endif
VOID *xmalloc PARAMS((size_t));
VOID *xrealloc PARAMS((VOID *, size_t));
extern int xmalloc_exit_failure;

/* Options: */

/* name of output file if -o spec'd */
static char *out_file;

/* do not print common lines if true, set by -s option */
static int suppress_common_flag;

static struct option const longopts[] =
{
  {"ignore-blank-lines", 0, 0, 'B'},
  {"speed-large-files", 0, 0, 'H'},
  {"ignore-matching-lines", 1, 0, 'I'},
  {"ignore-all-space", 0, 0, 'W'}, /* swap W and w for historical reasons */
  {"text", 0, 0, 'a'},
  {"ignore-space-change", 0, 0, 'b'},
  {"minimal", 0, 0, 'd'},
  {"ignore-case", 0, 0, 'i'},
  {"left-column", 0, 0, 'l'},
  {"output", 1, 0, 'o'},
  {"suppress-common-lines", 0, 0, 's'},
  {"expand-tabs", 0, 0, 't'},
  {"width", 1, 0, 'w'},
  {"version", 0, 0, 'v'},
  {"help", 0, 0, CHAR_MAX + 1},
  {0, 0, 0, 0}
};

static void
try_help (reason_msgid, operand)
     char const *reason_msgid;
     char const *operand;
{
  if (reason_msgid)
    error (0, 0, _(reason_msgid), operand);
  error (2, 0, _("Try `%s --help' for more information."), program_name);
  abort ();
}

static char const * const option_help_msgid[] = {
  "",
  "-o FILE  --output=FILE  Operate interactively, sending output to FILE.",
  "",
  "-i  --ignore-case  Consider upper- and lower-case to be the same.",
  "-W  --ignore-all-space  Ignore all white space.",
  "-b  --ignore-space-change  Ignore changes in the amount of white space.",
  "-B  --ignore-blank-lines  Ignore changes whose lines are all blank.",
  "-I RE  --ignore-matching-lines=RE  Ignore changes whose lines all match RE.",
  "-a  --text  Treat all files as text.",
  "",
  "-w NUM  --width=NUM  Output at most NUM (default 130) characters per line.",
  "-l  --left-column  Output only the left column of common lines.",
  "-s  --suppress-common-lines  Do not output common lines.",
  "",
  "-t  --expand-tabs  Expand tabs to spaces in output.",
  "",
  "-d  --minimal  Try hard to find a smaller set of changes.",
  "-H  --speed-large-files  Assume large files and many scattered small changes.",
  "",
  "-v  --version  Output version info.",
  "--help  Output this help.",
  "",
  0
};

static void
usage ()
{
  char const * const *p;

  printf (_("Usage: %s [OPTION]... FILE1 FILE2\n"), program_name);
  printf (_("If a FILE is `-', read standard input.\n"));
  for (p = option_help_msgid;  *p;  p++)
    if (**p)
      printf ("  %s\n", _(*p));
    else
      putchar ('\n');
  printf (_("Report bugs to <bug-gnu-utils@prep.ai.mit.edu>.\n"));
}

static void
cleanup ()
{
#if HAVE_FORK
  if (0 < diffpid)
    kill (diffpid, SIGPIPE);
#endif
  if (tmpmade)
    unlink (tmpname);
}

static void
exiterr ()
{
  cleanup ();
  untrapsig (0);
  checksigs ();
  exit (2);
}

static void
fatal (msgid)
     char const *msgid;
{
  error (0, 0, "%s", _(msgid));
  exiterr ();
}

static void
perror_fatal (msg)
     char const *msg;
{
  int e = errno;
  checksigs ();
  error (0, e, "%s", msg);
  exiterr ();
}

static FILE *
ck_fopen (fname, type)
     char const *fname, *type;
{
  FILE *r = fopen (fname, type);
  if (!r)
    perror_fatal (fname);
  return r;
}

static void
ck_fclose (f)
     FILE *f;
{
  if (fclose (f))
    perror_fatal ("fclose");
}

static size_t
ck_fread (buf, size, f)
     char *buf;
     size_t size;
     FILE *f;
{
  size_t r = fread (buf, sizeof (char), size, f);
  if (r == 0 && ferror (f))
    perror_fatal (_("read failed"));
  return r;
}

static void
ck_fwrite (buf, size, f)
     char const *buf;
     size_t size;
     FILE *f;
{
  if (fwrite (buf, sizeof (char), size, f) != size)
    perror_fatal (_("write failed"));
}

static void
ck_fflush (f)
     FILE *f;
{
  if (fflush (f) != 0)
    perror_fatal (_("write failed"));
}

static char const *
expand_name (name, is_dir, other_name)
     char *name;
     int is_dir;
     char const *other_name;
{
  if (strcmp (name, "-") == 0)
    fatal ("cannot interactively merge standard input");
  if (!is_dir)
    return name;
  else
    {
      /* Yield NAME/BASE, where BASE is OTHER_NAME's basename.  */
      char const *p = filename_lastdirchar (other_name);
      char const *base = p ? p+1 : other_name;
      size_t namelen = strlen (name), baselen = strlen (base);
      char *r = xmalloc (namelen + baselen + 2);
      memcpy (r, name, namelen);
      r[namelen] = '/';
      memcpy (r + namelen + 1, base, baselen + 1);
      return r;
    }
}



struct line_filter {
  FILE *infile;
  char *bufpos;
  char *buffer;
  char *buflim;
};

static void
lf_init (lf, infile)
     struct line_filter *lf;
     FILE *infile;
{
  lf->infile = infile;
  lf->bufpos = lf->buffer = lf->buflim = xmalloc (SDIFF_BUFSIZE + 1);
  lf->buflim[0] = '\n';
}

/* Fill an exhausted line_filter buffer from its INFILE */
static size_t
lf_refill (lf)
     struct line_filter *lf;
{
  size_t s = ck_fread (lf->buffer, SDIFF_BUFSIZE, lf->infile);
  lf->bufpos = lf->buffer;
  lf->buflim = lf->buffer + s;
  lf->buflim[0] = '\n';
  checksigs ();
  return s;
}

/* Advance LINES on LF's infile, copying lines to OUTFILE */
static void
lf_copy (lf, lines, outfile)
     struct line_filter *lf;
     int lines;
     FILE *outfile;
{
  char *start = lf->bufpos;

  while (lines)
    {
      lf->bufpos = (char *) memchr (lf->bufpos, '\n', lf->buflim - lf->bufpos);
      if (! lf->bufpos)
	{
	  ck_fwrite (start, lf->buflim - start, outfile);
	  if (! lf_refill (lf))
	    return;
	  start = lf->bufpos;
	}
      else
	{
	  --lines;
	  ++lf->bufpos;
	}
    }

  ck_fwrite (start, lf->bufpos - start, outfile);
}

/* Advance LINES on LF's infile without doing output */
static void
lf_skip (lf, lines)
     struct line_filter *lf;
     int lines;
{
  while (lines)
    {
      lf->bufpos = (char *) memchr (lf->bufpos, '\n', lf->buflim - lf->bufpos);
      if (! lf->bufpos)
	{
	  if (! lf_refill (lf))
	    break;
	}
      else
	{
	  --lines;
	  ++lf->bufpos;
	}
    }
}

/* Snarf a line into a buffer.  Return EOF if EOF, 0 if error, 1 if OK.  */
static int
lf_snarf (lf, buffer, bufsize)
     struct line_filter *lf;
     char *buffer;
     size_t bufsize;
{
  char *start = lf->bufpos;

  for (;;)
    {
      char *next = (char *) memchr (start, '\n', lf->buflim + 1 - start);
      size_t s = next - start;
      if (bufsize <= s)
	return 0;
      memcpy (buffer, start, s);
      if (next < lf->buflim)
	{
	  buffer[s] = 0;
	  lf->bufpos = next + 1;
	  return 1;
	}
      if (! lf_refill (lf))
	return s ? 0 : EOF;
      buffer += s;
      bufsize -= s;
      start = next;
    }
}



int
main (argc, argv)
     int argc;
     char *argv[];
{
  int opt;
  char const *prog;

  initialize_main (&argc, &argv);
  setlocale (LC_ALL, "");
  program_name = argv[0];
  xmalloc_exit_failure = 2;

  prog = getenv ("EDITOR");
  if (prog)
    editor_program = prog;

  prog = getenv ("DIFF");
  diffarg (prog ? prog : DEFAULT_DIFF_PROGRAM);

  /* parse command line args */
  while ((opt = getopt_long (argc, argv, "abBdHiI:lo:stvw:W", longopts, 0))
	 != -1)
    {
      switch (opt)
	{
	case 'a':
	  diffarg ("-a");
	  break;

	case 'b':
	  diffarg ("-b");
	  break;

	case 'B':
	  diffarg ("-B");
	  break;

	case 'd':
	  diffarg ("-d");
	  break;

	case 'H':
	  diffarg ("-H");
	  break;

	case 'i':
	  diffarg ("-i");
	  break;

	case 'I':
	  diffarg ("-I");
	  diffarg (optarg);
	  break;

	case 'l':
	  diffarg ("--left-column");
	  break;

	case 'o':
	  out_file = optarg;
	  break;

	case 's':
	  suppress_common_flag = 1;
	  break;

	case 't':
	  diffarg ("-t");
	  break;

	case 'v':
	  printf ("sdiff %s\n%s\n\n%s\n\n%s\n",
		  version_string, copyright_string,
		  _(free_software_msgid), _(authorship_msgid));
	  exit (0);

	case 'w':
	  diffarg ("-W");
	  diffarg (optarg);
	  break;

	case 'W':
	  diffarg ("-w");
	  break;

	case CHAR_MAX + 1:
	  usage ();
	  if (ferror (stdout) || fclose (stdout) != 0)
	    fatal ("write failed");
	  exit (0);

	default:
	  try_help (0, 0);
	}
    }

  if (argc - optind != 2)
    {
      if (argc - optind < 2)
	try_help ("missing operand after `%s'", argv[argc - 1]);
      else
	try_help ("extra operand `%s'", argv[optind + 2]);
    }

  if (! out_file)
    {
      /* easy case: diff does everything for us */
      if (suppress_common_flag)
	diffarg ("--suppress-common-lines");
      diffarg ("-y");
      diffarg ("--");
      diffarg (argv[optind]);
      diffarg (argv[optind + 1]);
      diffarg (0);
      execdiff ();
    }
  else
    {
      char const *lname, *rname;
      FILE *left, *right, *out, *diffout;
      int interact_ok;
      struct line_filter lfilt;
      struct line_filter rfilt;
      struct line_filter diff_filt;
      int leftdir = diraccess (argv[optind]);
      int rightdir = diraccess (argv[optind + 1]);

      if (leftdir && rightdir)
	fatal ("both files to be compared are directories");

      lname = expand_name (argv[optind], leftdir, argv[optind + 1]);
      left = ck_fopen (lname, "r");
      rname = expand_name (argv[optind + 1], rightdir, argv[optind]);
      right = ck_fopen (rname, "r");
      out = ck_fopen (out_file, "w");

      diffarg ("--sdiff-merge-assist");
      diffarg ("--");
      diffarg (argv[optind]);
      diffarg (argv[optind + 1]);
      diffarg (0);

      trapsigs ();

#if ! HAVE_FORK
      {
	size_t cmdsize = 1;
	char *p, *command;
	int i;

	for (i = 0;  diffargv[i];  i++)
	  cmdsize += quote_system_arg ((char *) 0, diffargv[i]) + 1;
	command = p = xmalloc (cmdsize);
	for (i = 0;  diffargv[i];  i++)
	  {
	    p += quote_system_arg (p, diffargv[i]);
	    *p++ = ' ';
	  }
	p[-1] = '\0';
	diffout = popen (command, "r");
	if (!diffout)
	  perror_fatal (command);
	free (command);
      }
#else /* HAVE_FORK */
      {
	int diff_fds[2];
#if HAVE_VFORK
	sigset_t procmask;
	sigset_t blocked;
#endif

	if (pipe (diff_fds) != 0)
	  perror_fatal ("pipe");

	not_found = _(": not found\n");

#if HAVE_VFORK
	/* Block SIGINT and SIGPIPE.  */
	sigemptyset (&blocked);
	sigaddset (&blocked, SIGINT);
	sigaddset (&blocked, SIGPIPE);
	sigprocmask (SIG_BLOCK, &blocked, &procmask);
#endif
	diffpid = vfork ();
	if (diffpid < 0)
	  perror_fatal ("fork");
	if (!diffpid)
	  {
	    /* Alter the child's SIGINT and SIGPIPE handlers;
	       this may munge the parent.
	       The child ignores SIGINT in case the user interrupts the editor.
	       The child does not ignore SIGPIPE, even if the parent does.  */
	    if (initial_handler (SIGINT) != SIG_IGN)
	      signal_handler (SIGINT, SIG_IGN);
	    signal_handler (SIGPIPE, SIG_DFL);
#if HAVE_VFORK
	    /* Stop blocking SIGINT and SIGPIPE in the child.  */
	    sigprocmask (SIG_SETMASK, &procmask, (sigset_t *) 0);
#endif
	    close (diff_fds[0]);
	    if (diff_fds[1] != STDOUT_FILENO)
	      {
		dup2 (diff_fds[1], STDOUT_FILENO);
		close (diff_fds[1]);
	      }

	    execdiff ();
	  }

#if HAVE_VFORK
	/* Restore the parent's SIGINT and SIGPIPE behavior.  */
	if (initial_handler (SIGINT) != SIG_IGN)
	  signal_handler (SIGINT, catchsig);
	if (initial_handler (SIGPIPE) != SIG_IGN)
	  signal_handler (SIGPIPE, catchsig);
	else
	  signal_handler (SIGPIPE, SIG_IGN);

	/* Stop blocking SIGINT and SIGPIPE in the parent.  */
	sigprocmask (SIG_SETMASK, &procmask, (sigset_t *) 0);
#endif

	close (diff_fds[1]);
	diffout = fdopen (diff_fds[0], "r");
	if (!diffout)
	  perror_fatal ("fdopen");
      }
#endif /* HAVE_FORK */

      lf_init (&diff_filt, diffout);
      lf_init (&lfilt, left);
      lf_init (&rfilt, right);

      interact_ok = interact (&diff_filt, &lfilt, lname, &rfilt, rname, out);

      ck_fclose (left);
      ck_fclose (right);
      ck_fclose (out);

      {
	int wstatus;

#if ! HAVE_FORK
	wstatus = pclose (diffout);
#else
	ck_fclose (diffout);
	while (waitpid (diffpid, &wstatus, 0) < 0)
	  if (errno == EINTR)
	    checksigs ();
	  else
	    perror_fatal ("waitpid");
	diffpid = 0;
#endif

	if (tmpmade)
	  {
	    unlink (tmpname);
	    tmpmade = 0;
	  }

	if (! interact_ok)
	  exiterr ();

	if (! (WIFEXITED (wstatus) && WEXITSTATUS (wstatus) < 2))
	  fatal ("subsidiary program failed");

	untrapsig (0);
	checksigs ();
	exit (WEXITSTATUS (wstatus));
      }
    }
  return 0;			/* Fool `-Wall'.  */
}

static void
diffarg (a)
     char const *a;
{
  static unsigned diffargs, diffarglim;

  if (diffargs == diffarglim)
    {
      diffarglim = diffarglim ? 2 * diffarglim : 16;
      diffargv = (char const **) xrealloc (diffargv,
					   diffarglim * sizeof (char const *));
    }
  diffargv[diffargs++] = a;
}

static void
execdiff ()
{
  execvp (diffargv[0], (char **) diffargv);
  write (STDERR_FILENO, diffargv[0], strlen (diffargv[0]));
  write (STDERR_FILENO, not_found, strlen (not_found));
  _exit (2);
}




/* Signal handling */

static int volatile ignore_SIGINT;
static int volatile signal_received;
static int sigs_trapped;

static RETSIGTYPE
catchsig (s)
     int s;
{
#if ! HAVE_SIGACTION
  signal (s, SIG_IGN);
#endif
  if (! (s == SIGINT && ignore_SIGINT))
    signal_received = s;
}

#if HAVE_SIGACTION
static struct sigaction catchaction;

static void
signal_handler (sig, handler)
     int sig;
     RETSIGTYPE (*handler) PARAMS((int));
{
  catchaction.sa_handler = handler;
  sigaction (sig, &catchaction, (struct sigaction *) 0);
}
#endif

static void
trapsigs ()
{
  int i;

#if HAVE_SIGACTION
#ifdef SA_INTERRUPT
  /* Non-Posix BSD-style systems like SunOS 4.1.x need this
     so that `read' calls are interrupted properly.  */
  catchaction.sa_flags = SA_INTERRUPT;
#endif
  sigemptyset (&catchaction.sa_mask);
  for (i = 0;  i < NUM_SIGS;  i++)
    sigaddset (&catchaction.sa_mask, sigs[i]);
#endif

  for (i = 0;  i < NUM_SIGS;  i++)
    {
#if HAVE_SIGACTION
      sigaction (sigs[i], (struct sigaction *) 0, &initial_action[i]);
#else
      initial_action[i] = signal (sigs[i], SIG_IGN);
#endif
      if (initial_handler (i) != SIG_IGN)
	signal_handler (sigs[i], catchsig);
    }

#if !defined (SIGCHLD) && defined (SIGCLD)
#define SIGCHLD SIGCLD
#endif
#ifdef SIGCHLD
  /* System V fork+wait does not work if SIGCHLD is ignored.  */
  signal (SIGCHLD, SIG_DFL);
#endif

  sigs_trapped = 1;
}

/* Untrap signal S, or all trapped signals if S is zero.  */
static void
untrapsig (s)
     int s;
{
  int i;

  if (sigs_trapped)
    for (i = 0;  i < NUM_SIGS;  i++)
      if ((!s || sigs[i] == s)  &&  initial_handler (i) != SIG_IGN)
#if HAVE_SIGACTION
	  sigaction (sigs[i], &initial_action[i], (struct sigaction *) 0);
#else
	  signal (sigs[i], initial_action[i]);
#endif
}

/* Exit if a signal has been received.  */
static void
checksigs ()
{
  int s = signal_received;
  if (s)
    {
      cleanup ();

      /* Yield an exit status indicating that a signal was received.  */
      untrapsig (s);
      kill (getpid (), s);

      /* That didn't work, so exit with error status.  */
      exit (2);
    }
}


static void
give_help ()
{
  fprintf (stderr, "%s", _("\
ed:\tEdit then use both versions, each decorated with a header.\n\
eb:\tEdit then use both versions.\n\
el:\tEdit then use the left version.\n\
er:\tEdit then use the right version.\n\
e:\tEdit a new version.\n\
l:\tUse the left version.\n\
r:\tUse the right version.\n\
s:\tSilently include common lines.\n\
v:\tVerbosely include common lines.\n\
q:\tQuit.\n\
"));
}

static int
skip_white ()
{
  int c;
  for (;;)
    {
      c = getchar ();
      if (!ISSPACE (c) || c == '\n')
	break;
      checksigs ();
    }
  if (ferror (stdin))
    perror_fatal (_("read failed"));
  return c;
}

static void
flush_line ()
{
  int c;
  while ((c = getchar ()) != '\n' && c != EOF)
    ;
  if (ferror (stdin))
    perror_fatal (_("read failed"));
}


/* interpret an edit command */
static int
edit (left, lname, lline, llen, right, rname, rline, rlen, outfile)
     struct line_filter *left;
     char const *lname;
     int lline, llen;
     struct line_filter *right;
     char const *rname;
     int rline, rlen;
     FILE *outfile;
{
  for (;;)
    {
      int cmd0, cmd1;
      int gotcmd = 0;

      cmd1 = 0; /* Pacify `gcc -W'.  */

      while (!gotcmd)
	{
	  if (putchar ('%') != '%')
	    perror_fatal (_("write failed"));
	  ck_fflush (stdout);

	  cmd0 = skip_white ();
	  switch (cmd0)
	    {
	    case 'l': case 'r': case 's': case 'v': case 'q':
	      if (skip_white () != '\n')
		{
		  give_help ();
		  flush_line ();
		  continue;
		}
	      gotcmd = 1;
	      break;

	    case 'e':
	      cmd1 = skip_white ();
	      switch (cmd1)
		{
		case 'b': case 'd': case 'l': case 'r':
		  if (skip_white () != '\n')
		    {
		      give_help ();
		      flush_line ();
		      continue;
		    }
		  gotcmd = 1;
		  break;
		case '\n':
		  gotcmd = 1;
		  break;
		default:
		  give_help ();
		  flush_line ();
		  continue;
		}
	      break;

	    case EOF:
	      if (feof (stdin))
		{
		  gotcmd = 1;
		  cmd0 = 'q';
		  break;
		}
	      /* Fall through.  */
	    default:
	      flush_line ();
	      /* Fall through.  */
	    case '\n':
	      give_help ();
	      continue;
	    }
	}

      switch (cmd0)
	{
	case 'l':
	  lf_copy (left, llen, outfile);
	  lf_skip (right, rlen);
	  return 1;
	case 'r':
	  lf_copy (right, rlen, outfile);
	  lf_skip (left, llen);
	  return 1;
	case 's':
	  suppress_common_flag = 1;
	  break;
	case 'v':
	  suppress_common_flag = 0;
	  break;
	case 'q':
	  return 0;
	case 'e':
	  if (! tmpname && ! (tmpname = private_tempnam ()))
	    perror_fatal ("tmpnam");

	  tmpmade = 1;

	  {
	    FILE *tmp = ck_fopen (tmpname, "w+");

	    switch (cmd1)
	      {
	      case 'd':
		if (llen)
		  {
		    if (llen == 1)
		      fprintf (tmp, "--- %s %d\n", lname, lline);
		    else
		      fprintf (tmp, "--- %s %d,%d\n", lname, lline,
			       lline + llen - 1);
		  }
		/* Fall through.  */
	      case 'b': case 'l':
		lf_copy (left, llen, tmp);
		break;

	      default:
		lf_skip (left, llen);
		break;
	      }

	    switch (cmd1)
	      {
	      case 'd':
		if (rlen)
		  {
		    if (rlen == 1)
		      fprintf (tmp, "+++ %s %d\n", rname, rline);
		    else
		      fprintf (tmp, "+++ %s %d,%d\n", rname, rline,
			       rline + rlen - 1);
		  }
		/* Fall through.  */
	      case 'b': case 'r':
		lf_copy (right, rlen, tmp);
		break;

	      default:
		lf_skip (right, rlen);
		break;
	      }

	    ck_fflush (tmp);

	    {
	      int wstatus;
#if ! HAVE_FORK
	      char *command =
		xmalloc (quote_system_arg ((char *) 0, editor_program)
			 + 1 + strlen (tmpname) + 1);
	      sprintf (command + quote_system_arg (command, editor_program),
		       " %s", tmpname);
	      wstatus = system (command);
	      free (command);
#else /* HAVE_FORK */
	      pid_t pid;

	      ignore_SIGINT = 1;
	      checksigs ();

	      not_found = _(": not found\n");
	      pid = vfork ();
	      if (pid == 0)
		{
		  char const *argv[3];
		  int i = 0;

		  argv[i++] = editor_program;
		  argv[i++] = tmpname;
		  argv[i++] = 0;

		  execvp (editor_program, (char **) argv);
		  write (STDERR_FILENO, editor_program,
			 strlen (editor_program));
		  write (STDERR_FILENO, not_found, strlen (not_found));
		  _exit (1);
		}

	      if (pid < 0)
		perror_fatal ("fork");

	      while (waitpid (pid, &wstatus, 0) < 0)
		if (errno == EINTR)
		  checksigs ();
		else
		  perror_fatal ("waitpid");

	      ignore_SIGINT = 0;
#endif /* HAVE_FORK */

	      if (wstatus != 0)
		fatal ("subsidiary program failed");
	    }

	    if (fseek (tmp, 0L, SEEK_SET) != 0)
	      perror_fatal ("fseek");
	    {
	      /* SDIFF_BUFSIZE is too big for a local var
		 in some compilers, so we allocate it dynamically.  */
	      char *buf = xmalloc (SDIFF_BUFSIZE);
	      size_t size;

	      while ((size = ck_fread (buf, SDIFF_BUFSIZE, tmp)) != 0)
		{
		  checksigs ();
		  ck_fwrite (buf, size, outfile);
		}
	      ck_fclose (tmp);

	      free (buf);
	    }
	    return 1;
	  }
	default:
	  give_help ();
	  break;
	}
    }
}



/* Alternately reveal bursts of diff output and handle user commands.  */
static int
interact (diff, left, lname, right, rname, outfile)
     struct line_filter *diff;
     struct line_filter *left;
     char const *lname;
     struct line_filter *right;
     char const *rname;
     FILE *outfile;
{
  int lline = 1, rline = 1;

  for (;;)
    {
      char diff_help[256];
      int snarfed = lf_snarf (diff, diff_help, sizeof (diff_help));

      if (snarfed <= 0)
	return snarfed;

      checksigs ();

      switch (diff_help[0])
	{
	case ' ':
	  puts (diff_help + 1);
	  break;
	case 'i':
	  {
	    int llen = atoi (diff_help + 1), rlen, lenmax;
	    char *p = strchr (diff_help, ',');

	    if (!p)
	      fatal (diff_help);
	    rlen = atoi (p + 1);
	    lenmax = max (llen, rlen);

	    if (suppress_common_flag)
	      lf_skip (diff, lenmax);
	    else
	      lf_copy (diff, lenmax, stdout);

	    lf_copy (left, llen, outfile);
	    lf_skip (right, rlen);
	    lline += llen;
	    rline += rlen;
	    break;
	  }
	case 'c':
	  {
	    int llen = atoi (diff_help + 1), rlen;
	    char *p = strchr (diff_help, ',');

	    if (!p)
	      fatal (diff_help);
	    rlen = atoi (p + 1);
	    lf_copy (diff, max (llen, rlen), stdout);
	    if (! edit (left, lname, lline, llen, right, rname, rline, rlen,
			outfile))
	      return 0;
	    lline += llen;
	    rline += rlen;
	    break;
	  }
	default:
	  fatal (diff_help);
	  break;
	}
    }
}



/* temporary lossage: this is torn from gnu libc */
/* Return nonzero if DIR is an existing directory.  */
static int
diraccess (dir)
     char const *dir;
{
  struct stat buf;
  return stat (dir, &buf) == 0 && S_ISDIR (buf.st_mode);
}

#if ! HAVE_TMPNAM

/* Return zero if we know that FILE does not exist.  */
static int
exists (file)
     char const *file;
{
  struct stat buf;
  return stat (file, &buf) == 0 || errno != ENOENT;
}

/* These are the characters used in temporary filenames.  */
static char const letters[] =
  "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

/* Generate a temporary filename and return it (in a newly allocated buffer).
   Use the prefix "dif" as in tempnam.
   This goes through a cyclic pattern of all possible
   filenames consisting of five decimal digits of the current pid and three
   of the characters in `letters'.  Each potential filename is
   tested for an already-existing file of the same name, and no name of an
   existing file will be returned.  When the cycle reaches its end
   return 0.  */
static char *
private_tempnam ()
{
  char const *dir = getenv (TMPDIR_ENV);
  static char const tmpdir[] = PVT_tmpdir;
  size_t index;
  char *buf;
  pid_t pid = getpid ();
  size_t dlen;

  if (!dir)
    dir = tmpdir;

  dlen = strlen (dir);

  /* Remove trailing slashes from the directory name.  */
  while (dlen && dir[dlen - 1] == '/')
    --dlen;

  buf = xmalloc (dlen + 1 + 3 + 5 + 1 + 3 + 1);

  sprintf (buf, "%.*s/.", (int) dlen, dir);
  if (diraccess (buf))
    {
      for (index = 0;
	   index < ((sizeof (letters) - 1) * (sizeof (letters) - 1)
		    * (sizeof (letters) - 1));
	   ++index)
	{
	  /* Construct a file name and see if it already exists.

	     We use a single counter in INDEX to cycle each of three
	     character positions through each of 62 possible letters.  */

	  sprintf (buf, "%.*s/dif%.5lu.%c%c%c", (int) dlen, dir,
		   (unsigned long) pid % 100000,
		   letters[index % (sizeof (letters) - 1)],
		   letters[(index / (sizeof (letters) - 1))
			   % (sizeof (letters) - 1)],
		   letters[index / ((sizeof (letters) - 1) *
				     (sizeof (letters) - 1))]);

	  if (!exists (buf))
	    return buf;
	}
      errno = EEXIST;
    }

  /* Don't free buf; `free' might change errno.  We'll exit soon anyway.  */
  return 0;
}

#endif /* ! HAVE_TMPNAM */
