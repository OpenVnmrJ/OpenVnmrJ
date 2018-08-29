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
#include "group.h"
#ifndef NVPSG
#include "acodes.h"
#else
#include "ACode32.h"
#endif
#include "rfconst.h"
#include "acqparms.h"
#include "shims.h"
#include "macros.h"
#include "abort.h"
#include "cps.h"

extern int bgflag;	/* debug flag */
extern int newacq;
extern int option_check(const char *option);

extern int ok2bumpflag;

int setshimflag(char *wshim, int *flag);
#ifdef NVPSG
extern void putmethod(int mode, int buffer[]);
#else
extern void putmethod(int mode);
#endif

#ifndef NVPSG
extern int ap_interface;
extern codeint adccntrl;

/*------------------------------------------------------------------
|
|	initauto1()/0
|	remove sample from magnet if a new sample is requested or
|	a 'change' command given. 
|	Set receiver gain and decoupler mode (homo or hetero)
|				Author Greg Brissey 6/26/86
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/15/89   Greg B.     1. Use global parameters instead of using getval()
+-----------------------------------------------------------------*/
void initauto1()
{
    codeint new;
    codeint mode;

    if (ix == getStartFidNum())
    {
     if (P_getstring(CURRENT,"presig",presig,1,15) >= 0)
     {
        if ((presig[0] == 'l') || (presig[0] == 'L') || 
	    (presig[0] == 'n') || (presig[0] == 'N')) 
	{
	   putcode(SETSIGREG);
	   putcode(NORMAL);
	}
	else if ((presig[0] == 'h') || (presig[0] == 'H')) 
	{
	   putcode(SETSIGREG); 
	   putcode(LARGE); 		
	}
	else
	{ 
	   text_error("Warning: presig has invalid value.  Set to default.");
	   putcode(SETSIGREG);
	   putcode(NORMAL);		
	}
     }
     else {
	/* If presig not present, Normal assumed */
	putcode(SETSIGREG);
	putcode(NORMAL);
     }
    }
    
    if (newacq)
    {
	if (ix == 1)
	{
	    putcode(GAINA);		/* set receiver gain Acode */
	    G_Delay(DELAY_TIME,3.0e-3,0); /*delay for setting receiver gain*/
	}
    }
    else
	putcode(GAINA);		/* set receiver gain Acode */

    /* set homo bit on automation board */
    if (ap_interface < 4)
    {  if ((homo[0] == 'y') || (homo[0] == 'Y'))
	   mode = 1;
       else
	   mode = 2;
       putcode(DECUP);		/* decoupler Mode Acode */
       putcode(mode);		/* homo bit */
       putcode(255);		/* max power from automation board */
    }
}

