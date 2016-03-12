/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* globals.c  07/09/07 - Global Parameters */
/* globals.c  2/1/95 - Global Parameters Glen D. SCCS */
#ifndef LINT
#endif
/* 
 */

#include "qspiObj.h"
#include "spinObj.h"

/*
   System Globals 
*/


QSPI_ID   pTheQspiObject; /* QSPI Object */
SPIN_ID	  pTheSpinObject; /* Spin Object */

char      *pShareMemoryAddr;   /* address of beginning of share memory, just past SM backplane */
long      EndMemoryAddr;	/* the real end of memory, for heart beat value */

int	  MSRType;		/* MSR board type: 1 - MSR1, 2- MSR2, etc... */

int shimDebug;	/* debug print control */
