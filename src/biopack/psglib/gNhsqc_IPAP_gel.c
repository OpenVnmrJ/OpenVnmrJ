/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gNhsqc_IPAP_gel.c

Sequence is modified gNhsqc_IPAP.c by Eric Condamine, 
IBS, Grenoble, France  

Based on Ref: J Biomol NMR. 2001 Oct;21(2):141-51.
"N15 IPAP-hsqc using 3919 watergate suppression 
technique, modified to suppress naturel-abundance 15N 
signals from NH2 groups of polyacrylamide gel"

First tests done by Eric C. seem to indicate a S/N 
reduction near 17% and a good suppression of the gel 
signals, comparatively to the standard Bax sequence. 
Parameters are coming from standard gNhsqc.c and 
have not yet been optimized specifically for this 
sequence. To setup the experiment call the Biopack 
gNhsqc_IPAP macro (i.e. Experiments>>Protein C13/N15 
experiments> N15 coupling measurements> N15 HSQC IPAP). 

Acquisition:
Gel='y' IPAP='n','y' phase=1,2 array='IPAP,phase'
May be usefull to play with d3 (for watergate). 
Excitation of all resonances except those of the 
offset of the carrier and at positions k/d3 
(k is an integer).

Processing:
you can acquire both antiphase and in-phase spectra 
at the same time. To obtain both components with 
the same phase use
  wft2d(1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0)   (in-phase) 
  wft2d(0,0,0,0,0,0,1,0,0,0,0,0,-1,0,0,0)  (anti-phase)

To look at individual satellites you can also build 
sum and difference of the two data sets:
  wft2d(1,0,0,0,0,0,1.11,0,0,0,1,0,-1.11,0,0,0) sum
  wft2d(1,0,0,0,0,0,-1.11,0,0,0,1,0,1.11,0,0,0) diff

Note the factor of 1.11. You can adjust this value 
to get a good match.

To measure exact signal positions in these component 
spectra, you need line fitting software 
(such as "fitspec" / deconvolution).

*/   
#include <standard.h>

static double d2_init = 0.0;

static int phi10[2] = {3,1},
           phi1[4] = {0,0,2,2},
           phi2[8] = {0,0,0,0,2,2,2,2}, /* phase for IP */
           phi2A[8] = {1,1,1,1,3,3,3,3},/* phase for AP */
           phi3[4] = {0,2,2,0},
           phi30[4] = {1,3,3,1},
	   rec[8] = {0,2,2,0,2,0,0,2};

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
   gzlvl3 = getval("gzlvl3"),    /* watergate gradient DAC */
   gt0=getval("gt0"),
   gt1=getval("gt1"),
   gt2=getval("gt2"),
   gt3=getval("gt3"),            /* watergate gradient pulse (s) */
   JNH = getval("JNH"),           /* 1JNH coupling constant (near 92 Hz) */
   pwN = getval("pwN"),
   pwNlvl = getval("pwNlvl"),      
   sw1 = getval("sw1"),
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   rf0,            	          /* maximum fine power when using pwC pulses */
   rfst,	                           /* fine power for the stCall pulse */

   compC = getval("compC");       /* adjustment for C13 amplifier compression */

   getstr("C13refoc",C13refoc);
   getstr("IPAP",IPAP);	 /* IPAP = 'y' for AP; IPAP = 'n' for IP */

/* maximum fine power for pwC pulses (and initialize rfst) */
	rf0 = 4095.0;    rfst=0.0;

/* 180 degree adiabatic C13 pulse from 0 to 200 ppm */
     if (C13refoc[A]=='y')
       {rfst = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35));   
	rfst = (int) (rfst + 0.5);
	if ( 1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n", 
	    (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) ); psg_abort(1); }}

   initval(0.0,v1); /* x phase */
   initval(3.0,v2); /* -y watergate first 1H train phase */
   initval(1.0,v3); /* y watergate second 1H train phase */
   initval(2.0,v4); /* not used in the pulse seq */

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

  settable(t10, 2, phi10);
  settable(t1, 4, phi1);
  if (IPAP[A]=='y') settable(t2,8,phi2A);
  else settable(t2, 8, phi2); 
  settable(t3, 4, phi3);
  settable(t30, 4, phi30);
  settable(t4, 8, rec);

/* INITIALIZE VARIABLES */

  tauxh = ((JNH != 0.0) ? 1/(4*(JNH)) : 2.25e-3);

/* Phase incrementation for hypercomplex data */

   if ( phase1 == 2 ) tsadd(t2,1,4);  /* Hypercomplex in t1 */
	
/* calculate modification to phases based on current t1 values
   to achieve States-TPPI acquisition */
 
 
   if(ix == 1)
      d2_init = d2;
      t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);

      if(t1_counter %2) {
        tsadd(t2,2,4);
        tsadd(t4,2,4);
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

  txphase(t10);
  dec2phase(t1);
  delay(tauxh-gt1 );               /* delay=1/4J(XH)   */
 
  rgpulse(pw, t10, rof1, rof1);

  zgradpulse(gzlvl2,gt2);
  delay(200.0e-6);

  dec2rgpulse(pwN, t1, rof1, 0.0);

  delay(tauxh-pwN);

  sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,rof1,0.0);

  delay(tauxh-pwN);

  sim3pulse(pw,0.0,pwN,zero,zero,t3,rof1,0.0);

  if (IPAP[A] == 'n') 
  {
	delay(tauxh-pwN); 
	sim3pulse(2*pw,0.0,2*pwN,t3,zero,zero,rof1,rof1);
	delay(tauxh-pwN);
	dec2rgpulse(pwN, t30, rof1, rof1);
  }	

  	if ( (C13refoc[A]=='y') && (tau1 > 0.5e-3 + WFG2_START_DELAY) )
           {delay(tau1 - 0.5e-3 - WFG2_START_DELAY);     /* WFG2_START_DELAY */
            simshaped_pulse("", "stC200", 0.0, 1.0e-3, zero, zero, 0.0, 0.0);
            delay(tau1 - 0.5e-3);}
	else
           delay(2*tau1);

  dec2rgpulse(pwN, t2, 0.0, 0.0);

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
   setreceiver(t4);

}
