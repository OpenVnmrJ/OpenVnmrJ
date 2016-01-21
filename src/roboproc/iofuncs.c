/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#ifndef LINUX
#include <sys/filio.h>
#include <sys/sockio.h>
#endif
#ifdef __INTERIX
#include <sys/time.h>
#endif
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <signal.h>
#include <ctype.h>

#include "iofuncs.h"
#include "termhandler.h"
#include "errLogLib.h"

#ifndef  DEFAULT_SOCKET_BUFFER_SIZE
#define  DEFAULT_SOCKET_BUFFER_SIZE     32768
#endif

#ifndef MILLISEC
#define MILLISEC 1000
#endif

/* used for most robots, and put/getSample for AS768 */
int abortActive = 1;    /* if abort is active */

extern int AbortRobo;
static int as_lan = 0;

/*
 * Port Numbers are hard coded.
 * IP Addresses retrieved from setupInfo file
 */
#define HERMES_ROBOT_PORT    400  /* at setupInfo IP addr   */

#define HERMES_RABBIT_PORT   400  /* at setupInfo IP addr+1 */
#define HERMES_BARCODE_PORT  401  /* at setupInfo IP addr+1 */
#define HERMES_GILSON_PORT   402  /* at setupInfo IP addr+1 */
#define AS4896_PORT   10100

#define FLAGS_BLOCK    0x01
#define FLAGS_ASYNC    0x02

ioDev devTable [] = {
  /* Defaults to the first entry in this list when no smsport file exists */
  { "SMS_TTYA",
        open_serial, read_serial, write_serial, close_serial, ioctl_serial },
  { "SMS_TTYB",
        open_serial, read_serial, write_serial, close_serial, ioctl_serial },
  { "SMS_COM1",
        open_serial, read_serial, write_serial, close_serial, ioctl_serial },
  { "ASM_TTYA",
        open_serial, read_serial, write_serial, close_serial, ioctl_serial },
  { "ASM_TTYB",
        open_serial, read_serial, write_serial, close_serial, ioctl_serial },
  { "GIL_TTYA",
        open_serial, read_serial, write_serial, close_serial, ioctl_serial },
  { "GIL_TTYB",
        open_serial, read_serial, write_serial, close_serial, ioctl_serial },
  { "GIL_COM1",
        open_serial, read_serial, write_serial, close_serial, ioctl_serial },
  { "NMS_TTYA",
        open_serial, read_serial, write_serial, close_serial, ioctl_serial },
  { "NMS_TTYB",
        open_serial, read_serial, write_serial, close_serial, ioctl_serial },
  { "NMS_COM1",
        open_serial, read_serial, write_serial, close_serial, ioctl_serial },
  { "HRM_ROBOT",
        open_lan,    read_lan,    write_lan,    close_lan,    ioctl_lan    },
  { "HRM_GILSON",
        open_lan,    read_lan,    write_lan,    close_lan,    ioctl_lan    },
  { "HRM_BARCODE",
        open_lan,    read_lan,    write_lan,    close_lan,    ioctl_lan    },
  { "HRM_RABBIT",
        open_lan,    read_lan,    write_lan,    close_lan,    ioctl_lan    },
  { "AS4896",
        open_as_lan, read_lan,    write_lan,    close_lan,    ioctl_lan    },
  { 0,               0,           0,            0,            0            }
};

#define MAX_LANDEVICES 10
typedef struct _t_LANDEV {
  int valid;
  int sock;
  int strip;
} LANDEV;

static LANDEV lanDevice[MAX_LANDEVICES];
static int numLanDevices = 0;

#define FIND_LANDEV(sock, thisDev) \
{   int __i; \
    for (__i=0; __i<MAX_LANDEVICES; __i++) { \
        if (lanDevice[__i].valid && (lanDevice[__i].sock == sock)) { \
            thisDev = &lanDevice[__i]; \
            break; \
        } \
    }\
}

#define FIND_EMPTY_LANDEV(thisDev) \
{   int __i; \
    for (__i=0, thisDev = NULL; __i<MAX_LANDEVICES; __i++) { \
        if (!lanDevice[__i].valid) { \
            thisDev = &lanDevice[__i]; \
            break; \
        } \
    }\
}

int open_hermes(int blocking, int async, char *ip, int port)
{
    int one, ival, pid, flags;
    int optlen, rcvbuff, sendbuff;
    int sock, result;
    struct sockaddr_in sin;


    if ((sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP )) < 0) {
        errLogSysRet(ErrLogOp, debugInfo, "create socket" );
        return( -1 );
    }

