/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#ifdef AIX 
#include <sys/select.h>	 /* the IBM needs this but SUN does't have it */
#endif
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>

#include <rpc/types.h>
#include <rpc/rpc.h>
#include "acqinfo.h"

#define ERROR		-1
#define COMPLETE	0
#define MAXPATHL	128

#ifdef DEBUG
static int	debug = TRUE;
#else DEBUG
static int	debug = FALSE;
#endif DEBUG

extern void	Werrprintf();


/*---------------------------------------
|					|
|	       execIPC()/5		|
|					|
+--------------------------------------*/
static int execIPC(host3D, autofilepath, prcmode, user3D, userpathenv)
char	*host3D,
	*user3D,
	*autofilepath,
	*prcmode,
	*userpathenv;
{
   int		result;
   ft3ddata	ft3darg;


   ft3darg.autofilepath = autofilepath;
   ft3darg.procmode = prcmode;
   ft3darg.username = user3D;
   ft3darg.pathenv = userpathenv;

   if (host3D[0] != NULL)
   {
      if ( callrpctcp(host3D, ACQINFOPROG, ACQINFOVERS, FT3D_START,
		xdr_ft3ddata, &ft3darg, xdr_int, &result) )
      {
         return(ERROR);
      }

      return(result);
   }

   return(ERROR);
}


/*---------------------------------------
|					|
|	     callrpctcp()/8		|
|					|
+--------------------------------------*/
static int callrpctcp(host, prognum, versnum, procnum, inproc, in,
				outproc, out)
char		*host,
		*in,
		*out;
int		prognum,
		versnum,
		procnum;
xdrproc_t	inproc,
		outproc;
{
   int			socket = RPC_ANYSOCK;
   struct sockaddr_in	server_addr;
   struct hostent	*hp;
   struct timeval	total_timeout;
   enum clnt_stat	clnt_stat;
   CLIENT		*client = NULL;


   if ( (hp = gethostbyname(host)) == NULL )
   {
      if (debug)
      {
         Werrprintf("\ncallrpctcp():  cannot get host information for `%s`",
		host);
      }

      return(ERROR);
   }

   /*bcopy(hp->h_addr, (caddr_t) &server_addr.sin_addr, hp->h_length);*/
   memcpy( (caddr_t) &server_addr.sin_addr, hp->h_addr, hp->h_length);
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = 0;

   if ( (client = clnttcp_create(&server_addr, ACQINFOPROG, ACQINFOVERS,
                &socket, BUFSIZ,  BUFSIZ)) == NULL )
   {
      if (debug)
         Werrprintf("\ncallrpctcp():  cannot create RPC client `%s`", host); 
      return(ERROR);
   }

   total_timeout.tv_sec = 20;
   total_timeout.tv_usec = 0;

   clnt_stat = clnt_call(client, procnum, inproc, in, outproc, out,
				total_timeout);
   clnt_destroy(client);

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	     remote_ft3d()/3		|
|					|
+--------------------------------------*/
void remote_ft3d(ndatafiles, autofilepath, procmode)
char	*autofilepath,
	*procmode;
int	ndatafiles;
{
   char		prochostpath[MAXPATHL],
		hostname[MAXPATHL],
		tmppathenv[1024],
		tmpstr[1024],
 		*username,
		*pathenv;
   FILE		*fdes,
		*fopen();
   extern char	*cuserid(),
		*getenv();


   (void) strcpy(prochostpath, "/etc/hosts.3D");
   if ( (fdes = fopen(prochostpath, "r")) == NULL )
      return;

   if ( (username = cuserid(NULL)) == NULL )
   {
      Werrprintf("\nremote_ft3d():  cannot get username");
      return;
   }

   if ( (pathenv = getenv("PATH")) == NULL )
   {
      Werrprintf("\nremote_ft3d():  cannot get PATH environmental variable");
      return;
   }

   (void) strcpy(tmppathenv, "PATH=");
   (void) strcpy(tmpstr, pathenv);
   (void) strcat(tmppathenv, tmpstr);

   while ( ndatafiles && (fscanf(fdes, "%s\n", hostname) != EOF) )
   {
      if ( execIPC(hostname, autofilepath, procmode, username, tmppathenv)
		!= ERROR )
      {
         ndatafiles -= 1;
      }
   }

   (void) fclose(fdes);
}
