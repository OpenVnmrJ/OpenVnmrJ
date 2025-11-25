/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*---------------------------------------------------------------------------
|	comm.c
|
|	This module contains procedures that initialize communication
|	protocols to the acquisition system.  Two schemes are
|	implemented: sockets and message queues
|       This file and socket.c  and socket1.c form a communication library
|
+---------------------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include "comm.h"
#include "acquisition.h"
#include "errLogLib.h"
#ifdef __INTERIX
typedef int socklen_t;
#endif

extern int initExpStatus(int clean);
extern int getStatLkLevel();
extern int getStatGradError();
extern int getStatAcqState();
extern int getStatVTAct();
extern int getStatVTSet();
extern int getStatAcqTickCountError();
extern int getStatSpinAct();
extern int getStatSpinSet();
extern int getStatGoFlag();
extern int getStatExpName(char *expname,int maxsize);
extern int getStatUserId(char *usrname,int maxsize);
extern int getStatAcqSample();
extern int getStatLSDV();
extern int getStatConsoleID();
extern void expStatusRelease();
extern int getStatRemainingTime();
extern int getStatInQue();

#undef DEBUG

#ifdef  DEBUG
#define TPRINT0(str) \
	fprintf(stderr,str)
#define TPRINT1(str, arg1) \
	fprintf(stderr,str,arg1)
#define TPRINT2(str, arg1, arg2) \
	fprintf(stderr,str,arg1,arg2)
#define TPRINT3(str, arg1, arg2, arg3) \
	fprintf(stderr,str,arg1,arg2,arg3)
#else
#define TPRINT0(str) 
#define TPRINT1(str, arg2) 
#define TPRINT2(str, arg1, arg2) 
#define TPRINT3(str, arg1, arg2, arg3) 
#endif

#define MAXRETRY   7
#define SLEEP      1
#define TRUE     (1 == 1)
#define FALSE    (0 == 1)

#ifdef WINBRIDGE
#define EXP_SOCKET 0
#define INFO_SOCKET 1
#endif


int acq_errno = 0;

#ifdef WINBRIDGE
COMM_INFO_STRUCT comm_addr[5];
#else
COMM_INFO_STRUCT comm_addr[4];
#endif

char acqID[64];


/*-----------------------------------------------------------------------
|
|       is_newacq()/0
|       tell the application (PSG) whether this is INOVA or UnityPLUS
|
+-----------------------------------------------------------------------*/
int
is_newacq()
{
	int	retval;
#ifdef NESSIE
	retval = 1;
#else
	retval = 0;
#endif
	return( retval );
}

/*-----------------------------------------------------------------------
|
|       InitRecverAddr()/2
|       Initialize the socket addresses for display and interactive use
|
+-----------------------------------------------------------------------*/
int
InitRecverAddr(char *host, int port, struct sockaddr_in *recver)
{
    struct hostent *hp;

    /* --- name of the socket so that we may connect to them --- */
    /* read in host and port numbers to use */
    hp = gethostbyname(host);
    if (hp == NULL)
    {
       errLogSysRet(ErrLogOp,debugInfo,"%s Unknown Host",host);
       return(RET_ERROR);
    }
 
    memset((char *) recver,0,sizeof(struct sockaddr_in));	/* clear socket info */
 
    memcpy( (char *)&(recver->sin_addr), hp->h_addr, hp->h_length);
    recver->sin_family = hp->h_addrtype;
    recver->sin_port = port;
    return(RET_OK);
}

#ifdef NESSIE

