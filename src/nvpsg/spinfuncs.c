/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <string.h>
#include "group.h"
#include "ACode32.h"
#include "acqparms.h"
#include "expDoneCodes.h"
#include "spinner.h"
#include "pvars.h"
#include "Bridge.h"

extern double getval(char *);
extern double sign_add(double, double);
extern int HSrotor;
extern int ok2bumpflag;

/*----------------------------------------------------------------
| 
|	spinnerStringToInt()/i
|	return the numerical value for the spinner parameter
+-----------------------------------------------------------------*/
int spinnerStringToInt()
{
char spintype[MAXSTR];
int  spinner;
   /* we now use 'spintype' ="liquids","tach","mas","nano" "none" */
   if ( P_getstring(GLOBAL,"spintype", spintype, 1, MAXSTR)  < 0)
      spinner=LIQUIDS_SPINNER;
   else if ( ! strcmp(spintype,"liquids") ) spinner = LIQUIDS_SPINNER;
   else if ( ! strcmp(spintype,"tach") )    spinner = SOLIDS_SPINNER;
   else if ( ! strcmp(spintype,"mas") )     spinner = MAS_SPINNER;
   else if ( ! strcmp(spintype,"nano") )    spinner = NANO_SPINNER;
   else    spinner = NO_SPINNER;
   return(spinner);
}


/*------------------------------------------------------------------
|
|	setspin()/0
|	set spin controller to proper value
+-----------------------------------------------------------------*/
void setspin()
{
int  codeStream[20],i;
int  spinner;

    if ( ((setupflag == GO) || (setupflag == SPIN) || (setupflag == SAMPLE))
         && spinactive )
    {
       spinner = spinnerStringToInt();
       if (spinner == MAS_SPINNER) return;
       i = 0;
       codeStream[i++] = spin;		/* spin rate (Hz) */
       codeStream[i++] = spinner;	/* liquids or solids ? */
       if (ok2bumpflag)
          codeStream[i++] = 1;
       else
          codeStream[i++] = 0;
       /* now write to all controllers, master will check, others wait */
       broadcastCodes(SETSPIN, i, codeStream);
       delay(0.100);  /* delays added to prevent DDR from reboot when spin=value, GMB 1/18/05 */
    }
}

/*------------------------------------------------------------------
|
|	wait4spin()/0
|	send acode to wait for spinner to obtain spin-speed 
+-----------------------------------------------------------------*/
void wait4spin()
{
char spintype[MAXSTR];
int  codeStream[20],i;
    if ( ((setupflag == GO) || (setupflag == SPIN) || (setupflag == SAMPLE)) &&
          spinactive )
    {
       if ( P_getstring(GLOBAL,"spintype", spintype, 1, MAXSTR)  == 0)
          if ( strcmp(spintype,"mas") == 0 ) 
             return;
       i = 0;
       codeStream[i++] = HSrotor ? 100 : 1;
       if ( (interLock[1] != 'N') && (interLock[1] != 'n'))
          codeStream[i++] = ( (interLock[1] == 'y') || (interLock[1] == 'Y') ) ?
                        HARD_ERROR : WARNING_MSG;
       else
          codeStream[i++] = 0;
       if (ok2bumpflag)
          codeStream[i++] = 1;
       else
          codeStream[i++] = 0;
       broadcastCodes(CHECKSPIN, i, codeStream);
       delay(0.100);  /* delays added to prevent DDR from reboot when spin=value, GMB 1/18/05 */
    }
}
 

