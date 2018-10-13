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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <signal.h>

#include "sockets.h"
#include "errLogLib.h"
#if   !defined(LINUX) && !defined(__INTERIX)
#include <thread.h>
#endif
#include <pthread.h>
#include <signal.h>
#include "comsocket.h"

#ifndef COM_DEBUG
#define COM_DEBUG 0
#endif

#define FREE(x) if(x!=NULL){free(x);x=NULL;}

typedef struct _com_rec {
    const char *procName;
    const char *infoFile;
} COM_RECORD;

// add to list as needed
static COM_RECORD comList[] = {
    "Expproc","acqinfo2",
    "Autoproc","autoinfo"
};


static char *getSystemDir(){
   return getenv("vnmrsystem");
}

static void getHostName(char *host){
    gethostname(host, 64);
}

static int getPid(){
   return getpid();
}

//-------------------------------------------------------------
// printComSocketInfo() 
//    - print out ComSocket information 
//-------------------------------------------------------------
void printComSocketInfo(ComSocket comm){   
    DPRINT5(COM_DEBUG,"ComSocket: target:%s info:%s host:%s port:0x%X pid:%d\n",
            comm->target,comm->info,comm->host,comm->port,comm->pid);  
}
//############## ComSocket writer functions ###################
//-------------------------------------------------------------
// newComSocketWriter() 
//    - initialize a ComSocket structure by reading 
//      information from a file in /vnmr/acqueue
//    - return 0: success -1: failure
//-------------------------------------------------------------
int newComSocketWriter(int type, ComSocket comm) {
    FILE *stream;
    int port1;
    int port2;
    char filepath[256];
    int err=-1;
    
    if(type>=0 && type<sizeof(comList)/sizeof(COM_RECORD)){
        strcpy(comm->target,comList[type].procName);
        strcpy(comm->info,comList[type].infoFile);
    }
    else{
        DPRINT1(COM_DEBUG,"newComSocketWriter: UNKNOWN type:%d\n",type);
        errLogSysRet(ErrLogOp, debugInfo, "illegal comm type");
        return -1;
    }
    comm->pid=-1;
    comm->port=-1;
    comm->type=COMWRITER;
    
    strcpy(filepath, getSystemDir());
    strcat(filepath, "/acqqueue/");
    strcat(filepath, comm->info);

    // fill in host name port and pid by reading info file in /vnmr/acqqueue
    if (stream = fopen( (const char*) filepath, "r")) {
        if (fscanf(stream, "%d%s%d%d%d", &(comm->pid), comm->host, &port1,
                &port2, &(comm->port)) != 5) {
            fclose(stream);
            comm->pid = comm->port = -1;
            strcpy(comm->host, "");
         } else {
            fclose(stream);
            comm->port = 0xFFFF & htons(comm->port);
            struct hostent *hp = gethostbyname(comm->host);
            if (hp == NULL) {
                errLogSysRet(ErrLogOp, debugInfo, "%s Unknown Host", comm->host);
                return -1;
            }
            err=0;
            DPRINT5(COM_DEBUG,"newComSocketWriter target:%s info:%s host:%s port:0x%X pid:%d\n",
                    comm->target, comm->info, comm->host,comm->port,comm->pid);
        }
    } 
    return err;
}
//-------------------------------------------------------------
// writeComSocket() 
//    - send a message through a ComSocket socket
//    - return 0: success -1: failure
//-------------------------------------------------------------
int writeComSocket(char *message, ComSocket comm) {

    if (message == NULL || strlen(message)<1 || comm->port<0)
        return ( -1 );
    int mlen=strlen(message);
    int ival;
    char msgstring[256];
    
    int replyPortAddr=comm->port;
    char *hostname=comm->host;
    Socket *pReplySocket;

    sprintf(msgstring, "%s", message);
    mlen=strlen(msgstring);

    pReplySocket = createSocket(SOCK_STREAM);
    if (pReplySocket == NULL)
        return ( -1 );
    ival = openSocket(pReplySocket);
    if (ival != 0)
        return ( -1 );
    int port=0xFFFF& htons(replyPortAddr) ;
    ival = connectSocket(pReplySocket, hostname, port );
    if (ival != 0){
        DPRINT2(COM_DEBUG,"writeComSocketMsg: FAILED port:0x%X msg:'%s'\n",port,msgstring);
        return ( -1 );
    }
    DPRINT2(COM_DEBUG,"writeComSocketMsg: target:%s msg:'%s'\n",comm->target,msgstring);
    writeSocket(pReplySocket, (char*) &mlen, sizeof(mlen));
    writeSocket(pReplySocket, msgstring, mlen);
    closeSocket(pReplySocket);
    free(pReplySocket);
    return ( 0 );
}

