/* Stack overflow handling.

   Copyright (C) 2002 Free Software Foundation, Inc.

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

/* Written by Paul Eggert.  */

/* This module assumes that each stack frame is smaller than a page.
   If you use alloca, dynamic arrays, or large local variables, your
   program may extend the stack by more than a page at a time.  If so,
   the code below may incorrectly report a program error, or worse
   yet, may not detect the overflow at all.  To avoid this problem,
   don't use large local arrays.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "gettext.h"
#define _(msgid) gettext (msgid)

#if HAVE_INTTYPES_H
# include <inttypes.h>
#else
# if HAVE_STDINT_H
#  include <stdint.h>
# endif
#endif

#include <signal.h>
#ifndef SA_NODEFER
# define SA_NODEFER 0
#endif
#ifndef SA_ONSTACK
# define SA_ONSTACK 0
#endif
#ifndef SA_RESETHAND
# define SA_RESETHAND 0
#endif
#ifndef SA_SIGINFO
# define SA_SIGINFO 0
#endif
#ifndef SIGSTKSZ
# define SIGSTKSZ 8192
#endif

#include <stdlib.h>
#include <string.h>

#if HAVE_UCONTEXT_H
# include <ucontext.h>
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifndef STDERR_FILENO
# define STDERR_FILENO 2
#endif
#ifndef _SC_PAGESIZE
# undef sysconf
# define sysconf(_SC_PAGESIZE) getpagesize ()
#endif

#include "c-stack.h"
#include "exitfail.h"

extern char *program_name;

#if HAVE_DECL_SIGALTSTACK || defined sigaltstack
# define ALTERNATE_STACK_SIZE SIGSTKSZ
# if ! HAVE_STACK_T
typedef struct sigaltstack stack_t;
# endif
#else
# define ALTERNATE_STACK_SIZE (2 * SIGSTKSZ)
typedef struct { void *ss_sp; size_t ss_size; int ss_flags; } stack_t;
static int
sigaltstack (stack_t const *ss, stack_t *oss)
{
  struct sigstack sigst;
  char *sp = ss->ss_sp;
  sigst.ss_sp = sp + SIGSTKSZ;
  sigst.ss_onstack = 1;
  return sigstack (&sigst, 0);
}
#endif


/* Storage for the alternate signal stack.  */

static union
{
  char buffer[ALTERNATE_STACK_SIZE];
  
  /* These other members are for proper alignment.  There's no
     standard way to guarantee stack alignment, but this seems enough
     in practice.  */
  long double ld;
  uintmax_t u;
  void *p;
} alternate_signal_stack;


/* Direction of the C runtime stack.  This function is
   async-signal-safe.  */

#if STACK_DIRECTION
# define find_stack_direction(ptr) STACK_DIRECTION
#else
static int
find_stack_direction (char const *addr)
{
  char dummy;
  return ! addr ? find_stack_direction (&dummy) : addr < &dummy ? 1 : -1;
}
#endif

/* The SIGSEGV handler.  */
static void (* volatile segv_action) (int, siginfo_t *, void *);

/* Translated messages for program errors and stack overflow.  Do not
   translate them in the signal handler, since gettext is not
   async-signal-safe.  */
static char const * volatile program_error_message;
static char const * volatile stack_overflow_message;

/* Output an error message, then exit with status EXIT_FAILURE if it
   appears to have been a stack overflow, or with a core dump
   otherwise.  This function is async-signal-safe.  */

void
c_stack_die (int signo, siginfo_t *info, void *context)
{
  char const *message =
    signo ? program_error_message : stack_overflow_message;
  write (STDERR_FILENO, program_name, strlen (program_name));
  write (STDERR_FILENO, ": ", 2);
  write (STDERR_FILENO, message, strlen (message));
  write (STDERR_FILENO, "\n", 1);
  if (! signo)
    _exit (exit_failure);
  if (context && info && 0 <= info->si_code)
    {
      /* Re-raise the exception at the same address.  */
      char *addr = info->si_addr;
      *addr = 0;
    }
  kill (getpid (), signo);
}

/* Handle a segmentation violation and exit.  This function is
   async-signal-safe.  */