int SendAsyncInova(CommPort to_addr, CommPort from_addr, char *msg)
{
	int	fgsd;   /* socket discriptor */
	int	result;
	int	iter;
	int ret __attribute__((unused));
#ifndef LINUX
	int	one = 1;
#endif
        int     msgsize;

        (void) from_addr;
	if (to_addr->port == -1 || to_addr->pid == -1)
	  return (RET_ERROR);		/* then we do not have a socket */

    /* --- try several times then fail --- */

	for (iter=0; iter < MAXRETRY; iter++) {
		fgsd = socket(AF_INET,SOCK_STREAM,0);	/* create a socket */

		if (fgsd == -1) {
			errLogSysRet(ErrLogOp,debugInfo,"Had trouble creating socket.");
			return(RET_ERROR);
		}
		TPRINT1("SendAsyncInova(): socket create for async trans %d\n",fgsd);
#ifndef LINUX
		setsockopt(fgsd,SOL_SOCKET,SO_USELOOPBACK,(const void*)&one,(size_t)sizeof(one));
#endif

       /* --- attempt to connect to the named socket --- */         
       /* retry if system interrupted */

                while ( ((result = connect(fgsd,(struct sockaddr*) &(to_addr->messname),
                                   sizeof(to_addr->messname))) != 0)  && (errno == EINTR) )
                   ;
                if (result != 0)
                {

		    /* --- Is Socket queue full ? --- */

			if (errno != ECONNREFUSED && errno != ETIMEDOUT) {

			    /* NO, some other error */

				errLogSysRet(ErrLogOp,debugInfo,"Connect error, message not sent");
				close(fgsd);
				return(RET_ERROR);
			}
		}                                /* Yes, try MAXRETRY times */
		else     /* connection established */
		  break;

		TPRINT1("SendAsyncInova(): Socket queue full, will retry %d\n",iter);
		close(fgsd);
		sleep(SLEEP);
	}
	if (result != 0) {   /* tried MAXRETRY without success  */
		errLogRet(ErrLogOp,debugInfo,"SendAsyncInova(): Max trys Exceeded, aborting send\n");
		errLogRet(ErrLogOp,debugInfo,"Problems sending message");
		return(RET_ERROR);
	}
	TPRINT0("SendAsyncInova(): Connection Established \n");

        
	msgsize = strlen(msg) + 1;	/* + 1  include null character */

	TPRINT2("SendAsyncInova(): msgsize: %d, msge: '%s'\n",msgsize,msg);
	/* TPRINT1("SendAsyncInova(): sizeof msgsize: %d, \n",sizeof(msgsize)); */
	/* TPRINT3("SendAsyncInova(): write(%d,0x%lx,%d) \n",fgsd,&msgsize,sizeof(msgsize)); */

	/* send message length */
        ret = write(fgsd,(char*) &msgsize,sizeof(msgsize));
       
	/* send message */
	ret = write(fgsd,msg,msgsize);
	/* TPRINT1("Message Sent: %s\n",msg); */

	close(fgsd);
	return(RET_OK);
}

/* made this since SendAsyncInova was also being used to communicate to Vnmr
   this fails since we changed Expproc communication scheme but not Vnmr's */
int SendAsyncInovaVnmr(CommPort to_addr, CommPort from_addr, char *msg)
{
	int	fgsd;   /* socket discriptor */
	int ret __attribute__((unused));
#ifndef LINUX
	int	one = 1;
#endif
        int     msgsize;

        (void) from_addr;
	if (to_addr->port == -1 || to_addr->pid == -1)
	  return (RET_ERROR);		/* then we do not have a socket */

    /* --- try several times then fail --- */

		fgsd = socket(AF_INET,SOCK_STREAM,0);	/* create a socket */

		if (fgsd == -1) {
			errLogSysRet(ErrLogOp,debugInfo,"Had trouble creating socket.");
			return(RET_ERROR);
		}
		TPRINT1("SendAsyncInovaVnmr(): socket create for async trans %d\n",fgsd);
#ifndef LINUX
		setsockopt(fgsd,SOL_SOCKET,SO_USELOOPBACK,(const void*)&one,(size_t)sizeof(one));
#endif

       /* --- attempt to connect to the named socket --- */         

		while ( (connect(fgsd,(struct sockaddr*) &(to_addr->messname),
					sizeof(to_addr->messname)) == -1)  &&
                        (errno != EISCONN) )
                {

			if (errno != EINTR)
                        {
			    /* NO, some other error */
				close(fgsd);
				return(RET_ERROR);
			}
		}

	TPRINT0("SendAsyncInovaVnmr(): Connection Established \n");

        
	msgsize = strlen(msg) + 1;	/* + 1  include null character */

	TPRINT2("SendAsyncInovaVnmr(): msgsize: %d, msge: '%s'\n",msgsize,msg);

	/* send message */
	ret = write(fgsd,msg,msgsize);

	close(fgsd);
	return(RET_OK);
}

#else

