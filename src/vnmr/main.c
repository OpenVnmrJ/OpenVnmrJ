/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-------------------------------------------------------------------------
|
|     Main routine of master/vnmr
|
|	This module combines the master program and the 
|	vnmr program.  Depending on arg 0, it calls one
|	or the other.
|
|	The VMS version always calls vnmr and does not check
|	argv[ 0 ]
|
|	These two programs were combined to save the 
|	enormous space required by sunwindows (hog).
|
/+-----------------------------------------------------------------------*/

#include <stdio.h>
#include "vnmrsys.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

char         HostName[MAXPATH] = ""; 	/* internet host name */
char         systemdir[MAXPATHL] = "";  /* vnmr system directory */
char         userdir[MAXPATHL] = "";    /* user directory */
char         UserName[MAXPATH] = "";    /* login name of user */
pid_t        HostPid;
int          isMaster;

/* prototypes */
static void setupsys(void);
 
main(argc,argv)		int argc;	char *argv[];
{

   setupsys();
   userdir[ 0 ] = '\0';

#ifdef DBXTOOL
   if (strcmp(argv[1],"master") == 0)
   {  isMaster = 1;
      master(argc,argv);
   }
   else
   if (strcmp(argv[0],"Vnmr") == 0)
   {  isMaster = 0;
      vnmr(argc,argv);
   }
   else
      fprintf(stderr,"Main VNMR Program failure: arg0='%s'\n",
      argv[0]);
#else
#ifdef VMS
   isMaster = 0;
   vf_init();
   vnmr(argc,argv);
#else
   if (strcmp(argv[0],"master") == 0)
   {  isMaster = 1;
      master(argc,argv);
   }
   else
   if (strcmp(argv[0],"Vnmr") == 0)
   {  isMaster = 0;
      vnmr(argc,argv);
   }
   else
      fprintf(stderr,"Main VNMR Program failure: arg0='%s'\n",
      argv[0]);
#endif
#endif
}

/*  The following two routines establish the values of userdir, systemdir
    and host name for the current process.  The host name is obtained in
    the usual manner.  The system directory is obtained from the value of
    the environment parameter "setupdirs"				*/

static void setupsys()
{
   char *ptr;
#ifndef UNIX
   extern char *vms_getenv();
#endif

   /* get host name */

   gethostname(HostName,sizeof(HostName));
   HostPid = getpid();

   /* get user's login name */

   get_username(UserName,sizeof(UserName));

   /* get system name */

#ifdef UNIX
   ptr = getenv("vnmrsystem");
#else
   ptr = vms_getenv("vnmrsystem");
#endif
   if (ptr)
   {  if ( strlen(ptr) < MAXPATHL - 32)
        strcpy(systemdir,ptr);
      else
      {   printf("Error:value of environment variable 'vnmrsystem' too long\n");
          exit(1);
      }
   }
   else     /* use default */
   {
#ifdef VMS
      strcpy(systemdir,"[vnmr]");
#else
      strcpy(systemdir,"/vnmr");
#endif
   }
   setupBlocking();
}

#define  UNDEFINED	0
#define  MASTER 	1
#define  CHILD		2
#define  TERMINAL	3
#define  BACKGROUND	4

void setupdirs(char *cptr )
{
   char        *ptr;  
   int		mode_of_call;
#ifndef UNIX
   extern char *vms_getenv();
#endif

   mode_of_call = UNDEFINED;
   if (strcmp( "master", cptr ) == 0)
     mode_of_call = MASTER;
   else if (strcmp( "child", cptr ) == 0)
     mode_of_call = CHILD;
   else if (strcmp( "terminal", cptr ) == 0)
     mode_of_call = TERMINAL;
   else if (strcmp( "background", cptr ) == 0)
     mode_of_call = BACKGROUND;
   if (mode_of_call == UNDEFINED)
   {
      fprintf( stderr,
	  "programmer error, %s not valid argument to setupdirs\n", cptr
      );
      exit(1);
   }

   /* setup userdir */

   if (mode_of_call == MASTER || mode_of_call == CHILD ||
       mode_of_call == TERMINAL || userdir[ 0 ] == '\0')
   {
#ifdef UNIX
      ptr = getenv("vnmruser");
#else
      ptr = vms_getenv("vnmruser");
#endif
      if (ptr)
      {  if ( strlen(ptr) < MAXPATHL - 32)
         {
	    strcpy(userdir,ptr);
         }
         else
         {   printf("Error:value of environment variable 'vnmruser' too long\n");
             exit(1);
         }
      }
      else
      {  printf("Error: environment variable 'vnmruser' missing\n");
         exit(1);
      }
   }
}
