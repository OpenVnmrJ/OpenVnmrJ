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
#include <stdlib.h>
#include <string.h>
#include "vnmrsys.h"

#include <unistd.h>

char         HostName[MAXPATH] = ""; 	/* internet host name */
char         systemdir[MAXPATHL] = "";  /* vnmr system directory */
char         userdir[MAXPATHL] = "";    /* user directory */
char         UserName[MAXPATH] = "";    /* login name of user */
pid_t        HostPid;
int          isMaster;
static void setupsys();
extern int get_username(char *username_addr, int username_len );
extern void master(int argc, char *argv[]);
extern int  vnmr(int argc, char *argv[]);
extern void setupBlocking();
 
int main(int argc, char *argv[])
{
   int   k;
/* FILE *fd; */

    /*fprintf(stderr,"%s starting.\n", argv[0]);*/

    /* Send error output to Console (instead of VnmrJ) */
    /*freopen("/dev/console", "w", stderr);*/

   setupsys();
   userdir[ 0 ] = '\0';

/*
fd = fopen("/tmp/argb", "w");
for (k = 0; k < argc; k++)
{
fprintf(fd, " arg %d: %s\n", k, argv[k]);
}
fclose(fd);
*/

  isMaster = 0;
#ifdef DBXTOOL
   if ((strcmp(argv[0],"Vnmrch")!=0) && (argc>1))
   {
      if (strcmp(argv[1],"master") == 0)
      {
         argv[1] = "Vnmrbg";
         isMaster = 1;
         master(argc,argv);
      }
      else
         fprintf(stderr,"Main VNMR Program failure: argc=%d arg0='%s'\n",
         argc,argv[0]);
   }
   else
   if (strcmp(argv[0],"Vnmrch") == 0)
   {
      isMaster = 0;
      vnmr(argc,argv);
   }
   else
      fprintf(stderr,"Main VNMR Program failure: argc=%d arg0='%s'\n",
      argc,argv[0]);
#else 
#ifdef VMS
   isMaster = 0;
   vf_init();
   vnmr(argc,argv);
#else 
   if ((strcmp(argv[0],"Vnmrch")!=0) && (argc>1))
   {
      for (k = 0; k < argc; k++)
      {
          if (strcmp("-mserver", argv[k]) == 0)
              isMaster = 1;
          else if (strcmp("master", argv[k]) == 0)
              isMaster = 1;
          if (isMaster)
              break;
      }
      if (strcmp(argv[1],"master") == 0)
      {
         argv[1] = "Vnmrbg";
         isMaster = 1;
/*
         master(argc,argv);
*/
      }

      if (isMaster) {
         master(argc,argv);
      }
      else if (argc>1)
      {
         isMaster = 0;
         vnmr(argc,argv);
      }
      else
      {
         fprintf(stderr,"Main VNMR Program failure: argc=%d arg0='%s'\n",
         argc,argv[0]);
      }
   }
   else if (strcmp(argv[0],"Vnmrch") == 0)
   {
      isMaster = 0;
      vnmr(argc,argv);
   }
   else
      fprintf(stderr,"Main VNMR Program failure: argc=%d arg0='%s'\n",
      argc,argv[0]);
#endif 
#endif 
   exit(EXIT_SUCCESS);
}

/*  The following two routines establish the values of userdir, systemdir
    and host name for the current process.  The host name is obtained in
    the usual manner.  The system directory is obtained from the value of
    the environment parameter "setupdirs"				*/

static void setupsys()
{
   char *ptr;

   /* get host name */

   gethostname(HostName,sizeof(HostName));
   HostPid = getpid();

   /* get user's login name */

   get_username(UserName,sizeof(UserName));

   /* get system name */

   ptr = getenv("vnmrsystem");
   if (ptr)
   {  if ( strlen(ptr) < MAXPATHL - 32)
        strcpy(systemdir,ptr);
      else
      {   fprintf(stderr,"Error:value of environment variable 'vnmrsystem' too long\n");
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
      ptr = getenv("vnmruser");
      if (ptr)
      {  if ( strlen(ptr) < MAXPATHL - 32)
         {
	    strcpy(userdir,ptr);
         }
         else
         {   fprintf(stderr,"Error:value of environment variable 'vnmruser' too long\n");
             exit(1);
         }
      }
      else
      {  fprintf(stderr,"Error: environment variable 'vnmruser' missing\n");
         exit(1);
      }
   }
}