/*------------------------------------------------------------
|
|    SendAsync()/4
|       connect to  an Async Process's Socket
|       then transmit a message to it and disconnect.
|
+-----------------------------------------------------------*/
int SendAsync(CommPort to_addr, CommPort from_addr, int cmd, char *args)
{
    int fgsd;   /* socket discriptor */
    int result;
    int i;
    int on = 1;
    char message[256];


    if (to_addr->port == -1 || to_addr->pid == -1) /* then we do not have a socket */
       return (RET_ERROR);
    /* --- try several times then fail --- */
    for (i=0; i < MAXRETRY; i++)
    {
        fgsd = socket(AF_INET,SOCK_STREAM,0);        /* create a socket */

        if (fgsd == -1)
        {
            errLogSysRet(ErrLogOp,debugInfo,"Had trouble creating socket.");
            return(RET_ERROR);
        }
	TPRINT1("SendAsync(): socket create for async trans %d\n",fgsd);
#ifndef LINUX
        setsockopt(fgsd,SOL_SOCKET,SO_USELOOPBACK,&on,sizeof(on));
#endif

       /* --- attempt to connect to the named socket --- */         
       if ((result = connect(fgsd, (struct sockaddr *) &(to_addr->messname),
                     sizeof(to_addr->messname))) != 0)
	{
	    /* --- Is Socket queue full ? --- */
	    if (errno != ECONNREFUSED && errno != ETIMEDOUT)
	    {  /* NO, some other error */
		errLogSysRet(ErrLogOp,debugInfo,"Connect error, message not sent");
	        close(fgsd);
		return(RET_ERROR);
	    }
	}                                /* Yes, try MAXRETRY times */
	else     /* connection established */
            break;
	TPRINT1("SendAsync(): Socket queue full, will retry %d\n",i);
	close(fgsd);
	sleep(SLEEP);
    }
    if (result != 0)    /* tried MAXRETRY without success  */
    {
	errLogRet(ErrLogOp,debugInfo,"SendAsync(): Max trys Exceeded, aborting send\n");
	errLogRet(ErrLogOp,debugInfo,"Problems sending message");
        return(RET_ERROR);
    }
    TPRINT0("SendAsync(): Connection Established \n");
    if (from_addr != NULL)
       sprintf(message,"%d %s %d %d",cmd, from_addr->host, from_addr->port, from_addr->pid);
    else
       sprintf(message,"%d",cmd);
    if (args != NULL)
    {
       strcat(message," ");
       strcat(message,args);
    }
    strcat(message,",");
    write(fgsd,message,(strlen(message)+1));
    TPRINT1("Message Sent: %s\n",message);

    close(fgsd);
    return(RET_OK);
}
#endif

/*------------------------------------------------------------
|
|    SendAsync2()/2
|       simplified SendAsync; make no assumptions about msge
|       connect to  an Async Process's Socket
|       then transmit a message to it and disconnect.
|
+-----------------------------------------------------------*/
int SendAsync2(CommPort to_addr, char *msge)
{
    int fgsd;   /* socket discriptor */
    int result;
    int i;
    int ret __attribute__((unused));
#ifndef LINUX
    int one = 1;
#endif


    if (to_addr->port == -1 || to_addr->pid == -1) /* then we do not have a socket */
       return (RET_ERROR);
    /* --- try several times then fail --- */
    for (i=0; i < MAXRETRY; i++)
    {
        fgsd = socket(AF_INET,SOCK_STREAM,0);        /* create a socket */

        if (fgsd == -1)
        {
            errLogSysRet(ErrLogOp,debugInfo,"Had trouble creating socket.");
            return(RET_ERROR);
        }
	TPRINT1("SendAsync2(): socket create for async trans %d\n",fgsd);
#ifndef LINUX
        setsockopt(fgsd,SOL_SOCKET,SO_USELOOPBACK,(const char*) &one,sizeof(one));
#endif

       /* --- attempt to connect to the named socket --- */         
       if ((result = connect(fgsd,(struct sockaddr*) &(to_addr->messname),sizeof(to_addr->messname))) != 0)
	{
	    /* --- Is Socket queue full ? --- */
	    if (errno != ECONNREFUSED && errno != ETIMEDOUT)
	    {  /* NO, some other error */
		errLogSysRet(ErrLogOp,debugInfo,"Connect error, message not sent");
	        close(fgsd);
		return(RET_ERROR);
	    }
	}                                /* Yes, try MAXRETRY times */
	else     /* connection established */
            break;
	TPRINT1("SendAsync2(): Socket queue full, will retry %d\n",i);
	close(fgsd);
	sleep(SLEEP);
    }
    if (result != 0)    /* tried MAXRETRY without success  */
    {
	errLogRet(ErrLogOp,debugInfo,"SendAsync2(): Max trys Exceeded, aborting send\n");
        return(RET_ERROR);
    }
    TPRINT0("SendAsync2(): Connection Established \n");
    ret = write(fgsd,msge,(strlen(msge)+1));
    TPRINT1("Message Sent: %s\n",msge);

    close(fgsd);
    return(RET_OK);
}