#if ( !defined(CYGWIN) && !defined(__INTERIX) )
    pid = getpid();
    if ((ival = ioctl( sock, FIOSETOWN, &pid )) < 0) {
        errLogSysRet(ErrLogOp, debugInfo, "set ownership" );
        return( -1 );
    }

    if ((ival = ioctl( sock, SIOCSPGRP, &pid )) < 0) {
        errLogSysRet(ErrLogOp, debugInfo, "set socket's process group" );
        return( -1 );
    }
#endif

    one = 1;
#ifndef LINUX
    if ((ival = setsockopt( sock, SOL_SOCKET, SO_USELOOPBACK,
                            (char *) &one, sizeof( one ) )) < 0) {
        errLogSysRet(ErrLogOp, debugInfo, "use loopback" );
        return( ival );
    }
#endif

    optlen = sizeof(sendbuff);
    sendbuff = DEFAULT_SOCKET_BUFFER_SIZE;
    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF,
                   (char *) &(sendbuff), optlen) < 0) {
        errLogSysRet(ErrLogOp, debugInfo,
                     "tcp_socket(), SO_SNDBUF setsockopt error");
        return( -1 );
    }

    optlen = sizeof(rcvbuff);
    rcvbuff = DEFAULT_SOCKET_BUFFER_SIZE;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF,
                   (char *) &(rcvbuff), optlen) < 0) {
        errLogSysRet(ErrLogOp, debugInfo,
                     "tcp_socket(), SO_RCVBUF setsockopt error");
        return( -1 );
    }

    /* Set Socket Blocking or Non-blocking */
    if ((flags = fcntl( sock, F_GETFL, 0 )) == -1) {
        errLogSysRet(ErrLogOp, debugInfo,
                     "set blocking, can't get current setting" );
        return( -1 );
    }

    /* Got mode bits, now set non/blocking */
    flags = (blocking ? (flags & ~FNDELAY) : (flags | FNDELAY));
    if (fcntl( sock, F_SETFL, flags ) == -1) {
        errLogSysRet(ErrLogOp, debugInfo,
                     "set blocking, can't change current setting" );
        return( -1 );
    }

    /* Set socket to be asynchronous or synchronous
     * If asynchronous, SIGIO delivered when data arrives
     */
#if defined(LINUX) && !defined(CYGWIN)
    ival = fcntl(sock, F_GETFL);
    ival |= O_ASYNC;
    fcntl(sock, F_SETFL, ival);
#else  /* Solaris or Cygwin */
    if ( (ival = ioctl( sock, FIOASYNC, &async )) < 0) {
        errLogSysRet(ErrLogOp, debugInfo, "set synchronous" );
        return -1;
    }
#endif

    memset( (char *) &sin, 0, sizeof( sin ) );
    sin.sin_port = htons((unsigned short)port);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(ip);

    if ((result = connect( sock, (struct sockaddr *) &sin,
                           sizeof( sin ) ) ) < 0) {
        DPRINT2(1, "connect error = %d, ip=%s\n", result, ip);
        errLogSysRet(ErrLogOp, debugInfo, "remote host connect failed" );
        shutdown( sock, 2 );
        close( sock );
        return -1;
    }

    return sock;
}

#define HERMES_ROBOT_IP      "172.16.0.242"
#define HERMES_BARCODE_IP    "172.16.0.244"
#define HERMES_GILSON_IP     "172.16.0.244"
#define HERMES_RABBIT_IP     "172.16.0.244"

