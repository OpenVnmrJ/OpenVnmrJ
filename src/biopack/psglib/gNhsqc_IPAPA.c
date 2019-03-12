/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gNhsqc_IPAPA.c - autocalibrated version of N15 IPAP-hsqc using 
                    3919 watergate suppression

   step cycling is 8; Ref: JMR v131, 373-378 (1998)

   With IPAP='n','y' phase=1,2 array='IPAP,phase'
   you can acquire both antiphase and in-phase spectra at the same time. To
   obtain both components with the same phase use
        wft2d(1,0,0,0,0,0,0,0,0,0,-1,0,0,0,0,0)     for in-phase signals
        wft2d(0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0)      for anti-phase signals

   To look at individual satellites you can also build sum and difference of
   the two data sets:
        wft2d(1,0,0,0,0,0,1.11,0,0,0,-1,0,1.11,0,0,0)       sum
        wft2d(1,0,0,0,0,0,-1.11,0,0,0,-1,0,-1.11,0,0,0)     difference

   Note the factor of 1.11. You can adjust this value to get good match.

   To measure exact signal positions in these component spectra, you need
   line fitting software (such as "fitspec" / deconvolution).

   The autocal and checkofs flags are generated automatically in Pbox_bio.h
   If these flags do not exist in the parameter set, they are automatically 
   set to 'y' - yes. In order to change their default values, create the flag(s) 
   in your parameter set and change them as required. 
   The available options for the checkofs flag are: 'y' (yes) and 'n' (no). 
   The offset (tof, dof, dof2 and dof3) checks can be switched off individually 
   by setting the corresponding argument to zero (0.0).
   For the autocal flag the available options are: 'y' (yes - by default), 
   'q' (quiet mode - suppress Pbox output), 'r' (read from file, no new shapes 
   are created), 's' (semi-automatic mode - allows access to user defined 
   parameters) and 'n' (no - use full manual setup, equivalent to the original code).

   Program written by Nagarajan Murali starting from the gNfhsqc.c Feb. 26, 2001
   Auto-calibrated version, E.Kupce, 27.08.2002.

*/

#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */

static int phi1[2] = {3,1},
           phi2[4] = {0,0,2,2},		/* phase for IP */
	   phi2A[4] = {3,3,1,1},	/* phase for AP */
           phi3[16] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3}, 
           phi4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
           rec[4] = {0,2,2,0}, 		/* receiver phase for IP */
	   recA[8] = {0,2,2,0,2,0,0,2}; /* receiver phase for AP */

static double d2_init = 0.0;
static double   H1ofs=4.7, C13ofs=0.0, N15ofs=120.0, H2ofs=0.0;

static shape  stC200;

void pulsesequence()
{

int     t1_counter;
char	    C13refoc[MAXSTR],		/* C13 sech/tanh pulse in middle of t1*/
	    IPAP[MAXSTR];		/* Flag for anti-phase spectrum */
double
   tau1, tauxh,
   gzlvl0=getval("gzlvl0"),
   gzlvl1=getval("gzlvl1"),
   gzlvl2=getval("gzlvl2"),
   gzlvl3 = getval("gzlvl3"),
   gt0=getval("gt0"),
   gt1=getval("gt1"),
   gt2=getval("gt2"),
   gt3=getval("gt3"),
   JNH = getval("JNH"),
   pwN = getval("pwN"),
   pwNlvl = getval("pwNlvl"),      
   sw1 = getval("sw1"),
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   rf0,            	          /* maximum fine power when using pwC pulses */
   rfst,	                           /* fine power for the stCall pulse */
                   /* temporary Pbox parameters */
   bw, pws, ofs, ppm, nst,  /* bandwidth, pulsewidth, offset, ppm, # steps */
   compC = getval("compC");       /* adjustment for C13 amplifier compression */

    getstr("C13refoc",C13refoc);
    getstr("IPAP",IPAP);	 /* IPAP = 'y' for AP; IPAP = 'n' for IP */

/* maximum fine power for pwC pulses (and initialize rfst) */
	rf0 = 4095.0;    rfst=0.0;

    setautocal();                        /* activate auto-calibration flags */ 
        
    if (autocal[0] == 'n') 
    {
      if (C13refoc[A]=='y') 
      {
        /* 180 degree adiabatic C13 pulse from 0 to 200 ppm */
        rfst = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)); 
        rfst = (int) (rfst + 0.5);
        if ( 1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n", 
	     (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) ); psg_abort(1); }
      }
    }
    else        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
    {
      if(FIRST_FID)                                            /* call Pbox */
      {
        if (C13refoc[A]=='y') 
        {
          ppm = getval("dfrq"); ofs = 0.0;   pws = 0.001;  /* 1 ms long pulse */
          bw = 200.0*ppm;       nst = 1000;          /* nst - number of steps */
          stC200 = pbox_makeA("stC200", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
          C13ofs = 100.0;
        }
        ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
      }
      rfst = stC200.pwrf;
    }

   initval(0.0,v1);
   initval(3.0,v2);
   initval(1.0,v3);
   initval(2.0,v4);

/* check validity of parameter range */

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
	{
	printf("incorrect Dec1 decoupler flags!  ");
	psg_abort(1);
    } 

    if((dm2[A] == 'y' || dm2[B] == 'y') )
	{
	printf("incorrect Dec2 decoupler flags!  ");
	psg_abort(1);
    } 


    if( dpwr > 0 )
    {
	printf("don't fry the probe, dpwr too large!  ");
	psg_abort(1);
    }

    if( dpwr2 > 50 )
    {
	printf("don't fry the probe, dpwr2 too large!  ");
	psg_abort(1);
    }

/* LOAD VARIABLES */

  settable(t1, 2, phi1);
  if (IPAP[A]=='y') settable(t2,4,phi2A);
  else settable(t2, 4, phi2); 
  settable(t3, 16, phi3);
  settable(t4, 16, phi4);
  if (IPAP[A]=='y') settable(t5,8,recA);
  else settable(t5, 4, rec);

/* INITIALIZE VARIABLES */

  tauxh = ((JNH != 0.0) ? 1/(4*(JNH)) : 2.25e-3);

/* Phase incrementation for hypercomplex data */

   if ( phase1 == 2 )     /* Hypercomplex in t1 */
   {
	if (IPAP[A] == 'y') tsadd(t3,1,4); 
        tsadd(t2, 1, 4); 
   } 


/* calculate modification to phases based on current t1 values
   to achieve States-TPPI acquisition */
 
 
   if(ix == 1)
      d2_init = d2;
      t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);

      if(t1_counter %2) {
	if (IPAP[A] == 'y') tsadd(t3,2,4);
        tsadd(t2,2,4);
        tsadd(t5,2,4);
      }
   tau1 = d2;
   tau1 = tau1/2.0 -rof1 -pw;
   if (tau1 < 0.0) tau1=0.0;

