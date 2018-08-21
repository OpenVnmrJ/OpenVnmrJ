/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "group.h"
#include "acodes.h"
#include "rfconst.h"
#include "acqparms.h"
#include "macros.h"

extern double getval();

extern int  bgflag;	/* debug flag */

double preacqtime;      /* value of pad used for time calculations */

/*-------------------------------------------------------------------
|
|	preacqdelay()/2 
|	pre-acquisition delay, occurs only once at beginning of
|	 the Pulse Sequence.
|	Allows greater than 8190 seconds of delay
|				Author Greg Brissey  7/13/86
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/15/89   Greg B.     1. Use new global parameters instead of var_active()
+------------------------------------------------------------------*/
preacqdelay()
{
    double predelay;		/* pre-acquisition delay time  'pad' */
    double delaytime;		/* temp delays */
    int    tword1;		/* timer word one */
    int    tword2;		/* timer word two */
    int    ntwords;		/* number of timerwords needed */
    int    skipcount;		/* codes to skip if to skip PADLY */
    codeint *skipcntadr;

   if (bgflag)
       fprintf(stderr,"preacqdelay(): \n");
/*     if (var_active("pad",CURRENT))	/* is variable in use ? */
    if (padactive)	/* is variable in use ? */
    {
        /* delay if pad or temp has changed & delay is > than min */
        /* if pad or temp changed oldpad is forced to -1 in func4pad_temp() */
    	if( ( pad >= 200.0001E-9) &&     /* be sure delay is > min delay */
	    (oldpad != pad) )		 /* and a new value is present */
	{
	    predelay = pad;
	    oldpad = pad;		/* update oldpad value */
	    preacqtime = pad;	/* update preacqtime for time calculation */
	    putcode(PADLY);		      /* Acode for pre-acq delay */
    	    skipcntadr = Codeptr;	/* save location for count */ 
    	    putcode(0);			/* put zero in for now */
	    do		/* while delay > 8190 (max delay possible) */
	    {
	    	if (predelay > 8190.0)
		    delaytime = 8190.0;
	    	else
		    delaytime = predelay;
		predelay -= 8190.0;
		G_Delay(DELAY_TIME,delaytime,0); /*create EVENT1/EVENT2 Acodes*/
	    } while( predelay > 0.0 );

	    putcode(STFIFO);  /* start fifo */
	    putcode(SFIFO);   /*  hope we are fast enough w/ haltop b4 foo */
            skipcount = ( (codeint) (Codeptr - skipcntadr)) - 1;
    	    *skipcntadr = (codeint) skipcount;
	}
    }
}