/*------------------------------------------------------------------
|	loadshims()/0
|	Load shims if load == 'y'.  Will be set by arrayfuncs if
|	any shims are arrayed.
+-----------------------------------------------------------------*/
void loadshims()
{
    char load[MAXSTR];

    if ((P_getstring(CURRENT,"load",load,1,15) >= 0) &&
        ((load[0] == 'y') || (load[0] == 'Y')) )
    {
        int index;
        const char *sh_name;

        P_setstring(CURRENT,"load","n",0);
        putcode(LOADSHIM);		/* Load Shim DAC values,  Acode */
        putcode(MAX_SHIMS);
        for (index= Z0 + 1; index < MAX_SHIMS; index++)
        {
           if ((sh_name = get_shimname(index)) != NULL)
              putcode( (codeint) sign_add(getval(sh_name), 0.0005) );
           else
              putcode(0);
        }
    }
}
/*------------------------------------------------------------------
|
|	initauto2()/0
|	load sample into magnet if a new sample is requested or
|	a 'change' command given. 
|	change spinner rate if changed from previous value
|	invoke autolock if required
|	preacquisition delay if a go,au,ga
|	invoke autoshimming if required
|				Author Greg Brissey 6/26/86
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/15/89   Greg B.     1. Use global parameters instead of using getval()
+-----------------------------------------------------------------*/
void initauto2()
{
    int new;
    int do_autolock;

    new = loc;  /* get sample location */

    if ( spinactive )  /* spin = 'y' */
        spin = (int) (getval("spin") + 0.0005);  /* get sample spin rate */
     else
        spin = -1; 		      /* spin = 'n' */

      /*----  SET SPINNER ---------*/
    if ( ((setupflag == GO) || (setupflag == SPIN) || (setupflag == SAMPLE))
	 && (spin >= 0) )
    {
       if (spin != oldspin)
       {
           int mode_thresh;

           mode_thresh = (getval("spinThresh") + 0.0005); /* spin type selector */
	   putcode(SPINA);		/* set spin rate acode */
	   putcode(spin);		/* spin rate (Hz) */
	   putcode(mode_thresh);	/* spin >= mode_thresh selects high speed
                                         * spinner (solids)
                                         * spin < mode_thresh selects low speed
                                         * spinner
                                         */
	   if (ok2bumpflag)
	     putcode( 1 );
	   else
	     putcode( 0 );
    	   oldspin = spin;
       }
    }

    /*----  PRE-ACQUISITION DELAY  ---------*/
    if (setupflag == 0) preacqdelay();		/*  pre-acquisition delay */

    /*----  AUTOLOCK ---------*/
/*    getlockmode(&lockmode);   */		/* type of autolocking */
    if ( (lockmode != 0) && 
       ( (setupflag == GO) || (setupflag == LOCK) || (setupflag == SAMPLE) )
       )
    {
	/*if ( (lockmode != oldlkmode) || (lockmode >= 8) )*/

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
             longer be relied upon.   05/1997			*/

	do_autolock = ( (lockmode != oldlkmode) || (lockmode & 8) );
	if (do_autolock)
	{
	    putcode(LOCKA);		/* Autolock Acode */
	    putcode(((codeint)(lockmode & 0x17))); /* lower 7bits of lockmode */
	    putcode(175);		/* delay (hundredths of seconds) to wait */
                                        /* for autolock hardware */
    	    oldlkmode = lockmode;	/* update to new value */
            if (newacq)
            {
               putcode(DISABLEOVRFLOW);
               putcode(adccntrl);
            }
	}
    }
}
#endif
/*------------------------------------------------------------------
|	initwshim()/0
|	invoke autoshimming if required
+-----------------------------------------------------------------*/
#ifdef NVPSG

