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
#include "ACode32.h"
#include "acqparms.h"
#include "expDoneCodes.h"
#include "Bridge.h"

extern int par_maxminstep(int tree, const char *name, double *maxv, double *minv, double *stepv);

/*------------------------------------------------------------------
|
|	autolock()/0
|	set start autolock  if specififed
|
|       Author: Greg Brissey  1/06/2005
+-----------------------------------------------------------------*/
void doAutolock()
{
   int do_autolock;
   int codeStream[20],i;


  if ( (lockmode != 0) &&
       ( (setupflag == GO) || (setupflag == LOCK) || (setupflag == SAMPLE) )
     )   
  {
/*  BEWARE:  Bit 8 means perform the autolock on each FID, using
             the other bits to determine exactly what kind of
             autolock is to be done.  Bit 8 is always cleared
             before the autolock mode value is sent to the console.

             Bit 16 means use the lock frequency (instead of z0)
             to find the lock signal.  The console must receive
             this bit intact.

             The programs use to rely in the fact that bit 8 was
             the largest bit that could be set (lockmode >= 8).
             With the choice of lockfreq vs. z0 this fact can no
             longer be relied upon.   05/1997                   */

        do_autolock = ( (lockmode != oldlkmode) || (lockmode & 8) );
        if (do_autolock)
        {
            double max,min,step;
            i = 0;
            codeStream[i++] = lockmode & 0x17;	/* lower 7bits of lockmode */
            par_maxminstep(GLOBAL, "lockpower", &max, &min, &step);
            /* fprintf(stdout,"lockpower: max: %lf, min: %lf, step: %lf\n",max,min,step); */
            codeStream[i++] = (int) max;	/* spin>=mode_thresh selects high speed */
            par_maxminstep(GLOBAL, "lockgain", &max, &min, &step);
            /* fprintf(stdout,"lockgain: max: %lf, min: %lf, step: %lf\n",max,min,step);  */
            codeStream[i++] = (int) max;	/* spin>=mode_thresh selects high speed */

            /* now write to all controllers, master will check, others wait */
            broadcastCodes(LOCKAUTO, i, codeStream);
            delay(0.100);  /* delays added to prevent DDR from reboot when spin=value, GMB 1/18/05 */
        }
   }
}
