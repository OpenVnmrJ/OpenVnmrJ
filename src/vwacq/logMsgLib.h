/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INClogmsglibh
#define INClogmsglibh


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LOGMSG_LENGTH 513
#define STD_PORT  1
#define ERR_PORT  2
#define LOG_PORT  3
#define ALL_PORTS 4

#define LOGIT 1
#define NOLOG 0

extern int acqerrno;	/* acquisition error number , like unix errno */
extern int DebugLevel;

#ifdef DEBUG

#define debugInfo (fileNline(__FILE__,__LINE__))

#define DPRINT(level, str) \
        if (DebugLevel > level) diagPrint(debugInfo,str)

#define DPRINT1(level, str, arg1) \
        if (DebugLevel > level) diagPrint(debugInfo,str,arg1)

#define DPRINT2(level, str, arg1, arg2) \
        if (DebugLevel > level) diagPrint(debugInfo,str,arg1,arg2)
 
#define DPRINT3(level, str, arg1, arg2, arg3) \
        if (DebugLevel > level) diagPrint(debugInfo,str,arg1,arg2,arg3)
 
#define DPRINT4(level, str, arg1, arg2, arg3, arg4) \
        if (DebugLevel > level) diagPrint(debugInfo, str,arg1,arg2,arg3,arg4)
 
#define DPRINT5(level, str, arg1, arg2, arg3, arg4, arg5 ) \
        if (DebugLevel > level) diagPrint(debugInfo,str,arg1,arg2,arg3,arg4,arg5)
 
#define DPRINT6(level, str, arg1, arg2, arg3, arg4, arg5, arg6 ) \
        if (DebugLevel > level) diagPrint(debugInfo,str,arg1,arg2,arg3,arg4,arg5,arg6)
 
#define DPRINT7(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7 ) \
   	if (DebugLevel > level)  \
	  diagPrint(debugInfo,str,arg1,arg2,arg3,arg4,arg5,arg6,arg7)
 
#define DPRINT8(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 ) \
   	if (DebugLevel > level)  \
	  diagPrint(debugInfo,str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8)
 
#define DPRINT9(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 ) \
        if (DebugLevel > level)  \
	  diagPrint(debugInfo,str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9)
 
#else

#define DPRINT(level, str)
#define DPRINT1(level, str, arg2)
#define DPRINT2(level, str, arg1, arg2)
#define DPRINT3(level, str, arg1, arg2, arg3)
#define DPRINT4(level, str, arg1, arg2, arg3, arg4)
#define DPRINT5(level, str, arg1, arg2, arg3, arg4, arg5)
#define DPRINT6(level, str, arg1, arg2, arg3, arg4, arg5, arg6 )
#define DPRINT7(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7 )
#define DPRINT8(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 )
#define DPRINT9(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 )
 
#define debugInfo NULL
 
#endif 

/* syslog.h defines put here to avoid having to includse a UNIX header file */
/*
 *  Facility codes
 */
#define LOG_KERN        (0<<3)  /* kernel messages */
#define LOG_USER        (1<<3)  /* random user-level messages */
#define LOG_MAIL        (2<<3)  /* mail system */
#define LOG_DAEMON      (3<<3)  /* system daemons */
#define LOG_AUTH        (4<<3)  /* security/authorization messages */
#define LOG_SYSLOG      (5<<3)  /* messages generated internally by syslogd */
#define LOG_LPR         (6<<3)  /* line printer subsystem */
#define LOG_NEWS        (7<<3)  /* netnews subsystem */
#define LOG_UUCP        (8<<3)  /* uucp subsystem */
#define LOG_CRON        (15<<3) /* cron/at subsystem */
        /* other codes through 15 reserved for system use */
#define LOG_LOCAL0      (16<<3) /* reserved for local use */
#define LOG_LOCAL1      (17<<3) /* reserved for local use */
#define LOG_LOCAL2      (18<<3) /* reserved for local use */
#define LOG_LOCAL3      (19<<3) /* reserved for local use */
#define LOG_LOCAL4      (20<<3) /* reserved for local use */
#define LOG_LOCAL5      (21<<3) /* reserved for local use */
#define LOG_LOCAL6      (22<<3) /* reserved for local use */
#define LOG_LOCAL7      (23<<3) /* reserved for local use */

#define LOG_NFACILITIES 24      /* maximum number of facilities */
#define LOG_FACMASK     0x03f8  /* mask to extract facility part */

/*
 *  Priorities (these are ordered)
 */
#define LOG_EMERG       0       /* system is unusable */
#define LOG_ALERT       1       /* action must be taken immediately */
#define LOG_CRIT        2       /* critical conditions */
#define LOG_ERR         3       /* error conditions */
#define LOG_WARNING     4       /* warning conditions */
#define LOG_NOTICE      5       /* normal but signification condition */
#define LOG_INFO        6       /* informational */
#define LOG_DEBUG       7       /* debug-level messages */

#define LOG_PRIMASK     0x0007  /* mask to extract priority part (internal) */

/*
 * arguments to setlogmask.
 */
#define LOG_MASK(pri)   (1 << (pri))            /* mask for one priority */
#define LOG_UPTO(pri)   ((1 << ((pri)+1)) - 1)  /* all priorities through pri */

/*
 *  Option flags for openlog.
 *
 *      LOG_ODELAY no longer does anything; LOG_NDELAY is the
 *      inverse of what it used to be.
 */
#define LOG_PID         0x01    /* log the pid with each message */
#define LOG_CONS        0x02    /* log on the console if errors in sending */
#define LOG_ODELAY      0x04    /* delay open until syslog() is called */
#define LOG_NDELAY      0x08    /* don't delay open */
#define LOG_NOWAIT      0x10    /* if forking to log on console, don't wait() */





/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)                          

/* syslog.h defines put here to avoid having to includse a UNIX header file */
void openlog(const char *, int, int);
void syslog(int, const char *, ...);
void closelog(void);
int setlogmask(int);

extern int logMsgCreate(int maxEntrys);
extern void logMsgShow(void);
char *fileNline(char* filename, int linenum);
extern void diagPrint(char* fileNline, ...);
extern void errLogRet(int logopt, char *fileNline, ...);
extern void errLogQuit(int logopt, char *fileNline, ...);
extern void errLogSysRet(int logopt, char *fileNline, ...);
extern void errLogSysQuit(int logopt, char *fileNline, ...);
extern void sysLog(int pri,char* message);

#else                                                   
/* --------- NON-ANSI/C++ prototypes ------------  */
 
/* syslog.h defines put here to avoid having to includse a UNIX header file */
void openlog();
void syslog();
void closelog();
int setlogmask();

extern int logMsgCreate();
extern int void logMsgShow();
char *fileNline();
extern diagPrint();
extern void errLogRet();
extern errLogQuit();
extern void errLogSysRet();
extern errLogSysQuit();
extern void sysLog();

#endif                  
 
#ifdef __cplusplus
}
#endif

#endif
