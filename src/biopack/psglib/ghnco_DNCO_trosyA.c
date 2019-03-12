/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*ghnco_DNCO_trosyA.c

    Pulse scheme uses TROSY principle in 15N and NH with transfer of
    magnetization achieved using the enhanced sensitivity pfg approach,

    Yang and Kay, J. Biomol. NMR, January 1999.

    E. COSY principle is used to get Co-N coupling on 15N dimension and 
    Co-NH coupling on proton dimension.
 
    The following expt is:

    3D hnco with trosy(enhanced sensitivity PFG)
    with selective pulses to minimize
    excitation of water
                F1      CO 
                F2      15N + JNH/2 + (Jf+1)*JNCO  (assume JNH is positive)
                F3(acq) 1H (NH) - JNH/2 + JHN-CO

    This sequence uses the standard three channel configuration
         1)  1H             - carrier (tof) @ 4.7 ppm [H2O]
         2) 13C             - carrier (dof) @ 174 ppm [CO]
         3) 15N             - carrier (dof2)@ 120 ppm [centre of amide 15N]  
    
    Set dm = 'nnn', dmm = 'ccc' 
    Set dm2 = 'nnn', dmm2 = 'ccc'

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in t1
    [CO] and t2 [N]. [The fids must be manipulated (add/subtract) with 
    'grad_sort_nd' program (or equivalent) prior to regular processing.]
    
    Flags
        fsat            'y' for presaturation of H2O
        fscuba          'y' for apply scuba pulse after presaturation of H2O
        f1180           'y' for 180 deg linear phase correction in F1
                            otherwise 0 deg linear phase correction
        f2180           'y' for 180 deg linear phase coreection in F2
                            otherwise 0 deg
        fc180           'y' for C180 at t2/2 when checking 15N/NH 2D

        sel_flg         'y' in general 
                        'n' for high fields and/or larger proteins 

	Standard Settings
        fsat='n',fscuba='n',f1180='y',f2180='n',fc180='n'

    
    Set f1180 to y for (-90,180) in C' and f2180 to n for (0,0) in N
    Set the carbon carrier on the C' and use the waveform to pulse the
        c-alpha

    Written by Daiwen Yang on Jan. 6, 1999

    This scheme uses an accordion-based method to 'increase the N-CO 
    couplings at the expense of the effective 15N linewidth.

    The coupling measured in F2 (15N) is the N-Co coupling * (Jf + 1),
    while the linewidth of the 15N line is 1 / [T2/(Jf + 1)]. 

    Must optimize the value of Jf to choose. Use of a value of Jf such
    that the net t2 acq. time, (Jf + 1) * (ni2 - 1)/sw2 is on the order
    of T2 or a bit longer. Example, for a 90 ms T2 (long relaxing trosy
    component) and for a chem. shift acq. time of about 25 ms, use a Jf
    value of 3, for an effective JN-CO of (Jf + 1)*JN-CO.

    Carbon Pulse Widths.
    Carrier at 174 ppm

    pwco90: delta/sq(15), Co 90 selective (null at 56 ppm)
            at dpwr
    pwco180: delta/sq(3), Co 180 selective (null at 56 ppm)
            at dvhpwr
    pwca180h: delta/sq(3), Ca 180 selective (null at 174 ppm)
            at dvhpwr and applied as an off-res pulse

    Nagarajan Murali: Modified with auto shaped pulse generation compatible with 
    BioPack (11-21-2006).
     (based on hnco_NCo_ecosy_trosy_ydw.c)
    Modified by Marco Tonelli, NMRFAM, to use single coherence transfer 
    refocussing gradient (level gzlvl2) in place of split gradient (gives 
    better results in cold probe).
*/

#include <standard.h>
#include "Pbox_bio.h"
#define CA180     "square180n 118p -118p"       /* hard 180 on CA, at 58 ppm 118 ppm away  */
#define CO90      "square90n 118p"               /* hard 90 on C', at 176 ppm on resonance */
#define CO180	  "square180n 118p"	      /* hard 180 on CO with first null at 118ppm away */
#define CA180ps   "-s 0.2 -attn d"           /* shape parameters */
static shape   ca180, co90, co180;

static int  phi1[4]  = {0,0,2,2},
            phi2[2]  = {0,2},
            phi3[4]  = {0,0,2,2},
            phi4[1]  = {0},
            rec[4]   =  {0,2,2,0};

static double d2_init=0.0, d3_init=0.0;
            
