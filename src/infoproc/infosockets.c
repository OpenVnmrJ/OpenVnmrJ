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

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#if !defined(AIX) && !defined(LINUX)
#include <sys/filio.h>
#include <sys/sockio.h>
#endif

#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include "ACQPROC_strucs.h"

#define MAXRETRY 8
#define SLEEP 1
#define OK 0
#define NOTFOUND -1
#define BUFLEN 22
#define BUFSIZE 4096

#define NOMESSAGE 0
#define ERROR 0
#define MESSAGE 1

extern int messocket;   /* message process async socket descriptor */
extern int    Acqdebug;         /* debugging flag */
extern messpacket MessPacket;


/* may want to do this for Solaris and Linux as well in the future not just Interix */
#ifdef __INTERIX

#include "sockets.h"
#include <pthread.h>
#include <signal.h>
#include "errLogLib.h"
#include "rngBlkLib.h"



extern pthread_t main_threadId;

static Socket   *pApplSocket;

static pthread_t AcceptThreadId;

static RINGBLK_ID pAcceptQueue;
 
 
static int PortNum = 0;

extern int LocalAcqPid;

extern char vnmrsystem[128];	/* vnmrsystem path */
 
/*-----------------------------------------------------------------------
|	wrtacqinfo()/0
|	write the acquisitions pid, and socket port numbers out for
|	  access by other processes
+-----------------------------------------------------------------------*/
wrtacqinfo()
{
    char filepath[256];
    char hostname[256];
    char buf[256];
    int fd;
    int bytes;
    int pid;

    LocalAcqPid = pid = getpid();
    /* get Host machine name */
    gethostname(hostname,sizeof(hostname));

    if (Acqdebug)
        fprintf(stderr," msge ports = %d\n", PortNum);
    sprintf(buf,"%d %s %d %d %d",pid,hostname, -9, -9, PortNum);

    strcpy(filepath,vnmrsystem);	/* path to acqinfo */
    strcat(filepath,"/acqqueue/acqinfo");
    if ( (fd = open(filepath,O_WRONLY | O_CREAT | O_TRUNC,0666)) == -1)
    {
        fprintf(stderr,"Could Not Open Acquisition Info File: '%s'\n",
            filepath);
        exit(1);
    }
    bytes = write(fd,buf,strlen(buf)+1);
    if ( (bytes == -1) )
    {
        fprintf(stderr,"Could Not Write Acquisition Info File: '%s'\n",
            filepath);
        exit(1);
    }
    close(fd);
}



/*
 * initInfoSocket()
 *
 * Creates the listen socket for communication to Infoproc from other processes
 *
 * instead of signal handler etc, this routine create a thread to handle the accept
 *
 *    Author greg Brissey 7/12/2006
 */
int initInfoprocSocket()
{
        int     status, ival;
        int     applPort;
        void *AcceptConnection( void *arg);
 
        pApplSocket = createSocket( SOCK_STREAM );
        if (pApplSocket == NULL)                /* each call to program in */
          return( -1 );                            /* sockets.c sets errno */
        ival = openSocket( pApplSocket );
        if (ival != 0)
          return( -1 );
        ival = bindSocketAnyAddr( pApplSocket );
        if (ival != 0)
        {
          errLogSysRet(ErrLogOp,debugInfo,"initExpprocSocket: bindSocketAnyAddr failed:" );
          return( -1 );
         }

        PortNum = returnSocketPort( pApplSocket );
        ival = listenSocket( pApplSocket );

        ival = listenSocket( pApplSocket );
        if (ival != 0)
        {
          errLogSysRet(ErrLogOp,debugInfo,"initExpprocSocket: listenSocket failed:" );
          return( -1 );
         }
 
 
         pAcceptQueue = rngBlkCreate(128,"AcceptQ", 1);  /* accepted socket queue */
 
        /* create thread to handle the accept */
        status = pthread_create (&AcceptThreadId, NULL, AcceptConnection, (void*) pApplSocket);
        return( 0 );
 
}


/*
 * AcceptConnection() is a posix thread which waits on Accept,
 * when accept returns the newly created accept socket is placed on a queue
 * and a SIGIO signal is sent to the main thread to perform the read and processing
 * the socket message
 *
 *   Author greg Brissey 7/12/2006
 */
