/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "group.h"
#include "rfconst.h"
#include "acqparms.h"
#define  MAXSTATUSES	255	/* Maximum size of flag arrays */
extern void gatedecoupler(int statindex, double delaytime);

/*-------------------------------------------------------------------
|
|	status(index)/1 
|	sets the global statusindx variable to the status index selected.
|	This index is used in conjucntion with the status control
|	variables like dm,dmm: where dm='yny' then status(B) sets the
|	global statusindx = 1; therefore when dm is check the 2nd character
|	is used as its value.
|	If the index is out of range then it is set to the first char
|				Author Greg Brissey  6/17/86
+------------------------------------------------------------------*/
void status(int index)
{

    if (index < 0 || index > MAXSTATUSES)
	index = 0;			/* set to first char */
    else
    {
	statusindx = index;
	gatedecoupler(statusindx,0.0);	/* setup decoupler HSlines */
    }
}

/*-------------------------------------------------------------------
|
|  statusdelay(index,delay)/2 
|	Performs the same function as status except a delay is
|	is used to ensure that the delay of status is always the
|	same.  So whenever an APbus action is performed the delay
|	for that action is subtracted from the delay given.  At
|	the end of the status element a delay is performed for any
|	remaining time.
+------------------------------------------------------------------*/
void S_statusdelay(int index, double delaytime)
{

    if (index < 0 || index > MAXSTATUSES)
	index = 0;			/* set to first char */
    else
    {
	statusindx = index;
	gatedecoupler(statusindx,delaytime);	/* setup decoupler HSlines */
    }
}
