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

/* --- Acquisition Processing Commands --- */

#define ACQQUEUE 1		/* Enter New Experiment into QUEUE */
#define REGISTERPORT 2		/* Register Acq. Display Update port */
#define UNREGPORT 3		/* Remove Acq. Display Update port */
#define ACQABORT 4		/* Abort Specified Acquisition */
#define FGREPLY 5		/* VNMR FG Complete Reply */
#define ACQSTOP 6		/* Stop Specified Acquisition */
#define ACQSUPERABORT 7		/* Abort Acq & HAL to initial state */
#define ACQDEBUG 8
#define IPCTST 9		/* Echo back to Vnmr */
#define PARMCHG 10		/* change parameter (wexp,wnt,wbt,etc.) */
#define AUTOMODE 11		/* change to automation mode */
#define NORMAL 12		/* return to normal mode */
#define RESUME 13		/* send resume to autoproc */
#define SUPPEND 14		/* send suppend to autoproc */
#define ACQHALT 15		/* send halt to HAL, an abort w/ Wexp processing */
#define ACQHARDWARE 16		/* insert, eject, set DACs, etc. */
#define READACQHW 17		/* read acquisition hardware parameters */
#define QUEQUERY 18		/* send acqproc queue status back */
#define ACCESSQUERY 19		/* check user for access permission  */
#define RECONREQUEST 20		/* request reconnection to Vnmr */