void *AcceptConnection( void *arg)
{
    int result;
    Socket  *pAcceptSocket;
    Socket   *pListenSocket;
 
    pListenSocket = (Socket *) arg;
 
    pthread_detach(AcceptThreadId);   /* if thread terminates no need to join it to recover resources */

                                                                                                         
    for ( ;; )
    {
        pAcceptSocket = (Socket *) malloc( sizeof( Socket ) );
        if (pAcceptSocket == NULL) {
                return( NULL );
        }
        pListenSocket = (Socket *) arg;
         
        memset( pAcceptSocket, 0, sizeof( Socket ) );
        result = acceptSocket_r( pListenSocket, pAcceptSocket );
        if (result < 0) {
            errLogSysRet(ErrLogOp,debugInfo,"acceptSocket_r" );
        }
        else
        {
             DPRINT2(2,"AcceptConnection: 0x%lx, fd: %d\n", pAcceptSocket,pAcceptSocket->sd);
             rngBlkPut(pAcceptQueue, &pAcceptSocket, 1);
             pthread_kill(main_threadId,SIGIO); /* signal console socket msg arrival to main thread */
        }
   }
  
} 
  
/*
 * processInfoSock() 
 * called by main thread to process the accepted socket, 
 * via the SIGIO sent by pthread_kill() from the above thread functions 
 *
 *   Author greg Brissey 7/12/2006
 */
int processInfoSock()
{ 
   int  fd, iter, maxfd, nfound;
   fd_set  readfd, writefd;
   Socket  *pAcceptSocket;
   void readAcceptSocket( Socket *pSocket );
   int     rngBlkIsEmpty (register RINGBLK_ID ringId);
   int sockInQ;
 
   while ((sockInQ = rngBlkNElem(pAcceptQueue)) > 0 )
   {

     DPRINT1(2,"Accepted Sockets in Queue: %d\n",sockInQ);
     rngBlkGet(pAcceptQueue, &pAcceptSocket, 1);
     DPRINT2(+3,"processExpSock: 0x%lx, fd: %d\n", pAcceptSocket,pAcceptSocket->sd);
 
     FD_ZERO( &readfd );
     FD_SET( pAcceptSocket->sd, &readfd);

     DPRINT1(+3,"readfd mask: 0x%lx\n",readfd);
 
  try_again:
     if ( (nfound = select( pAcceptSocket->sd+1, &readfd, 0, 0, 0 ) ) < 0)
     {  
        if (errno == EINTR)
           goto try_again;
        else
           errLogSysRet(ErrLogOp,debugInfo, "select Error:\n" );
     }  
     DPRINT1(3,"select: readfd mask: 0x%lx\n",readfd);
       
     if (nfound < 1)  /* Nobody */
     {
        /*fprintf( stderr, "SIGIO received, but nothing active was found\n" );*/
        free(pAcceptSocket);
        return;
     }


     if (FD_ISSET(pAcceptSocket->sd, &readfd) )
       readAcceptSocket( pAcceptSocket );

     free(pAcceptSocket);                 
   }
 
   return 0;
}


void readAcceptSocket( Socket *pSocket )
{
        char*                   allocbuffer = NULL;
        char*                   buffer = NULL;
        char                    constbuf[ 4096 ];
        long                    msgeSize;
        int                     bcount;
        sigset_t    savemask;
 
 
          bcount = readSocket( pSocket, constbuf, sizeof(constbuf) );
          DPRINT1(3,"bcount: %d of msge\n",bcount);
 
          if (bcount > 0)
          {
            constbuf[4095] = (char) 0;
            DPRINT1(2,"msge str: '%s'\n", constbuf);
            getmpacket(constbuf);
            Smessage();   /* will call GetMessage below */
          }
          else
          {
             errLogSysRet(ErrLogOp,debugInfo,
                "processAcceptSocket: Read of Async Message failed: " );
             errLogRet(ErrLogOp,debugInfo,
                "processAcceptSocket: Message Ignored.\n" );
          }
        closeSocket( pSocket );
}        
/*-----------------------------------------------------------------------
|
|    getmpacket()/1
|	read data from socket and parse information into the message
|       packet structure
+------------------------------------------------------------------------*/
getmpacket(char *buffer)
{
    char *bufptr;
    char *ptr;
    char *ptr1;
    int   nchr;

    bufptr = buffer;

    /* extract Command */
    MessPacket.CmdOption = getinttoken(&bufptr);

    /* extract Host name from string */
    getstrtoken(MessPacket.Hostname,sizeof(MessPacket.Hostname),&bufptr);

    /* extract Inter-Net Port Number from string */
    MessPacket.Port = getinttoken(&bufptr);

    /* extract Inter-Net Port Process ID Number from string */
    MessPacket.PortPid = getinttoken(&bufptr);

    /* extract message from string */
    strcpy(MessPacket.Message,bufptr);
    DPRINT4(1,"Command: %d, Hostname: '%s' Port #: %d Port pid: %d\n",
		MessPacket.CmdOption,MessPacket.Hostname,MessPacket.Port,
		MessPacket.PortPid);
    DPRINT1(1,"message: '%s'\n",MessPacket.Message);
}

