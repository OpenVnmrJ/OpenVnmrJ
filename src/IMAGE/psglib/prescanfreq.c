/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */
/**********************************************************************
 * NAME:
 *    prescanfreq.c
 *
 * DESCRIPTION:
 *    Slice selective One-pulse sequence for PRESCAN center frequency
 *
 *********************************************************************/
#include <standard.h>
#include "prescan.h"
void pulsesequence()
{
    /*******************************************************/
    /* Internal variable declarations                      */
    /*******************************************************/
    double  predelay;
    double  agss,grate,gssint, gssrint;      
    double  t_after, acq_delay, min_tr;
    double  t_rampslice, t_plateau_sr, t_plateau_min, t_ramp_sr;
    double  slice_offset,f_offset;
    double  pss0;

    char     slice_select[MAXSTR];
 
    initparms_sis();                        /* Sets default state of receiver to ON */
                                            /*- will be obsolete on future VNMR versions */
      
    /***************************/
    /* initialize variables    */
    /***************************/  
    gssf      = 1.0;                        /* slice select fractional refocus */
    predelay  = PREDELAY;                   /* define predelay [s] */
    acq_delay = ACQ_DELAY;                  /* time delay between end of refocus and acq */
    slice_offset = 0.0;                     /* force slice offset to 0.0 [cm] */
    t_rampslice = 0.0;			    /* slice select ramp time */
    t_ramp_sr   = 0.0;                      /* slice refocusing ramp time */
    t_plateau_min = 0.0005;                 /* min time slice refocusing plateau */
    t_plateau_sr  =0.0;                     /* slice refocusing plateau time*/
    f_offset=getval("resto");

    agss = fabs(gss);			    /* absolute slice select gradient */
    gssr  = gmax;                           /* maximum slice refocusing gradient */
    grate = trise/gmax;                     /* define gradient slew rate [s*cm/g]
                                              trise = gradient rise time
                                              gmax = maximum gradient strength [G/cm] */
        
    ticks = IGNORE_TRIGGER;                 /* ignore trigger pulses */
                                        
    /***************************/
    /* Get parameter           */
    /***************************/  
    at = getval("at");
    pss0 = getval("pss0");    
					       
    getstr("slice_select",slice_select);      /* slice select flag
                                               [y] = ON, [n] = OFF */
					       
    /*******************************************************/
    /* Slice Select gradient area                          */
    /*******************************************************/
    t_rampslice = grate * agss;
    gssint = (agss*p1/2.0) + (agss*t_rampslice/2.0);
    gssrint=gssint;
    /*******************************************************
     * Calculate slice refocussing gradient                *
     *******************************************************/
    t_plateau_sr = (gssint / gssr) - trise;
    if (t_plateau_sr <= 0.0)            /* traingular gradients */
       {
       t_plateau_sr = 0.0;
       gssr = sqrt(gssint / grate);
       }  
    t_ramp_sr = gssr * grate;           /* ramp time for refocusing gradient*/
    gssrint = (gssr * t_plateau_sr) + (t_ramp_sr * gssr);	    

    /***************************************************************************
     * timing calculation                                                      *
     ***************************************************************************/   
    if (slice_select[0] == 'y')
       {
       t_after = tr - (predelay + p1 + t_plateau_sr + at + acq_delay + 2* (t_rampslice + t_ramp_sr));
       min_tr  = predelay + p1 + t_plateau_sr + at + acq_delay + 2* (t_rampslice + t_ramp_sr);
       }
    else
       {
       t_after = tr - (predelay + p1 +  at + acq_delay );
       min_tr  = predelay + p1 + at + acq_delay ;
       }   


    if (t_after < 0.0)
        {
        abort_message("Requested repetition time (TR) too short.  Min tr = %.f[ms]\n",min_tr*1000);
        }

    /******************************************************/
    /*                                                    */
    /*                  S T A R T                         */
    /*        P U L S E    S E Q U E N C E                */
    /*                                                    */
    /******************************************************/
    obspower(tpwr1);                          /* set tranmitter power */
    /***************************************************************************
     *   Predelay                                                              *
     ***************************************************************************/
    obsoffset(f_offset);                    /* set transmitter offset */ 
    delay(predelay);                    
    xgate(ticks);                           /* set gating */
    if (slice_select[0] == 'y')
         {  
	 /***************************************************************************
	  * Slice select gradient & RF pulse                                        *
	  ***************************************************************************/
	 obl_gradient(0.0,0.0,gss);               /* slice select gradient */
	 delay(t_rampslice);                      /* delay - time to ramp up gradient */
	 shaped_pulse(p1pat,p1,oph,rof1,rof1);
	 zero_all_gradients();                    /* force all gradients back to 0 [G/cm] */
	 delay(t_rampslice);                      /* time to ramp down gradient */

	 /***************************************************************************
	  * Slice refocus gradient                                                  *
	  ***************************************************************************/
	 obl_gradient(0.0,0.0,-gssr);             /* slice refocus gradient */
	 delay(t_ramp_sr+t_plateau_sr);          /* ramp up of refocus gradient */
	 zero_all_gradients();                   /* force refocus gradient back to 0 [G/cm] */
	 delay(t_ramp_sr);                       /* time to ramp down gradient */
	 }
    else
         {
         shaped_pulse(p1pat,p1,oph,rof1,rof1);
         }
    /***************************************************************************
     * Pre-acquire delay                                                       *
     ***************************************************************************/
    delay(acq_delay);

    /***************************************************************************
     * Acquire echo                                                            *
     ***************************************************************************/
    startacq(alfa);
    acquire(np,1.0/sw);                 /* acquire FID */
    endacq();

    delay(t_after);                     /* time padding to fill TR */
    /******************************************************/
    /*                                                    */
    /*                    E N D                           */
    /*        P U L S E    S E Q U E N C E                */
    /*                                                    */
    /******************************************************/

}