void pulsesequence()
{
/* DECLARE VARIABLES */

 char       autocal[MAXSTR],
	    fsat[MAXSTR],
	    fscuba[MAXSTR],
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell             */
            spca180[MAXSTR],  /* string for the waveform 180 */
            fc180[MAXSTR], 
            shp_sl[MAXSTR],   /* string for shape of water pulse */
            sel_flg[MAXSTR];

 int         phase, phase2, ni2, icosel, /* icosel changes sign with gds  */ 
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter;   /* used for states tppi in t2           */ 

 double      pwC,
             pwClvl,
	     compC,
             compN,
	     tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             taua,         /*  ~ 1/4JNH =  2.25 ms */
             taub,         /*  ~ 1/4JNH =  2.25 ms */
             zeta,         /* time for C'-N to refocuss set to 0.5*24.0 ms */
             timeTN,        /* nitrogen T period */
             BigT1,        /* delay to compensate for gradient */
             pwN,          /* PW90 for 15N pulse              */
             pwco90,       /* PW90 for co nucleus @ dhpwr         */
             pwca180h,     /* PW180 for ca at dvhpwr               */
             pwco180,      /* PW180 for co at dhpwr180               */
             tsatpwr,      /* low level 1H trans.power for presat  */
             dhpwr,        /* power level for 13C pulses on dec1 - 64 us 
                              90 for part a of the sequence  */
             dhpwr180,     /* power level for 13C pulses on dec1 - 64 us 
                              180 for part a of the sequence  */
             dvhpwr,       /* power level for 180 13C pulses at 54 ppm
                                using a 55.6 us 180 so that get null in
                                co at 178 ppm */
             pwNlvl,       /* high dec2 pwr for 15N hard pulses    */
             sw1,          /* sweep width in f1                    */             
             sw2,          /* sweep width in f2                    */             
             pw_sl,        /* pw90 for H selective pulse on water ~ 2ms */
             phase_sl,     /* pw90 for H selective pulse on water ~ 2ms */
             tpwrsl,       /* power level for square pw_sl       */
 	     Jf,	   /* scale factor for JNCo, set to 4-5 */
             gt0,
             gt1,
             gt2,
             gt3,
             gt4,
             gt5,
             gt6,
             gt7,
             gt8,
             gstab,
             gzlvl0,
             gzlvl1,
             gzlvl2,
             gzlvl3,
             gzlvl4,
             gzlvl5,
             gzlvl6,
             gzlvl7, 
             gzlvl8; 

/* LOAD VARIABLES */

  getstr("autocal",autocal);
  getstr("fsat",fsat);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("fscuba",fscuba);
  getstr("spca180",spca180);
  getstr("fc180",fc180);
  getstr("shp_sl",shp_sl);
  getstr("sel_flg",sel_flg);

  taua   = getval("taua"); 
  taub   = getval("taub"); 
  zeta  = getval("zeta");
  timeTN = getval("timeTN");
  BigT1 = getval("BigT1");
  pwca180h = getval("pwca180h");
  pwco180 = getval("pwco180");
  pwco90 = getval("pwco90");
  pwN = getval("pwN");
  tpwr = getval("tpwr");
  tsatpwr = getval("tsatpwr");
  dhpwr = getval("dhpwr");
  dhpwr180 = getval("dhpwr180");
  dpwr = getval("dpwr");
  pwNlvl = getval("pwNlvl");
  phase = (int) ( getval("phase") + 0.5);
  phase2 = (int) ( getval("phase2") + 0.5);
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  dvhpwr = getval("dvhpwr");
  ni = getval("ni");
  ni2 = getval("ni2");
  pw_sl = getval("pw_sl");
  phase_sl = getval("phase_sl");
  tpwrsl = getval("tpwrsl");
  Jf = getval("Jf");

  gt0 = getval("gt0");
  gt1 = getval("gt1");
  gt2 = getval("gt2");
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");
  gt6 = getval("gt6");
  gt7 = getval("gt7");
  gt8 = getval("gt8");
  gstab = getval("gstab");

  gzlvl0 = getval("gzlvl0");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl6 = getval("gzlvl6");
  gzlvl7 = getval("gzlvl7");
  gzlvl8 = getval("gzlvl8");
 
  if (autocal[0] == 'y')
   {
    strcpy(spca180,"Phard_-118p");
    if (FIRST_FID)
    {
      compC = getval("compC");
      pwC = getval("pwC");
      pwClvl = getval("pwClvl");
      ca180 = pbox(spca180, CA180, CA180ps, dfrq, compC*pwC, pwClvl);
      co90 = pbox("Phard90", CO90, CA180ps, dfrq, compC*pwC, pwClvl);
      co180 = pbox("Phard180",CO180,CA180ps, dfrq, compC*pwC, pwClvl);
      pwN = getval("pwN"); compN = getval("compN"); pwNlvl = getval("pwNlvl");
    }

    pwca180h = ca180.pw;
    dvhpwr = ca180.pwr;
    pwco90 = co90.pw;
    dhpwr = co90.pwr;
    pwco180 = co180.pw;
    dhpwr180 = co180.pwr;
   }


/* LOAD PHASE TABLE */

  settable(t1,4,phi1);
  settable(t2,2,phi2);
  settable(t3,4,phi3);
  settable(t4,1,phi4);
  settable(t6,4,rec);

/* CHECK VALIDITY OF PARAMETER RANGES */


    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
    {
        printf("incorrect dec1 decoupler flags!  ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' ))
    {
        printf("incorrect dec2 decoupler flags! Should be 'nnn' ");
        psg_abort(1);
    }


    if( tsatpwr > 6 )
    {
        printf("TSATPWR too large !!!  ");
        psg_abort(1);
    }

    if( dpwr > 46 )
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

    if( dpwr2 > 46 )
    {
        printf("don't fry the probe, DPWR2 too large!  ");
        psg_abort(1);
    }

    if( dhpwr > 62 )
    {
        printf("don't fry the probe, DHPWR too large!  ");
        psg_abort(1);
    }

    if( pw > 200.0e-6 )
    {
        printf("dont fry the probe, pw too high ! ");
        psg_abort(1);
    } 
    if( pwN > 200.0e-6 )
    {
        printf("dont fry the probe, pwN too high ! ");
        psg_abort(1);
    } 
    if( pwco90 > 200.0e-6 )
    {
        printf("dont fry the probe, pwco90 too high ! ");
        psg_abort(1);
    } 
    if( pwca180h > 200.0e-6 )
    {
        printf("dont fry the probe, pwca180h too high ! ");
        psg_abort(1);
    } 

    if( gt3 > 2.5e-3 ) 
    {
        printf("gt3 is too long\n");
        psg_abort(1);
    }
    if( gt0 > 10.0e-3 || gt1 > 10.0e-3 || gt2 > 10.0e-3 ||
        gt4 > 10.0e-3 || gt5 > 10.0e-3 || gt6 > 10.0e-3 || 
        gt7 > 10.0e-3 || gt8 > 10.0e-3)
    {
        printf("gti values are too long. Must be < 10.0e-3\n");
        psg_abort(1);
    } 

/*  Phase incrementation for hypercomplex 2D data */

    if (phase == 2)
      tsadd(t1,1,4);
    if (phase2 == 2) {
       tsadd(t4, 2, 4);
       icosel = 1; 
       }               /* change sign of gradient */
    else icosel = -1;

/*  Set up f1180  tau1 = t1               */
   
    tau1 = d2;
    if(f1180[A] == 'y') {
        tau1 += ( 1.0 / (2.0*sw1) - 2*pwN - pwca180h - 4.0/PI*pwco90 - 2*POWER_DELAY
		  - WFG_START_DELAY - 8.0e-6 - WFG_STOP_DELAY );
        if(tau1 < 0.2e-6) tau1 = 0.4e-6;
    }
        tau1 = tau1/2.0;

/*  Set up f2180  tau2 = t2               */

    tau2 = d3;
    if(f2180[A] == 'y') {
        tau2 += ( 1.0 / (2.0*sw2) ); 
        if(tau2 < 0.2e-6) tau2 = 0.4e-6;
    }
        tau2 = tau2/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) {
      tsadd(t1,2,4);     
      tsadd(t6,2,4);    
    }

   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) {
      tsadd(t2,2,4);  
      tsadd(t6,2,4);    
    }

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tsatpwr);      /* Set transmitter power for 1H presaturation */
   decpower(dvhpwr);        /* Set Dec1 power for hard 13C pulses         */
   dec2power(pwNlvl);      /* Set Dec2 power for 15N hard pulses         */