void hermesReadIpAddress(char *dev, char *ipAddr)
{
    FILE *setupFile;
    char *setupFname = getenv("vnmrsystem");
    char fname[255];
    int len, i, subnet;

    if (!strcmp(dev, "ROBOT"))
        strcpy(ipAddr, HERMES_ROBOT_IP);
    else if (!strcmp(dev, "GILSON"))
      strcpy(ipAddr, HERMES_GILSON_IP);
    else if (!strcmp(dev, "BARCODE"))
      strcpy(ipAddr, HERMES_BARCODE_IP);
    else
      strcpy(ipAddr, HERMES_RABBIT_IP);

    if (setupFname == NULL) {
        DPRINT(0, "\r\nEnvironment variable vnmrsystem not set!\r\n");
        return;
    }

    /* Read setupInfo file to get IP address */
    strcpy(fname, setupFname);
    strcpy(fname+strlen(fname), "/asm/info/setupInfo");

    if ((setupFile = fopen(fname, "r")) == NULL) {
        DPRINT1(0, "\r\nError opening 768AS setup file %s\r\n", fname);
        return;
    }

    /* Read IP Address from Setup File */
    if (fscanf(setupFile, "IP Address: %s\r\n", ipAddr) != 1) {
        DPRINT3(0, "\r\nError reading 768AS setup file %s %d %s\r\n",
                   fname, errno, strerror(errno));
        fclose(setupFile);
        return;
    }

    len = strlen(ipAddr);
    if (ipAddr[len-3] == '2'){
    	if (ipAddr[len-2] == 'x')
        	ipAddr[len-2] = '4';
    	if (ipAddr[len-1] == 'x')
        	ipAddr[len-1] = '2';
    } else {
    	if (ipAddr[len-2] == 'x')
        	ipAddr[len-2] = '0';
    	if (ipAddr[len-1] == 'x')
        	ipAddr[len-1] = '0';
    }

    fclose(setupFile);

    /* This is the Robot IP Address */
    if (!strcmp(dev, "ROBOT")) {
        DPRINT2(0, "\r\n%s IP = %s\r\n", dev, ipAddr);
        return;
    }

    if (ipAddr[len-3] == '2'){
    /* IP Address of other peripherals is the robot IP + 2 */
    	for (i=len; i>0; i--) {
        	if (ipAddr[i] == '.')
            	break;
    	}
    	if (i > 0) {
        	subnet = atoi(ipAddr+i+2);
        	sprintf(ipAddr+i+2, "%d", subnet+2);
    	}
    
    } else {
    /* IP Address of other peripherals is the robot IP + 1 */
    	for (i=len; i>0; i--) {
        	if (ipAddr[i] == '.')
            	break;
    	}
    	if (i > 0) {
        	subnet = atoi(ipAddr+i+1);
        	sprintf(ipAddr+i+1, "%d", subnet+1);
    	}
    }

    DPRINT2(0, "\r\n%s IP = %s\r\n", dev, ipAddr);
    return;
}

int open_as_lan(char *devName)
{
    LANDEV *thisDev;
    struct hostent *hp;
    struct sockaddr_in recv;

    DPRINT1(1, "Sample changer type: %s\n", devName);

    /* Find an Empty Slot in the Lan Device Table */
    FIND_EMPTY_LANDEV(thisDev);
    if (thisDev == NULL)
       /* Too Many LAN Devices!? */
       return -1;

    as_lan = 1;
    if ( (hp = gethostbyname("V-Autosampler")) == NULL)
    {
       DPRINT1(1,"Hostname for %s (V-Autosampler) not found\n", devName);
       return -1;
    }
    memcpy( &recv.sin_addr, hp->h_addr, hp->h_length);
    DPRINT1(1,"hp->h_name: %s\n", hp->h_name);
    DPRINT1(1,"hp->h_length: %d\n", hp->h_length);
    DPRINT1(1,"hp->h_addr: %p\n", hp->h_addr);
    DPRINT1(1,"IP (h_addr)= %s\n", inet_ntoa(recv.sin_addr));


    thisDev->sock = open_hermes(1 /* block */, 0 /* synch */,
                                    inet_ntoa(recv.sin_addr), AS4896_PORT);
    if (thisDev->sock > 0) {
       numLanDevices++;
       thisDev->valid = 1;
    }

    return (thisDev->sock);
}

