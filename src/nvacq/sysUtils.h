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

// #define _POSIX_SOURCE /* defined when source is waited tobe POSIX-compliant */
// #define _SYSV_SOURCE /* defined when source is System V */
// #ifdef __STDC__ /* used to determine if using an ANSI compiler */


#ifndef INCsysutilsh
#define INCsysutilsh

#ifdef VXWORKS
#include <semLib.h>
#include <msgQLib.h>
#endif

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* --------- ANSI/C++ compliant function prototypes --------------- */
 
#if defined(__STDC__) || defined(__cplusplus)
 
#ifndef VXWORKS
extern char *get_console_hostname(void);
#endif

extern char *getHostIP(char *hostname, char *localIP);

#ifdef VXWORKS

extern char *getLocalIP(char *localIP);
extern char *getRemoteHostIP(char *hostIP);

extern void msgQInfoPrint(MSG_Q_ID pMsgQ);
extern void semInfoPrint(SEM_ID pSemId,char *msge,int level);
extern void getFutureTime(unsigned int usec, unsigned int *top, unsigned int *bot );
extern int checkTime(unsigned int *top, unsigned int *bot );
extern int execFunc(char *funcName, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6, void *arg7, void *arg8);
long long marksysclk();
double deltaTime(long long end,long long start);
void resetTimeStamp();
void updateTimeStamp(int initflag);
double getTimeStampDurations(double *deltatime, double *durationtime);

#define TSPRINT(strarg) \
             { \
               double delta,duration; \
               updateTimeStamp(0); \
               getTimeStampDurations(&delta,&duration); \
	            diagPrint(debugInfo,"%s delta: %lf usec, duration: %lf usec\n",strarg,delta,duration); \
               updateTimeStamp(0); /* remove time use to perform this printing for next TSPRINT */ \
               /* DPRINT2(-19,"%s delta: %lf usec, duration: %lf usec\n",str,arg1,delta,duration); */ \
             }

#endif

#else
/* --------- NON-ANSI/C++ prototypes ------------  */
 
extern char *getLocalIP();
extern char *getHostIP();
extern void msgQInfoPrint();
extern void semInfoPrint();

#endif
 
#ifdef __cplusplus
}
#endif
 
#endif

