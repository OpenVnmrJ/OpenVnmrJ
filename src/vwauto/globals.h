/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* globals.h 1- Global Parameters */
#ifndef LINT
/* "globals.h Copyright (c) 1994-1996 Varian Assoc.,Inc. All Rights Reserved" */
#endif
/* 
 */

#include "qspiObj.h"
#include "spinObj.h"

/*
   System Globals 
*/


extern QSPI_ID   pTheQspiObject; /* QSPI Object */
extern SPIN_ID	  pTheSpinObject; /* Spin Object */

extern char      *pShareMemoryAddr;   /* address of beginning of share memory, just past SM backplane */
extern long      EndMemoryAddr;	/* the real end of memory, for heart beat value */

extern int	  MSRType;		/* MSR board type: 1 - MSR1, 2- MSR2, etc... */

extern int shimDebug;	/* debug print control */
