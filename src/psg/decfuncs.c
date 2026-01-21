/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <stdio.h>
#include <string.h>
#include "oopc.h"
#include "group.h"
#include "acodes.h"
#include "rfconst.h"
#include "acqparms.h"
#include "decfuncs.h"
#include "abort.h"

extern int ap_interface;
extern int SetRFChanAttr(Object obj, ...);
extern void decouplerattn(int mode);
extern void HSgate(int ch, int state);


static int  sisdecblankoff_flag = FALSE; /* SIS decoupler blanking flag; */
/*-------------------------------------------------------------------
|
|	declvlon()/0 
|
|	Turn on the decoupler high power pulse 
|
+------------------------------------------------------------------*/
void declvlon()
{
    /* if (newdecamp) */
    if (cattn[DODEV] != 0.0 )
    {   /* set coarse attenuator to max value
         * Attenuators can have cattn values of 79 or 63.
         * However, the maximum value is fixed at 63.
         * The minimum values are -16 and 0, respectively.
         */
	SetRFChanAttr(RF_Channel[DODEV], SET_PWR, (int) 63, NULL);
    }
    else
    {
        decpwr(255.0);
    }
}
/*-------------------------------------------------------------------
|
|	declvloff()/0 
|
|	Turn off the decoupler high power pulse 
+------------------------------------------------------------------*/
void declvloff()
{
    /* if (newdecamp) */
    if (cattn[DODEV] != 0.0 )
    {   /* set coarse attenuator back to dpwr */
	SetRFChanAttr(RF_Channel[DODEV], SET_PWR, (int)dpwr, NULL);
    }
    else
    {
        decpwr(dhp);
    }
}

/*-------------------------------------------------------------------
|
|	decpwr(level)/1 
|
|	Change the power of high power decoupler to 'level' 
|  Modified:  10/25/88  Greg Brissey
|	      test newdecamp instead of newdec.
+------------------------------------------------------------------*/
void decpwr(double level)
{
    /* if (newdecamp) */
    if (cattn[DODEV] != 0.0)  /* coarse attenuator present ? */
    {
      text_error("DECPWR(): use POWER for this hardware configuration.\n");
      psg_abort(1);
    }

    if ( (rftype[DODEV-1] != 'a') && (rftype[DODEV-1] != 'A') )
    {
       text_error("DECPWR():  requires a fixed frequency 1H decoupler.\n");
       psg_abort(1);
    }

    okinhwloop();
    if (dhpflag)		/* decoupler on high power */
    {
       double save;
       save = dhp;
       dhp = level;
       putcode(APBOUT);	/* set decoupler attenuator */
       putcode(1);
       decouplerattn(TRANSIENT_APVAL);
       dhp = save;
     }
}
/*------------------------------------------------------------------
|
|	spareon()
|	 turn on decoupler high power bit on the high speed lines.
|				Author Greg Brissey 7/10/86
+-----------------------------------------------------------------*/
void spareon()
{
   if (ap_interface < 3)
   {
    okinhwloop();
    HSgate(DECLVL,TRUE);
   }
   else
   {
     text_error(
	  "SPAREON():  Not supported on systems with `apinterface` > 2\n");
     psg_abort(1);
   }
}
/*------------------------------------------------------------------
|
|	spareoff()
|	 turn off decoupler high power bit on the high speed lines.
|				Author Greg Brissey 7/10/86
+-----------------------------------------------------------------*/
void spareoff()
{
   if (ap_interface < 3)
   {
    okinhwloop();
    HSgate(DECLVL,FALSE);
   }
   else
   {
     text_error(
	  "SPAREOFF():  Not supported on systems with `apinterface` > 2\n");
     psg_abort(1);
   }
}

/*------------------------------------------------------------------
| SIS decoupler blanking
| Turns decoupler blanking on (high-power section of decoupler amplifier off)
| 0 in the DECLVL bit turns blanking on - this was done to provide decoupler
| amplifier blanking upon bootup and FIFO underflow.
| Default state of the blanking bit at the beginning of the pulse sequence is
| blanking on.
| This element should be followed by a delay of approximately 5 microseconds
| to allow switching time.
+-----------------------------------------------------------------*/

void decblankon()
{
   if ((cattn[DODEV] == SIS_UNITY_ATTN_MAX) && (ap_interface < 3))
   {
	okinhwloop();
	HSgate(DECLVL,FALSE);
	sisdecblankoff_flag = FALSE;
   }
   else
   {
     text_error(
	  "decblankon():  Only supported on SIS VXR or Unity systems.\n");
     psg_abort(1);
   }
}


/*------------------------------------------------------------------
| Turns decoupler blanking off (high-power section of decoupler amplifier on).
| This element should be followed by a delay of approximately 30 microseconds
| to allow switching time.
+-----------------------------------------------------------------*/

void decblankoff()
{
   if ((cattn[DODEV] == SIS_UNITY_ATTN_MAX) && (ap_interface < 3))
   {
	okinhwloop();
	HSgate(DECLVL,TRUE);
	sisdecblankoff_flag = TRUE;
   }
   else
   {
     text_error(
	  "decblankon():  Only supported on SIS VXR or Unity systems.\n");
     psg_abort(1);
   }
}

/*----------------------------------------------------------------------*/
/* initsisdecblank							*/
/*	Initializes the decoupler power amp gate to the blanked (off	*/
/*	or nonblanked (on) state depending on if any of the fields in	*/
/*	"dm" are 'y'.  This line should be set at the beginning of a	*/
/*	transient and reset (to off) at the end of a transient.		*/
/*	The sisdecblankoff_flag is set to TRUE if blanking is 		*/
/*	initialized off							*/
/*----------------------------------------------------------------------*/
void initsisdecblank()
{
   if ((cattn[DODEV] == SIS_UNITY_ATTN_MAX) && (ap_interface < 3))
   {
	if (strchr(dm,'y') == NULL)
	{
		HSgate(DECLVL,FALSE);
		sisdecblankoff_flag = FALSE;
	}
	else
	{
		HSgate(DECLVL,TRUE);
		sisdecblankoff_flag = TRUE;
	}
   }
}

/*----------------------------------------------------------------------*/
/* sisdecblank()								*/
/*	Used internally in psg to blank on and off decoupler amps for	*/
/*	SIS VXR and Unity systems.					*/
/*----------------------------------------------------------------------*/
void sisdecblank(int action)
{
   if ((cattn[DODEV] == SIS_UNITY_ATTN_MAX) && (ap_interface < 3))
   {
	if (action == OFF)
	{
	   HSgate(DECLVL,TRUE);
	}
	else if (action == ON)
	{
	   if (sisdecblankoff_flag == FALSE) /* blank decoupler amp if it is not */
		HSgate(DECLVL,FALSE);		/* globally unblanked */
	}
	else
	{
	   text_error("sisblank():  Invalid Argument.\n");
	   psg_abort(1);
	}
   }

}

