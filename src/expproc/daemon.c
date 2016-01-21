/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
DESCRIPTION
  This routine performs the necessary tasks to make a process into a 
well behaved daemon.
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <errno.h>

#ifndef NOFILE
#define NOFILE 256
#endif

/* For now define DEBUG */
#ifndef DEBUG
#define DEBUG
#endif


/**************************************************************
*
*  makeItaDaemon - Makes a daemon out of the calling process
*
*  This routine performs the necessary tasks to make a process into a 
*well behaved daemon.
*
* RETURNS:
* void 
*
*       Author Greg Brissey 6/24/94
*/
void makeItaDaemon(int ignoreSigChld)
/* int ignoreSigChld - if > 0 then process can ignore children deaths */
{
   int childpid, fd, strtfd;
   struct sigaction    intserv;
   sigset_t            qmask;
   void sig_child();

   strtfd = 0;	/* fd to start when closing file descriptors */


   /* if started by init via /etc/inittab then skip the detaching
      stuff, this test however maybe unreliable */
   if (getppid() == 1)
     goto skipstuff;

   /* Ignore Terminal Signals */
    sigemptyset( &qmask );
    intserv.sa_handler = SIG_IGN;
    intserv.sa_mask = qmask;
    intserv.sa_flags = 0;

#ifdef SIGTSTP
    sigaction( SIGTSTP, &intserv, NULL );
#endif
#ifdef SIGTTOU
    sigaction( SIGTTOU, &intserv, NULL );
#endif
#ifdef SIGTTIN
    sigaction( SIGTTIN, &intserv, NULL );
#endif

   /*
	if not started in the background then fork and let parent exit.
	This guarantees the 1st child is not a process group
	leader.
   */
   if ( (childpid = fork()) < 0)
	perror("can't fork 1st child");
   else
	if (childpid > 0)
	   exit(0);	/* parent exits */

   /*
      1st child can now disassociate from controlling terminal and
      process group. Ensure the process can't reacquire a new controlling
      terminal.
   */

#ifdef XXX
   /* the Berkeley Way  (BSD)*/
   /* this is not used anymore */

   if (setpgrp(0, getpid()) == -1)
	perror("can't change process group");

   /* remove controlling terminal  BSD style */
   if ( (fd = open("dev/tty", O_RDWR)) >= 0)
   {
      ioctl(fd, TIOCNOTTY, (char*)NULL);
      close(fd);
   }
#endif

   /* The AT & T Way (System V) */

   if (setpgid(0,0) == -1)
        perror("can't change process group");

   signal(SIGHUP, SIG_IGN);  /* ignore process group leaders death */

   if ( (childpid = fork()) < 0)
        perror("can't fork 2nd child");
   else
        if (childpid > 0)
           exit(0);     /* 1st child exits */


  skipstuff:

#ifdef DEBUG
   /* 
      For Debugging redirect stdin, stdout, & stderr descriptors to
      the console. Otherwise close them.
   */
   freopen("/dev/null","r",stdin);
   freopen("/dev/console","a",stdout);
   freopen("/dev/console","a",stderr);
   strtfd = 3;
#endif

   /* Now close the rest of the file descriptors */
   for (fd=strtfd; fd < NOFILE; fd++)
     close(fd);

   errno = 0;	/* clear error from any above close() calls */
   /* Move current directory to root, avoid mounted FileSystems */
   chdir("/");

   /* Clear the file mode creation mask */
   umask(000);

   /* Ignore Children's Deaths */
   if (ignoreSigChld)
   {
       sigemptyset( &qmask );
       intserv.sa_handler = SIG_IGN;
       intserv.sa_mask = qmask;
       intserv.sa_flags = 0;

       sigaction( SIGCHLD, &intserv, NULL );
   }
}

/*
sig_child()
{
#ifdef  SIGTSTP
    int pid;
    union wait status;

    while( (pid = wait3(&status, WNOHANG, (struct rusage *) 0)) > 0)
	;
#endif
}
*/
