/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/file.h>

#include "lock3D.h"
#ifndef FT3D
#include "wjunk.h"
#endif 


#define MAX_LOCK_ITER	10
#define MAXPATHL	128
#define ERROR		-1
#define COMPLETE	0
#define FALSE		0
#define TRUE		1
#define	READ		0
#define WRITE		1


int removelock(char *filepath);

/*---------------------------------------
|                                       |
|         checkforanylocks()/2          |
|                                       |
+--------------------------------------*/
int checkforanylocks(char *datadirpath, int ndatafd)
{
   char	lockprimary[MAXPATHL*2],
	maindatafilepath[MAXPATHL],
	fext[32];
   int	i;


   (void) strcpy(maindatafilepath, datadirpath);
   (void) strcat(maindatafilepath, "/data3d");

   for (i = 0; i < ndatafd; i++)
   {
      (void) sprintf(fext, "_%1d", i+1);
      (void) strcpy(lockprimary, maindatafilepath);
      (void) strcat(lockprimary, fext);
      (void) strcat(lockprimary, ".lock1");

      if ( access(lockprimary, F_OK) == 0 )
         return(FOUND_LOCKFILE);
   }

   return(NO_LOCKFILES);
}


/*---------------------------------------
|                                       |
|            createlock()/2             |
|                                       |
+--------------------------------------*/
int createlock(char *filepath, int type)
{
   char	lockprimary[MAXPATHL],
	locksecondary[MAXPATHL],
	HostName[MAXPATHL];
   int	iter,
	lfd;
   FILE	*lkfptr,
	*fopen();


   (void) strcpy(lockprimary, filepath);
   (void) strcpy(locksecondary, filepath);
   (void) strcat(lockprimary, ".lock1");
   (void) strcat(locksecondary, ".lock2");


/********************************
*  Create secondary lock file.  *
********************************/

   for (iter = 0; iter < MAX_LOCK_ITER; iter++)
   {
      if ( (lfd = open(locksecondary, (O_CREAT|O_EXCL), 0666 )) >= 0 )
      {
         (void) close(lfd);
         iter = MAX_LOCK_ITER;
      }
      else
      {
         switch (type)
         {
            case INFO_LFILE:  sleep(1);
			      if ( (iter - 1) == MAX_LOCK_ITER )
			         return(ERROR);
			      break;
            case DATA_LFILE:  return(LOCKED_DFILE);
            default:	      return(ERROR);
         }
      }
   }


/*******************************
*   Create primary lock file.  *
*******************************/

   if ( access(lockprimary, F_OK) == 0 )
   {
      if ( removelock(filepath) )
      {
         unlink(locksecondary);
         switch (type)
         {
            case DATA_LFILE:  return(LOCKED_DFILE);
            case INFO_LFILE:
            default:          return(ERROR);
         }
      }
   }

   if ( (lkfptr = fopen(lockprimary, "w")) == NULL )
   {
      unlink(locksecondary);
      return(ERROR);
   }

   (void) gethostname(HostName, sizeof(HostName));
   (void) fprintf( lkfptr, "%s %d\n", HostName, getpid() );
   (void) fclose(lkfptr);


/********************************
*  Remove secondary lock file.  *
********************************/

   if ( unlink(locksecondary) )
   {
      unlink(lockprimary);
      return(ERROR);
   }

   return(COMPLETE);
}


/*---------------------------------------
|                                       |
|            removelock()/1             |
|                                       |
+--------------------------------------*/
int removelock(char *filepath)
{
   char	lockprimary[MAXPATHL],
	hostname[MAXPATHL],
	HostName[MAXPATHL];
   int	pid;
   FILE	*lkfptr,
	*fopen();


   (void) strcpy(lockprimary, filepath);
   (void) strcat(lockprimary, ".lock1");

   if ( (lkfptr = fopen(lockprimary, "r")) == NULL )
   {
#ifndef FT3D
      Werrprintf("cannot properly unlock `set3dproc` process\n");
#endif 
      return(ERROR);
   }

   if ( fscanf(lkfptr, "%s %d\n", hostname, &pid) == EOF )
   {
#ifndef FT3D
      Werrprintf("cannot properly unlock `set3dproc` process\n");
#endif 
      (void) fclose(lkfptr);
      return(ERROR);
   }

   (void) gethostname(HostName, sizeof(HostName));
   if ( ( strcmp(hostname, HostName) != 0 ) ||
	( pid != getpid() ) ) 
   {
#ifndef FT3D
      Werrprintf("cannot properly unlock `set3dproc` process\n");
#endif 
      (void) fclose(lkfptr);
      return(ERROR);
   }

   (void) fclose(lkfptr);

   if ( unlink(lockprimary) )
   {
#ifndef FT3D
      Werrprintf("cannot properly unlock `set3dproc` process\n");
#endif 
      return(ERROR);
   }
  
   return(COMPLETE);
}
