/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "oopc.h"
#include "acodes.h"
#include "acqparms.h"
#include "rfconst.h"
#include "macros.h"

extern int      curfifocount;
extern int      HSlines;
extern int      ap_interface;

incpulsech(wbase, wincr, mult, phaseptr, rx1, rx2, device)
int             device;
double          wbase,
                wincr;
codeint         mult;
codeint         phaseptr;
double          rx1,
                rx2;
{
   char msge[128];

   if ((mult < v1) || (mult > v14))
   {
      text_error("INCPULSE USES AN INVALID MULTIPLIER");
      abort(1);
   }
   if ((wbase > 0.0) || (wincr > 0.0))
   {
      if ((device < 1) || (device > NUMch))
      {
        sprintf(msge,"incpulsech: device #%d is not within bounds 1 - %d\n",
          device,NUMch);
        text_error(msge);
        abort(1);
      }
      if ( RF_Channel[device] == NULL )
      {
        sprintf(msge,"incpulsech: device #%d is not present.\n",
          device,NUMch);
        text_error(msge);
        return(-1);
      }
      SetRFChanAttr(RF_Channel[device], SET_RTPHASE90, phaseptr, 0);
      /* --- zero all phase bits in curqui, the phase bits are or'd in at */
      /* --- event1(),event2() with rfphpt & curdec */
      if ( ap_interface < 3 )
         gate(RFP270 | DC270 , FALSE);
      else
         gate(RFP270 | DC270 | DC2_270, FALSE);
      gate(RXOFF, TRUE);	/* gate receiver off */
      G_Delay(DELAY_TIME, rx1, 0);	/* (i.e.rof1) */
      SetRFChanAttr(RF_Channel[device], SET_XMTRGATE, ON, 0);
      incxdelay(wbase, wincr, mult);	/* wait for wbase+wincr*mult */
      SetRFChanAttr(RF_Channel[device], SET_XMTRGATE, OFF, 0);
      G_Delay(DELAY_TIME, rx2, 0);	/* wait for i.e.rof2 */
      gate(RXOFF, FALSE);	/* turn receiver on */
   }
}

static
incxdelay(wbase, wincr, mult)
double          wbase,
                wincr;
codeint         mult;
{
   int             tword1,
                   tword2;

/*incrementable or decrementable delay
 effective delay is wbase + (mult * wincr)
 mult is a user variable
 element has no effect (produces no acodes) if both wbase and wincr are <= 0.0
 element produces acodes but will have no effect at execution time if
 wbase <= 0.0 and wincr > 0.0 and (value of mult) < 1*/
   okinhwloop();
   if ((mult < v1) || (mult > v14))
   {
      /* not a valid user variable */
      text_error("INCDELAY USES AN INVALID MULTIPLIER");
      abort(1);
   }
   if ((wbase > 0.0) || (wincr > 0.0))
   {
      G_Delay(DELAY_TIME, wbase, 0);	/* load wbase if > 0.0 as event 1 or
					 * 2 */
      timerwords(wincr, &tword1, &tword2);	/* calc tw array for event n */
      if ((tword1 != 0) || (tword2 != 0))
      {
	 /* only if wincr > 0.0 ! */
	 putcode(EVENTN);	/* an event n */
	 putcode(mult);		/* a user variable */
	 putcode((codeint) tword1);	/* timeword 1 */
	 putcode((codeint) HSlines);	/* High Speed gate patterns */
	 if (tword2 == 0)
	 {			/* one event ? */
	    putcode(0);
	    curfifocount++;
	 }
	 else
	 {			/* event 2 */
	    putcode((codeint) tword2);
	    curfifocount += 2;
	 }
      }
   }
}
