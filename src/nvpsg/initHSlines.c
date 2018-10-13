/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <stdio.h>
#include "acodes.h"
#include "rfconst.h"
#include "group.h"
#include "acqparms.h"

#define WHOLEBODY_SHIMSET	11	/*shimset number for WHOLEBODY Shims*/

extern void initorgHSlines();

extern int  bgflag;	/* debug flag */
extern int  presHSlines;
extern int  sync_flag;	/* did we synchronize decoupler mode yet? 1=yes */
extern int  newacq;

/*------------------------------------------------------------------
|
|	clearHSlines()
|	clear the acquisitions High Speed Lines to safe state.
|				Author Greg Brissey 10/29/92
|
+-----------------------------------------------------------------*/
clearHSlines()
{
}
/*------------------------------------------------------------------
|
|	initHSlines()
|	initialize the acquisitions High Speed Lines to their
|	initial safe state. This state is also store in the structure
|	acqparams section of code for the acquisition CPU 
|       Note: HSlines is passed to acquisition in EVENT1 & EVENT2 
|	      Acodes, only then are the lines actually set
|				Author Greg Brissey 6/26/86
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/15/89   Greg B.     1. Use new global parameters to calc acode offsets 
+-----------------------------------------------------------------*/
initHSlines()
{
}