/*----------------------------------------------------------------------
|
|    GetMessage()/0
|	Test for pending connections, if none return(NOCONNECT);
|	Else connect and put message into the message packet structure
|	return(MESSAGE);
|
+---------------------------------------------------------------------*/
GetMessage()
{
    /* already done the work in the above function so return */
    return(MESSAGE);
}

#else  /* original way */

/*-----------------------------------------------------------------n
|
|    make_a_socket
|    setup_a_socket
|    render_socket_async
|
+-------------------------------------------------------------------*/

int
make_a_socket()
{
	int	tsd;

	tsd = socket( AF_INET, SOCK_STREAM, 0 );
	if (tsd < 0)
	  return( -1 );
	else
	  return( tsd );
}

int
setup_a_socket( tsd )
int tsd;
{
	int	ival, one, pid;

#ifdef __INTERIX
        ival = fcntl(tsd, F_SETOWN, (int) getpid());
        if ( ival == -1 ) {
           perror( "setup_a_socket: set ownership" );
                close( tsd );
                return( ival );
        }
#else
	pid = getpid();
	ival = ioctl( tsd, FIOSETOWN, &pid );
	if (ival < 0) {
		perror( "set ownership" );
		close( tsd );
		return( ival );
	}

	ival = ioctl( tsd, SIOCSPGRP, &pid );
	if (ival < 0) {
		perror( "set socket's process group" );
		close( tsd );
		return( ival );
	}
#endif

	one = 1;
#if !defined( LINUX ) && !defined( __INTERIX)
	ival = setsockopt( tsd, SOL_SOCKET, SO_USELOOPBACK,
		(char *)&one, sizeof( one ) );
	if (ival < 0)  {
		perror( "use loopback" );
		close( tsd );
		return( ival );
	}
#endif

        /*ival = setsockopt(tsd,SOL_SOCKET,(~SO_LINGER),&one,sizeof(one));
	if (ival < 0) {
		perror( "don't linger" );
	}*/

	return( 0 );
}

int
render_socket_async( tsd )
int tsd;
{
	int	result, ival, one;

#ifdef LINUX
        result = fcntl(tsd, F_SETOWN, (int) getpid());
        if ( result == -1 ) {
           perror( "render_socket_async: set ownership" );
                close( tsd );
                return( ival );
        }
        ival = fcntl(tsd, F_GETFL);
        if ( ival == -1 ) {
           perror( "render_socket_async: get Flags" );
                close( tsd );
                return( ival );
        }
        ival |= O_ASYNC;
        result = fcntl(tsd, F_SETFL, ival);
        if ( result == -1 ) {
           perror( "render_socket_async: set Flags" );
                close( tsd );
                return( ival );
        }
#else
	one = 1;
	result = ioctl( tsd, FIOASYNC, &one );
#endif
	if (result < 0)  {
		perror( "set nonblocking/asynchronous" );
		return( ival );
	}
}


/*----------------------------------------------------------------------
|
|    GetMessage()/0
|	Test for pending connections, if none return(NOCONNECT);
|	Else connect and put message into the message packet structure
|	return(MESSAGE);
|
+---------------------------------------------------------------------*/
GetMessage()
{
    struct sockaddr from;
    int fromlen = sizeof(from);
    int fromsocket;
    int flags;

    /* test for pending connection */
    fromsocket = accept(messocket,&from,&fromlen);
    if (fromsocket == -1) 
    {
	if (Acqdebug)
	    fprintf(stderr,"GetMessage(): No connection pending\n");
	return(NOMESSAGE);
    }
    if ((flags = fcntl(fromsocket,F_GETFL,0)) == -1) /* set to blocking */
    {
         perror("Smessage():  fcntl error ");
	 return(ERROR);
    }
    if (Acqdebug)
        fprintf(stderr,"socket flags = %d\n",flags);

    flags &=  ~FNDELAY;
    if (Acqdebug)
        fprintf(stderr,"socket flags = %d\n",flags);
    if (fcntl(fromsocket,F_SETFL,flags) == -1) /* set to blocking */
    {
         perror("Smessage():  fcntl error ");
	 return(ERROR);
    } 


    getmpacket(fromsocket);

    close(fromsocket);	/* close socket descriptor */

    return(MESSAGE);
}