void initwshim(int buffer[])
{
    int shimmode;

    /*----  BACKGROUND SHIMMING ---------*/
    whenshim = setshimflag(wshim,&shimatanyfid);
    shimmode = 0;
    if ( (shimatanyfid || (whenshim != oldwhenshim)) &&
	 ( (setupflag == GO) || (setupflag == SHIM) || (setupflag == SAMPLE) )
       )
    {
        shimmode += 1;
        oldwhenshim = whenshim;
    }
    if (bgflag)
        fprintf(stderr,"initauto2(): shimmask: %d whenshim = %d mode=%d \n",
	    shimatanyfid,whenshim, shimmode);
    if (shimmode > 0)
       putmethod(shimmode, buffer);        /* generate Acodes for auto shimming */
}
#else
void initwshim()
{
    int shimmode;
    char  flg[MAXSTR];

    /*----  BACKGROUND SHIMMING ---------*/
    whenshim = setshimflag(wshim,&shimatanyfid);
    if (bgflag)
        fprintf(stderr,"initauto2(): shimmask: %d whenshim = %d \n",
	    shimatanyfid,whenshim);
    if ((P_getstring(GLOBAL,"hdwshim",flg,1,15)) < 0)
    	shimmode = 0;
    else if ((flg[0] == 'y') || (flg[0] == 'Y'))
    	shimmode = 2;
    else
    	shimmode = 0;
    if ( (shimatanyfid || (whenshim != oldwhenshim)) &&
	 ( (setupflag == GO) || (setupflag == SHIM) || (setupflag == SAMPLE) )
       )
    {
        shimmode += 1;
        oldwhenshim = whenshim;
	ss4autoshim();
    }
    putmethod(shimmode);        /* generate Acodes for auto shimming */
}
#endif
#ifndef NVPSG
/*------------------------------------------------------------------
|	initautogain()/0
|	invoke autogain if required
+-----------------------------------------------------------------*/
void initautogain()
{
    if ( (! (gainactive)) && (setupflag == GO))/* gain being used?*/
    {
	putcode(AUTOGAIN);		/* Acode for autogainning */
        gainactive = TRUE;
    }
}
#endif
/*-------------------------------------------------------------------
|
|	getlockmode()
|	determine the type of autlocking to perform
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   9/20/89   Greg B.     1. Corrected arguments to new call form in cps.c
+-------------------------------------------------------------------*/
void getlockmode(char *alock, int *mode)
{
    int lockmode = 0;
    int z0active;

/* For INOVA systems (newacq is TRUE), finding the lock using lockfreq is
   available if z0 is inactive.  This feature is (currently) only available
   on INOVA systems, so for all other systems, assume z0 is active.          */

    if (newacq)
      z0active = var_active( "z0", GLOBAL );
    else
      z0active = 1;

    if ((setupflag == LOCK) || (setupflag == SAMPLE))
    {
	/* if z0 option, find resonance only
         * else find z0, and optimize phase, power, and gain
         */
        lockmode = option_check("z0") ? 1 : 2;
    }
    else
    {
	/* --- when and what kind of autolocking performed --- */
	switch( alock[0] )
	{
	    case 'Y':
	    case 'y': lockmode = 1;	/* hardware autolock */
		      break;
	    case 'A':
	    case 'a': lockmode = 3;	/* allways power & gain */
		      break;
	    case 'S':
	    case 's': lockmode = 4;	/* if sample change, power & gain */
		      break;
	    case 'U':
	    case 'u': lockmode = 5;	/* turn hardware lock off */
		      break;
	    case 'F':
	    case 'f': lockmode = 3+8;	/* for each fid, do autolocking */
		      break;
	    case 'N':
	    case 'n':			/* no autolocking */
		      lockmode = 0;
		      break;
	    default:
		      text_error("alock has an invalid value");
		      psg_abort(1);
	}
    }

/* Only turn on the Use Lockfreq bit if lockmode is already non-zero */

    if (!z0active && lockmode != 0)
      lockmode |= 16;

    *mode = lockmode;
}
/*-------------------------------------------------------------------
|
|	setshimflag()
|	From wshim set shimatanyfid flag and the automation whenmask
|	return whenmask;
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   9/20/89   Greg B.     1. Corrected arguments to new call form in cps.c
+-------------------------------------------------------------------*/
int setshimflag(char *wshim, int *flag)
{
    int shimatanyfid = FALSE;
    int whenmask = 0;

    if (setupflag == SHIM)
	strcpy(wshim,"fid");		/* force wshim=fid when SHIM given */
    
    switch( wshim[0] )
    {
	case 'N':
	case 'n': shimatanyfid = FALSE;		/* no autoshimming */
		  whenmask = 0;
		  break;
	case 'G':
	case 'g': shimatanyfid = FALSE;		/* no autoshimming */
		  whenmask = 0;		/* dummy case for gradient shimming */
		  break;
	case 'E':
	case 'e': shimatanyfid = FALSE;		/* shimming between experiment*/
		  whenmask = 1;
		  break;
	case 'F':				/* shimming between fids */
	case 'f': if ((wshim[1] >= '0') && (wshim[1] <= '9'))
                  {  int count = 0;
                     int i = 1;
	             while ((wshim[i] >= '0') && (wshim[i] <= '9'))
                     {
                       count = count * 10 + (wshim[i] - '0');
                       i++;
                     }
                    shimatanyfid = (((ix - 1) % count) == 0) ? TRUE : FALSE;
                  }
                  else
                    shimatanyfid = TRUE;
		  whenmask = 2;
		  break;
	case 'B':
	case 'b': shimatanyfid = TRUE;		/* shimming between block size*/
		  whenmask = 4;
		  break;
	case 'S':
	case 's': shimatanyfid = FALSE;		/*shimming after sample change*/
		  whenmask = 8;
		  break;
	default:  text_error("wshim has an invalid value");
		  psg_abort(1);
    }
    *flag = shimatanyfid;
    return(whenmask);
}
