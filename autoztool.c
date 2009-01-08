/* $Header: /cvs/autoztool/autoztool.c,v 1.5 2003-08-18 22:23:59 richard Exp $ */
/* 
 *  This file is part of autoztool
 *  Copyright (C) 2001, 2003 Richard Kettlewell
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 *  USA
 */ 

#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>

#ifndef PATH_MAX
# define PATH_MAX 4096
#endif

static const struct {
  const char *ext;
  const char *prog;
} tab[] = {
  {".Z", "gunzip"},
  {".gz", "gunzip"},
  {".bz", "bunzip2"},			/* hoping this works... */
  {".bz2", "bunzip2"},
};

#define NTAB (int)(sizeof tab / sizeof *tab)

static int wrapped_open(const char *path, int flags, mode_t mode,
			int (*real_open)(const char *path, int flags, ...)) {
  int n;
  size_t l;
  char tmp[PATH_MAX];
  int readfd = -1, tmpfd = -1, retfd = -1, nullfd = -1;
  const char *tmpdir;
  sigset_t old, allsigs;
  pid_t pid, r;
  int save;
  int w;

  /* we assume that O_RDONLY, O_WRONLY, O_RDWR occupy the bottom two
     bits only. */
  if((flags & 3) != O_RDONLY)
    return (*real_open)(path, flags, mode);
  /* see if the file extension is that of a compressed file */
  l = strlen(path);
  for(n = 0; n < NTAB; ++n) {
    size_t e = strlen(tab[n].ext);
    if(e <= l
       && !strcmp(path + l - e, tab[n].ext))
      break;
  }
  if(n >= NTAB)
    return (*real_open)(path, flags, mode);
  tmp[0] = 0;			/* suppress unlink */

  /* open() is documented as using the lowest currently-unused FD, so
     we occupy that FD number until we're ready to open the file we'll
     eventually return.  An alternative would be to delay the open
     until the end and close everything else first, but that means we
     can't unlink the file early and thus increases the chance of a
     dropping in /tmp.  (We can't completely prevent droppings,
     consider e.g. SIGKILL during or just after mkstemp, but we can
     minimize their likelihood.) */

  /* occupy the first free slot in the FD table */
  if((nullfd = (*real_open)("/dev/null", O_RDONLY, 0)) < 0)
    goto error;
  
  /* open the real file - we could do this in the child, but then we
     couldn't report errors as easily */
  if((readfd = (*real_open)(path, flags, mode)) < 0)
    goto error;
  if(!(tmpdir = getenv("TMPDIR")))
    tmpdir = "/tmp";
  snprintf(tmp, sizeof tmp, "%s/zXXXXXX", tmpdir);

  /* create the temporary file */
  if((tmpfd = mkstemp(tmp)) < 0)
    goto error;

  /* free up the FD table slot */
  if(close(nullfd) < 0)
    goto error;
  nullfd = -1;

  /* re-open the temporary file, using the free slot in the FD table */
  if((retfd = (*real_open)(tmp, flags, mode)) < 0)
    goto error;

  /* now we've opened retfd, we can unlink the temporary file */
  unlink(tmp);
  tmp[0] = 0;			/* suppress unlink */

  /* Block all signals for the duration.  Signals *must* be blocked in
     the fork; otherwise an application-installed handler may be
     invoked in the child process, which it is unlikely to be able to
     cope with gracefully.

     After the fork it it would probably be safe to scale back to just
     SIGCHLD blocked (and indeed I check for EINTR on waitpid to
     support doing this).

     SIGCHLD remaining blocked is non-negotiable however; if the
     application has installed a SIGCHLD handler then it may call a
     wait() function, which would prevent us from picking up the
     subprocess exit status, which would prevent us from being able to
     distinguish between subprocess success and subprocess failure. */
  sigfillset(&allsigs);
  if(sigprocmask(SIG_BLOCK, (const sigset_t *)&allsigs, &old) < 0)
      goto error;

  switch(pid = fork()) {
  case 0: {
    int s;

    /* any non-ignored signals with non-default handlers must be reset
       before we restore the signal mask */
    for(s = 1; s < _NSIG; ++s) {
      struct sigaction old;
      
      sigaction(s, 0, &old);
      if(old.sa_handler != SIG_DFL
	 && old.sa_handler != SIG_IGN
	 && signal(s, SIG_DFL) == SIG_ERR) {
	perror("signal");
	_exit(-1);
      }
    }

    /* XXX actually we just want to remove ourselves from LD_PRELOAD,
       preferrably regardless of what name we were called under */
    unsetenv("LD_PRELOAD");

    /* dup the FDs into place */
    if(dup2(readfd, 0) < 0
       || dup2(tmpfd, 1) < 0) {
      perror("dup2");
      _exit(-1);
    }

    /* restore the signal mask */
    if(sigprocmask(SIG_SETMASK, (const sigset_t *)&old, 0) < 0) {
      perror("sigprocmask");
      _exit(-1);
    }

    /* execute the decompressor */
    execlp(tab[n].prog, tab[n].prog, (const char *)0);
    perror("execlp");
    _exit(-1);
  }
  case -1:
    save = errno;
    /* if resetting the signal mask fails then we can't guarantee what
       the signal mask is on return, and this could confuse the caller
       - if it depends on signals to drive itself, it could hang, for
       example.  We "solve" this problem by crashing. */
    if(sigprocmask(SIG_SETMASK, (const sigset_t *)&old, 0) < 0) {
      perror("sigprocmask");
      abort();
    }
    errno = save;
    goto error;
  }

  /* close files we don't need any more */
  close(readfd);
  readfd = -1;
  close(tmpfd);
  tmpfd = -1;

  /* wait for the decompressor to finish */
  while((r = waitpid(pid, &w, 0)) < 0 && errno == EINTR)
    ;
  save = errno;
  /* see above regarding failing to reset the signal mask */
  if(sigprocmask(SIG_SETMASK, (const sigset_t *)&old, 0) < 0) {
    perror("sigprocmask");
    abort();
  }
  /* We may take a spurious SIGCHLD at this point.  The application
     had better do something sensible with it. */
  if(r < 0 || w != 0) {
    close(retfd);
    if(r >= 0) {
      fprintf(stderr, "%s exited with wstat %#x\n", tab[n].prog, (unsigned)w);
      errno = EIO;		/* child failed */
    } else
      errno = save;
    return -1;
  }
  return retfd;
error:
  save = errno;
  /* tidy up */
  if(tmp[0])
    unlink(tmp);
  if(nullfd >= 0)
    close(nullfd);
  if(retfd >= 0)
    close(retfd);
  if(readfd >= 0)
    close(readfd);
  if(tmpfd >= 0)
    close(tmpfd);
  errno = save;
  return -1;
}

int __autoztool_open(const char *path, int flags, ...) {
  static int depth;
  static int (*real_open)(const char *path, int flags, ...);

 int n;
  mode_t mode;
  va_list ap;

  if(!real_open)
    real_open = dlsym(RTLD_NEXT, "__open");
  if(flags & O_CREAT) {
    va_start(ap, flags);
    mode = va_arg(ap, mode_t);
    va_end(ap);
  } else
    mode = 0;
  depth++;
  if(depth > 1)
    n = (*real_open)(path, flags, mode);
  else
    n = wrapped_open(path, flags, mode, real_open);
  depth--;
  return n;
}

int open(const char *path, int flags, ...)
  __attribute__((weak, alias("__autoztool_open")));

int __open(const char *path, int flags, ...)
  __attribute__((weak, alias("__autoztool_open")));

int open64(const char *path, int flags, ...)
  __attribute__((weak, alias("__autoztool_open")));

int __open64(const char *path, int flags, ...)
  __attribute__((weak, alias("__autoztool_open")));

/*
Local Variables:
mode:c
comment-column:40
c-basic-offset:2
End:
*/
