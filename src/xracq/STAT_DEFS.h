/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef STAT_DEFS
#define STAT_DEFS
/*-------------------------------------------------------------------
|   acquisition status codes for acqproc & status display on Host
+-------------------------------------------------------------------*/
#define ACQ_REBOOT 	5
#define ACQ_IDLE 	10
#define ACQ_PARSE 	15
#define ACQ_PREP        16
#define ACQ_SYNCED      17
#define ACQ_ACQUIRE 	20
#define ACQ_PAD		25
#define ACQ_VTWAIT 	30
#define ACQ_SPINWAIT 	40
#define ACQ_AGAIN 	50
#define ACQ_HOSTGAIN 	55
#define ACQ_ALOCK 	60
#define ACQ_AFINDRES 	61
#define ACQ_APOWER 	62
#define ACQ_APHASE 	63
#define ACQ_FINDZ0 	65
#define ACQ_SHIMMING 	70
#define ACQ_HOSTSHIM 	75
#define ACQ_SMPCHANGE 	80
#define ACQ_RETRIEVSMP 	81
#define ACQ_LOADSMP 	82
#define ACQ_ACCESSSMP   83   /* Access open status */
#define ACQ_ESTOPSMP    84   /* Estop status */
#define ACQ_MMSMP       85   /* Magnet motion status */
#define ACQ_HOMESMP     86   /* Homing */
#define ACQ_INTERACTIVE 90
#define ACQ_TUNING	100
#define ACQ_PROBETUNE	105
#define ACQ_INACTIVE    0

/*  These defines were added to represent situations
    local to Expproc and Acqproc and are not expected
    to necessarily be status value on the console.	*/

#define ACQ_AUTO	110
#define ACQ_NEEDSU	120
#define ACQ_UNKNOWN	-1

#endif
