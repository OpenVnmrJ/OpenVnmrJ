/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  Modified to check if the foreground Child is busy before
    sending stuff to "foregroundFdW".  If the child is busy,
    the message is placed on the Acquisition Message Queue.

    Set AQMDEBUG switch to see the messages from the Acqproc.  */
/*---------------------------------------------------------------------
|   Modified   Author     Purpose
|   --------   ------     -------
|    12/13/90   Greg B.    Modified Notify_value AcqSocketIsRead(me,fd)
|    			   1. acq message buffers to use define ACQMBUFSIZE 
|			   2. test for overflowing buffers - if message exceeds
|			      buffer message fragment is printed but is not 
|			      sent to Vnmr.
|			   3. test for missing EOT, if EOT is missing
|			      print a warning message and then preform proper
|			      actions to recover and send message to Vnmr.
|
|    2/08/91   Greg B.    Modified Notify_value AcqSocketIsRead(me,fd)
|			   1. eliminated usage of EOT, now we read till
|			      socket is closed.
+----------------------------------------------------------------------*/


#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>


#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef __INTERIX
#include <sys/stat.h>
typedef int socklen_t;
#endif
#include <netdb.h>
#include <setjmp.h>
#include <fcntl.h>
#include <errno.h>
#include "acquisition.h"
#include "comm.h"

extern COMM_INFO_STRUCT comm_addr[];
extern int set_comm_port(CommPort ptr);

/*  Definition of the acquisition message entry uses trickery in 
    that the actual entry may be smaller than 1024+4 bytes.  The
    actual size is determined by the length of the message received
    from Acqproc.  The structure definition is only referenced by
    pointers and never explicitly allocated.			*/

#ifdef TODO
static jmp_buf 	   saved_stack;
#endif


#undef DEBUG

#ifdef  DEBUG
#define GPRINT0(str) \
        fprintf(stderr,str)
#define GPRINT1(str, arg1) \
        fprintf(stderr,str,arg1)
#else 
#define GPRINT0(str)
#define GPRINT1(str, arg1)
#endif 

#define NOMESSAGE 0
#define MESSAGE 1

#ifdef SOLARIS
/*---------------------------------------------------------------
|
|   receive(sd,buffer,bufsize)/3
|	receive message from socket
|
+--------------------------------------------------------------*/
int receive(char buffer[], int bufsize, int *read_more)
{
    char tbuffer[1024];
    int  nchr;
    int i,j;
    CommPort vnmr_addr = &comm_addr[VNMR_COMM_ID];

    struct sockaddr from;
    int fromlen = sizeof(from);
    int flags, on;
    static int acqSockfd;

    i = 0;
    GPRINT0("Calling receive\n");
/* accept the pending connection */
    if (!(*read_more))
    {
       acqSockfd = accept(vnmr_addr->msgesocket,&from,&fromlen);
       if (acqSockfd == -1)
       {
           GPRINT0("AsynConnect(): No connection pending\n");
           return(0);
       }
    }
    GPRINT1("vnAcqAccept: Got connection File Descript=%d \n",acqSockfd);
    if ( (nchr = read(acqSockfd,tbuffer,sizeof(tbuffer))) > 0)
    { 
        GPRINT1("Received %d characters from socket.\n",nchr);
        GPRINT1("message: '%s'\n",tbuffer);
	for (j=0;j<nchr;i++,j++)
	{
	    if (i > bufsize - 2)
	    {
		buffer[bufsize-1] = NULL;
		i--;
	    }
  	    buffer[i] = tbuffer[j];
	}
    }
#ifdef NESSIE
    if (nchr > 0)
    {
       if (buffer[i-1] != '\n')
       {
          buffer[i] = '\n';
          i++;
          nchr++;
       }
    }
#endif 
    buffer[i] = NULL;
    GPRINT1("Received %d characters from socket.\n",i);
    GPRINT1("message: '%s'\n",buffer);
    *read_more = 1;

    if (nchr <= 0)
    {
       close(acqSockfd);
       acqSockfd = -1;
       *read_more = 0;
    }

    return(nchr);
}

