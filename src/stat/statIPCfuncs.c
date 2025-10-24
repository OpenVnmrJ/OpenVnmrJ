/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*------------------------------------------------------------------------
|	File contains the interprocess communications functions
+-----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#ifdef AIX 
#include <sys/select.h>
#endif
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>

#ifdef USE_RPC
#include <rpc/types.h>
#include <rpc/rpc.h>
#include "acqinfo.h"
#else
#include <signal.h>
#endif

#include "ACQPROC_strucs.h"
#include "statusextern.h"
extern void register_input_event(int fd);
int Ping_Pid();
int sendacq(int acqpid, char *message);

#define ERROR 1
#define MAXRETRY 8
#define SLEEP 4
#define REGISTERPORT 2
#define UNREGPORT 3
#define DISCONNECTSIZ 7		/* disconnect window size */
#define CONNECTSIZ 14		/* connect window size */
#define DATAGRAM 0		/* DataGram type socket  */
#define STREAM 1		/* Stream type socket  */

static int			statussocket = -1;
static struct sockaddr_in	statussockname,msgsockname;

#ifdef USE_RPC
static int  prog_num = -1;   /* rpc program number and version */
static int  prog_ver = -1;
#endif

static int  Acqpid;    /* acquisitions process ID number for async usage */
static int  Acqrdport; /* acquisition's read stream socket port */
static int  Acqwtport; /* acquisition's write stream socket port */
static int  Acqmsgport; /* acquisition's async message socket port */
static char AcqHost[HOSTLEN]; /* acquisition's machine name */
static char filepath[128];
static char Acqmessage[100];

#ifdef USE_RPC
static CLIENT *client = NULL;  /* RPC client handle */
#endif

int    newAcq = 0;

extern char RemoteHost[];

extern struct hostent	local_entry;

/*-------------------------------------------------------------------
|    Acqproc_ok()/1
|       determine if Acqproc is running
|	if remote host has been specified use RPC requests.
|       if local machine use Ping_Pid().
|
|     mod.   8/15/89  Greg Brissey for RPC usage.
+-------------------------------------------------------------------*/
int Acqproc_ok(char *remotehost)
{
#ifdef USE_RPC
    int active;
 
    if ( (strcmp(remotehost,LocalHost) != 0) && (remotehost[0] != 0) )
    {
      if ( callrpctcp(remotehost,
        prog_num, prog_ver, ACQPID_PING,
        xdr_int, &Acqpid, xdr_int, &active) != 0)
      {
         /* fprintf(stderr,"error: callrpc\n"); */
	 Acqpid = -1;
         return(0);
      }
      if (debug)
        fprintf(stderr,"RPC Pinging PID: %d, resulted in %d\n",Acqpid,active);
      return(active);
    }
    else
    {
       return(Ping_Pid());
    }
#else
   return(Ping_Pid());
#endif
}
/*-------------------------------------------------------------------
|    Ping_Pid()/0
|       determine if Acqproc is running on Local machine
|
+-------------------------------------------------------------------*/
int Ping_Pid()
{
    int ret;

    if (Acqpid < 0)
	return(0);
    ret = kill(Acqpid,0);           /*  check if Acqpid is active */
    if ((ret == -1) && (errno == ESRCH))
    {
       errno = 0;          /* reset errno */
       Acqpid = -1;
       return(0);
    }
    else
    {
       return(1);
    }
}
/*-------------------------------------------------------------------
|    initIPCinfo()/0
|       obtain system,user, acquisition process information
|
+-------------------------------------------------------------------*/
int initIPCinfo(char *remotehost)
{
    char *tmpptr;
    FILE *stream;
 
    newAcq = 0;
    Acqpid = -1;

#ifdef USE_RPC
    if ( (strcmp(remotehost,LocalHost) != 0) && (remotehost[0] != 0) )
    {
      return(getinfo(remotehost));
    }
    else
    {
#endif
      tmpptr = (char *)getenv("vnmrsystem");            /* vnmrsystem */
      if (tmpptr)
         strcpy(filepath,tmpptr);
      else
         strcpy(filepath,"/vnmr");
      strcat(filepath,"/acqqueue/acqinfo");
      if ( (stream = fopen(filepath,"r")) )
      {
       if (fscanf(stream,"%d%s%d%d%d",&Acqpid,AcqHost,&Acqrdport,&Acqwtport,
		&Acqmsgport) != 5)
       {
	  Acqpid = -1;
          fclose(stream);
          return(0);
       }
       else
       {
          if (debug)
	  {
	     fprintf(stderr,"Acq Hostname: '%s'\n",AcqHost);
             fprintf(stderr,"Acq PID = %d, Read Write Port = %d, %d,Msge %d\n",
        	Acqpid,Acqrdport,Acqwtport,Acqmsgport);
	  }
          fclose(stream);
#ifdef LINUX
          Acqmsgport = Acqmsgport & 0xFFFF;
#else
          Acqmsgport = 0xFFFF & ntohs(Acqmsgport);
#endif
          if (debug)
             fprintf(stderr," Using Infoproc Port %d\n",Acqmsgport);

	  if (Acqrdport == -9 && Acqwtport == -9)
                newAcq = 1;
          return(Ping_Pid());
       }
      }
      else
       return(0);
#ifdef USE_RPC
    }
#endif
}

