/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef COMMFUNCS_H
#define COMMFUNCS_H

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

extern int chanId;	/* the Channel Number that has Valid connection to console */
extern int chanIdSync;	/* the Expproc Channel to update  Console parameters  (JPSG) */

/* --------- ANSI/C++ compliant function prototypes --------------- */
 
#if defined(__STDC__) || defined(__cplusplus)
 
extern void 	shutdownComm(void);
extern int	initiateChan(int chan_no);
extern int	initiateAsyncChan(int chan_no);
extern int	consoleConn(void);
extern int 	setRtimer(double timsec, double interval);
extern int      wallMsge(char *fmt, ...);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern int 	shutdownComm();
extern int	initiateChan();
extern int	initiateAsyncChan();
extern int	consoleConn();
extern int 	setRtimer();
extern int      wallMsge();

#endif
 
#ifdef __cplusplus
}
#endif
 
#endif

