/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef SOCKETS_H_
#define SOCKETS_H_

typedef struct _Socket {
	int	sd;
	int	port;
	int	host;
	int	protocol;
	int	direction;
} Socket;

#ifdef __STDC__

/* following include needed for definition of fd_set */
#include <sys/time.h>

extern Socket	*createSocket( int type );
extern int	 openSocket( Socket *pSocket );
extern int 	 bindSocketSearch( Socket *pSocket, int baseAddr );
extern int	 bindSocket( Socket *pSocket, int baseAddr );
extern Socket	*acceptSocket( Socket *pSocket );
extern int	 listenSocket( Socket *pSocket );
extern int	 connectSocket( Socket *pSocket, char *hostName, int portAddr );
extern void	 addMaskForSocket( Socket *pSocket, fd_set *fdmaskp );
extern void	 rmMaskForSocket( Socket *pSocket, fd_set *fdmaskp );
extern int	 isSocketActive( Socket *pSocket, fd_set *fdmaskp );
extern int	 returnSocketPort( Socket *pSocket );
extern int	 setSocketNonblocking( Socket *pSocket );
extern int	 setSocketBlocking( Socket *pSocket );
extern int	 setSocketAsync( Socket *pSocket );
extern int	 setSocketNonAsync( Socket *pSocket );
extern int	 readSocket( Socket *pSocket, char *datap, int bcount );
extern int	 readProtectedSocket( Socket *pSocket, char *datap, int bcount );
extern int	 readSocketNonblocking( Socket *pSocket, char *datap, int bcount );
extern int 	 flushSocket( Socket *pSocket);
extern int	 writeSocket( Socket *pSocket, const char *datap, int bcount );
extern int	 closeSocket( Socket *pSocket );
#else
extern Socket	*createSocket();
extern int	 openSocket();
extern int 	 bindSocketSearch();
extern int	 bindSocket();
extern Socket	*acceptSocket();
extern int	 listenSocket();
extern int	 connectSocket();
extern void	 addMaskForSocket();
extern void	 rmMaskForSocket();
extern int	 isSocketActive();
extern int	 returnSocketPort();
extern int	 setSocketNonblocking();
extern int	 setSocketBlocking();
extern int	 setSocketAsync();
extern int	 setSocketNonAsync();
extern int	 readSocket();
extern int	 readProtectedSocket();
extern int	 readSocketNonblocking();
extern int 	 flushSocket();
extern int	 writeSocket();
extern int	 closeSocket();
#endif
#endif