#ifdef USE_RPC
/*-------------------------------------------------------------
|  getinfo()/1 - use RPC to regisitered server on remote host
|	         to obtain the IPC socket information
|
|		 Returns -  if the pid is running.
|			Author:  Greg Brissey 8/15/89
+-------------------------------------------------------------*/
getinfo(hostname)
char *hostname;
{
  static acqdata info;
  int  svc_debug;

  svc_debug = debug;
  if ( callrpctcp(hostname,
        prog_num, prog_ver, ACQINFO_GET,
        xdr_int, &svc_debug, xdr_acqdata, &info) != 0)
  {
     /*
     fprintf(stderr,"error: callrpctcp, cannot obtain acqinfo, aborting.\n");
     */
     Acqpid = -1;
     return(0);
  }
/*
  strcpy(AcqHost,info.host);
*/
  strcpy(AcqHost,hostname);
  Acqpid = info.pid;
  Acqrdport=info.rdport;
  Acqwtport=info.wtport;
  Acqmsgport=info.msgport;

  if (Acqrdport == -9 && Acqwtport == -9)
        newAcq = 1;
  else
        newAcq = 0;
  if (debug)
  {
    fprintf(stderr,"host: '%s'\n",AcqHost);
    fprintf(stderr,"Pid: %d, rdPort: %d, wtPort: %d, msgPort: %d\n",
        info.pid,info.rdport,info.wtport,info.msgport);
  }
  return(info.pid_active);
}

/*-------------------------------------------------------------
|  initrpctcp()/1 - initialize client handle for 
|		    Remote Procedure Call (RPC) using TCP protocal
|                       Author:  Greg Brissey 9/12/89
+-------------------------------------------------------------*/
initrpctcp(hostname)
char *hostname;
{
   char errmsg[256];
   struct sockaddr_in server_addr;
   int socket = RPC_ANYSOCK;
   enum clnt_stat clnt_stat;
   struct hostent *hp;
   char *tmpptr;
   FILE *stream;

   if ( (hp = gethostbyname(hostname)) == NULL)
   {
      sprintf(errmsg,"initrpctcp(): gethostbyname(`%s')",hostname);
      perror(errmsg);
      return(-1);
   }
   /*bcopy(hp->h_addr, (caddr_t) &server_addr.sin_addr,hp->h_length);*/
   memcpy( (caddr_t) &server_addr.sin_addr, hp->h_addr, hp->h_length);
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = 0;

   if (prog_num == -1)
   {
      tmpptr = (char *)getenv("vnmrsystem");            /* vnmrsystem */
      if (tmpptr)
         strcpy(filepath,tmpptr);
      else
         strcpy(filepath,"/vnmr");
      strcpy(filepath,tmpptr);
      strcat(filepath,"/acqqueue/acqinfo.port");
      if (stream = fopen(filepath,"r"))
      {
           if (fscanf(stream,"%d%d", &prog_num, &prog_ver) != 2)
           {
                prog_num = ACQINFOPROG;
                prog_ver = ACQINFOVERS;
           }
           fclose(stream);
      }
      else
      {
           prog_num = ACQINFOPROG;
           prog_ver = ACQINFOVERS;
      }
    }

   if ((client = clnttcp_create(&server_addr, prog_num, prog_ver,
                &socket,BUFSIZ, BUFSIZ)) == NULL)
   {
      sprintf(errmsg,"Remote Host: %s, acqinfo service request ",hostname);
      clnt_pcreateerror(errmsg);
      return(-1);
   }
   return(0);
}