void sendToTclDg(char *msge)
{
   SendAsync2(&comm_addr[TCL_COMM_ID],msge);
}

void initToTclDg(int port)
{
   CommPort ptr = &comm_addr[TCL_COMM_ID];
   strcpy(ptr->host,"");
   ptr->port = port;
   ptr->pid = getpid();
   ptr->msg_uid = getuid();
   ptr->messname.sin_family = AF_INET;
   ptr->messname.sin_port = port;
   ptr->messname.sin_addr.s_addr = INADDR_ANY;
}

#ifdef WINBRIDGE
// note to myself 12 oct 07:  in the old pre-nessie version, there was only one
// "proc" and one socket, and the port number was communicated in acqinfo -- for
// nessie, non-status communication is separate from status communication, from
// the perspective of the procs, and status is communicated via mmap so that the
// port at the proc end is for other messages (still necessary) between the
// procs and vnmrbg, hence that port is now kept in acqinfo2 -- although I have
// not completely decided yet, I think the easiest thing is to keep two ports in
// winBridge, and connect to them separately here in vnmrbg
/*------------------------------------------------------------------------
|	getAcqProcParam/0
|
|	This routine reads acqinfo file and loads up the important info
|	such as port, pid and hostname of acquition task 
|
+--------------------------------------------------------------------------*/
static int getAcqProcParam(CommPort ptr, int type)
{
   FILE *stream;
   int   port1;
   int   port2;
   int   infopath[256];

   strcpy( (char*) &infopath[ 0 ], ptr->vnmrsysdir );
   if (type == EXP_SOCKET) {
      strcat( (char*) &infopath[ 0 ], "/acqqueue/acqinfo2");
   } else {
      strcat( (char*) &infopath[ 0 ], "/acqqueue/acqinfo");
   }
   fprintf(stdout, "DEBUG jgw: comm.c getAcqProcParam infopath = %s\n", (char*) &infopath[ 0 ]);
   if (stream = fopen( (const char*) &infopath[ 0 ], "r"))
   {  if (fscanf(stream,"%d%s%d%d%d",&(ptr->pid),ptr->host,&port1,&port2,&(ptr->port) ) != 5)
      {
         fprintf(stdout, "DEBUG jgw: comm.c getAcqProcParam fscanf error\n");
         fclose(stream);
         ptr->pid = ptr->port = -1;
         strcpy(ptr->host,"");
         acq_errno = READ_ERROR;
         return(RET_ERROR);
      }
      else
      {
         TPRINT3("Read in Acqproc stuff, pid=%d host=%s port=%d\n",
		ptr->pid,ptr->host,ptr->port);
         fclose(stream);

#ifdef USE_HTONS

         /* Under Interx nvExpproc writes it's port number correctly, rather
            than network order number which on X86 machine is a very large number.
            Since all the code herein expects the port to be already in 
            network order (big endian) we we made the switch here     GMB
         */
         ptr->port = 0xFFFF & htons(ptr->port);

#endif
         fprintf(stdout, "DEBUG jgw: comm.c getAcqProcParam ptr->port=%d\n", ptr->port);
         fprintf(stdout, "DEBUG jgw: comm.c getAcqProcParam type=%d\n", type);
         if ( InitRecverAddr(ptr->host,ptr->port,&(ptr->messname)) == -1)
	     return(RET_ERROR);
	 sprintf( ptr->path, "%s %d %d", ptr->host, ptr->port, ptr->pid );
         return(RET_OK);
      }
   }
   else
   {
      fprintf(stdout, "DEBUG jgw: comm.c getAcqProcParam fopen error\n");
      ptr->pid = ptr->port = -1;
      strcpy(ptr->host,"");
      acq_errno = OPEN_ERROR;
      return(RET_ERROR);
   }
}
#else   /* non-winBridge */
/*------------------------------------------------------------------------
|	getAcqProcParam/0
|
|	This routine reads acqinfo file and loads up the important info
|	such as port, pid and hostname of acquition task 
|
+--------------------------------------------------------------------------*/
static int getAcqProcParam(CommPort ptr)
{
   FILE *stream;
   int   port1;
   int   port2;
   int   infopath[256];

   strcpy( (char*) &infopath[ 0 ], ptr->vnmrsysdir );
#ifdef NESSIE
   strcat( (char*) &infopath[ 0 ], "/acqqueue/acqinfo2");
#else
   strcat( (char*) &infopath[ 0 ], "/acqqueue/acqinfo");
#endif
   if ( (stream = fopen( (const char*) &infopath[ 0 ], "r")) )
   {  if (fscanf(stream,"%d%s%d%d%d",&(ptr->pid),ptr->host,&port1,&port2,&(ptr->port) ) != 5)
      {
         fclose(stream);
         ptr->pid = ptr->port = -1;
         strcpy(ptr->host,"");
         acq_errno = READ_ERROR;
         return(RET_ERROR);
      }
      else
      {
         TPRINT3("Read in Acqproc stuff, pid=%d host=%s port=%d\n",
		ptr->pid,ptr->host,ptr->port);
         fclose(stream);

#ifdef USE_HTONS

         /* Under Interx nvExpproc writes it's port number correctly, rather
            than network order number which on X86 machine is a very large number.
            Since all the code herein expects the port to be already in 
            network order (big endian) we we made the switch here     GMB
         */
         ptr->port = 0xFFFF & htons(ptr->port);

#endif
         if ( InitRecverAddr(ptr->host,ptr->port,&(ptr->messname)) == -1)
	     return(RET_ERROR);
	 sprintf( ptr->path, "%s %d %d", ptr->host, ptr->port, ptr->pid );
         return(RET_OK);
      }
   }
   else
   {
      ptr->pid = ptr->port = -1;
      strcpy(ptr->host,"");
      acq_errno = OPEN_ERROR;
      return(RET_ERROR);
   }
}
#endif  /* ifdef WINBRIDGE  */