#else
/*---------------------------------------------------------------
|
|   receive(sd,buffer,bufsize)/3
|	receive message from socket
|
+--------------------------------------------------------------*/
int receive(char buffer[], int bufsize, int *read_more)
{
    char tbuffer[1024];
    int  nchr;
    int i,j;
    CommPort vnmr_addr = &comm_addr[VNMR_COMM_ID];

    struct sockaddr from;
    socklen_t fromlen = sizeof(from);
    int nbytes;
    static int acqSockfd = -1;
/*    struct timeval timeout; */
#ifdef __INTERIX
    struct stat     sinfo;
#endif

    i = 0;
    GPRINT0("Calling receive\n");
/* accept the pending connection */
    if (acqSockfd == -1)
    {
       acqSockfd = accept(vnmr_addr->msgesocket,&from,&fromlen);
       if (acqSockfd == -1)
       {
           GPRINT0("AsynConnect(): No connection pending\n");
           return(0);
       }
    }
    GPRINT1("vnAcqAccept: Got connection File Descript=%d \n",acqSockfd);
    /* set alarm to break out of blocking read */
/*
 *  For some reason, this prevents autotest from sending messages back to VJ.
 *  Since the connect call in Expproc is fixed, we probably don't need this
 *
 *  timeout.tv_sec=1;
 *  timeout.tv_usec=0;
 *  setsockopt(acqSockfd, SOL_SOCKET,  SO_RCVTIMEO,
 *             (char *) &timeout, sizeof(struct timeval));
 */

    if ( (nchr = read(acqSockfd,tbuffer,sizeof(tbuffer))) > 0)
    { 
        GPRINT1("Received %d characters from socket.\n",nchr);
        GPRINT1("message: '%s'\n",tbuffer);
	for (j=0;j<nchr;i++,j++)
	{
	    if (i > bufsize - 2)
	    {
		buffer[bufsize-1] = '\0';
		i--;
	    }
  	    buffer[i] = tbuffer[j];
	}
    }
#ifdef NESSIE
    if (nchr > 0)
    {
       if (buffer[i-1] != '\n')
       {
          buffer[i] = '\n';
          i++;
          nchr++;
       }
    }
#endif 
    buffer[i] = '\0';
    GPRINT1("Received %d characters from socket.\n",i);
    GPRINT1("message: '%s'\n",buffer);

    if (nchr <= 0)
    {
       close(acqSockfd);
       acqSockfd = -1;
    }
    *read_more = 0;
    nbytes = 0;
    if (acqSockfd != -1)
    {
#ifdef __INTERIX
       if ( fstat(acqSockfd, &sinfo) == 0)
       {
          nbytes = sinfo.st_size;
       }
#else
       if (ioctl( acqSockfd, FIONREAD, &nbytes ) != 0) {
          nbytes = 0;
       }
#endif
    }
    if (nbytes == 0)
    {
       *read_more = 0;
       if (acqSockfd != -1)
          close(acqSockfd);
       acqSockfd = -1;
    }
    else
       *read_more = 1;

    return(nchr);
}
#endif


void setupVnmrAsync(void (*func)())
{
    struct sigaction intserv;
    sigset_t         qmask;

    GPRINT0("setupVnmrAsync: called....\n");
    set_comm_port( &comm_addr[VNMR_COMM_ID] );
 
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGIO );
    sigaddset( &qmask, SIGALRM );
    sigaddset( &qmask, SIGCHLD );
    /* --- set up signal handler --- */
    intserv.sa_handler = func;
    intserv.sa_mask    = qmask;
    intserv.sa_flags   = 0;
    sigaction(SIGIO,&intserv,0L);
}

#ifdef TODO
void ReadAcqAsync()
{
   extern int receive();
   if ( setjmp(saved_stack) != 0 )
   {
      return;
   }
   AcqSocketIsRead(receive);
   longjmp(saved_stack,1);               /* setjmp returns 1 */
}
#endif 

int setMasterInterfaceAsync(void (*func)())
{
    /*fprintf( stderr, "exit set master interface async\n" );*/
        (void) func;
	return( 0 );
}