/*-------------------------------------------------------------
|  killrpctcp()/1 - destroy client handle for 
|		    Remote Procedure Call (RPC) using TCP protocal
|                       Author:  Greg Brissey 9/12/89
+--------------------------------------------------------------*/
killrpctcp()
{
	if (client != NULL)
	{
	  clnt_destroy(client);
 	  client = NULL;
	}
}

/*-------------------------------------------------------------
|  callrpctcp()/8 - Remote Procedure Call (RPC) using TCP protocal
|                       Author:  Greg Brissey 8/15/89
| 9/12/89 - split this routine into 2 additional routines
|	    initrpctcp() & killrpctcp().    GMB
+-------------------------------------------------------------*/
callrpctcp(host,prognum,versnum,procnum,inproc,in,outproc,out)
char *host, *in, *out;
xdrproc_t inproc,outproc;
{
   char errmsg[256];
   enum clnt_stat clnt_stat;
   struct timeval total_timeout;

   if (client != NULL)
	killrpctcp();
   if (initrpctcp(host) == -1)
        return(-1);
   total_timeout.tv_sec = 20;
   total_timeout.tv_usec = 0;
   clnt_stat = clnt_call(client,procnum,inproc,in,outproc,out,total_timeout);
   if (clnt_stat  != RPC_SUCCESS)
   {
      sprintf(errmsg,"Remote Host: %s, acqinfo service request ",host);
      clnt_perror(client,errmsg);
      /* clnt_destroy(client); */
      return(-1);
   }
   return( (int) clnt_stat);
}

#endif    /* #ifdef USE_RPC */

/*-----------------------------------------------------------------------
|
|	initsocket()/0
|	Initialize the socket addresses for display and interactive use
|                
+-----------------------------------------------------------------------*/
void initsocket()    
{
    struct hostent *hp;

    /* --- name of the socket so that we may connect to them --- */
    /* read in acquisition host and port numbers to use */
    hp = gethostbyname(AcqHost);
    if (hp == NULL)
    {
       fprintf(stderr,"Acqproc():, Unknown Host");
       exit(1); 
    }
 
    memset((char *) &msgsockname,0,sizeof(msgsockname));  /* clear socket info */
 
    /*bcopy(hp->h_addr,(char *)&msgsockname.sin_addr,hp->h_length);*/
    memcpy((char *)&msgsockname.sin_addr, hp->h_addr, hp->h_length);
    msgsockname.sin_family = hp->h_addrtype;
    msgsockname.sin_port = 0xFFFF & (Acqmsgport);
    /* fprintf(stderr,"msgsockname.sin_port = %d\n",msgsockname.sin_port); */
}
/*--------------------------------------------------------------------
|
|	CreatSocket()/1
|	    create either a stream or datagram socket
|	    return the socket discriptor
|
+-------------------------------------------------------------------*/
int CreateSocket(int type)
{
#ifndef LINUX
    int on = 1;
#endif
    int sd;	/* socket descriptor */
    int flags;

    if (type == STREAM)
    {
        sd = socket(AF_INET,SOCK_STREAM,0);  /* create a stream socket */
    }
    else
    {
        sd = socket(AF_INET,SOCK_DGRAM,0);  /* create a datagram socket */
    }
    if (sd == -1)
    {
        perror("CreateSocket(): socket error");
        return(-1);
    }

    /* --- set up the socket options --- */
#ifndef LINUX
    setsockopt(sd,SOL_SOCKET,SO_USELOOPBACK,&on,sizeof(on));
#endif
    /*setsockopt(sd,SOL_SOCKET,(~SO_LINGER),&on,sizeof(on));*/

    /* --- We explicitly setup the descriptor as desired because ----*/
    /*     the notifier remembers the old settings for a file descriptor */
    /* --- even if it was closed, so we take no chances ---- */

    /* Special Note:  There is nothing inherent in UDP sockets (type != STREAM)     */
    /*                requiring them to be nonblocking.  The UDP socket will be     */
    /*                used to get the status block from the Acqproc.  The procedure */
    /*                is to get the latest stat block by continuing to read stat    */
    /*                blocks until there are no more.  A blocking socket would not  */
    /*                work here; it would eventually just block.  A read on a non-  */
    /*                blocking socket however will eventually return -1, with ERRNO */
    /*                set to EWOULDBLOCK.  At this point the program knows it has   */
    /*                the most current stat block from Acqproc. */

    if (type == STREAM)
    {
        if ((flags = fcntl(sd,F_GETFL,0)) == -1) /* get mode bits */
        {
             perror("CreateSocket():  fcntl error ");
             return(-1);
        }    
        flags &=  ~FNDELAY;
        if (fcntl(sd,F_SETFL,flags) == -1) /* set to blocking */
        {
             perror("CreateSocket(): fcntl error ");
             return(-1);
        }
    }
    else
    {
         if ( fcntl(sd,F_SETFL,FNDELAY) == -1)
         {
              perror("fcntl error");
              return(-1);
         }
    }

    return(sd);
}
 
