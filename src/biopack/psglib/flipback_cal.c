/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  flipback_cal.c */
/* calibrates soft pulse phase and fine power for
    soft-hard  (satmode='d')
    hard-soft  (satmode='u')
    INEPT-soft (satmode='n') for gNhsqc Trosy
    INEPT-soft (satmode='i') 
    INEPT-soft (satmode='t') for TROSY sequences with INEPT at start
*/

#include <standard.h>
pulsesequence()
{
 double
   	pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
       gt0= getval("gt0"),
       gzlvl0= getval("gzlvl0"),
       phincr=getval("phincr"),
       compH = getval("compH"),
   	tpwrs,	  	              /* power for the pwHs ("H2Osinc") pulse */
        tpwrsf = getval("tpwrsf");    /* fine power for pwHs pulse            */
                                      /* use to adjust for radiation-damping  */

 char   shape[MAXSTR];
     getstr("shape",shape);

    /* selective H20 one-lobe sinc pulse */
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69)); /* needs 1.69 times more */
    tpwrs=tpwrs+6;
    tpwrs = (int) (tpwrs);                       /* power than a square pulse */

   obsstepsize(1.0);
   if (phincr < 0.0) phincr=360+phincr;
   initval(phincr,v3);

   	delay(0.01);
        zgradpulse(1.5*gzlvl0,0.001);
        delay(d1);
        obspower(tpwr);
	rcvroff();
      if (satmode[A] == 'u')            /* calibrate flipback pulse following hard 90 */
       {
        obspwrf(4095.0);
        obspower(tpwr);	  				
 	rgpulse(pw, two, rof1, 0.0);
        obspwrf(tpwrsf);
        obspower(tpwrs);	  				
        xmtrphase(v3);
        shaped_pulse(shape, pwHs, zero, rof1, rof2);
       }
      if (satmode[A] == 'd')            /* calibrate flipdown pulse prior to hard 90 */
       {
        obspwrf(tpwrsf);
        obspower(tpwrs);	  				
        xmtrphase(v3);
        shaped_pulse(shape, pwHs, zero, rof1, 0.0);
        obspwrf(4095.0);
        obspower(tpwr);	  				
        xmtrphase(zero);
        delay(SAPS_DELAY);
 	rgpulse(pw, two, rof1, rof2);
       }
      if (satmode[A] == 'i')      /* calibrate flipback pulse following
                       INEPT ghn... non-TROSY mode */
       {
   	rgpulse(pw,zero,rof1,0.0);  
        zgradpulse(gzlvl0,gt0);
        delay(2.4e-3-gt0);
 	rgpulse(2.0*pw, zero, 0.0, 0.0);
	txphase(one);
        zgradpulse(gzlvl0,gt0);
        delay(2.4e-3-gt0);
 	rgpulse(pw, one, 0.0, 0.0);
        obspwrf(tpwrsf);
        obspower(tpwrs);	  				
        xmtrphase(v3);
        shaped_pulse(shape, pwHs, zero, rof1, rof2);
       }
      if (satmode[A] == 't')   /* calibrate flipback pulse following INEPT
                (gNhsqc non-Trosy, ghn..TROSY mode) */
       {
   	rgpulse(pw,zero,rof1,0.0);  
        zgradpulse(gzlvl0,gt0);
        delay(2.4e-3-gt0);
 	rgpulse(2.0*pw, zero, 0.0, 0.0);
	txphase(one);
        zgradpulse(gzlvl0,gt0);
        delay(2.4e-3-gt0);
 	rgpulse(pw, one, 0.0, 0.0);
        obspwrf(tpwrsf);
        obspower(tpwrs);	  				
        xmtrphase(v3);
        shaped_pulse(shape, pwHs, two, rof1, 0.0);
        obspower(tpwr); xmtrphase(zero); obspwrf(4095.0);
        rgpulse(2.0*pw,zero,rof1,rof2);
       }
      if (satmode[A] == 'n') /*calibrate flipback pulse following INEPT (gNhsqc TROSY mode) */
       {
   	rgpulse(pw,two,rof1,0.0);  
        zgradpulse(gzlvl0,gt0);
        delay(2.4e-3-gt0);
 	rgpulse(2.0*pw, zero, 0.0, 0.0);
	txphase(one);
        zgradpulse(gzlvl0,gt0);
        delay(2.4e-3-gt0);
 	rgpulse(pw, one, 0.0, 0.0);
        obspwrf(tpwrsf);
        obspower(tpwrs);	  				
        xmtrphase(v3);
        shaped_pulse(shape, pwHs, two, rof1, 0.0);
        obspower(tpwr); xmtrphase(zero); obspwrf(4095.0);
       }
}		 
