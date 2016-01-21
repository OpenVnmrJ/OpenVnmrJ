/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef AIX
#include <sys/select.h>	/* IBM AIX OS needs it, but not present on SUN */
#endif
#include <stdio.h>
#include <rpc/rpc.h>
#include "acqinfo.h"
/*-------------------------------------------------------------
|   acqinfo_svc.c  - Server routines to transfer acqinfo structure
|		     data via RPC    SUN OS 3.5
|
|  9-12-89   Changed for SUN OS 4.0.3   Greg Brissey
|
+--------------------------------------------------------------*/
static void     acqinfoprog_2();

static int
sigchld_irpt()
{
	int		cpid, cstatus;

    	while ((cpid = waitpid( -1, &cstatus, WNOHANG )) > 0)
	  ;
}

main()
{
   int			 ival;
   sigset_t		 qmask;
   struct sigaction	 sigchld_vec;
   SVCXPRT        	*transp;

   sigemptyset( &qmask );
   sigchld_vec.sa_handler = (void *) sigchld_irpt;
   sigchld_vec.sa_mask    = qmask;
   sigchld_vec.sa_flags   = 0;
   ival = sigaction( SIGCHLD, &sigchld_vec, 0 );
   if (ival < 0) {
	perror( "sigaction failed for SIGCHLD" );
	exit( 10 );
   }


   /*
    * Unregister our server before registering it again.  This makes sure we
    * clear out any old versions
    */

   (void) pmap_unset(ACQINFOPROG, ACQINFOVERS);

   /* Create a TCP socket for the server */
   transp = svctcp_create(RPC_ANYSOCK, 0, 0);
   if (transp == NULL)
   {
      (void) fprintf(stderr, "cannot create tcp service.\n");
      exit(1);
   }

   /*
    * Register the server with the portmapper so that clients can find out
    * what our port number is
    */
   if (!svc_register(transp, ACQINFOPROG, ACQINFOVERS, acqinfoprog_2, IPPROTO_TCP))
   {
      (void) fprintf(stderr, "unable to register (ACQINFOPROG, ACQINFOVERS, tcp).\n");
      exit(1);
   }

   /*
    * The portmapper table now has a program number, version, and protocol,
    * associated with a port number for our server. Now go to sleep until a
    * request comes in for this server.
    */

   svc_run();
   (void) fprintf(stderr, "svc_run returned\n");
   exit(1);
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
   switch (rqstp->rq_proc)
   {
      case NULLPROC:
	 (void) svc_sendreply(transp, xdr_void, (char *) NULL);
	 return;

      case ACQINFO_GET:
	 xdr_argument = xdr_int;
	 xdr_result = xdr_acqdata;
	 local = (char *(*) ()) acqinfo_get_2;
	 break;

      case ACQPID_PING:
	 xdr_argument = xdr_int;
	 xdr_result = xdr_int;
	 local = (char *(*) ()) acqpid_ping_2;
	 break;

      case FT3D_START:
	 xdr_argument = xdr_ft3ddata;
	 xdr_result = xdr_int;
	 local = (char *(*) ()) ft3d_start_2;
	 break;

      default:
	 svcerr_noproc(transp);
	 return;
   }

   memset((char *) &argument, 0, sizeof(argument));

   /* Get the aproppriate arguments */
   if (!svc_getargs(transp, xdr_argument, &argument))
   {
      svcerr_decode(transp);
      return;
   }

   /* Call the routine requested with arguments  */
   result = (*local) (&argument, rqstp);

   /* Send the results back */
   if (result != NULL && !svc_sendreply(transp, xdr_result, result))
   {
      svcerr_systemerr(transp);
   }

   /* free up internal structures, etc. */
   if (!svc_freeargs(transp, xdr_argument, &argument))
   {
      (void) fprintf(stderr, "unable to free arguments\n");
      exit(1);
   }
}
