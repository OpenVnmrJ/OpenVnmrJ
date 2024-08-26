/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifdef USE_RPC

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <rpc/types.h>
#include <rpc/rpc.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "acqinfo.h"


#define ERROR		-1
#define COMPLETE	0
#define MAXPATHL	128

static int	vpid;

extern int    Acqdebug;
extern int    consoleActive;

extern char   vnmrsystem[];
char 	      acqinfo_file[ MAXPATHL ];
u_long	      prog_num, prog_ver;


bool_t
xdr_a_string(xdrs, objp)
XDR            *xdrs;
a_string       *objp;
{
   if (!xdr_string(xdrs, objp, ~0))
   {
      return (FALSE);
   }
   return (TRUE);
}

bool_t
xdr_acqdata(xdrs, objp)
XDR            *xdrs;
acqdata        *objp;
{
   if (!xdr_int(xdrs, &objp->pid))
   {
      return (FALSE);
   }
   if (!xdr_int(xdrs, &objp->pid_active))
   {
      return (FALSE);
   }
   if (!xdr_int(xdrs, &objp->rdport))
   {
      return (FALSE);
   }
   if (!xdr_int(xdrs, &objp->wtport))
   {
      return (FALSE);
   }
   if (!xdr_int(xdrs, &objp->msgport))
   {
      return (FALSE);
   }
   if (!xdr_string(xdrs, &objp->host, ~0))
   {
      return (FALSE);
   }
   return (TRUE);
}

bool_t
xdr_ft3ddata(xdrs, objp)
XDR		*xdrs;
ft3ddata	*objp;
{
   if (!xdr_string(xdrs, &objp->autofilepath, ~0))
   {
      return (FALSE);
   }
   if (!xdr_string(xdrs, &objp->procmode, ~0))
   {
      return (FALSE);
   }
   if (!xdr_string(xdrs, &objp->username, ~0))
   {
      return (FALSE);
   }
   if (!xdr_string(xdrs, &objp->pathenv, ~0))
   {
      return (FALSE);
   }
   return (TRUE);
}

int *
acqpid_ping_2(acqpid)
int *acqpid;
{
   static int proc_live;

   proc_live = 1;
   if (Acqdebug)
       fprintf(stderr, "  acqpid_ping_2:  \n");
   return(&proc_live);
}


acqdata *
acqinfo_get_2(debug)
int *debug;
{
   static acqdata info;
   static char AcqHost[50];
   static int  *pid_running;

   FILE *stream;


    sprintf(acqinfo_file, "%s/acqqueue/acqinfo", vnmrsystem);

    if (Acqdebug)
	fprintf(stderr, " acqinfo_get_2: open file %s\n", acqinfo_file);
    if (stream = fopen( &acqinfo_file[ 0 ],"r"))
    {   
      if (fscanf(stream,"%d%s%d%d%d",&info.pid,AcqHost,&info.rdport,&info.wtport,
                &info.msgport) != 5)	/* did we read in the five values */
      {
        fclose(stream);
        if (Acqdebug)
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
        fclose(stream);
        if (Acqdebug)
        {
          fprintf(stderr,"\n");
          fprintf(stderr, "acqinfo_svc(): Host  name: '%s'\n", info.host);
          fprintf(stderr,
           "acqinfo_svc(): Acq PID = %d, Active: %d, Read & Write Port = %d, %d,Msge Port: %d\n",
                info.pid,info.pid_active,info.rdport,info.wtport,info.msgport);
        }
      }
    }
    else
    {
       if (Acqdebug)
          perror("acqinfo_svc: '/vnmr/acqqueue/acqinfo' ");
       AcqHost[0] = 0;
       info.host = AcqHost;
       info.pid = info.pid_active = info.rdport = info.wtport = info.msgport = 0;
    }
   return(&info);
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
         if (Acqdebug)
            printf("Error in accessing system configuration\n");
         exit(ERROR);
      }
      else if (!r)
      {
         if (Acqdebug)
            printf("System configuration does not support chanig user ID\n");
         exit(ERROR);
      }

      if ( getugID(ft3dargs->username, &uid, &gid) )
      {
         if (Acqdebug)
         {
            printf("cannot get user/group ID for user `%s`\n",
			ft3dargs->username);
         }

         exit(ERROR);
      }

      if ( setgid(gid) < 0 )
      {
         if (Acqdebug)
            printf("cannot set group ID for user `%s`\n", ft3dargs->username);
         exit(ERROR);
      }

      if ( setuid(uid) < 0 )
      {
         if (Acqdebug)
            printf("cannot set user ID for user `%s`\n", ft3dargs->username);
         exit(ERROR);
      }

      if ( putenv(ft3dargs->pathenv) )
      {
         if (Acqdebug)
            printf("cannot reset `path` environmental variable\n");
         exit(ERROR);
      }

      if ( execlp("ft3d", "ftr3d", procopt, ft3dargs->procmode, autoopt,
		ft3dargs->autofilepath, NULL) )
      {
         if (Acqdebug)
            printf("`ft3d` could not be executed by `execlp`\n");
         exit(ERROR);
      }
   }

   return(&status);
}