/*----------------------------------------------------------------
|
|	readacqstatblock()/1
|       read datagram sock to which acqusition status is sent
|
|       This socket must be registerd with the acquisition process
|       for status updates to be sent. Upon process termination, the
|       acquisition process must be informed to remove this socket
|       from its update list.
|
|         if nothing to read nchr = -1
|         if zero chars sent then nchr = 0;
|         else nchr equal the number of bytes sent
|
+-----------------------------------------------------------------*/
int readacqstatblock(AcqStatBlock *statblock)
{
    int nchr;
    int bytes;

    if (debug)
        fprintf(stderr,"readacqstatblock(): \n");
    nchr = 1;
    bytes = -1;

/* Special Note:  statussocket is a UDP socket, set to nonblocking.  We want 
                  the latest stat block from Acqproc.  However there may be
                  a whole bunch of stat blocks waiting in the socket buffer.
                  So the program keeps reading stat blocks until the read
                  program returns -1, at which point we know there are no
                  more stat blocks.  The last one we read (assuming we did
                  read one - the program is set up for this error situation)
                  is the most current one.                                     */

    while(nchr > 0)
    {
        nchr = read(statussocket,statblock,sizeof(AcqStatBlock));
        if (debug)
            fprintf(stderr,"returned %d \n",nchr);
        if (nchr > 0 )
            bytes = nchr;
    }
    return(bytes);
}

/*---------------------------------------------------------------
|
|	acqregister()/0
|	register the datagram socket for acquisition status updates
|
+---------------------------------------------------------------*/
void acqregister()
{
    int localaddr;
    socklen_t namlen;
 
    statussockname.sin_family = AF_INET;
    statussockname.sin_addr.s_addr = INADDR_ANY;
    /*statussockname.sin_port = IPPORT_RESERVED + 15;*/
    statussockname.sin_port = 0;
    /* --- create the connectless socket for Acq status display --- */
    if (statussocket != -1)
       close(statussocket);
    statussocket = CreateSocket(DATAGRAM);

    if (statussocket == -1)
    {
        perror("Register(): error");
        exit(1);
    }

    if (bind(statussocket,(struct sockaddr *)&statussockname,
             sizeof(statussockname)) != 0)    {
        perror("Register(): bind error");
        exit(0);
    }
    namlen = sizeof(statussockname);
    getsockname(statussocket,(struct sockaddr *)&statussockname,&namlen);
    if (debug)
        fprintf(stderr,"Status Port Number: %d\n",
                0xFFFF & ntohs(statussockname.sin_port));
 
    if (local_entry.h_length > sizeof( int ))
    {
        fprintf( stderr, "Error: length of host address is %d, expected %zd\n",
			  local_entry.h_length, sizeof( int )
	);
	exit(0);
    }
    else
      memcpy( (caddr_t) &localaddr, local_entry.h_addr, local_entry.h_length);

    /* ---- register this status socket with the acquisition process --- */
    /* in this case do not ntohs() the port number */
    sprintf(Acqmessage,"%d,%s,%d,%d,%s,%d,%d,%d,%d,",
		REGISTERPORT,LocalHost,
                0xffff & (statussockname.sin_port),Procpid,User,Procpid,
		localaddr,local_entry.h_addrtype,local_entry.h_length);
    if (debug)
       fprintf(stderr,"sendacq: '%s'\n",Acqmessage);
    if (sendacq(Acqpid,Acqmessage))
        fprintf(stderr,"message unable to be sent\n");
    register_input_event(statussocket);
}
/*----------------------------------------------------------------
|
|	unregister()/0
|	unregister the datagram socket for acquisition status updates
|
+------------------------------------------------------------------*/
void unregister()

