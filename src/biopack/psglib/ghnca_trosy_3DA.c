/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghnca_trosy_3DA.c - auto-calibrated version of the original sequence
 
    This pulse sequence will allow one to perform the following
    experiment:

    TROSY-based 3D hnca with deuterium decoupling
	F1 	C-alpha		(constant-time with fCT='y')
	F2 	N + JNH/2
	F-acq	HN - JNH/2

    This sequence uses the four channel configuration
         1)  1H             - carrier @ 4.7 ppm [H2O]
         2) 13C             - carrier @ 56 ppm (CA)
         3) 15N             - carrier @ 120 ppm  
         4) 2H		    - carrier @ 4.5 ppm 

    Set dm = 'nnn', dmm = 'ccc' 
    Set dm2 = 'nnn', dmm2 = 'ccc' [NO 15N decoupling during acquisition]
    Set dm3 = 'nnn', dmm2 = 'ccn' 

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI
    acquisition in t1 [Ca]  and t2 [N]. [The fids must be manipulated
    (add/subtract) with 'grad_sort_nd' program (or equivalent) before regular
    processing.]
    
    Flags
	fsat		'y' for presaturation of H2O
	fscuba		'y' for apply scuba pulse after presaturation of H2O
	f1180		'y' for 180 deg linear phase correction in F1 
		        	otherwise 0 deg linear phase correction
	f2180		'y' for 180 deg linear phase correction in F2
			    otherwise 0 deg linear phase correction
	fCT		'y' for constant time of t1 (C-alpha)
	fc180		'y' for C-alpha refocusing in t1 (only for fCT='n')
        sel_flg         'y' for active suppression of the fast relaxing
                            component
                        'n' for relaxation suppression of the fast relaxing
                            component
                         NO longer needed as of Nov 30, 2004 
        nietl_flg       'y' suppresses the anti-trosy component with no losses in s/n
                         (D.Nietlispach, J.Biomol.NMR, 31,161(2005))
        shared_flg      'y' allows acquisition in F2 for longer than 2bigTN
                            using the shared constant time principle

	Standard Settings (constant-time Ca evolution)
	fsat='n',fscuba='n',f1180='n',f2180='n',fCT='y'
	fc180='n'

      Use ampmode statement = 'dddp'

    This scheme uses TROSY principle in 15N and NH with transfer of
    magnetization achieved using the enhanced sensitivity pfg approach,
 
    Yang and Kay, J. Biomol. NMR, 1999, 13, 3-9.
    Yang and Kay, J. Am. Chem. Soc, 1999, In Press.
 
    If sel_flg =='n' then the component of interest is not actively
    selected for; relaxation will attenuate the undesired component.
    Selection is achieved during the first N---> 13C' transfer.
 
    In general it is very difficult to present rules indicating when
    sel_flg should be set to y or n. In the case of applications at
    750 or 800 MHz and for correlation times in excess of 20 ns the
    sel_flg can be set to no and selection via relaxation should suffice.
 
    At lower field or for smaller molecules set sel_flg == 'y', for
    example at 500 or 600 MHz and for correlation times between 15 and 25ns
    set sel_flg=='y'. We have used sel_flg=='n' for a molecule with a
    correlation time of 43 ns and not observed any residual signal from
    the undesired component.
 
    See Figure 3 in Yang and Kay, J. Biomol NMR, 1999, 13, 3-9.

REF: Yamazaki et. al.   J. Am. Chem. Soc. 116, 11655 (1994)
     Yamazaki et. al.   J. Am. Chem. Soc. 116,  6464 (1994)
     Pervushin et al. PNAS, 94, 12366-12371 (1997)
     Pervushin et al. J. Biomol. NMR Aug 1998
     Yang and Kay, J. Biomol. NMR, 13, 3-9.
     Yang and Kay, J. Am. Chem. Soc., 1999, In Press.

     Written by L.E.Kay, Oct 30, 1998  based on hnca_tydw.c

     Setup: Delta = difference in Hz between 176 and 58 ppm

     pwca90:  Use a field of delta/sq(15), on res.
     pwca180: Use a field of delta/sq(3), on res.
     pwcareb: Use a 375 us pulse centered at 45 ppm
     pwcosed: Use a ~ 220 us (at 600) seduce pulse for C' inversion
     pwco180: Used only if fCT == 'n'. Set to delta/sq(3), phase modulated
              by +delta Hz.

     Modified by L.E.Kay on March 3, 1999 to insert a Ca purge after the
     15N evolution time

     Modified by L.E.Kay to include the Nietlispach method
        so that the anti-trosy component is eliminated without setting
        sel_flg to y

    USAGE: set nietl_flg to y
    Modified by E.Kupce for autocalibration using hnca_D_trosy_lek_500a.c
     January 2005
    Modified by G. Gray for BioPack, Feb 2005.