int get_comm(int index, int attr, char *val)
{
   CommPort ptr = &comm_addr[index];

   if ( ptr != NULL )
   {
      if (attr == CONFIRM)
      {
	int	ival;

        TPRINT0("Confirm acq port\n");

/*  Call getAcqProcParam again, in case someone
    has done an su acqproc in the interrum	*/

#ifdef WINBRIDGE
        if (index == INFO_COMM_ID) {
            ival = getAcqProcParam(ptr, INFO_SOCKET);
        } else {
	    ival = getAcqProcParam(ptr, EXP_SOCKET);
        }
#else
        if ( (ptr->pid > 0) && ! strcmp(ptr->host,val) )
        {
	   if ( ! kill(ptr->pid,0) || (errno != ESRCH) )
      {
              return(1);
      }
        }
	ival = getAcqProcParam(ptr);	/* To send stuff to acq process */
#endif
	if (ival == RET_ERROR)
	  return( 0 );

        /*  check if acqPid is active */
	if (( kill(ptr->pid,0) == -1) && (errno == ESRCH))
	  return( 0 );
	else if (val != NULL)
	  return( (strcmp(ptr->host,val) == 0) );
      }
      else if (attr == ADDRESS_VNMR)
      {
         TPRINT3("get vnmr address: send %s %d %d\n",ptr->host,ptr->port,ptr->pid);
         sprintf(val, "%s %d %d", ptr->host,ptr->port,ptr->pid);
      }
      else if (attr == ADDRESS_ACQ)
      {
         TPRINT0("get acq address \n");
         if (strcmp(ptr->host,"") )
            sprintf(val, "%s %d %d", ptr->host,ptr->port,ptr->pid);
         else
            strcpy(val,"NoAcq");
      }
      else if (attr == VNMR_CONFIRM)
      {
         TPRINT0("confirm address \n");
         if (!strcmp(ptr->host,""))
            gethostname( ptr->host, sizeof( ptr->host ) );
      }
      return( 1 );
   }
   return( 0 );
}