int open_lan(char *devName)
{
    LANDEV *thisDev;
    char ipAddr[16];

    as_lan = 0;
    if (strstr(devName, "ROBOT") != NULL) {

        diagPrint(NULL, "Sample changer type: %s\n", devName);

        /* Find an Empty Slot in the Lan Device Table */
        FIND_EMPTY_LANDEV(thisDev);
        if (thisDev == NULL)
            /* Too Many LAN Devices!? */
            return -1;

        hermesReadIpAddress("ROBOT", ipAddr);

        thisDev->sock = open_hermes(1 /* block */, 0 /* synch */,
                                    ipAddr, HERMES_ROBOT_PORT);
        if (thisDev->sock > 0) {
            numLanDevices++;
            thisDev->valid = 1;
        }

        return (thisDev->sock);
    }
    else if (strstr(devName, "GILSON") != NULL) {

        DPRINT(0, "open_lan called for HRM_GILSON\r\n");

        /* Find an Empty Slot in the Lan Device Table */
        FIND_EMPTY_LANDEV(thisDev);
        if (thisDev == NULL)
            /* Too Many LAN Devices!? */
            return -1;

        thisDev->strip = 1;

        hermesReadIpAddress("GILSON", ipAddr);
        DPRINT1(0, "HRM_GILSON is at ip address %s\r\n", ipAddr);

        thisDev->sock = open_hermes(1 /* block */, 0 /* synch */,
                                    ipAddr, HERMES_GILSON_PORT);

        if (thisDev->sock > 0) {
            numLanDevices++;
            thisDev->valid = 1;
        }

        return (thisDev->sock);
    }
    else if (strstr(devName, "BARCODE") != NULL) {

        /* Find an Empty Slot in the Lan Device Table */
        FIND_EMPTY_LANDEV(thisDev);
        if (thisDev == NULL)
            /* Too Many LAN Devices!? */
            return -1;

        hermesReadIpAddress("BARCODE", ipAddr);

        thisDev->sock = open_hermes(1 /* block */, 0 /* synch */,
                                    ipAddr, HERMES_BARCODE_PORT);

        thisDev->strip = 0;
        if (thisDev->sock > 0) {
            numLanDevices++;
            thisDev->valid = 1;
        }

        return (thisDev->sock);
    }
    else if (strstr(devName, "RABBIT") != NULL) {

        /* Find an Empty Slot in the Lan Device Table */
        FIND_EMPTY_LANDEV(thisDev);
        if (thisDev == NULL)
            /* Too Many LAN Devices!? */
            return -1;

        hermesReadIpAddress("RABBIT", ipAddr);

        thisDev->sock = open_hermes(1 /* block */, 0 /* synch */,
                                    ipAddr, HERMES_RABBIT_PORT);

        thisDev->strip = 0;
        if (thisDev->sock > 0) {
            numLanDevices++;
            thisDev->valid = 1;
        }

        return (thisDev->sock);
    }
    return -1;
}


int lan_socktxrdy(int sock, int milliseconds)
{
    fd_set fdWrite, fdError;
    struct timeval tmout;
    
    FD_ZERO(&fdWrite);
    FD_ZERO(&fdError);
    FD_SET(sock, &fdWrite);
    FD_SET(sock, &fdError);

    memset((void *)&tmout, 0, sizeof(tmout));
    tmout.tv_sec = milliseconds / MILLISEC;
    tmout.tv_usec = (milliseconds % MILLISEC) * MILLISEC;

    if (select(10, 0, &fdWrite, &fdError, &tmout) == -1)
        /* Indicate error */
        return -1;

    if (FD_ISSET(sock, &fdWrite) || FD_ISSET(sock, &fdWrite))
        /* Indicate ready to write */
        return 1;

    /* Indicate timeout */
    return 0;
}

int lan_sockrxrdy(int sock, int milliseconds)
{
    int timerExpired = 0, secondsLeft;
    time_t orig_t, exp_t;
    struct timeval tmVal;
    fd_set rdFds;
#ifndef LINUX
    time_t t;
    char tbuf[48], orig_tbuf[48];
#endif
    int result, seconds = milliseconds / 1000;

    orig_t = time(0);
    exp_t = orig_t + seconds;
    secondsLeft = ((seconds <= 0) ? 1 : seconds);
    DPRINT1(1, "read_lan_timed: seconds=%d\r\n", secondsLeft);

    while (!timerExpired) {

        FD_ZERO(&rdFds);
        FD_SET(sock, &rdFds);

        memset(&tmVal, 0, sizeof(tmVal));
        tmVal.tv_sec = secondsLeft;
        tmVal.tv_usec = 0;

        result = select(FD_SETSIZE, &rdFds, NULL, NULL, &tmVal);

        if ((result > 0) && (FD_ISSET(sock, &rdFds))) {
            /* Data is pending at the socket */
            return result;
        }

        else if (result < 0) {
            if (errno == EINTR) {
                secondsLeft = exp_t - time(0);
                if (secondsLeft <= 0)
                    timerExpired = 1;
                else if ((AbortRobo == 1) && !as_lan)
                    timerExpired = 1;
            }
            else {
                DPRINT2(1, "Robot Read Error: result=%d errno=%d\r\n",
                    result, errno);
                return result;
            }
        }
        else {
            DPRINT(1, "Robot Read Error TIMEOUT\r\n");
            timerExpired = 1;
        }
    }

#ifdef DEBUG
#ifndef LINUX
    t = time(0);
    cftime(tbuf, "%T", &t);
    cftime(orig_tbuf, "%T", &orig_t);

    if (AbortRobo == 1) {
        DPRINT2(1, "Robot Read ABORTED, %s...%s\r\n",
                orig_tbuf, tbuf);
    }
    else {
        DPRINT2(1, "Robot Read Error TIMEOUT, %s...%s\r\n",
                orig_tbuf, tbuf);
    }
#endif
#endif
    return 0; /* timeout */
}