*/


#include <standard.h>
#define DELAY_BLANK 0.0e-6
#include "Pbox_bio.h"

#define CA180reb  "reburp 80p -13p"             /* RE-BURP 180 on Cab at 43 ppm, 13 ppm away */
#define CA180ps   "-s 0.5 -attn i"                           /* RE-BURP 180 shape parameters */
#define CO180     "square180n 118p 118p"          /* hard 180 on C', at 174 ppm 118 ppm away */
#define CO180ps   "-s 0.2 -attn d"                              /* hard 180 shape parameters */
#define CA180     "square180n 118p"                /* hard 180 on CA, at 56 ppm on-resonance */
#define CA90      "square90n 118p"                  /* hard 90 on CA, at 56 ppm on-resonance */

static shape ca90, ca180, ca180reb, co180;

static int  phi1[2]  = {1,3},
            phi2[8]  = {0,0,0,0,2,2,2,2}, 
	    phi3[1]  = {0},
	    phi4[4]  = {0,0,2,2},
	    phi5[8]  = {0,0,0,0,2,2,2,2},
            rec[4]   = {0,2,2,0};

static double d2_init=0.0, d3_init=0.0;

void pulsesequence()

{

/* DECLARE VARIABLES */

 char       fsat[MAXSTR],
	    fscuba[MAXSTR],
            fCT[MAXSTR],       /* Flag for constant time C13 evolution */
            fc180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell             */
            ddseq[MAXSTR],    /* deuterium decoupling sequence */
            spcosed[MAXSTR],  /* CO seduce 180 pulse during t1 and t2  */  
            spco180[MAXSTR],   
            spcareb[MAXSTR],  /* Ca 180 reburp pulse */
            shp_sl[MAXSTR],
            sel_flg[MAXSTR],
            shared_CT[MAXSTR],
            nietl_flg[MAXSTR],
            autocal[MAXSTR];  /* auto-calibration flag */

 int         phase, phase2, ni, ni2,icosel, 
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter;   /* used for states tppi in t2           */ 

 double      tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             taua,         /*  ~ 1/4JNH =  2.25 ms */
             taub,         /*  ~ 1/4JNH =  2.75 ms */
             tauc,         /*  ~ 1/4JNCa =  13 ms */
             bigTN,        /* nitrogen T period */
             bigTC,        /* carbon T period */
             pwca90,       /* PW90 for ca nucleus @ dhpwr         */
             pwca180,       /* PW180 for ca nucleus @ dhpwr         */
             pwcareb,       /* PW180 for ca nucleus @ dhpwr         */
             pwcosed,       /* PW180 for ca nucleus @ d_csed         */
             pwco180,       /* PW180 for ca nucleus @ d_co180         */
             tsatpwr,      /* low level 1H trans.power for presat  */
             d_c90,        /* power level for 13C pulses on dec1 - 64 us 
                              90 for part a of the sequence  */
             d_c180,
             d_creb,
	     d_csed,
	     d_co180,

             pwD,          /* PW90 for higher power (pwDlvl) deut 90 */
             pwDlvl,       /* high power level for deuterium decoupling */

             sw1,          /* sweep width in f1                    */             
             sw2,          /* sweep width in f2                    */             
             pw_sl,        /* selective pulse on water      */
             tpwrsl,       /* power for pw_sl               */
             at,
             sphase,
             phase_sl,
             phase_sl1,

             pw_sl1,
             tpwrsl1,

             compC,       /* C-13 RF calibration parameters */
             pwC,
             pwClvl,

             pwN,          /* PW90 for 15N pulse              */
	     pwNlvl,       /* high dec2 pwr for 15N hard pulses    */

             gstab,

             gt1,
             gt2,
             gt3,
             gt4,
             gt5,
             gt6,
             gt7,
             gt8,
             gt9,
             gt10,

             gzlvl1,
             gzlvl2,
             gzlvl3,
             gzlvl4,
             gzlvl5,
             gzlvl6,
             gzlvl7,
             gzlvl9,
             gzlvl10;
            
/* LOAD VARIABLES */

  getstr("fsat",fsat);
  getstr("fCT",fCT);
  getstr("fc180",fc180);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("fscuba",fscuba);
  getstr("ddseq",ddseq);
  getstr("shp_sl",shp_sl);
  getstr("autocal",autocal);

  getstr("sel_flg",sel_flg);
  getstr("shared_CT",shared_CT);

  getstr("nietl_flg",nietl_flg);

  taua   = getval("taua"); 
  taub   = getval("taub"); 
  tauc   = getval("tauc"); 
  bigTN = getval("bigTN");
  bigTC = getval("bigTC");
  tpwr = getval("tpwr");
  tsatpwr = getval("tsatpwr");
  dpwr = getval("dpwr");
  pwN = getval("pwN");
  pwNlvl = getval("pwNlvl");
  pwD = getval("pwD");
  pwDlvl = getval("pwDlvl");
  phase = (int) ( getval("phase") + 0.5);
  phase2 = (int) ( getval("phase2") + 0.5);
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  ni2 = getval("ni2");
  ni = getval("ni");
  pw_sl = getval("pw_sl");
  tpwrsl = getval("tpwrsl");
  at = getval("at");
  sphase = getval("sphase");
  phase_sl = getval("phase_sl");
  phase_sl1 = getval("phase_sl1");

  pw_sl1 = getval("pw_sl1");
  tpwrsl1 = getval("tpwrsl1");

  gstab = getval("gstab");

  gt1 = getval("gt1");
  if (getval("gt2") > 0) gt2=getval("gt2");
    else gt2=gt1*0.1;
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");
  gt6 = getval("gt6");
  gt7 = getval("gt7");
  gt8 = getval("gt8");
  gt9 = getval("gt9");
  gt10 = getval("gt10");

  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl6 = getval("gzlvl6");
  gzlvl7 = getval("gzlvl7");
  gzlvl9 = getval("gzlvl9");
  gzlvl10 = getval("gzlvl10");

  if(autocal[0]=='n')
  {
    getstr("spcosed",spcosed);
    getstr("spco180",spco180);
    getstr("spcareb",spcareb);
    
    pwca90 = getval("pwca90");
    pwca180 = getval("pwca180");
    pwcareb = getval("pwcareb");
    pwcosed = getval("pwcosed");
    pwco180 = getval("pwco180");
    d_c90 = getval("d_c90");
    d_c180 = getval("d_c180");
    d_creb = getval("d_creb");
    d_csed = getval("d_csed");
    d_co180 = getval("d_co180");  
  }
  else
  {
    strcpy(spcosed,"Phard_118p");
    strcpy(spco180,"Phard_118p");
    strcpy(spcareb,"Preburp_-15p");
    if (FIRST_FID)
    {
      compC = getval("compC");
      pwC = getval("pwC");
      pwClvl = getval("pwClvl");
      ca180reb = pbox(spcareb, CA180reb, CA180ps, dfrq, compC*pwC, pwClvl);      
      co180 = pbox(spco180, CO180, CO180ps, dfrq, compC*pwC, pwClvl);
      ca90 = pbox("Pca90", CA90, CO180ps, dfrq, compC*pwC, pwClvl);  
      ca180 = pbox("Pca180", CA180, CO180ps, dfrq, compC*pwC, pwClvl);            
    }
    
    pwca90 = ca90.pw;
    pwca180 = ca180.pw;
    pwcareb = ca180reb.pw;
    pwcosed = co180.pw;
    pwco180 = co180.pw;
    d_c90 = ca90.pwr;
    d_c180 = ca180.pwr;
    d_creb = ca180reb.pwr;
    d_csed = co180.pwr;
    d_co180 = co180.pwr;    
  }   

/* LOAD PHASE TABLE */

  settable(t1,2,phi1);
  settable(t2,8,phi2);
  settable(t3,1,phi3);
  settable(t4,4,phi4);
  settable(t5,8,phi5);
  settable(t6,4,rec);

/* CHECK VALIDITY OF PARAMETER RANGES */

  if (shared_CT[A] == 'n')
   if (bigTN - 0.5*(ni2-1)/sw2 + pwca180 < 0.2e-6)
    {
        text_error(" ni2 is too big\n");
        text_error(" please set ni2 smaller or equal to %d\n",
   			(int) ((bigTN +pwca180)*sw2*2.0) +1 );
        psg_abort(1);
    }
   
   if(fCT[A] == 'y')
    if(bigTC - 0.5*(ni-1)/sw1 - WFG_STOP_DELAY
                - gt10 - 100.2e-6
                - POWER_DELAY - 4.0e-6 - pwD
                - POWER_DELAY - WFG_START_DELAY
		- POWER_DELAY < 0.2e-6)
    {
        text_error(" ni is too big\n");
        text_error(" please set ni smaller or equal to %d\n",
    				(int) ((bigTC - WFG_STOP_DELAY
                			- gt10 - 100.2e-6
                			- POWER_DELAY - 4.0e-6 - pwD
                			- POWER_DELAY - WFG_START_DELAY
					- POWER_DELAY)*sw1*2.0) +1 );
        psg_abort(1);
    }

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
    {
        text_error("incorrect dec1 decoupler flags!  ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y'))
    {
        text_error("incorrect dec2 decoupler flags! Should be 'nnn' ");
        psg_abort(1);
    }

    if( tsatpwr > 6 )
    {
        text_error("TSATPWR too large !!!  ");
        psg_abort(1);
    }

    if( dpwr > 46 )
    {
        text_error("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

    if( dpwr2 > 47 )
    {
        text_error("don't fry the probe, DPWR2 too large!  ");
        psg_abort(1);
    }


    if( pwNlvl > 63 )
    {
        text_error("don't fry the probe, pwNlvl too large!  ");
        psg_abort(1);
    }

    if( pw > 200.0e-6 )
    {
        text_error("dont fry the probe, pw too high ! ");
        psg_abort(1);
    } 
    if( pwN > 200.0e-6 )
    {
        text_error("dont fry the probe, pwN too high ! ");
        psg_abort(1);
    } 


    if( gt1 > 15e-3 || gt2 > 15e-3 || gt3 > 15e-3 
	|| gt4 > 15e-3 || gt5 > 15e-3 || gt6 > 15e-3 
	|| gt7 > 15e-3 || gt8 > 15e-3 || gt9 > 15e-3 || gt10 > 15e-3 ) 
    {
       text_error("gti values must be < 15e-3\n");
       psg_abort(1);
    } 

    if(tpwrsl > 25) {
       text_error("tpwrsl must be less than 25\n");
       psg_abort(1);
    }

    if(tpwrsl1 > 25) {
       text_error("tpwrsl1 must be less than 25\n");
       psg_abort(1);
    }

    if( pwDlvl > 59) {
       text_error("pwDlvl too high\n");
       psg_abort(1);
    }

    if( dpwr3 > 50) {
       text_error("dpwr3 too high\n");
       psg_abort(1);
    }

    if( pw_sl > 10e-3) {
       text_error("too long pw_sl\n");
       psg_abort(1);
    }

    if( pw_sl1 > 10e-3) {
       text_error("too long pw_sl1\n");
       psg_abort(1);
    }

    if(sel_flg[A] == 'y' && nietl_flg[A] == 'y') {
       text_error("both flags cannot by y\n");
       psg_abort(1);
   }

    if (fCT[A] == 'n' && fc180[A] =='y' && ni > 1.0) {
       text_error("must set fc180='n' to allow Calfa evolution (ni>1)\n");
       psg_abort(1);
   }

/*  Phase incrementation for hypercomplex 2D data */

   if (phase == 2) tsadd(t4,1,4);

   if (shared_CT[A] == 'n') 
     {
      if (phase2 == 2) { tsadd(t3,2,4); icosel = 1; }
        else icosel = -1;
     }
    else 
     {
      if (phase2 == 2) { tsadd(t3,2,4); icosel = -1; }
        else icosel = 1;
     }

   if (nietl_flg[A] == 'y') icosel = -1*icosel;

/*  Set up f1180  tau1 = t1               */
      tau1 = d2;

if (fCT[A]=='y' && f1180[A] == 'y')
  {
          tau1 += ( 1.0 / (2.0*sw1) );
          if(tau1 < 0.2e-6) tau1 = 0.4e-6;
  }

if (fCT[A] == 'n' && f1180[A] == 'y' && fc180[A] =='n')
  {
          tau1 += ( 1.0 / (2.0*sw1) - 4.0*pwca90/PI - WFG_START_DELAY
		    - pwco180 - WFG_STOP_DELAY - 8.0e-6 - 2.0*POWER_DELAY);
          if(tau1 < 0.2e-6) {
              tau1 = 0.4e-6;
              text_error("tau1 is negative; decrease sw1\n");
          }
  }  

if(fCT[A] == 'n' && f1180[A] == 'n' && fc180[A] =='n') 
  {
          tau1 = (tau1 - 4.0*pwca90/PI - WFG_START_DELAY
		    - pwco180 - WFG_STOP_DELAY - 8.0e-6 - 2.0*POWER_DELAY);
          if(tau1 < 0.2e-6) {
              text_error("tau1 is %f; set to zero\n",tau1);
              tau1 = 0.4e-6;
          }
  }  

tau1 = tau1/2.0;

/*  Set up f2180  tau2 = t2               */

    tau2 = d3;
    if(f2180[A] == 'y') {
        tau2 += ( 1.0 / (2.0*sw2) ); 
        if(tau2 < 0.2e-6) tau2 = 0.2e-6;
    }
        tau2 = tau2/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) {
      tsadd(t4,2,4);     
      tsadd(t6,2,4);    
    }

   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) {
      tsadd(t1,2,4);  
      tsadd(t6,2,4);    
    }


/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tsatpwr);     /* Set transmitter power for 1H presaturation */
   decpower(d_c90);       /* Set Dec1 power for Ca 90 pulses         */
   dec2power(pwNlvl);     /* Set Dec2 power for 15N hard pulses         */

/* Presaturation Period */

   if (fsat[0] == 'y')
   {
	delay(2.0e-5);
    	rgpulse(d1,zero,2.0e-6,2.0e-6);
   	obspower(tpwr);      /* Set transmitter power for hard 1H pulses */
	delay(2.0e-5);
	if(fscuba[0] == 'y')
	{
		delay(2.2e-2);
		rgpulse(pw,zero,2.0e-6,0.0);
		rgpulse(2*pw,one,2.0e-6,0.0);
		rgpulse(pw,zero,2.0e-6,0.0);
		delay(2.2e-2);
	}
   }
   else
   {
    delay(d1);
   }
   obspower(tpwr);           /* Set transmitter power for hard 1H pulses */
   txphase(zero);
   dec2phase(zero);
   delay(1.0e-5);

/* Begin Pulses */

status(B);

   rcvroff();
   lk_hold();
   delay(20.0e-6);

   decrgpulse(pwca90,zero,4.0e-6,4.0e-6);

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */

   delay(0.2e-6);
   zgradpulse(gzlvl5,gt5);
   delay(2.0e-6);

   delay(taua - gt5 - 2.2e-6);   /* taua <= 1/4JNH */ 

   sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);

   dec2phase(zero); decphase(zero); 

   delay(taua - gt5 - 200.2e-6); 

   delay(0.2e-6);
   zgradpulse(gzlvl5,gt5);
   delay(200.0e-6);
 
   if (sel_flg[A] =='y') 
     {
      rgpulse(pw,one,4.0e-6,0.0);
      initval(1.0,v2);
      obsstepsize(phase_sl);
      xmtrphase(v2);
 
      /* shaped pulse */
      obspower(tpwrsl);
      shaped_pulse(shp_sl,pw_sl,two,4.0e-6,0.0);
      xmtrphase(zero);
      delay(2.0e-6);
      obspower(tpwr);
      /* shaped pulse */

      initval(1.0,v6);
      dec2stepsize(45.0);
      dcplr2phase(v6);

      decpower(d_c180);

      delay(0.2e-6);
      zgradpulse(gzlvl3,gt3);
      delay(200.0e-6);

      dec2rgpulse(pwN,zero,0.0,0.0);
      dcplr2phase(zero);

      delay(1.34e-3 - SAPS_DELAY);

      rgpulse(pw,zero,0.0,0.0);
      rgpulse(2.0*pw,one,2.0e-6,0.0);
      rgpulse(pw,zero,2.0e-6,0.0);

      delay(tauc - 1.34e-3 - 4.0*pw - 4.0e-6);
     }
    else 
     {
      rgpulse(pw,three,4.0e-6,0.0);
      initval(1.0,v2);
      obsstepsize(phase_sl);
      xmtrphase(v2);
 
      /* shaped pulse */
      obspower(tpwrsl);
      shaped_pulse(shp_sl,pw_sl,zero,4.0e-6,0.0);
      xmtrphase(zero);
      delay(2.0e-6);
      obspower(tpwr);
      /* shaped pulse */

      decpower(d_c180);

      delay(0.2e-6);
      zgradpulse(gzlvl3,gt3);
      delay(200.0e-6);

      dec2rgpulse(pwN,zero,0.0,0.0);

      delay(tauc);
     }

   dec2rgpulse(2*pwN,zero,0.0,0.0);
   decrgpulse(pwca180,zero,0.0,0.0);
   dec2phase(one); 

   delay(tauc - pwca180);

   dec2rgpulse(pwN,one,0.0e-6,0.0);

   decphase(t4);

   decpower(d_c90);

   delay(0.2e-6);
   zgradpulse(gzlvl4,gt4);
   delay(200.0e-6);

   dec2phase(zero);  txphase(zero);

   /* Turn on D decoupling using the third decoupler */
   dec3phase(one);
   dec3power(pwDlvl);
   dec3rgpulse(pwD,one,4.0e-6,0.0);
   dec3phase(zero);
   dec3power(dpwr3);
   dec3unblank();
   setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
   /* Turn on D decoupling */
   
   decrgpulse(pwca90,t4,2.0e-6,0.0);

   if (fCT[A]=='y')
     {
      delay(tau1);

      decpower(d_csed);
      dec2rgpulse(pwN,one,0.0,0.0); 
      dec2rgpulse(2*pwN,zero,0.0,0.0); 
      dec2rgpulse(pwN,one,0.0,0.0); 
      
      decshaped_pulse(spcosed,pwcosed,zero,4.0e-6,0.0);
      
      /* now set phase of C 180 pulse */
       
      decpower(d_creb);
       
      initval(1.0,v3);
      decstepsize(sphase);
      dcplrphase(v3);
      	
      delay(bigTC - POWER_DELAY
      		- 4.0*pwN - 4.0e-6 - WFG_START_DELAY - pwcosed - WFG_STOP_DELAY
      		- POWER_DELAY
      		- PRG_STOP_DELAY - POWER_DELAY - 4.0e-6 - pwD
      		- gt10 - 100.2e-6 - WFG_START_DELAY);
      
      /* Turn off D decoupling */
      setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
      dec3blank();  
      dec3power(pwDlvl);
      dec3rgpulse(pwD,three,4.0e-6,0.0);
      /* Turn off D decoupling */
      
      delay(0.2e-6);
      zgradpulse(gzlvl10,gt10);
      delay(100.0e-6);
      
      decshaped_pulse(spcareb,pwcareb,zero,0.0,0.0);
      dcplrphase(zero);
      
      delay(0.2e-6);
      zgradpulse(gzlvl10,gt10);
      delay(100.0e-6);
      
      /* Turn on D decoupling using the third decoupler */
      dec3phase(one);
      dec3power(pwDlvl);
      dec3rgpulse(pwD,one,4.0e-6,0.0);
      dec3phase(zero);
      dec3power(dpwr3);
      dec3unblank();
      setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
      /* Turn on D decoupling */
      
      decpower(d_c90);
      
      delay(bigTC - tau1 - WFG_STOP_DELAY
      		- gt10 - 100.2e-6
      		- POWER_DELAY - 4.0e-6 - pwD
      		- POWER_DELAY - WFG_START_DELAY
      		- POWER_DELAY);
      
      decrgpulse(pwca90,zero,0.0,0.0);
     }
    else /* BEGIN (fCT[A]=='n') */
     {
      if (fc180[A]=='n' && (tau1 - pwN > 0.2e-6))
        {
         decphase(t5); dec2phase(zero);
         decpower(d_co180);
         delay(tau1 - pwN);
         decshaped_pulse(spco180,pwco180,t5,4.0e-6,0.0);
         dec2rgpulse(2.0*pwN,zero,0.0,0.0);
         decphase(zero);
         decpower(d_c90);
         delay(tau1 - pwN);
        }
       else if (fc180[A]=='n' && (tau1 - pwN <= 0.2e-6))
        {
         decphase(t5); dec2phase(zero);
         decpower(d_co180);
         delay(tau1);
         decshaped_pulse(spco180,pwco180,t5,4.0e-6,0.0);
         decphase(zero);
         decpower(d_c90);
         delay(tau1);
        }
       else if (fc180[A]=='y') 
        {
         decpower(d_c180);
         decrgpulse(pwca180,zero,4.0e-6,0.0);
         decpower(d_c90);
        }

      decrgpulse(pwca90,zero,4.0e-6,0.0);

      if (fc180[A]=='n' && (tau1 - pwN <= 0.2e-6))
        dec2rgpulse(2.0*pwN,zero,0.0,0.0);

      if (fc180[A]=='n' && (tau1 - pwN > 0.2e-6))
        delay(2.0*pwN);
     }
/* END (fCT[A]=='n') */

   /* Turn off D decoupling */
   setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
   dec3blank();  
   dec3power(pwDlvl);
   dec3rgpulse(pwD,three,4.0e-6,0.0);
   /* Turn off D decoupling */

   decpower(d_c180);
   txphase(zero);

   delay(2.0e-7);
   zgradpulse(gzlvl9,gt9);
   delay(100.0e-6);

   if (shared_CT[A] == 'n') 
     {
      dec2rgpulse(pwN,t1,2.0e-6,0.0);

      dec2phase(t2);

      delay(bigTN - tau2 + pwca180);

      dec2rgpulse(2*pwN,t2,0.0,0.0);
      decrgpulse(pwca180,zero,0.0,0.0);

      dec2phase(t3);

      decpower(d_csed);

      delay(bigTN - gt1 - 502.0e-6 - 2.0*GRADIENT_DELAY - POWER_DELAY
	    - WFG_START_DELAY - pwcosed - WFG_STOP_DELAY);

      delay(2.0e-6);
      zgradpulse(gzlvl1,gt1);
      delay(500.0e-6);

      decshaped_pulse(spcosed,pwcosed,zero,0.0,0.0); 

      delay(tau2);

      sim3pulse(pw,0.0e-6,pwN,zero,zero,t3,0.0,0.0);
     }
    else if(shared_CT[A] == 'y') 
     {
      dec2rgpulse(pwN,t1,2.0e-6,2.0e-6);
      dec2phase(t2);

      if (bigTN - tau2 >= 0.2e-6) 
        {
         delay(tau2);

         decpower(d_csed);
         decshaped_pulse(spcosed,pwcosed,zero,4.0e-6,0.0); 
         decpower(d_c180);

         delay(2.0e-6);
         zgradpulse(gzlvl1,gt1);
         delay(500.0e-6);

         delay(bigTN - POWER_DELAY
                - 4.0e-6 - WFG_START_DELAY - pwcosed
                - WFG_STOP_DELAY - POWER_DELAY
                - gt1 - 502.0e-6 - 2.0*GRADIENT_DELAY
                - pwca180); 

         decrgpulse(pwca180,zero,0.0,0.0);
         dec2rgpulse(2*pwN,t2,0.0,0.0);
         txphase(zero);
         dec2phase(t3);

         delay(bigTN - tau2);
        }
       else 
        {
         delay(tau2);

         decpower(d_csed);
         decshaped_pulse(spcosed,pwcosed,zero,4.0e-6,0.0); 
         decpower(d_c180);

         delay(2.0e-6);
         zgradpulse(gzlvl1,gt1);
         delay(500.0e-6);

         delay(bigTN - POWER_DELAY - 4.0e-6 - WFG_START_DELAY
                - pwcosed - WFG_STOP_DELAY - POWER_DELAY
                - gt1 - 502.0e-6 - 2.0*GRADIENT_DELAY
                - pwca180);

         decrgpulse(pwca180,zero,0.0,0.0);

         delay(tau2 - bigTN);
         dec2rgpulse(2*pwN,t2,0.0,0.0);
        }
      sim3pulse(pw,0.0e-6,pwN,zero,zero,t3,2.0e-6,0.0);
     }   
/* end of shared_CT   */


  if (nietl_flg[A] == 'n') 
    {
     decpower(d_c90);
     decrgpulse(pwca90,zero,4.0e-6,0.0);

     delay(0.2e-6);
     zgradpulse(gzlvl6,gt6);
     delay(2.0e-6);
 
     dec2phase(zero);
     delay(taub - POWER_DELAY - 4.0e-6 - pwca90 - gt6 - 2.2e-6);
 
     sim3pulse(2*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);
 
     delay(0.2e-6);
     zgradpulse(gzlvl6,gt6);
     delay(200.0e-6);

     txphase(one);
     dec2phase(one);
   
     delay(taub - gt6 - 200.2e-6);
 
     sim3pulse(pw,0.0e-6,pwN,one,zero,one,0.0,0.0);
 
     delay(0.2e-6);
     zgradpulse(gzlvl7,gt7);
     delay(2.0e-6);
 
     txphase(zero);
     dec2phase(zero);
 
     delay(taub - gt7 - 2.2e-6);
 
     sim3pulse(2*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);
 
     delay(0.2e-6);
     zgradpulse(gzlvl7,gt7);
     delay(200.0e-6);
 
     delay(taub - gt7 - 200.2e-6);

     sim3pulse(pw,0.0e-6,pwN,zero,zero,zero,0.0,0.0);
    }
   else if (nietl_flg[A] == 'y') 
    {
     /* shaped pulse */

     initval(1.0,v4);
     obsstepsize(phase_sl1);
     xmtrphase(v4);

     obspower(tpwrsl1);
     shaped_pulse(shp_sl,pw_sl1,zero,4.0e-6,0.0);
     delay(2.0e-6);
     xmtrphase(zero);
     obspower(tpwr);
     /* shaped pulse */

     decpower(d_c90);
     decrgpulse(pwca90,zero,4.0e-6,0.0);

     delay(0.2e-6);
     zgradpulse(gzlvl6,gt6);
     delay(2.0e-6);
 
     dec2phase(zero);
     delay(taub - POWER_DELAY - 4.0e-6 - pwca90 
            - POWER_DELAY - WFG_START_DELAY - 4.0e-6 - pw_sl1 - WFG_STOP_DELAY
            - 2.0*SAPS_DELAY - 2.0e-6 - POWER_DELAY 
            - gt6 - 2.2e-6);
 
     sim3pulse(2*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);
 
     delay(0.2e-6);
     zgradpulse(gzlvl6,gt6);
     delay(200.0e-6);

     txphase(one);
     dec2phase(zero);
   
     delay(taub - gt6 - 200.2e-6);
 
     sim3pulse(pw,0.0e-6,pwN,one,zero,zero,0.0,0.0);
 
     delay(0.2e-6);
     zgradpulse(gzlvl7,gt7);
     delay(2.0e-6);
 
     txphase(zero);
     dec2phase(zero);
 
     delay(taub - gt7 - 2.2e-6);
 
     sim3pulse(2*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);
     txphase(one); dec2phase(one);
 
     delay(0.2e-6);
     zgradpulse(gzlvl7,gt7);
     delay(200.0e-6);
   
     delay(taub - gt7 - 200.2e-6);

     sim3pulse(pw,0.0e-6,pwN,one,zero,one,0.0,0.0);
    }
/* END nietl_flg */
 
  txphase(zero);

  delay(gstab +gt2 -0.5*(pwN -pw) -2.0*pw/PI);
 
  rgpulse(2*pw,zero,0.0,0.0);
 
  delay(2.0e-6);
  zgradpulse(icosel*gzlvl2,gt2);
  decpower(dpwr);
  dec2power(dpwr2);
  delay(gstab -2.0e-6 -2.0*GRADIENT_DELAY -2.0*POWER_DELAY);

  lk_sample();
/* BEGIN ACQUISITION */
status(C);
  setreceiver(t6);
}