int get_vnmr_handle(int index)
{
   CommPort ptr = &comm_addr[index];

   return(ptr->msgesocket);
}

int set_comm(int index, int attr, char *val)
{
   CommPort ptr = &comm_addr[index];

   if ( (ptr != NULL) && (val != NULL) )
   {
      if (attr == ADDRESS)
      {
         TPRINT1("read address %s\n",val);

         sscanf(val, "%s %d %d", ptr->host,&(ptr->port),&(ptr->pid));
         TPRINT3("got address %s %d %d \n",ptr->host,ptr->port,ptr->pid);
         strcpy( ptr->path, val );
      }
   }
   return(0);
}

void init_acq(char *val)
{
   CommPort ptr = &comm_addr[ACQ_COMM_ID];

#ifdef NESSIE
   initExpStatus( 0 ); /*  mapin statBlock /tmp/ExpStatus */
#endif
   ptr->pid = ptr->port = -1;

   strcpy(ptr->host,"");
   if (val != NULL)
   {
      strcpy(ptr->vnmrsysdir,val);
#ifdef WINBRIDGE
      //fprintf(stdout, "DEBUG jgw: comm.c init_acq calling getAcqProcParam type=%d\n", EXP_SOCKET);
      getAcqProcParam(ptr, EXP_SOCKET);
      ptr = &comm_addr[INFO_COMM_ID];
      strcpy(ptr->vnmrsysdir,val);
      //fprintf(stdout, "DEBUG jgw: comm.c init_acq calling getAcqProcParam type=%d\n", INFO_SOCKET);
      getAcqProcParam(ptr, INFO_SOCKET);
#else
      getAcqProcParam(ptr);
#endif
   }
}


int set_comm_port(CommPort ptr)
{
   int on = 1;
   socklen_t meslength;
         TPRINT0("set vnmr port \n");

 /*===================================================================*/
 /*   ---  create the asynchronous message socket for PSG,VNMR  ---   */
 /*===================================================================*/
 
    strcpy(ptr->path,"");
    ptr->msgesocket = socket(AF_INET,SOCK_STREAM,0);  /* create a socket */
    if (ptr->msgesocket == -1)
    {
        perror("INITSOCKET(): socket error");
        exit(1);
    }   
#ifndef LINUX
    /* if on same machine, do not send out ethernet */
    setsockopt(ptr->msgesocket,SOL_SOCKET,SO_USELOOPBACK,(char*) &on,sizeof(on));
#endif
    setsockopt(ptr->msgesocket,SOL_SOCKET,(~SO_LINGER),(char *)&on,sizeof(on));
    /* close socket immediately if close request */
 
    /* --- bind a name to the socket so that others may connect to it --- */
 
   /* Use IPPORT_RESERVED +4 for port until we setup better way */
    /* Find first available port */
    ptr->messname.sin_family = AF_INET;
    ptr->port = ptr->messname.sin_port = 0; /* set to 0 for port search */
    ptr->messname.sin_addr.s_addr = INADDR_ANY;
    /* name socket */
    if (bind(ptr->msgesocket,(struct sockaddr*) &(ptr->messname),sizeof(ptr->messname)) != 0)
    {
        perror("INITSOCKET(): msgesocket bind error");
        exit(1);
    }   
    meslength = sizeof(ptr->messname);
    getsockname(ptr->msgesocket,(struct sockaddr*) &ptr->messname,&meslength);
    ptr->port = ptr->messname.sin_port;
         TPRINT1("listen of socket %d \n",ptr->msgesocket);
    listen(ptr->msgesocket,5);        /* set up listening queue ?? */

    sprintf( ptr->path, "%s %d %d", ptr->host, ptr->port, ptr->pid );
    return(0);
}

void init_comm_port(char *hostval)
{
   int ret __attribute__((unused));
   CommPort ptr = &comm_addr[VNMR_COMM_ID];

   TPRINT0("init vnmr port \n");
   strcpy(ptr->host,hostval);
   ptr->pid = getpid();
   ptr->msg_uid = getuid();

   ret = seteuid( ptr->msg_uid );
   set_comm_port(ptr);

   TPRINT3("vnmr pid=%d host=%s port=%d\n",ptr->pid,ptr->host,ptr->port);
   TPRINT1("vnmr path=%s\n",ptr->path);
}

void get_acq_id(char *id)
{
   strcpy(id,acqID);
}

