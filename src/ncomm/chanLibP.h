/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
modification history
--------------------
01b,15Aug94    removed unused field in the channel object
01a,15Aug94    written
*/

#ifndef __INCchanLibPh
#define __INCchanLibPh

#ifdef __cplusplus
extern "C" {
#endif

#include "sockets.h"
#include "chanLib.h"

typedef struct _Channel {
	Socket	*pClientCB;
	Socket	*pClientReadS;
	Socket	*pClientWriteS;
	int	 state;
	int	 socketInUse;
} Channel;

#define  CB_PORT	5000

/* definitions for state */

#define  INITIAL	0
#define  ERROR		1
#define  CONNECTED	2

#ifdef __cplusplus
}
#endif

#endif /* __INCchanLibPh */