#if HAVE_DECL_SIGACTION && SA_SIGINFO
static void
segv_handler (int signo, siginfo_t *info, void *context)
{
  if (! (SA_RESETHAND && SA_NODEFER))
    signal (signo, SIG_DFL);

  /* Clear SIGNO if it seems to have been a stack overflow.  */
#if HAVE_XSI_STACK_OVERFLOW_HEURISTIC
  if (0 < info->si_code)
    {
      /* If the faulting address is within the stack, or within one
	 page of the stack end, assume that it is a stack
	 overflow.  */
      ucontext_t const *user_context = context;
      char const *stack_min = user_context->uc_stack.ss_sp;
      size_t stack_size = user_context->uc_stack.ss_size;
      char const *faulting_address = info->si_addr;
      size_t s = faulting_address - stack_min;
      size_t page_size = sysconf (_SC_PAGESIZE);
      if (find_stack_direction (0) < 0)
	s += page_size;
      if (s < stack_size + page_size)
	signo = 0;
    }
#else
  /* SEGV-based stack overflow detection is not known to work, so
     assume that every segmentation violation is a stack overflow.  */
  signo = 0;
#endif

  segv_action (signo, info, context);
}
#else
static void
segv_handler (int signo)
{
  if (! (SA_RESETHAND && SA_NODEFER))
    signal (signo, SIG_DFL);

  segv_action (0, 0, 0);
}
#endif


/* Set up ACTION so that it is invoked on C stack overflow.  Return -1
   (setting errno) if this cannot be done.

   ACTION must invoke only async-signal-safe functions.  ACTION
   together with its callees must not require more than SIGSTKSZ bytes
   of stack space.  */

int
c_stack_action (void (*action) (int, siginfo_t *, void *))
{
  stack_t st;
  int r;
  
  st.ss_flags = 0;
  st.ss_sp = alternate_signal_stack.buffer;
  st.ss_size = sizeof alternate_signal_stack.buffer;
  r = sigaltstack (&st, 0);
  if (r != 0)
    return r;

  program_error_message = _("program error");
  stack_overflow_message = _("stack overflow");
  segv_action = action;

#if HAVE_DECL_SIGACTION
  {
    struct sigaction act;
    sigemptyset (&act.sa_mask);

    /* POSIX 1003.1-2001 says SA_RESETHAND implies SA_NODEFER, but
       this is not true on Solaris 8 at least.  It doesn't hurt to use
       SA_NODEFER here, so leave it in.  */
    act.sa_flags = SA_NODEFER | SA_ONSTACK | SA_RESETHAND | SA_SIGINFO;

#if SA_SIGINFO
    act.sa_sigaction = segv_handler;
#else
    act.sa_handler = segv_handler;
#endif

    return sigaction (SIGSEGV, &act, 0);
  }
#else
  return signal (SIGSEGV, segv_handler) == SIG_ERR ? -1 : 0;
#endif
}

#if DEBUG

#include <stdio.h>

int volatile exit_failure = 1;

static long
recurse (long frames, size_t framesize, char *p)
{
  if (! frames)
    return 0;
  else
    {
      char array[framesize];
      array[0] = 1;
      return *p + recurse (frames - 1, framesize, array);
    }
}

char *program_name;

int
main (int argc, char **argv)
{
  program_name = argv[0];
  if (argc != 3)
    {
      fprintf (stderr, "%s: usage: %s FRAMES FRAMESIZE\n",
	       program_name, program_name);
      return 1;
    }
  else
    {
      long frames = atol (argv[1]);
      long framesize = atol (argv[2]);
      c_stack_action (c_stack_die);
      return recurse (frames, framesize, "\1") != frames;
    }
}

#endif /* DEBUG */

/*
Local Variables:
compile-command: "gcc -D_GNU_SOURCE -DDEBUG -DHAVE_DECL_GETCONTEXT \
  -DHAVE_DECL_SIGACTION -DHAVE_DECL_SIGALTSTACK -DHAVE_INTTYPES_H \
  -DHAVE_LONG_DOUBLE -DHAVE_XSI_STACK_OVERFLOW_HEURISTIC \
  -DHAVE_SIGINFO_T -DHAVE_STACK_T -DHAVE_UCONTEXT_H -DHAVE_UINTPTR_T \
  -DHAVE_UNISTD_H \
  -Wall -W -g c-stack.c -o c-stack"
End:
*/
