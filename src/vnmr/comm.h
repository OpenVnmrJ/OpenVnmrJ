/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
 * Definitions for Socket style connections to acquisition
 */

#ifndef INCcommh
#define INCcommh

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct _comm_info {
    char vnmrsysdir[256];
    char path[256*2];
    char host[256];
    int  pid;
    int  port;
    struct sockaddr_in messname;
    int                msgesocket;
    int  msg_uid;
    } COMM_INFO_STRUCT;

typedef COMM_INFO_STRUCT *CommPort;

extern int SendAsync(CommPort to_addr, CommPort from_addr, int cmd, char *args);

#endif
