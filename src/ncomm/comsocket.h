/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef COMSOCKET_H_
#define COMSOCKET_H_
#include <netdb.h>
#include "sockets.h"

#ifdef COM_READER
#include "rngBlkLib.h" // only needed for COMREADER
#endif
// add as needed
#define EXPCOM  0
#define AUTOCOM 1

#define COMWRITER 1
#define COMREADER 2

typedef struct _com_socket {
    char target[64];
    char host[64];
    char name[64];
    char info[64];
    int type;
    int pid;
    int port;
#ifdef COM_READER
    char *msg_buffer;
    int msg_buffer_size;
    int msg_size;
    Socket *pListenSocket;
    Socket *pAcceptSocket;
    pthread_t AcceptThreadId;
    pthread_t main_threadId;
    RINGBLK_ID pAcceptQueue;
#endif
} COM_SOCKET, *ComSocket;

#ifdef COM_READER
extern int newComSocketReader(int target, ComSocket comm);
extern int readComSocket(ComSocket comm);
#endif
extern int newComSocketWriter(int target, ComSocket comm);
extern int freeComSocket(ComSocket comm);

extern int writeComSocketFile(ComSocket comm);
extern int deleteComSocketFile(ComSocket comm);
extern int writeComSocket(char *message, ComSocket comm);
extern void printComSocketInfo(ComSocket comm);

#endif /*COMSOCKET_H_*/
