/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdlib.h>

#include "group.h"
#include "acodes.h"
#include "rfconst.h"
#include "acqparms.h"
#include "expDoneCodes.h"

extern int HSrotor;
extern int newacq;
extern int ok2bumpflag;

/*-------------------------------------------------------------------
|
|	interlocktests.c()/0 
|	if in=y then generate Acodes to test lock on each transient.
|	if in=y then generate Acodes to test spin on each transient.
|	if tin=y then generate Acodes to test VT on each transient.
|				Author Greg Brissey  6/25/86
|   Modified   Author     Purpose
|   --------   ------     -------
|   1/9/88     Greg B.    1. Added Code to Select between Liquid's or 
|			     High Speed Rotor for interlock tests.
|			     HS rotor does not test lock, and checks
|			     high speed rotor spinning frequency. 
|
|   6/29/89   Greg B.     1. Use new global parameters instead of getval(),
|			     getstr()
|
+------------------------------------------------------------------*/
interlocktests()
{
 
    if ( (interLock[0] != 'n') && (interLock[0] != 'N') )
    {
       putcode(CKLOCK);
       putcode( ( (interLock[0] == 'y') || (interLock[0] == 'Y') ) ?
                HARD_ERROR : WARNING_MSG);
    }

    if ( ((setupflag == GO) || (setupflag == SPIN) || (setupflag == SAMPLE)) &&
         (( spinactive ) &&  ((interLock[1] != 'n') && (interLock[1] != 'N'))) )
    {
	  putcode(CHKSPIN);
          putcode( (HSrotor) ? 100 : 1);
          putcode( ( (interLock[1] == 'y') || (interLock[1] == 'Y') ) ?
                   HARD_ERROR : WARNING_MSG);
	  if (ok2bumpflag)
	    putcode( 1 );
	  else
	    putcode( 0 );
    }
    if (!newacq && HSrotor)
    {
       putcode(RD_HSROTOR); /* rd HS rotor, put value in srate */
       putcode((codeint) rttmp);
       if ( (interLock[1] != 'n') && (interLock[1] != 'N') )
       {
	  putcode(CHKHDWARE);
	  putcode(RTVAR_BIT | GT); /* if(spinact > spinrate+10 then abort */
	  putcode((codeint) sratert);
	  putcode((codeint) 100);
          putcode( ( (interLock[1] == 'y') || (interLock[1] == 'Y') ) ?
                   HARD_ERROR : WARNING_MSG);
       }
    }

    if ( tempactive  && vttype )
    {
       if ( (interLock[2] != 'n') && (interLock[2] != 'N') )
       {
	  putcode(CKVTR);
          putcode( ( (interLock[2] == 'y') || (interLock[2] == 'Y') ) ?
                   HARD_ERROR : WARNING_MSG);
       }
    }
}