/*-----------------------------------------------------------------------
|
|    getmpacket()/1
|	read data from socket and parse information into the message
|       packet structure
+------------------------------------------------------------------------*/
getmpacket(fromsocket)
int fromsocket;
{
    char  buffer[BUFSIZE];
    char *bufptr;
    char *ptr;
    char *ptr1;
    int   nchr;

    receive(fromsocket,buffer,sizeof(buffer));
    bufptr = buffer;

    /* extract Command */
    MessPacket.CmdOption = getinttoken(&bufptr);

    /* extract Host name from string */
    getstrtoken(MessPacket.Hostname,sizeof(MessPacket.Hostname),&bufptr);

    /* extract Inter-Net Port Number from string */
    MessPacket.Port = getinttoken(&bufptr);

    /* extract Inter-Net Port Process ID Number from string */
    MessPacket.PortPid = getinttoken(&bufptr);

    /* extract message from string */
    strcpy(MessPacket.Message,bufptr);
    if (Acqdebug)
    {
        fprintf(stdout,"Command: %d, Hostname: '%s' Port #: %d Port pid: %d\n",
		MessPacket.CmdOption,MessPacket.Hostname,MessPacket.Port,
		MessPacket.PortPid);
        fprintf(stdout,"message: '%s'\n",MessPacket.Message);
    }
}
/*---------------------------------------------------------------
|
|   receive(sd,buffer,bufsize)/3
|	receive message from socket
|
+--------------------------------------------------------------*/
receive(sd,buffer,bufsize)
int sd;
char buffer[];
int bufsize;		/* buffer size */
{
    char tbuffer[256];
    int  nchr;
    int i,j;

    i = 0;
    while (1)
    { 
	block_signals();
	nchr = read(sd,tbuffer,sizeof(tbuffer));
	unblock_signals();
    	if (nchr == -1)
    	{   perror("receive(): socket read error");
            return(ERROR);
        }
        if (nchr == 0)
        {   
	    if (Acqdebug)
	        fprintf(stderr,"bg Connection closed\n");
            break;
        }
	if (Acqdebug)
	{
            fprintf(stderr,"bg Received %d characters from socket.\n",nchr);
            fprintf(stderr,"bg message: '%s'\n",tbuffer);
	}
	for (j=0;j<nchr;i++,j++)
	{
	    if (i > bufsize - 2)
	    {
		buffer[bufsize-1] = '\0';
		return(ERROR);
	    }
  	    buffer[i] = tbuffer[j];
	}
    }
    buffer[i] = '\0';
    if (Acqdebug)
    {
        fprintf(stderr,"RECEIVE(): Received %d characters from socket.\n",i);
        fprintf(stderr,"RECEIVE(): message: '%s'\n",buffer);
    }
    return(OK);
}

/*-----------------------------------------------------------------------
|
|       InitRecverAddr()/2
|       Initialize the socket addresses for display and interactive use
|
+-----------------------------------------------------------------------*/
static
InitRecverAddr(AcqHost,port,recver)
int port;
char *AcqHost;
struct sockaddr_in *recver;
{
    struct hostent *hp;
    extern struct hostent	*this_hp;
 
    /* --- name of the socket so that we may connect to them --- */
    /* read in acquisition host and port numbers to use */
    /* see acqproc.c where this_hp is defined for */
    /* explanation why next line is now a comment... */
    /*hp = gethostbyname(AcqHost);*/
    hp = this_hp;
    if (hp == NULL)
    {
       if (Acqdebug)
          fprintf(stderr,"Infoproc():, Unknown Host");
       return(-1);
    }

    /*bzero((char *) recver,sizeof(struct sockaddr_in)); clear socket info */
    memset((char *) recver,0,sizeof(struct sockaddr_in)); /* clear socket info */
     
    /*bcopy(hp->h_addr,(char *)&(recver->sin_addr),hp->h_length);*/
    memcpy( (char *)&(recver->sin_addr), hp->h_addr, hp->h_length );
    recver->sin_family = hp->h_addrtype;
    recver->sin_port = port;
    return(OK);
}