/* Presaturation Period */

   if (fsat[0] == 'y')
   {
        rgpulse(d1,zero,2.0e-6,2.0e-6); /* presaturation */
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
   delay(20.0e-6);

   initval(1.0,v2);
   obsstepsize(phase_sl);
   xmtrphase(v2);

   /* shaped pulse  */
   obspower(tpwrsl);
   shaped_pulse(shp_sl,pw_sl,one,2.0e-6,0.0);
   xmtrphase(zero);
   delay(2.0e-6);
   obspower(tpwr);
   txphase(zero);
   /* shaped pulse  */

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */

   delay(0.2e-6);
   zgradpulse(gzlvl5*1.3,gt5);

   delay(taua - gt5 - 0.2e-6);   /* taua <= 1/4JNH */ 

   sim3pulse(2*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);

   dec2phase(zero); decphase(zero); 

   delay(taua -gt5 -gstab -4.0e-6); 

   zgradpulse(gzlvl5*1.3,gt5);
   delay(gstab);


   if(sel_flg[A] == 'y') {    /* active suppression of one of 
                                 the two components */

   rgpulse(pw,one,4.0e-6,0.0);

   /* shaped pulse  */
   initval(1.0,v3);
   obsstepsize(45.0);
   dcplr2phase(v3);

   delay(0.2e-6);
   zgradpulse(gzlvl3,gt3);
   delay(gstab);

   dec2rgpulse(pwN,zero,0.0,0.0);
   dcplr2phase(zero);

   delay( 1.34e-3 - SAPS_DELAY - 2.0*pw);
   rgpulse(pw,one,0.0,0.0);
   rgpulse(2*pw,zero,0.0,0.0);
   rgpulse(pw,one,0.0,0.0);

   delay( zeta - 1.34e-3 - 2.0*pw + pwco180 );


   }

   else {

   rgpulse(pw,three,4.0e-6,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl4,gt4);
   delay(gstab);

   dec2rgpulse(pwN,zero,0.0,0.0);

   delay( zeta + pwco180 );

   }
  
   dec2rgpulse(2*pwN,zero,0.0,0.0);
   decpower(dhpwr180);
   decrgpulse(pwco180,zero,0.0,0.0);

   delay(zeta - 2.0e-6);

   dec2rgpulse(pwN,one,2.0e-6,0.0);

   dec2phase(zero); decphase(t1);
   decpower(dhpwr);

   delay(0.2e-6);
   zgradpulse(gzlvl7,gt7);
   delay(gstab);
   decpower(dhpwr);
   decrgpulse(pwco90,t1,2.0e-6,0.0);

   if( fc180[A] == 'n' ) {
      decphase(zero);
      delay(tau1);

      dec2rgpulse(2*pwN,zero,0.0,0.0);
      decpower(dvhpwr);
      decshaped_pulse(spca180,pwca180h,zero,4.0e-6,0.0);
      decpower(dhpwr);

      delay(tau1);
   }

   else
     decrgpulse(2*pwco90,zero,2.0e-7,2.0e-7);

   decrgpulse(pwco90,zero,4.0e-6,0.0);

      decpower(dvhpwr);
   delay(0.2e-6);
   zgradpulse(gzlvl3,gt3);
   delay(gstab);


   dec2rgpulse(pwN,t2,2.0e-6,0.0);

   delay(tau2);
      decshaped_pulse(spca180,pwca180h,zero,0.0,0.0);
   delay(tau2);
   decpower(dhpwr180);
   delay(tau2*Jf);
   decrgpulse(pwco180,zero,0.0,0.0);

   delay(0.2e-6); 
   zgradpulse(-icosel*gzlvl1,gt1/2.0);
   delay(50.0e-6);

   delay(timeTN - 50.0e-6 -0.2e-6 - 2.0*GRADIENT_DELAY - gt1/2.0);

   dec2rgpulse(2*pwN,t3,0.0,0.0);
   delay(0.2e-6); 
   zgradpulse(icosel*gzlvl1,gt1/2.0);
   delay(50.0e-6);

   delay(tau2*Jf + timeTN - 50.0e-6 -0.2e-6 - 2.0*GRADIENT_DELAY - gt1/2.0
	 + WFG_START_DELAY + pwca180h + WFG_STOP_DELAY + pwco180 );

   sim3pulse(pw,0.0e-6,pwN,zero,zero,t4,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl6,gt6);
   delay(2.0e-6);

   dec2phase(zero);
   delay(taub - gt6 - 2.2e-6);

   sim3pulse(2*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl6,gt6);
   delay(gstab);
   
   txphase(one);
   dec2phase(one);

   delay(taub - gt6 - gstab -0.2e-6);

   sim3pulse(pw,0.0e-6,pwN,one,zero,one,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl5,gt5);
   delay(2.0e-6);
 
   txphase(zero);
   dec2phase(zero);

   delay(taub - gt5 - 2.2e-6);

   sim3pulse(2*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl5,gt5);
   delay(gstab);

   delay(taub - gt5 - gstab -0.2e-6);

   sim3pulse(pw,0.0e-6,pwN,zero,zero,zero,0.0,0.0);

   delay(gt2 +gstab +2.0*GRADIENT_DELAY +2.0*POWER_DELAY -0.5*(pwN - pw) -2.0*pw/PI);

   rgpulse(2.0*pw,zero,0.0,0.0);

   dec2power(dpwr2);
   decpower(dpwr);
   zgradpulse(gzlvl2,gt2);
   delay(gstab);

status(C);
         setreceiver(t6);

}