/*sequence starts!!*/

   status(A);
   obspower(tpwr);
   dec2power(pwNlvl);
   decpower(pwClvl);
   decpwrf(rfst);
   delay(d1);
   status(B);
  dec2rgpulse(pwN,v1,rof1,rof1);
  zgradpulse(gzlvl0,gt0);
  delay(1e-3); 

  rgpulse(pw, v1, rof1, rof1);

  zgradpulse(gzlvl1,gt1);

  txphase(zero);
  dec2phase(zero);

  delay(tauxh-gt1);               /* delay=1/4J(XH)   */

  sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,rof1,rof1);

  zgradpulse(gzlvl1,gt1);

  txphase(t1);
  dec2phase(t2);
  delay(tauxh-gt1 );               /* delay=1/4J(XH)   */
 
  rgpulse(pw, t1, rof1, rof1);

  zgradpulse(gzlvl2,gt2);
  delay(200.0e-6);

  dec2rgpulse(pwN, t2, rof1, 0.0);

  if (IPAP[A] == 'y') 
  {
	delay(tauxh-pwN); 
	sim3pulse(2*pw,0.0,2*pwN,zero,zero,t3,rof1,rof1);
	delay(tauxh-pwN);
	rgpulse(pw,t4,rof1,rof1);
  }	


  	if ( (C13refoc[A]=='y') && (tau1 > 0.5e-3 + WFG2_START_DELAY) )
           {delay(tau1 - 0.5e-3 - WFG2_START_DELAY);     /* WFG2_START_DELAY */
            simshaped_pulse("", "stC200", 0.0, 1.0e-3, zero, zero, 0.0, 0.0);
            delay(tau1 - 0.5e-3);}
	else
           delay(2*tau1);

  dec2rgpulse(pwN, zero, 0.0, 0.0);

  zgradpulse(gzlvl2,gt2);
  delay(200.0e-6);

  rgpulse(pw, v1, rof1, rof1);

  dec2phase(zero);


   zgradpulse(gzlvl3,gt3);
   txphase(v2);
   delay(200e-6);

   delay(tauxh-gt3-200e-6-pw*2.385-6.0*rof1 -d3*2.5);
 
   rgpulse(pw*0.231,v2,rof1,rof1);
   delay(d3);
   rgpulse(pw*0.692,v2,rof1,rof1);
   delay(d3);
   rgpulse(pw*1.462,v2,rof1,rof1);

   delay(d3/2-pwN);
   dec2rgpulse(2*pwN, zero, rof1, rof1);
   delay(d3/2-pwN);

   rgpulse(pw*1.462,v3,rof1,rof1);
   delay(d3);
   rgpulse(pw*0.692,v3,rof1,rof1);
   delay(d3);
   rgpulse(pw*0.231,v3,rof1,rof1);
   zgradpulse(gzlvl3,gt3);
   delay(tauxh-gt3-pw*2.385-6.0*rof1 -d3*2.5);
   dec2power(dpwr2);
status(C);
   setreceiver(t5);

}