void set_acq_id(char *id)
{
   strcpy(acqID,id);
}

void set_effective_user()
{
#ifdef NESSIE
   CommPort ptr = &comm_addr[VNMR_COMM_ID];
   (void) ptr;
#endif
}

void set_real_user()
{
#ifdef NESSIE
   int ret __attribute__((unused));
   CommPort ptr = &comm_addr[VNMR_COMM_ID];
   ret = seteuid( ptr->msg_uid );
#endif
}

void init_comm_addr()
{
   int ret __attribute__((unused));
   CommPort ptr = &comm_addr[VNMR_COMM_ID];

   TPRINT0("init vnmr address \n");
   ptr->pid = getppid();
   ptr->port = -1;
   ptr->msg_uid = getuid();

   ret = seteuid( ptr->msg_uid );
   strcpy(ptr->host,"");
   set_acq_id("-1");
}

int clear_acq()
{
#ifdef NESSIE
   expStatusRelease();
#endif
   TPRINT0("clear_acq called \n");
   return(0);
}

#ifndef NESSIE

#include "ACQPROC_strucs.h"

#define REGISTERPORT    2
#define UNREGPORT       3

AcqStatBlock        statBlock;

static int
readAcqStat(int sd, AcqStatBlock *statblock )
{
	int		ival, nchr;
	AcqStatBlock	tmpblock;

	ival = wait_for_select( sd, 10 /* seconds */ );
	if (ival < 1)
        {
	   if (ival < 0)
              acq_errno = SELECT_ERROR;
	   else if (ival == 0)
              acq_errno = CONNECT_ERROR;
	  return( RET_ERROR );
	}
	nchr = read( sd, &tmpblock, sizeof(AcqStatBlock) );
        if (nchr == sizeof(AcqStatBlock) )
        {
	  memcpy( (char *) statblock, (char *) &tmpblock, nchr );
	  return( RET_OK );
        }
        else
        {
           acq_errno = READ_ERROR;
	   return( RET_ERROR );
        }
}

#endif

int GetAcqStatus(int to_index, int from_index, char *host, char *user)
{
#ifndef NESSIE
    char message[100];
    int meslength;
    int on = 1;

    CommPort tmp_from = &comm_addr[LOCAL_COMM_ID];
    CommPort to_addr = &comm_addr[to_index];
    CommPort from_addr = &comm_addr[from_index];

    tmp_from->path[0] = '\0';;
    tmp_from->pid = from_addr->pid;
    strcpy(tmp_from->host,host);

    tmp_from->messname.sin_family = AF_INET;
    tmp_from->messname.sin_addr.s_addr = INADDR_ANY;
    tmp_from->messname.sin_port = 0;

    tmp_from->msgesocket = socket( AF_INET, SOCK_DGRAM, 0 );
    if (tmp_from->msgesocket < 0) {
        acq_errno = tmp_from->msgesocket;
        return( -1 );					/*  Not ABORT !! */
    }

/* Specify socket options */

#ifndef LINUX
    setsockopt(tmp_from->msgesocket,SOL_SOCKET,SO_USELOOPBACK,&on,sizeof(on));
#endif
    setsockopt(tmp_from->msgesocket,SOL_SOCKET,(~SO_LINGER),&on,sizeof(on));

    if (bind(tmp_from->msgesocket,(struct sockaddr *) &(tmp_from->messname),
             sizeof(tmp_from->messname)) != 0) {
        acq_errno = BIND_ERROR;
        close( tmp_from->msgesocket );
        return( -1 );					/*  Not ABORT !! */
    }

    meslength = sizeof(tmp_from->messname);
    getsockname(tmp_from->msgesocket,(struct sockaddr *) &tmp_from->messname,
                &meslength);
    tmp_from->port = tmp_from->messname.sin_port;
 
/* register this status socket with the acquisition process */

    sprintf(message,"%s %d", user,tmp_from->pid);
    if (SendAsync(to_addr,tmp_from,REGISTERPORT,message) == RET_ERROR)
    {
        close( tmp_from->msgesocket );
        return( -1 );					/*  Not ABORT !! */
    }

    if (readAcqStat( tmp_from->msgesocket, &statBlock ) == RET_ERROR)
    {
        statBlock.Acqstate = RET_ERROR;
    }

    if (SendAsync(to_addr,tmp_from,UNREGPORT,message) == RET_ERROR)
    {
        close( tmp_from->msgesocket );
        return( -1 );					/*  Not ABORT !! */
    }
    close( tmp_from->msgesocket );
#else
    (void) to_index;
    (void) from_index;
    (void) host;
    (void) user;
#endif

    return(0);
}

