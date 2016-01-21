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
#include <sys/file.h>

#ifdef UNIX
#include <sys/file.h>           /*  bits for `access', e.g. F_OK */
#else
#define F_OK            0       /* does file exist */
#define X_OK            1       /* is it executable by caller */
#define W_OK            2       /* writable by caller */
#define R_OK            4       /* readable by caller */
#endif

#include <errno.h>

/* for openSocket() etc. */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "sockets.h"

/* for socket() etc. */
#include <strings.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <netdb.h>

#define  MAXPATHL   128
#define  DEFAULT_SOCKET_BUFFER_SIZE     32768


extern int StatPortId;
extern char LocalHost[];
/* extern char RemoteHost[]; */
/* static char statfile[ MAXPATHL ]; */
static Socket sockVnmrJ;


static int createVSocket( type, tSocket )
int type;
Socket *tSocket;
{

        memset( tSocket, 0, sizeof( Socket ) );
        tSocket->sd = -1;
        tSocket->protocol = type;
        return(0);
}

static int sendToVnmrJinit( nodelay, pSD )
int nodelay;
Socket *pSD;
{
    int i, ival, trys, on=1, buff=DEFAULT_SOCKET_BUFFER_SIZE;

    if (pSD == NULL)
      return(-1);
    if (openSocket(pSD) == -1)
      return(-1);
/*  if (setSocketNonblocking(pSD) == -1)
      return(-1); */
    setsockopt(pSD->sd,SOL_SOCKET,(~SO_LINGER),(char *) &on,sizeof(on));
    if (nodelay==1)
      setsockopt(pSD->sd,SOL_SOCKET,TCP_NODELAY,(char *) &on,sizeof(on));
/*  setsockopt(pSD->sd,SOL_SOCKET, SO_SNDBUF, (char *) &(buff), sizeof(buff));
    setsockopt(pSD->sd,SOL_SOCKET, SO_RCVBUF, (char *) &(buff), sizeof(buff)); */

    trys = 5;
    for( i=trys; i>=0; i--)
    {
      ival = connectSocket(pSD,LocalHost,StatPortId);
      if (ival == 0)
      {
        break;
      }
      else
      {
        if (i==0)
          return(-1);
        sleep(1);
      }
    }
    return(0);
}

static int messageSendToVnmrJ(Socket *pSD, char* message, int messagesize)
{
   int bytes;
   bytes = writeSocket( pSD, message, messagesize );
   return(0); /* return(bytes); */
}

/* called by disp_string() disp_val() in statdispfuncs.c */
int writestatToVnmrJ( char *cmd, char *message ) 
{
    char mstr[1024];
    int val;
    {
      if (sockVnmrJ.sd == -1)
      {
        sendToVnmrJinit(1, &sockVnmrJ);
	strcpy(mstr,"infostat init\n");
        if (sockVnmrJ.sd != -1)
          messageSendToVnmrJ( &sockVnmrJ, mstr, strlen(mstr));
      }
    }
    sprintf(mstr,"%s %s",cmd,message);
    val = strlen(mstr);
    if (mstr[val] != '\n')
      strcat(mstr,"\n");
    if (sockVnmrJ.sd != -1)
      messageSendToVnmrJ( &sockVnmrJ, mstr, strlen(mstr));
    return(0);
}

int initStatSocket() /* called by main() in statusscrn.c */
{
/*	sprintf( statfile, "/tmp/infostat%d", StatPortId ); */
	createVSocket( SOCK_STREAM, &sockVnmrJ );
/*	writestatToVnmrJ( "infostat", "start" ); */
	return( 0 );
}
