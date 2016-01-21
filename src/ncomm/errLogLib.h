/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCerrloglibh
#define INCerrloglibh

#ifndef VNMRS_WIN32
#include <syslog.h>
#endif

#define LOGIT 1
#define NOLOG 2


/* syslog facility, see errLogLib, syslog for details */
/* LOG_LOCAL0 - LOG_LOCAL7 possible values */
#define EXPLOG_FACILITY		LOG_LOCAL0
#define RECVLOG_FACILITY	LOG_LOCAL0
#define SENDLOG_FACILITY	LOG_LOCAL0
#define PROCLOG_FACILITY	LOG_LOCAL0
#define ROBOLOG_FACILITY	LOG_LOCAL0
#define AUTOLOG_FACILITY	LOG_LOCAL0

/* __FILE__ __LINE__  __DATE__ __TIME__ */

#ifdef DEBUG

#define debugInfo (fileNline(__FILE__,__LINE__))

#define DPRINT(level, str) \
        if (DebugLevel >= level) diagPrint(debugInfo,str)

#define DPRINT1(level, str, arg1) \
        if (DebugLevel >= level) diagPrint(debugInfo,str,arg1)

#define DPRINT2(level, str, arg1, arg2) \
        if (DebugLevel >= level) diagPrint(debugInfo,str,arg1,arg2)

#define DPRINT3(level, str, arg1, arg2, arg3) \
        if (DebugLevel >= level) diagPrint(debugInfo,str,arg1,arg2,arg3)

#define DPRINT4(level, str, arg1, arg2, arg3, arg4) \
        if (DebugLevel >= level) diagPrint(debugInfo, str,arg1,arg2,arg3,arg4)

#define DPRINT5(level, str, arg1, arg2, arg3, arg4, arg5 ) \
        if (DebugLevel >= level) diagPrint(debugInfo,str,arg1,arg2,arg3,arg4,arg5)

#define DPRINT6(level, str, arg1, arg2, arg3, arg4, arg5, arg6 ) \
        if (DebugLevel >= level) diagPrint(debugInfo,str,arg1,arg2,arg3,arg4,arg5,arg6)

#define DPRINT7(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7 ) \
        if (DebugLevel >= level) diagPrint(debugInfo,str,arg1,arg2,arg3,arg4,arg5,arg6,arg7)

#define DPRINT8(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 ) \
        if (DebugLevel >= level) diagPrint(debugInfo,str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8)

#define DPRINT9(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 ) \
        if (DebugLevel >= level) diagPrint(debugInfo,str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9)


#else

#define DPRINT(level, str)
#define DPRINT1(level, str, arg2)
#define DPRINT2(level, str, arg1, arg2)
#define DPRINT3(level, str, arg1, arg2, arg3)
#define DPRINT4(level, str, arg1, arg2, arg3, arg4)
#define DPRINT5(level, str, arg1, arg2, arg3, arg4, arg5)
#define DPRINT6(level, str, arg1, arg2, arg3, arg4, arg5, arg6)
#define DPRINT7(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
#define DPRINT8(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
#define DPRINT9(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)

#define debugInfo NULL


#endif

/* This allows the usage of LOGOPT in errLogXXX calls that can be
   changed from stdout output for diagnostic phase to logging in
   daemon optional phase of program.
*/

#ifdef LOGMSG
#define LOGOPT LOGIT
#else
#define LOGOPT NOLOG
#endif

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

extern int DebugLevel;
extern int ErrLogOp;		/* used with errLogXXX  */

/* --------- ANSI/C++ compliant function prototypes --------------- */
 
#if defined(__STDC__) || defined(__cplusplus)
 
extern void logSysInit(char* idstr, int facility);
extern void logSysClose();
extern void diagPrint(char* fileNline, char *fmt, ...) __attribute__((format(printf,2,3)));
extern void errLogRet(int logopt, char *fileNline, char *fmt, ...) __attribute__((format(printf,3,4)));
extern void errLogQuit(int logopt, char *fileNline, char *fmt, ...) __attribute__((format(printf,3,4)));
extern void errLogSysRet(int logopt, char *fileNline, char *fmt, ...) __attribute__((format(printf,3,4)));
extern void errLogSysQuit(int logopt, char *fileNline, char *fmt, ...) __attribute__((format(printf,3,4)));
extern void errLogSysDump(int logopt, char *fileNline, char *fmt, ...) __attribute__((format(printf,3,4)));
extern char *fileNline(char* filename, int linenum);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern void logSysInit();
extern void logSysClose();
extern void diagPrint();
extern void errLogRet();
extern void errLogQuit();
extern void errLogSysRet();
extern void errLogSysQuit();
extern void errLogSysDump();
extern char *fileNline();
 
#endif

 
#ifdef __cplusplus
}
#endif
 
#endif