int read_lan_timed(int sock, void *buf, int len, int msTimeout)
{
    int result = lan_sockrxrdy(sock, msTimeout);

    if (result <= 0)
        return result;
    else
        return (read(sock, buf, len));
}


int read_lan(int sock, void *buf, int len)
{
    if (lan_sockrxrdy(sock, 3000 /* milliseconds */) <= 0)
        return -1;
    else {
        int retVal, i;
        LANDEV *thisDev = NULL;

        retVal = read(sock, buf, len);

        FIND_LANDEV(sock, thisDev);
        if (thisDev->strip) {
            char *ptr = (char *)buf;
            for (i=0; i<len; i++)
              ptr[i] &= 0x7F;
        }

        return retVal;
    }
}

int write_lan(int sock, void *buf, int len)
{
    return (write(sock, buf, len));
}

int close_lan(int sock)
{
    LANDEV *thisDev = NULL;
    FIND_LANDEV(sock, thisDev);

    if (thisDev != NULL) {
        thisDev->valid = 0;
        numLanDevices--;
    }

    shutdown( sock, 2 );
    close( sock );
    return 0;
}

#if 0  /*---- This worked on the Gilson ----*/
int ioctl_lan(int sock, int action, ...)
{
    char buf[1];

    switch (action) {
    case IO_FLUSH:

        if (lan_sockrxrdy(sock, 250) > 0) {
            while (lan_sockrxrdy(sock, 0) > 0)
                read(sock, buf, 1);
        }

        /* Not sure this really does anything */
        if (ioctl(sock, I_FLUSH, FLUSHRW)< 0) {
            errLogSysRet(ErrLogOp, debugInfo, "flush" );
            return -1;
        }
        break;

    case IO_DRAIN:
        while (lan_socktxrdy(sock, 250 /* milliseconds */) <= 0) {
            /* not sure this really works */ ;
        }
        break;

    default:
        break;
    }

    return 0;
}

#else
int ioctl_lan(int sock, int action, ...)
{
    fd_set rdFds;
    struct timeval tmVal;
    char buf[1];

    switch (action) {
    case IO_FLUSH:
        FD_ZERO(&rdFds);
        FD_SET(sock, &rdFds);
        memset(&tmVal, 0, sizeof(tmVal));

        while (select(FD_SETSIZE, &rdFds, NULL, NULL, &tmVal) > 0) {
            if (read(sock, buf, 1) <= 0) return 0;

            FD_ZERO(&rdFds);
            FD_SET(sock, &rdFds);
            memset(&tmVal, 0, sizeof(tmVal));
        }

        break;

    default:
        break;
    }

    return 0;
}

#endif

int open_serial(char *devName)
{
    char *serDev;

    if (strstr(devName, "TTYB") != NULL)
      serDev = "/dev/term/b";
    else if  (strstr(devName, "COM1") != NULL)
      serDev = "/dev/ttyS0";
    else
      serDev = "/dev/term/a";

    if (strstr(devName, "SMS_")) {
        return (initPort(serDev, SMS_SERIAL_PORT));
    }
    else if (strstr(devName, "ASM_")) {
        return (initPort(serDev, SMS_SERIAL_PORT));
    }
    else if (strstr(devName, "NMS")) {
        return (initPort(serDev, SMS_SERIAL_PORT));
    }
    else if (strstr(devName, "GIL")) {
        return (initPort(serDev, GILSON215_SERIAL_PORT));
    }
    else {
        return (initPort(serDev, SMS_SERIAL_PORT));
    }
}

int read_serial(int sPort, void *buf, int len)
{
    return (read(sPort, buf, len));
}

int write_serial(int sPort, void *buf, int len)
{
    return (write(sPort, buf, len));
}

int close_serial(int sPort)
{
    restorePort(sPort);
    close(sPort);
    return(0);
}

