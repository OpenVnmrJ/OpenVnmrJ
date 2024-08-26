/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifdef USE_RPC
/*
* acqinfo_proc.c: implementation of remote procedure for acqinfo_svc
*       There must be a file in /vnmr/acqqueue named acqinfo in the form:
*    pid hostname port# port# port#
*
*/
#include <sys/types.h>
#ifdef AIX
#include <sys/select.h>	/* IBM AIX OS needs it, but not present on SUN */
#endif
#include <stdio.h>
#include <errno.h>
#include <rpc/types.h>
#include <rpc/rpc.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include "acqinfo.h"

#define ERROR		-1
#define COMPLETE	0
#define MAXPATHL	128

#ifdef DEBUG
static debug = TRUE;
#else DEBUG
static debug = FALSE;
#endif DEBUG

static int	vpid;


acqdata *
acqinfo_get_2(debug)
int *debug;
{
   static acqdata info;
   static char AcqHost[50];
   static int  *pid_running;

   FILE *stream;
   char acqinfo_file[ MAXPATHL ];
   char *vnmrsystem;

   extern int *acqpid_ping_2();
   extern char *getenv();

    vnmrsystem = getenv( "vnmrsystem" );
    if (vnmrsystem == NULL) {
       strcpy( &acqinfo_file[ 0 ], "/vnmr" );
    }
    else {
       strcpy( &acqinfo_file[ 0 ], vnmrsystem );
    }
    strcat( &acqinfo_file[ 0 ], "/acqqueue/acqinfo" );

    if (stream = fopen( &acqinfo_file[ 0 ],"r"))
    {   
      if (fscanf(stream,"%d%s%d%d%d",&info.pid,AcqHost,&info.rdport,&info.wtport,
                &info.msgport) != 5)	/* did we read in the five values */
      {
        fclose(stream);
        if (*debug)
        {
	  fprintf(stderr,"Failure to read the five values in '/vnmr/acqqueue/acqinfo'\n");
	  fprintf(stderr,"Possible file corruption.\n");
        }
        AcqHost[0] = 0;
        info.host = AcqHost;
        info.pid = info.pid_active = info.rdport = info.wtport = info.msgport = 0;
      }
      else
      {
        info.host = AcqHost;
        pid_running = acqpid_ping_2(&info.pid);
        info.pid_active = *pid_running;
        if (*debug)
        {
          fprintf(stderr,"\n");
          fprintf(stderr,
           "acqinfo_svc(): Host  name: '%s'\n", info.host);
          fprintf(stderr,
           "acqinfo_svc(): Acq PID = %d, Active: %d, Read & Write Port = %d, %d,Msge Port: %d\n",
                info.pid,info.pid_active,info.rdport,info.wtport,info.msgport);
        }
        fclose(stream);
      }
    }
    else
    {
       perror("acqinfo_svc: '/vnmr/acqqueue/acqinfo' ");
       AcqHost[0] = 0;
       info.host = AcqHost;
       info.pid = info.pid_active = info.rdport = info.wtport = info.msgport = 0;
    }
   return(&info);
}

int *
acqpid_ping_2(acqpid)
int *acqpid;
{
   static int proc_live;

   proc_live = kill(*acqpid,0);           /*  check if Acqpid is active */
   if ((proc_live == -1) && (errno == ESRCH))
   {
      proc_live = 0;          /* No process with that pid */
      return(&proc_live);
   }
   else
   {
      proc_live = 1;          /* Process with that pid found */
      return(&proc_live);
   }
}


/*---------------------------------------
|                                       |
|             getugID()/3               |
|                                       |
|  Purloined from expfuncs.c in the     |
|  /common/sysacqproc directory.        |
|                                       |
+--------------------------------------*/
static int getugID(username, uid, gid)
char    *username;
int     *uid,
        *gid;
{
   struct passwd        *pswdptr,
                        *getpwnam();
 
 
   if ( (pswdptr = getpwnam(username)) == NULL )
      return(ERROR);
 
   *uid = pswdptr->pw_uid;
   *gid = pswdptr->pw_gid;
 
   return(COMPLETE);
}


/*---------------------------------------
|					|
|	     ft3d_start()/1		|
|					|
+--------------------------------------*/
int *ft3d_start_2(ft3dargs)
ft3ddata	*ft3dargs;
{
   int	status = COMPLETE;


#if defined AIX || defined IRIX
   vpid = fork();
#else
   vpid = vfork();
#endif

   if (vpid == 0)
   { /* child process */
      char		autoopt[5],
			procopt[5];
      int		gid,
			uid;
      long		r;
      extern void	exit();


      (void) strcpy(autoopt, "-a");
      (void) strcpy(procopt, "-p");

      if ( (r = sysconf(_POSIX_SAVED_IDS)) == ERROR )
      {
         if (debug)
            printf("Error in accessing system configuration\n");
         exit(ERROR);
      }
      else if (!r)
      {
         if (debug)
            printf("System configuration does not support chanig user ID\n");
         exit(ERROR);
      }

      if ( getugID(ft3dargs->username, &uid, &gid) )
      {
         if (debug)
         {
            printf("cannot get user/group ID for user `%s`\n",
			ft3dargs->username);
         }

         exit(ERROR);
      }

      if ( setgid(gid) < 0 )
      {
         if (debug)
            printf("cannot set group ID for user `%s`\n", ft3dargs->username);
         exit(ERROR);
      }

      if ( setuid(uid) < 0 )
      {
         if (debug)
            printf("cannot set user ID for user `%s`\n", ft3dargs->username);
         exit(ERROR);
      }

      if ( putenv(ft3dargs->pathenv) )
      {
         if (debug)
            printf("cannot reset `path` environmental variable\n");
         exit(ERROR);
      }

      if ( execlp("ft3d", "ftr3d", procopt, ft3dargs->procmode, autoopt,
		ft3dargs->autofilepath, NULL) )
      {
         if (debug)
            printf("`ft3d` could not be executed by `execlp`\n");
         exit(ERROR);
      }
   }

   return(&status);
}
#endif // USE_RPC