int getAcqStatusValue(int index, double *val)
{
#ifndef NESSIE

   if (index == LOCKLEVEL)
   {
      if (statBlock.AcqLockLevel > 1300)
      {
        *val = 1300.0 / 16.0 +
                  (double) (statBlock.AcqLockLevel - 1300) / 15.0;
        if (*val > 100.0)
           *val = 100.0;				/*  Saturated. */
      }
      else
        *val = (double) statBlock.AcqLockLevel / 16.0;
   }

#else  /* NESSIE */

   int lkval;
   if ( (index == LOCKLEVEL) || (index == (LOCKLEVEL+1)) )
   {
      if (index == LOCKLEVEL)
         lkval = getStatLkLevel();
      else
         lkval = getStatGradError();
      if (lkval > 1300)
      {
        *val = 1300.0 / 16.0 +
                  (double) (lkval - 1300) / 15.0;
        if (*val > 100.0)
           *val = 100.0;				/*  Saturated. */
      }
      else
        *val = (double) lkval / 16.0;
   }

#endif
   return(0);
}

int getAcqStatusInt(int index, int *val)
{
#ifndef NESSIE

   if (index == STATE)
   {
      *val = statBlock.Acqstate;
   }
   else if (index == SUFLAG)
   {
      *val = statBlock.AcqSuFlag;
   }

#else	/* NESSIE */

   if (index == STATE)
   {
      *val = getStatAcqState();
   }
   else if (index == SUFLAG)
   {
      *val = getStatGoFlag();
   }

#endif
  return(0);
}

int getAcqInQue()
{
#ifndef NESSIE
   return(0);
#else	/* NESSIE */
   return( (int) getStatInQue() );
#endif
}

int getAcqRemainingTime()
{
#ifndef NESSIE
   return(0);
#else	/* NESSIE */
   return( (int) getStatRemainingTime() );
#endif
}

int getAcqStatusStr(int index, char *val, int maxlen)
{
#ifndef NESSIE

   if (index == EXPID)
   {
      strcpy(val,statBlock.AcqExpID);
   }
   else if (index == USERID)
   {
      strcpy(val,statBlock.AcqUserID);
   }

#else	/* NESSIE */

   if (index == EXPID)
   {
      getStatExpName(val,maxlen);
   }
   else if (index == USERID)
   {
      getStatUserId(val,maxlen);
   }

#endif
  return(0);
}

int getAcqTemp()
{
#ifndef NESSIE
   return( statBlock.AcqVTAct );
#else	/* NESSIE */
   return( getStatVTAct() );
#endif
}

int getAcqTempSet()
{
#ifndef NESSIE
   return( statBlock.AcqVTSet );
#else	/* NESSIE */
   return( getStatVTSet() );
#endif
}

int getAcqTickCountError()
{
#ifndef NESSIE
   return( statBlock.AcqTickCountError );
#else   /* NESSIE */
   /*TPRINT1("getAcqTickCountError(): tickcount error is %d",getStatAcqTickCountError());*/
   return( getStatAcqTickCountError() );
#endif
}

int getAcqSpin()
{
#ifndef NESSIE
   return( statBlock.AcqSpinAct );
#else	/* NESSIE */
   return( getStatSpinAct() );
#endif
}

int getAcqSpinSet()
{
#ifndef NESSIE
   return( statBlock.AcqxSpinSet );
#else	/* NESSIE */
   return( getStatSpinSet() );
#endif
}

int getAcqSample()
{
#ifndef NESSIE
   return( (int) statBlock.AcqSample );
#else	/* NESSIE */
   return( getStatAcqSample() );
#endif
}

int getAcqLSDV()
{
#ifndef NESSIE
   return( (int) statBlock.AcqLSDV );
#else	/* NESSIE */
   return( getStatLSDV() );
#endif
}

int getAcqConsoleID()
{
#ifndef NESSIE
   return( (int) 0);
#else	/* NESSIE */
   return( getStatConsoleID() );
#endif
}