//-------------------------------------------------------------
// writeComSocketFile() 
//    - initialize a ComSocket info file in /vnmr/acqueue  
//    - return 0: success -1: failure
//-------------------------------------------------------------
int writeComSocketFile(ComSocket comm) {
    char buff[ 256 ];
    char filepath[256];
    int bytes, fd;

    sprintf(buff, "%d %s -1 -1 %d", comm->pid, comm->host, comm->port);

    strcpy(filepath, getSystemDir());
    strcat(filepath, "/acqqueue/");
    strcat(filepath, comm->info);
    if ( (fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
        errLogSysRet(ErrLogOp, debugInfo,
                "Could Not Open ComSocket Info File: '%s'\n", filepath);
        return -1;
    }
    bytes = write(fd, buff, strlen(buff)+1);
    if ( (bytes == -1)) {
        errLogSysRet(ErrLogOp, debugInfo,
                "Could Not Write ComSocket Info File: '%s'\n", filepath);
        return -1;
    }
    close(fd);
    return 0;
}

//-------------------------------------------------------------
// deleteComSocket() 
//    - remove info file in /vnmr/acqueue  
//    - return 0: success -1: failure
//-------------------------------------------------------------
int deleteComSocketFile(ComSocket comm) {
    char filepath[256];

    strcpy(filepath, getSystemDir());
    strcat(filepath, "/acqqueue/");
    strcat(filepath, comm->info);
    unlink(filepath);
    FILE *stream= fopen( (const char*) &filepath[ 0 ], "r");
    if (stream>0) {
        errLogSysRet(ErrLogOp, debugInfo,
                "Could Not Remove Comm Info File: '%s'\n", filepath);
        fclose(stream);
        DPRINT1(COM_DEBUG,"deleteComSocketFile (%s) failed \n",filepath);
        return -1;
    }
    DPRINT1(COM_DEBUG,"deleteComSocketFile (%s) suceeded \n",comm->info);
    return 0;
}

//-------------------------------------------------------------
// freeComSocket() 
//    release comm socket resources
//    COMREADER:
//    - remove info file in /vnmr/acqueue  
//    - return 0: success -1: failure
//-------------------------------------------------------------
int freeComSocket(ComSocket comm) {
#ifdef COM_READER
    if(comm->type==COMREADER){
        char filepath[256];
        deleteComSocketFile(comm);
    }
    FREE(comm->msg_buffer);
    comm->msg_buffer_size=comm->msg_size=0;
    comm->port=-1;
#endif
    return 0;
}

#ifdef COM_READER

//############## ComSocket reader functions ###################
//-------------------------------------------------------------
// AcceptCommConnection() 
//   - read a single ComSocket message
//   - adapted from AcceptConnection in expsocket.c
//-------------------------------------------------------------
static void *AcceptCommConnection(ComSocket comm) {
    int result;
    Socket *pAcceptSocket=NULL;
    static int errcount=0;

    // if thread terminates no need to join it to recover resources
    
    pthread_detach(comm->AcceptThreadId); 

    for (;;) {
        pAcceptSocket = (Socket *) malloc(sizeof(Socket));
        if (pAcceptSocket == NULL) 
            return ( NULL );
       
        memset(pAcceptSocket, 0, sizeof(Socket));
        result = acceptSocket_r(comm->pListenSocket, pAcceptSocket);
        if (result < 0) {
            if(errcount==0)
                errLogSysRet(ErrLogOp, debugInfo, "acceptSocket_r");
            errcount++;
        } else {
            DPRINT2(1,"AcceptCommConnection: 0x%lx, fd: %d\n", pAcceptSocket,pAcceptSocket->sd);
            rngBlkPut(comm->pAcceptQueue, &pAcceptSocket, 1);
            // signal socket msg arrival to main thread
            pthread_kill(comm->main_threadId, SIGIO); 
        }
    }
}
//-------------------------------------------------------------
// readComSocketMsg() 
//   - read a single ComSocket message
//   - adapted from in expsocket.c
//-------------------------------------------------------------
static int readComSocketMsg(ComSocket comm) {
    long msgeSize;
    int bcount;
    
    comm->msg_size=0;

    bcount = readSocketNonblocking(comm->pAcceptSocket, (char*) &msgeSize, sizeof(long));
    DPRINT2(+2,"readComSocketMsg: bcount: %d, msgesize:  %d\n",bcount,msgeSize);
    if (bcount > 0) {
        if (msgeSize >= comm->msg_buffer_size) {
            FREE(comm->msg_buffer);
            comm->msg_buffer_size=msgeSize+1;
            comm->msg_buffer = malloc(comm->msg_buffer_size);
            DPRINT1(1,"readComSocketMsg: Mallocing msg buffer (%d)\n",comm->msg_buffer_size);
        } 
        // Had to block signals because were getting interrupted system call errors
        bcount = readSocket(comm->pAcceptSocket, comm->msg_buffer, msgeSize);
        comm->msg_size=bcount;
        if (bcount > 0) {
            comm->msg_buffer[ bcount ] = '\0';
            if(DebugLevel>COM_DEBUG){
                #define MAXMSG 63
                char tmp[MAXMSG+1];
                int n=bcount<MAXMSG?bcount:MAXMSG;
                int i;
                strncpy(tmp,comm->msg_buffer,n);
                for(i=0;i<n;i++){
                    if(comm->msg_buffer[i]=='\n')
                        tmp[i]=' ';
                    else
                        tmp[i]=comm->msg_buffer[i];                       
                }
                tmp[i]=0;
                DPRINT2(COM_DEBUG,"readComSocketMsg: target: %s '%s'\n",comm->target,tmp);
            }

        } else {
            errLogSysRet(ErrLogOp, debugInfo,
                    "readComSocketMsg: Read of Async Message failed: ");
            errLogRet(ErrLogOp, debugInfo,
                    "readComSocketMsg: Message Ignored.\n");
        }

    } else {
        errLogRet(ErrLogOp, debugInfo,
                "readComSocketMsg: No Message,  Ignored.\n");
    }
    closeSocket(comm->pAcceptSocket);
    return comm->msg_size;
}


//-------------------------------------------------------------
// newComSocketReader() 
//    - initialize a ComSocket structure and listener Socket using 
//      system information for the current process
//    - write port info file in /vnmr/acqueue
//    - return 0: success, -1: failure
//-------------------------------------------------------------
int newComSocketReader(int type, ComSocket comm) {
    int status, ival;
    int applPort;
    comm->pListenSocket=NULL;

    if(type>=0 && type<sizeof(comList)/sizeof(COM_RECORD)){
        strcpy(comm->target,comList[type].procName);
        strcpy(comm->info,comList[type].infoFile);
    }
    else{
        DPRINT1(COM_DEBUG,"newComSocketReader: UNKNOWN type:%d\n",type);
        errLogSysRet(ErrLogOp, debugInfo, "illegal comm type");
        return -1;
    }
    comm->pid=-1;
    comm->port=-1;
    comm->type=COMREADER;
        
    comm->msg_buffer_size=0;
    comm->msg_size=0;
    comm->msg_buffer=NULL;
  
    getHostName(comm->host);
    comm->pid=getPid();   
    comm->AcceptThreadId=0;   
    comm->main_threadId=pthread_self();
    
    comm->pListenSocket = createSocket(SOCK_STREAM);
    if (comm->pListenSocket == NULL)
        return -1;
    ival = openSocket(comm->pListenSocket);
    if (ival != 0)
        return -1;
    ival = bindSocketAnyAddr(comm->pListenSocket);
    if (ival != 0) {
        errLogSysRet(ErrLogOp, debugInfo,
                "newComSocketReader: bindSocketAnyAddr failed:");
        return -1;
    }
    applPort = returnSocketPort(comm->pListenSocket);
    ival = listenSocket(comm->pListenSocket);
    if (ival != 0) {
        errLogSysRet(ErrLogOp, debugInfo,
                "newComSocketReader: listenSocket failed:");
        return -1;
    }
    comm->port=applPort;
    comm->pAcceptQueue = rngBlkCreate(128, "AcceptQ", 1); // accepted socket queue

    // create thread to handle the accept 
    status = pthread_create(&comm->AcceptThreadId, NULL, AcceptCommConnection,
            (void*) comm);

    DPRINT5(COM_DEBUG,"newComSocketReader target:%s info:%s host:%s port:0x%X pid:%d\n",
            comm->target, comm->info, comm->host,comm->port,comm->pid);
    return 0;
 }

//-------------------------------------------------------------
// readComSocket() 
//   - read messages from rngBlk buffer
//   - adapted from in expsocket.c 
//   - returns:
//     size of next message (copied into comm->message_buffer)
//     0 if all messages have been read
//     -1 on error
//-------------------------------------------------------------
int readComSocket(ComSocket comm) {
    int nfound;
    fd_set readfd;
    Socket *pAcceptSocket;
    int sockInQ;
    int msg_size=0;

    // while queue is not empty keep reading sockets
    if ((sockInQ = rngBlkNElem(comm->pAcceptQueue)) > 0) {

        DPRINT1(1,"readComSocket:Accepted Sockets in Queue: %d\n",sockInQ);
        rngBlkGet(comm->pAcceptQueue, &pAcceptSocket, 1);
        DPRINT2(+3,"readComSocket: 0x%lx, fd: %d\n", pAcceptSocket,pAcceptSocket->sd);
        FD_ZERO( &readfd );
        FD_SET(pAcceptSocket->sd, &readfd);

        DPRINT1(+3,"readComSocket:readfd mask: 0x%lx\n",readfd);

        // who's got input ?
        try_again: if ( (nfound = select(pAcceptSocket->sd+1, &readfd, 0, 0, 0) )
                < 0) {
            if (errno == EINTR)
                goto try_again;
            else{
                errLogSysRet(ErrLogOp, debugInfo, "select Error:\n");
                return -1;
            }
        }
        DPRINT1(+3,"readComSocket:select: readfd mask: 0x%lx\n",readfd);

        if (nfound < 1){
            free(pAcceptSocket);
            return (0);
        }

        if (FD_ISSET(pAcceptSocket->sd, &readfd)) {
            comm->pAcceptSocket=pAcceptSocket;
            msg_size=readComSocketMsg(comm);
        }
        free(pAcceptSocket);
        return msg_size;
    }
    return 0;
}
#endif // COM_READER