/*------------------------------------------------------------
|
|    sendasync()/4
|       connect to  an Async Process's Socket
|       then transmit a message to it and disconnect.
|
+-----------------------------------------------------------*/
sendasync(machine,port,acqpid,message)
char *machine;
char *message;
int acqpid;
int port;
{
    char buffer[256];
    int buflen = 256;
    int flags = 0;
    int fgsd;   /* socket discriptor */
    int i;
    int on = 1;
    int result;
    struct sockaddr_in RecvAddr;
 
    if (Acqdebug)
        fprintf(stderr,"Sendasync():Machine: '%s',Port: %d, PID: %d,\nMsge: '%s'\n",
             machine,port,acqpid,message);

    /* --- initialize the Receiver's Internet Address --- */
    if ( InitRecverAddr(machine,port,&RecvAddr) == -1)
        return(ERROR);
 
    /* --- try several times then fail --- */
    for (i=0; i < MAXRETRY; i++)
    {
        fgsd = make_a_socket();        /* create a socket */
 
        if (fgsd == -1)
        {
            perror("sendacq(): socket");
            fprintf(stderr,"sendacq(): Error, could not create socket\n");
            return(ERROR);
        }
	if (Acqdebug)
          fprintf(stderr,"sendasync(): socket create for async trans %d\n",fgsd);

        setup_a_socket( fgsd );

/* Do not render this socket asynchronous; it is used
   solely in a synchronous, task-blocking manner.	*/

	if (Acqdebug)
	{
            fprintf(stderr,"sendasync(): socket process group: %d\n",acqpid);
            fprintf(stderr,"sendasync(): socket created fd=%d\n",fgsd);       
            printf("sendasync(): send signal that pipe connection is requested.\n");
	}
 
       /* --- attempt to connect to the named socket --- */
       if ((result = connect(fgsd,(struct sockaddr *)&RecvAddr,sizeof(RecvAddr))) != 0)
       {
          /* --- Is Socket queue full ? --- */
          if (errno != ECONNREFUSED && errno != ETIMEDOUT)
          {                             /* NO, some other error */
              perror("sendasync():aborting,  connect error");
              return(ERROR);
          }
       }                                /* Yes, try MAXRETRY times */
       else     /* connection established */
       {
            break;
       }
       if (Acqdebug)
          fprintf(stderr,"sendasync(): Socket queue full, will retry %d\n",i);
       close(fgsd);
       sleep(SLEEP);
    }
    if (result != 0)    /* tried MAXRETRY without success  */
    {
       if (Acqdebug)
         fprintf(stderr,"sendasync(): Max trys Exceeded, aborting send\n");
       return(ERROR);
    }
    if (Acqdebug)
        fprintf(stderr,"Sendacq(): Connection Established \n");
    write(fgsd,message,strlen(message));
    close(fgsd);
    return(1);
}            

#endif

static char *
findAchar( strptr, srchptr )
char *strptr;
char *srchptr;                  /* string of characters to look for */
{
        char    *tmpptr;

        if (strptr == NULL || srchptr == NULL)
          return( NULL );
        if (*strptr == '\0' || *srchptr == '\0')
          return( NULL );

        do {
                tmpptr = srchptr;
                do {
                        if (*strptr == *tmpptr)
                          return( strptr );

                        tmpptr++;
                } while (*tmpptr != '\0');

                strptr++;
        } while (*strptr != '\0');

        return( NULL );
}

getinttoken(strptr)
char **strptr;
{
    char  buffer[BUFLEN];
    char *ptr1;
    int   nchr;

    /* extract integer token from string */
    ptr1 = findAchar(*strptr,", ");
    if (ptr1 == NULL)
    {
        if (Acqdebug)
           fprintf(stderr,"Could not find int token.\n");
        return(NOTFOUND);
    }
    nchr = ptr1 - *strptr;
    if (nchr < BUFLEN-2)
    {
        strncpy(buffer,*strptr,nchr);
        buffer[nchr] = '\0';
    }
    else
    {
        if (Acqdebug)
           fprintf(stderr,"GETINTTOKEN(): NUmber to Large.\n");
        return(NOTFOUND);
    }
    if (Acqdebug > 2)
        fprintf(stdout,"Getinttoken(): value: '%s'\n",buffer);
    *strptr += nchr+1;
    return(atoi(buffer));
}


getstrtoken(substring,maxlen,strptr)
char *substring;
char **strptr;
int maxlen;
{
    char *ptr1;
    int   nchr;

    /* extract substring token from string */
    ptr1 = findAchar(*strptr,", ");
    if (ptr1 == NULL)
    {
        if (Acqdebug)
           fprintf(stderr,"Could not find substring token.\n");
        return(NOTFOUND);
    }
    nchr = ptr1 - *strptr;
    if (nchr < maxlen-2)
    {
        strncpy(substring,*strptr,nchr);
        substring[nchr] = '\0';
    }
    else
    {
        strncpy(substring,*strptr,maxlen);
        substring[maxlen-1] = '\0';
    }
    if (Acqdebug > 2)
        fprintf(stdout,"GETSTRTOKEN(): substring: '%s'\n",substring);
    *strptr += nchr+1;
}