int ioctl_serial(int fd, int action, ...)
{
    switch (action) {
    case IO_FLUSH:
        return flushPort(fd);

    case IO_DRAIN:
       return drainPort(fd);

    default:
        break;
    }

    return 0;
}


/* Read /vnmr/smsport to get list of supported devices.
 * For the first device in the list and perform an "open".
 * Return pointer to the ioDev entry for the first device in the list.
 */
ioDev *setupSmsComms(char *smsPortPath)
{
    int fd, bytes=0, i=0;
    char devName[10], *chrptr, *portchr, *type, port[30];

    fd = open(smsPortPath,O_RDONLY);
    if (fd == -1) {
        /* NO smsport file, so default to serial port A and SMS */
        DPRINT(-1,"No /vnmr/smsport file, defaulting to /dev/term/a SMS\n");
        return &devTable[0];
    }

    bytes = read(fd, port, sizeof(port));
    DPRINT3(1, "File: '%s', read %d bytes, port: '%c'\n",
            smsPortPath, bytes, port[0]);
    chrptr = &port[0];
    portchr = strtok(chrptr," ");
    *portchr = toupper(*portchr);
    DPRINT1(1, "portchr: '%s'\n", portchr);

    /* test if 'not used' selected, then default to serial port a */
    if ((*portchr == 'N') || (bytes == 0)) {
      *portchr = 'A';
    }

    if (bytes > 4)
    {
      type = strtok(NULL, " ");
      DPRINT1(2, "Sample Changer Type: '%s'\n", type);
    }
    else {
      type = "SMS";
      DPRINT(1, "No Sample Changer Type Specified, Defaulting to 'SMS'\n");
    }
    if (! strncmp(type,"AS4896", 6) )
    {
       strcpy(devName, "AS4896");
    }
    else
    {
       switch (*portchr) {
       case 'A':
       case 'B':
           strncpy(devName, type, 3);
           strncpy(devName+3, "_TTY", 4);
           devName[7] = *portchr;
           devName[8] = '\0';
           break;
       case 'C':
           strncpy(devName, type, 3);
           strncpy(devName+3, "_COM", 4);
           devName[7] = *(portchr+1);
           devName[8] = '\0';
           break;
       case 'E':
           /* Ethernet only applicable to Hermes Robot */
           strncpy(devName, type, 3);
           strncpy(devName+3, "_ROBOT", 7);
           break;
       }
    }

    while (devTable[i].devName != NULL) {
        if (strncmp(devTable[i].devName, devName, strlen(devName)) == 0) {
            return &devTable[i];
        }

        i++;
    }

    DPRINT1(-1, "Unrecognized device in /vnmr/smsport file, "
                "defaulting to %s\n", devTable[0].devName);
    return &devTable[0];
}

/*****************************************************************************
                           a b o r t R o b o t
    Description:
    Called from the SIGUSR2 signal handler.  Vnmr sends a SIGUSR2 to
    abort the sample changer.  Other programs leveraging this code may
    also use this mechanism for halting the sample changer.

*****************************************************************************/
void abortRobot()
{
    char abrt;

    DPRINT(1,"abortRobot() SIGUSR2 Handler: Abort Sample Changer \n");
    if (abortActive)
       AbortRobo = 1;

    if ((smsDevEntry != NULL) && (strncmp(smsDevEntry->devName, "HRM_", 4))
        && (strcmp(smsDevEntry->devName, "AS4896"))) {
        /* Send a Control C to Robot */
        abrt = 3;
        if (smsDev != -1)
            smsDevEntry->write(smsDev, &abrt, 1);
    }

    return;
}
 

/*****************************************************************************
                      s e t u p A b o r t R o b o t
    Description:
    Setup the exception handlers for aborting sample changer, SIGUSR2.

*****************************************************************************/
void setupAbortRobot()
{
    sigset_t            qmask;
    struct sigaction    intquit;
 
    /* --- set up interrupt handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGINT );
    sigaddset( &qmask, SIGALRM );
    sigaddset( &qmask, SIGIO );
    sigaddset( &qmask, SIGCHLD );
    sigaddset( &qmask, SIGQUIT );
    sigaddset( &qmask, SIGPIPE );
    sigaddset( &qmask, SIGALRM );
    sigaddset( &qmask, SIGTERM );
    sigaddset( &qmask, SIGUSR1 );
 
    intquit.sa_handler = abortRobot;
    intquit.sa_mask = qmask;
    intquit.sa_flags = 0;
    sigaction(SIGUSR2,&intquit,0L);
}

