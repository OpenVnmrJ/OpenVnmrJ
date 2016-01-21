/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
*/

/* program to fork & execute a shell script, for the passwd file entry 'acqproc' */
#include <stdio.h>
main(argc,argv)
int argc;
char *argv[];
{
   /*fprintf(stderr,"execl('/bin/sh','-c','/vnmr/bin/execkillacqproc')\n"); */
   if (execl("/bin/sh","-c","/vnmr/bin/execkillacqproc",NULL) == -1)
   {
	perror("startmekillme: execl Error");
   }
   exit(0);
}
