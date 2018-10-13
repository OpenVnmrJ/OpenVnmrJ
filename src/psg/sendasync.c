/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include "abort.h"

#define MAXRETRY 8
#define SLEEP 1
#define ERROR 1
#define OK 0

extern int bgflag;  /* debug flag */

static int InitRecverAddr(char *AcqHost, int port, struct sockaddr_in *recver);

/*------------------------------------------------------------
|
|    sendasync()/4
|       connect to  an Async Process's Socket
|       then transmit a message to it and disconnect.
|
+-----------------------------------------------------------*/
int sendasync(char *addr, char *message)
{
    char machine[60];
    int acqpid;
    int port;
    int fgsd;   /* socket discriptor */
    int i;
    int on = 1;
    int result;
    struct sockaddr_in RecvAddr;

    sscanf(addr, "%s %d %d", machine,&port,&acqpid);
    if (bgflag)
      fprintf(stderr,"Sendasync():Machine: '%s',Port: %d, PID: %d,\nMsge: '%s'\n",        machine,port,acqpid,message);

    /* --- initialize the Receiver's Internet Address --- */
    if (InitRecverAddr(machine,port,&RecvAddr) != OK)
        return(ERROR);

    /* --- try several times then fail --- */
    for (i=0; i < MAXRETRY; i++)
    {
        fgsd = socket(AF_INET,SOCK_STREAM,0);        /* create a socket */

        if (fgsd == -1)                                                    
        {
            perror("sendacq(): socket");
            text_error("Error, could not communicate with acquisition\n");
            return(ERROR);
        }
        if (bgflag)
         fprintf(stderr,"sendasync(): socket create for async trans %d\n",fgsd);
#ifndef LINUX
        setsockopt(fgsd,SOL_SOCKET,SO_USELOOPBACK,&on,sizeof(on));
#endif
        /*setsockopt(fgsd,SOL_SOCKET,(~SO_LINGER),&on,sizeof(on));*/

        /* ---- make socket async and set owner to acqproc PID --- */
 
	on = 1;
	/*if (ioctl( fgsd, FIOASYNC, &on ) == -1)
        {
            perror("fcntl error");
            text_error("Error, could not communicate with acquisition\n");
            close(fgsd);
            return(ERROR);
        }*/
        /*if (fcntl(fgsd,F_SETOWN,acqpid) == -1)
        {
            perror("fcntl error");
            text_error("Error, could not communicate with acquisition\n");
            close(fgsd);
            return(ERROR);
        }*/
        if (bgflag)
          fprintf(stderr,
             "sendasync(): socket process group and fd: %d  %d\n",acqpid,fgsd);

       /* --- attempt to connect to the named socket --- */         
       if ((result = connect(fgsd, (struct sockaddr *) &RecvAddr,sizeof(RecvAddr))) != 0)
       {
          /* --- Is Socket queue full ? --- */
          if (errno != ECONNREFUSED && errno != ETIMEDOUT)
          {                             /* NO, some other error */
              perror("sendasync():aborting,  connect error");
              text_error("Error, could not communicate with acquisition\n");
              return(ERROR);
          }
       }                                /* Yes, try MAXRETRY times */
       else     /* connection established */
       {
            break;
       }
       if (bgflag)
         fprintf(stderr,"sendasync(): Socket queue full, will retry %d\n",i);
       close(fgsd);
       sleep(SLEEP);
    }
    if (result != 0)    /* tried MAXRETRY without success  */
    {
       text_error("Maximum communication retrys exceeded\n");
       return(ERROR);
    }
    if (bgflag)
      fprintf(stderr,"Sendacq(): Connection Established \n");
    write(fgsd,message,strlen(message));
 
    close(fgsd);
    return(OK);
}
/*-----------------------------------------------------------------------
|
|       InitRecverAddr()/2
|       Initialize the socket addresses for display and interactive use
|
+-----------------------------------------------------------------------*/
static int InitRecverAddr(char *AcqHost, int port, struct sockaddr_in *recver)
{
    struct hostent *hp;

    /* --- name of the socket so that we may connect to them --- */
    /* read in acquisition host and port numbers to use */
    hp = gethostbyname(AcqHost);
    if (hp == NULL)
    {
       text_error("Unknown Acquisition Host\n");
       return(ERROR);
    }

    /*bzero((char *) recver,sizeof(struct sockaddr_in)); clear socket info */
    memset((char *) recver,0,sizeof(struct sockaddr_in));/* clear socket info */

    /*bcopy(hp->h_addr,(char *)&(recver->sin_addr),hp->h_length);*/
    memcpy( (char *)&(recver->sin_addr), hp->h_addr, hp->h_length);
    recver->sin_family = hp->h_addrtype;
    recver->sin_port = port;
    return(OK);
}

