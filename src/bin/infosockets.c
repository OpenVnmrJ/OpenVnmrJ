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

#ifndef AIX
#include <sys/filio.h>
#include <sys/sockio.h>
#endif

#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include "SUN_HAL.h"
#include "ACQ_HALstruc.h"
#include "ACQPROC_strucs.h"

#define MAXRETRY 8
#define SLEEP 1
#define OK 0

extern int messocket;   /* message process async socket descriptor */
extern int    Acqdebug;         /* debugging flag */
extern messpacket MessPacket;


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

	one = 1;
	ival = setsockopt( tsd, SOL_SOCKET, SO_USELOOPBACK,
		(char *)&one, sizeof( one ) );
	if (ival < 0)  {
		perror( "use loopback" );
		close( tsd );
		return( ival );
	}

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
	int	ival, one;

	one = 1;
	ival = ioctl( tsd, FIOASYNC, &one );
	if (ival < 0)  {
		perror( "set nonblocking/asynchronous" );
		return( ival );
	}
}


#define NOMESSAGE 0
#define ERROR 0
#define MESSAGE 1
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


#define NOTFOUND -1
#define BUFLEN 22

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
        buffer[nchr] = NULL;
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


#define BUFSIZE 4096

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
        substring[nchr] = NULL;
    }
    else
    {
        strncpy(substring,*strptr,maxlen);
        substring[maxlen-1] = NULL;
    }
    if (Acqdebug > 2)
        fprintf(stdout,"GETSTRTOKEN(): substring: '%s'\n",substring);
    *strptr += nchr+1;
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
	nchr = read(sd,tbuffer,sizeof(tbuffer));
    	if (nchr == -1)
    	{   perror("recieve(): socket read error");
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
		buffer[bufsize-1] = NULL;
		return(ERROR);
	    }
  	    buffer[i] = tbuffer[j];
	}
    }
    buffer[i] = NULL;
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