{
    char message[100];
 
    /* ---- register this status socket with the acquisition process --- */
    sprintf(message,"%d,%s,%d,%d,",UNREGPORT,LocalHost,statussockname.sin_port,
			Procpid);
    if (sendacq(Acqpid,message))
        fprintf(stderr,"message unable to be sent\n");
}

/*----------------------------------------------------------------
|
|	reregister()/0
|	reregister the datagram socket for acquisition status updates
|       must reregister otherwise Infoproc will assume this port exited.
|
+------------------------------------------------------------------*/
void reregister()

{
    /* ---- re-register this status socket with the acquisition process --- */
    /*      ignore any errors */
    sendacq(Acqpid,Acqmessage);
}

/*------------------------------------------------------------
|
|    sendacq()/2
|       connect to Acquisition's Async Process's Socket
|       then transmit a message to it and disconnect.
|
+-----------------------------------------------------------*/
int sendacq(int acqpid, char *message)
{
    int fgsd;   /* socket discriptor */
    int result;
    int i;
    int on = 1;
 
    if (acqpid < 0)
        return(0);

    /* --- try several times then fail --- */
    for (i=0; i < MAXRETRY; i++)
    {
        fgsd = socket(AF_INET,SOCK_STREAM,0);        /* create a socket */

        if (fgsd == -1)
        {
            perror("sendacq(): socket");
            fprintf(stderr,"sendacq(): Error, could not create socket\n");
            exit(0);
        }
        if (debug)
            fprintf(stderr,
		"sendacq(): socket create for async trans %d\n",fgsd);
#ifndef LINUX
        setsockopt(fgsd,SOL_SOCKET,SO_USELOOPBACK,&on,sizeof(on));
#endif
        /* fprintf(stderr,"sendacq: SO_LINGER OPTIONS\n"); option doesn't exist on LINUX */
        setsockopt(fgsd,SOL_SOCKET,(~SO_LINGER),&on,sizeof(on));
 
        /*if (ioctl(fgsd,SIOCSPGRP,&acqpid) == -1)
        {
            perror("sendacq(): ioctl error");
            return(-1);
        }*/  

        if (debug)
	{
          fprintf(stderr,"sendacq(): socket process group: %d\n",acqpid);
          fprintf(stderr,"sendacq(): socket created fd=%d\n",fgsd);
	}
 
       /* --- attempt to connect to the named socket --- */
       if ((result = connect(fgsd,(struct sockaddr *) &msgsockname,
                      sizeof(msgsockname))) != 0)
       {
          /* --- Is Socket queue full ? --- */
          if (errno != ECONNREFUSED && errno != ETIMEDOUT)
          {                             /* NO, some other error */
              if (debug)
                 fprintf(stderr,"sendacq(): errno: %d ECONN %d\n",
			errno,ECONNREFUSED);
              shutdown(fgsd,2);
              close(fgsd);
              return(-1);
          }
       }                                /* Yes, try MAXRETRY times */
       else     /* connection established */
       {
            break;
       }
       fprintf(stderr,"sendacq(): Socket queue full, will retry %d\n",i);
       close(fgsd);
       sleep(SLEEP);
    }
    if (result != 0)    /* tried MAXRETRY without success  */
    {
       if (debug)
          fprintf(stderr,"Sendacq(): Max trys Exceeded, aborting send\n");
       shutdown(fgsd,2);
       close(fgsd);
       return(-1);
    }
    if (debug)
        fprintf(stderr,"Sendacq(): Connection Established \n");
    result = write(fgsd,message,strlen(message));

    shutdown(fgsd,2);
    close(fgsd);
    return(0);
}
/*-------------------------------------------------------------
|  quit/0  terminate process
|
+------------------------------------------------------------*/
void exitproc()
{
    if (Acqproc_ok(RemoteHost))
       unregister();       /* inform acquisition status port closed */
    exit(0);
}