/*-------------------------------------------------------------
|   acqinfo_svc.c  - Server routines to transfer acqinfo structure
|		     data via RPC    SUN OS 3.5
|
|  9-12-89   Changed for SUN OS 4.0.3   Greg Brissey
|
+--------------------------------------------------------------*/
static void     acqinfoprog_2();


acqinfo_svc()
{
   int			 ival;
   sigset_t		 qmask;
   struct sigaction	 sigchld_vec;
   SVCXPRT        	*transp;
   FILE		 	*fport;


   /*
    * Unregister our server before registering it again.  This makes sure we
    * clear out any old versions
    */

    strcpy( &acqinfo_file[ 0 ], vnmrsystem);
    strcat( &acqinfo_file[ 0 ], "/acqqueue/acqinfo.port" );

    if (fport = fopen( &acqinfo_file[ 0 ],"r"))
    {
	if (fscanf(fport,"%d%d", &prog_num, &prog_ver) != 2)
	{
	    prog_num = ACQINFOPROG;
	    prog_ver = ACQINFOVERS;
	}
	fclose(fport);
    }
    else
    {
	prog_num = ACQINFOPROG;
	prog_ver = ACQINFOVERS;
    }
    if (Acqdebug)
	fprintf(stderr, " Acqinfo prog_num= %d  version = %d\n", prog_num, prog_ver);

   (void) pmap_unset(prog_num, prog_ver);

   /* Create a TCP socket for the server */
   transp = (SVCXPRT *)svctcp_create(RPC_ANYSOCK, 0, 0);
   if (transp == NULL)
   {
      (void) fprintf(stderr, "Error: cannot create tcp service.\n");
      exit(1);
   }

   /*
    * Register the server with the portmapper so that clients can find out
    * what our port number is
    */
   transp->xp_port = htons(transp->xp_port);
   if (!svc_register(transp, prog_num, prog_ver, acqinfoprog_2, IPPROTO_TCP))
   {
      (void) fprintf(stderr, "Error: unable to register (ACQINFOPROG, ACQINFOVERS, tcp).\n");
      exit(1);
   }

   /*
    * The portmapper table now has a program number, version, and protocol,
    * associated with a port number for our server. Now go to sleep until a
    * request comes in for this server.
    */

   svc_run();
   if (Acqdebug)
	    fprintf(stderr, "Infoproc: svc_run was terminated.\n");
   close_rpc();
   exit(1);
}


close_rpc()
{
    svc_unregister(prog_num, prog_ver);
}


/* Resume here, when a request comes in */
static void
acqinfoprog_2(rqstp, transp)
struct svc_req *rqstp;
SVCXPRT        *transp;
{
   union
   {
      int             acqinfo_get_2_arg;
      int             acqpid_ping_2_arg;
      ft3ddata	      ft3d_start_2_arg;
   }               argument;

   char           *result;
   char           *(*local) ();
   bool_t(*xdr_argument) (), (*xdr_result) ();

   /*
    * Setup routine to call to preform request & XDR routines to call to
    * handle arguments
    */
   if (Acqdebug)
	fprintf(stderr, " acqinfoprog_2...\n");
   switch (rqstp->rq_proc)
   {
      case NULLPROC:
   	 if (Acqdebug)
	    fprintf(stderr, "   NULLPROC\n");
	 (void) svc_sendreply(transp, xdr_void, (char *) NULL);
	 return;

      case ACQINFO_GET:
   	 if (Acqdebug)
	    fprintf(stderr, "   ACQINFO_GET\n");
	 xdr_argument = xdr_int;
	 xdr_result = xdr_acqdata;
	 local = (char *(*) ()) acqinfo_get_2;
	 break;

      case ACQPID_PING:
   	 if (Acqdebug)
	    fprintf(stderr, "   ACQPID_PING\n");
	 xdr_argument = xdr_int;
	 xdr_result = xdr_int;
	 local = (char *(*) ()) acqpid_ping_2;
	 break;

      case FT3D_START:
   	 if (Acqdebug)
	    fprintf(stderr, "   FT3D_START\n");
	 xdr_argument = xdr_ft3ddata;
	 xdr_result = xdr_int;
	 local = (char *(*) ()) ft3d_start_2;
	 break;

      default:
   	 if (Acqdebug)
	    fprintf(stderr, "   NON_CALL\n");
	 svcerr_noproc(transp);
	 return;
   }

   memset((char *) &argument, 0, sizeof(argument));

   /* Get the aproppriate arguments */
   if (!svc_getargs(transp, xdr_argument, (char *)&argument))
   {
      svcerr_decode(transp);
      return;
   }

   /* Call the routine requested with arguments  */
   result = (*local) (&argument, rqstp);

   /* Send the results back */
   if (Acqdebug)
	fprintf(stderr, "    reply\n");
   if (result != NULL && !svc_sendreply(transp, xdr_result, result))
   {
      svcerr_systemerr(transp);
   }

   /* free up internal structures, etc. */
   if (!svc_freeargs(transp, xdr_argument, (char *)&argument))
   {
      (void) fprintf(stderr, "unable to free arguments\n");
      exit(1);
   }
}

#endif // USE_RPC
