/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghncaco_trosy_4DA.c - auto-calibrated version 


changes by MT@NMRFAM
 - added missing gt10
 - switched command for H2 decoupling
 

    This pulse sequence will allow one to perform the following
    experiment:

    4D hncaco with deuterium decoupling
	F1 	C-alpha
	F2 	Co
	F3 	N + JNH/2
	F-acq	HN- JNH/2

    This sequence uses the four channel configuration
         1)  1H             - carrier @ 4.7 ppm [H2O]
         2) 13C             - carrier @ 56 ppm (CA) and 174 ppm(Co)
         3) 15N             - carrier @ 120 ppm  
         4) 2H		    - carrier @ 4.5 ppm 

    Set dm = 'nnn', dmm = 'ccc' 
    Set dm2 = 'nnn', dmm2 = 'ccc' 
    Set dm3 = 'nnn', dmm2 = 'ccc' 

    Must set phase = 1,2, phase2=1,2, and phase3 = 1,2 for States-TPPI
    acquisition in t1 [Ca],t2[Co],and t2 [N]. [The fids must be manipulated
    (add/subtract) with 'grad_sort_nd' program (or equivalent) before regular
    processing for non-VNMR processing.]
    
    Flags
	fsat		'y' for presaturation of H2O
	fscuba		'y' for apply scuba pulse after presaturation of H2O
	f1180		'y' for 180 deg linear phase correction in F1 
		        	otherwise 0 deg linear phase correction
	f2180		'y' for 180 deg linear phase correction in F2
			    otherwise 0 deg linear phase correction
	fc180   	'y' for Co refocusing in t2
        sel_flg         'y' for active suppression of the anti-TROSY peak
        sel_flg         'n' for relaxation suppression of the anti-TROSY peak

	Standard Settings
   fsat='n',fscuba='n',mess_flg='n',f1180='n',f2180='y',f3180='n',fc180='n'

   Use ampmode statement = 'dddp'
   Note the final coherence transfer gradients have been split
   about the last 180 (H)

   Calibration of carbon pulses
	
	pwC       hard pulse (about 12 us at 600 MHz) at pwClvl
        pwc90     delta/sqrt(15) selective pulse applied at d_c90
        pwca180   delta/sqrt(3) selective pulse applied at d_c180
        pwca180dec pwca180+pad
        pwcareb   reburp 180 pulse (about 1600 us at 600MHz) applied at d_creb
	pwcosed    delta/sqrt(3) pulse applied at d_sed
                 USE delta/sqrt(3) and not seduce pulse
               In the case that jab_evolve == y
                  can use 250 us g3 pulses centered at 191.5 ppm at 800 MHz
        pwrbhp at d_rbhp (shrbhp) reburp pulses centered at 42 ppm; only used
                 when composite_flg == n

   Calibration of small agnle phase shift (set ni=1, ni2=1, ni3=1, phase=1,
   phase2=1,phase3=1)
        sphase  set fc180='y' and change sphase until get a null(no sginal).
                The right sphase is the value at the null plus 45 degrees

        sphase1 about zero.  Calibration is the same as that for sphase

    Ref:  Daiwen Yang and Lewis E. Kay, J.Am.Chem.Soc., 121, 2571(1999)
          Diawen Yang and Lewis E. Kay, J.Biomol.NMR, 13, 3(1999)

 

Written by Daiwen Yang on Sep. 9, 1998
Modified by L.E.Kay on Sep. 27, 1999 to include sel_flg 

Modified by L.E.Kay on Aug. 5, 2001 to include composite_flg. In the published
  version of the experiment during 13Ca evolution (periods A-D in Fig. 1
  of the paper) the 13Ca and 13Co 180 pulses are applied simultaneously using
  a composite 180. Need 14 us for 500, 12 us for 600 and 10 us for 800 for
  greater than 97% inversion/refocusing. If composite_flg == y then this is
  the approach to be taken. If composite_flg == n then C' and Ca pulses are
  applied as delta/sq(3) pulses.

There is an additional option and that is to ignore the evolution of Ca-Cb
couplings. At 800 MHz this may be the best alternative as the acq. times
have to be short of necessity. Set  jab_evolve = y

To set up as in Jacs paper: jab_evolve = n, composite_flg = y
To refocus jab (c-c alfa-beta couplings) set composite_flg = n and jab_evolve = n
To allow jab to evolve set jab_evolve = y

  Modified by E. Kupce, Jan 2005 for autocalibration of
     hncaco_trosy_4D_ydw.c
  Modified by G. Gray, Feb 2005 for BioPack

*/


#include <standard.h>
#define DELAY_BLANK 0.0e-6
#include "Pbox_bio.h"
    
#define CA180reb  "reburp 20p -4p"                /* RE-BURP 180 on Ca at 52 ppm, 4 ppm away */
#define CA180rbhp "reburp 80p -13p"            /* RE-BURP 180 on CaCb at 43 ppm, 13 ppm away */
#define CA180ps   "-s 1.0 -attn i"                           /* RE-BURP 180 shape parameters */
#define CO180     "square180n 118p 118p"          /* hard 180 on C', at 174 ppm 118 ppm away */
#define CA180     "square180n 118p -118p"         /* hard 180 on CA, at 56 ppm 118 ppm away  */
#define C90       "square90n 118p"                            /* hard  90 on C, on resonance */
#define C180      "square180n 118p"                           /* hard  90 on C, on resonance */
#define CO180ps   "-s 0.2 -attn d"                              /* hard 180 shape parameters */

static shape c90, c180, ca180, co180, ca180reb, ca180rbhp;

static int  phi1[1]  = {1},
            phi2[4]  = {0,0,2,2}, 
	    phi3[1]  = {0},
	    phi4[1]  = {0},
	    phi5[4]  = {0,1,2,3},
	    phi7[4]  = {0,0,2,2},
            rec[4]  = {0,2,2,0};

static double d2_init=0.0, d3_init=0.0, d4_init=0.0;


pulsesequence()

{

/* DECLARE VARIABLES */

 char       autocal[MAXSTR],  /* auto-calibration flag */
            fsat[MAXSTR],
	    fscuba[MAXSTR],
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell             */
            f3180[MAXSTR],    /* Flag to start t3 @ halfdwell             */
            fc180[MAXSTR],    /* Flag for checking sequence               */
            ddseq[MAXSTR],    /* deuterium decoupling sequence */
            spcosed[MAXSTR],  /* waveform Co seduce 180 */
            spcareb[MAXSTR],  /* waveform Ca reburp 180 */
            spca180[MAXSTR],  /* waveform Ca hard 180   */
            sel_flg[MAXSTR],
            shp_sl[MAXSTR],
            composite_flg[MAXSTR], 
            shrbhp[MAXSTR],
            jab_evolve[MAXSTR];

 int         phase, phase2, phase3,ni, ni2,ni3,icosel, 
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter,   /* used for states tppi in t2           */ 
             t3_counter;   /* used for states tppi in t3           */ 

 double      tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             tau3,         /*  t3 delay */
             taua,         /*  ~ 1/4JNH =  2.25 ms */
             taub,         /*  ~ 1/4JNH =  2.25 ms */
             tauc,         /*  ~ 1/4JNCa =  ~13 ms */
             taud,         /*  ~ 1/4JCaC' =  3~4.5 ms ms */
             bigTN,        /* nitrogen T period */
             bigTC,        /* carbon T period */
             pwc90,       /* PW90 for ca nucleus @ d_c90         */
             pwca180,      /* PW180 for ca nucleus @ d_c180         */
             pwca180dec,   /* pwca180+pad         */
             pwcareb,      /* pw180 at d_creb  ~ 1.6 ms at 600 MHz */ 
             pwcosed,      /* PW180 at d_csed  ~ 200us at 600 MHz  */

             pwD,         /* PW90 for higher power (pwDlvl) deut 90 */
             pwDlvl,      /* power deut 90 */

             tsatpwr,      /* low level 1H trans.power for presat  */
             d_c90,        /* power level for 13C pulses(pwc90=sqrt(15)/4delta
			      delta is the separation between Ca and Co */ 
             d_c180,	   /* power level for pwca180(sqrt(3)/2delta) */
             d_creb,	   /* power level for pwcareb */
	     d_csed,       /* power level for pwcosed */
             sw1,          /* sweep width in f1                    */             
             sw2,          /* sweep width in f2                    */             
             sw3,          /* sweep width in f3                    */             
             pw_sl,        /* selective pulse on water      */
             tpwrsl,       /* power for pw_sl               */
             at,
             sphase,	   /* small angle phase shift  */
             sphase1,
             phase_sl,

             d_rbhp,
             pwrbhp,

             compC,       /* C-13 RF calibration parameters */
             pwC,
             pwClvl,

             pwN,          /* PW90 for 15N pulse              */
	     pwNlvl,       /* high dec2 pwr for 15N hard pulses    */

             gstab,        /* delay about 200 us */

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
             gt11,
             gt12,
             gzlvl1,
             gzlvl2,
             gzlvl3,
             gzlvl4,
             gzlvl5,
             gzlvl6,
             gzlvl7,
             gzlvl8,
             gzlvl9,
             gzlvl10,
             gzlvl11,
             gzlvl12;
            
/*  variables commented out are already defined by the system      */


/* LOAD VARIABLES */


  getstr("autocal",autocal);
  getstr("fsat",fsat);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("f3180",f3180);
  getstr("fc180",fc180);
  getstr("fscuba",fscuba);
  getstr("ddseq",ddseq);
  getstr("shp_sl",shp_sl);
  getstr("sel_flg",sel_flg);
  getstr("composite_flg",composite_flg);
  getstr("jab_evolve",jab_evolve);

  taua   = getval("taua"); 
  taub   = getval("taub"); 
  tauc   = getval("tauc"); 
  taud   = getval("taud"); 
  bigTN = getval("bigTN");
  bigTC = getval("bigTC");
  gstab = getval("gstab");
  pwN = getval("pwN");
  tpwr = getval("tpwr");
  tsatpwr = getval("tsatpwr");
  dpwr = getval("dpwr");
  pwNlvl = getval("pwNlvl");
  pwD = getval("pwD");
  pwDlvl = getval("pwDlvl");
  phase = (int) ( getval("phase") + 0.5);
  phase2 = (int) ( getval("phase2") + 0.5);
  phase3 = (int) ( getval("phase3") + 0.5);
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  sw3 = getval("sw3");
  ni3 = getval("ni3");
  ni2 = getval("ni2");
  ni = getval("ni");
  pw_sl = getval("pw_sl");
  tpwrsl = getval("tpwrsl");
  at = getval("at");
  sphase = getval("sphase");
  sphase1 = getval("sphase1");
  phase_sl = getval("phase_sl");

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
  gt11 = getval("gt11");
  gt12 = getval("gt12");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl6 = getval("gzlvl6");
  gzlvl7 = getval("gzlvl7");
  gzlvl8 = getval("gzlvl8");
  gzlvl9 = getval("gzlvl9");
  gzlvl10 = getval("gzlvl10");
  gzlvl11 = getval("gzlvl11");
  gzlvl12 = getval("gzlvl12");

  if(autocal[0]=='n')
  {     
    pwC = getval("pwC");
    pwClvl = getval("pwClvl");
    getstr("shrbhp",shrbhp);
    getstr("spcosed",spcosed);
    getstr("spcareb",spcareb);
    getstr("spca180",spca180);
    pwc90 = getval("pwc90");
    pwca180 = getval("pwca180");
    pwca180dec = getval("pwca180dec");
    pwcareb = getval("pwcareb");
    pwcosed = getval("pwcosed");
    pwrbhp = getval("pwrbhp");
    d_c90 = getval("d_c90");
    d_c180 = getval("d_c180");
    d_creb = getval("d_creb");
    d_csed = getval("d_csed");
    d_rbhp = getval("d_rbhp");
  }
  else
  {        
    strcpy(spcareb,"PrebCa_-4p");    
    strcpy(shrbhp,"Preburp_-15p");    
    strcpy(spcosed,"Phard_118p");    
    strcpy(spca180,"Phard_-118p");    
    compC = getval("compC");
    pwC = getval("pwC");
    pwClvl = getval("pwClvl");
    if (FIRST_FID)
    {
      ca180reb = pbox(spcareb, CA180reb, CA180ps, dfrq, compC*pwC, pwClvl);                  
      ca180rbhp = pbox(shrbhp, CA180rbhp, CA180ps, dfrq, compC*pwC, pwClvl);                  
      co180 = pbox(spcosed, CO180, CO180ps, dfrq, compC*pwC, pwClvl);      
      ca180 = pbox(spca180, CA180, CO180ps, dfrq, compC*pwC, pwClvl);                  
      c180 = pbox("Phard180", C180, CO180ps, dfrq, compC*pwC, pwClvl);            
      c90 = pbox("Phard90", C90, CO180ps, dfrq, compC*pwC, pwClvl);  
    }    
    pwc90 = c90.pw;
    pwca180 = c180.pw;
    pwca180dec = ca180.pw;
    pwcareb = ca180reb.pw;
    pwcosed = co180.pw;
    pwrbhp = ca180rbhp.pw;
    d_c90 = c90.pwr;
    d_c180 = c180.pwr;
    d_creb = ca180reb.pwr;
    d_csed = co180.pwr;
    d_rbhp = ca180rbhp.pwr;
  }   

/* LOAD PHASE TABLE */

  settable(t1,1,phi1);
  settable(t2,4,phi2);
  settable(t3,1,phi3);
  settable(t4,1,phi4);
  settable(t5,4,phi5);
  settable(t7,4,phi7);
  settable(t6,4,rec);

/* CHECK VALIDITY OF PARAMETER RANGES */

   if (bigTN - 0.5*(ni3-1)/sw3 + pwca180 < 0.2e-6)
     {
        text_error(" ni3 is too big\n");
        text_error(" please set ni3 smaller or equal to %d\n",
			(int) ((bigTN +pwca180)*sw3*2.0) +1);
      psg_abort(1);
     }

if(composite_flg[A] == 'y' && jab_evolve[A] == 'n')
   if(bigTC - 0.25*(ni-1)/sw1 - POWER_DELAY - 2.0e-6 - WFG_STOP_DELAY < 0.2e-6)
    {
        text_error(" ni is too big\n");
        text_error(" please set ni smaller or equal to %d\n",
   			(int) ((bigTC - POWER_DELAY - 2.0e-6 - WFG_STOP_DELAY)*sw1*4) +1);
        psg_abort(1);
    }

if(composite_flg[A] == 'n' && jab_evolve[A] =='n')
   if(bigTC - 0.25*(ni-1)/sw1 - POWER_DELAY
	- WFG_START_DELAY - pwcosed - WFG_STOP_DELAY
	- POWER_DELAY - 4.0e-6 - WFG_START_DELAY < 0.2e-6) 
    {
        text_error(" ni is too big\n");
        text_error(" please set ni smaller or equal to %d\n",
	             (int) ((bigTC - POWER_DELAY
	   		     - WFG_START_DELAY - pwcosed - WFG_STOP_DELAY
	   		     - POWER_DELAY - 4.0e-6 - WFG_START_DELAY)*sw1*4) +1);
        psg_abort(1);
    }

 if(jab_evolve[A] == 'y') 
       if(2.0*bigTC - 0.5*(ni-1)/sw1 
           - POWER_DELAY - 4.0e-6 - WFG_START_DELAY - pwcosed
           - WFG_STOP_DELAY
           - 100.2e-6 - 2.0*GRADIENT_DELAY
	   - gt12
	   - POWER_DELAY - 4.0e-6 - WFG_START_DELAY < 0.2e-6) 
    {
        text_error(" ni is too big\n");
        text_error(" please set ni smaller or equal to %d\n",
       			(int) ((2.0*bigTC - POWER_DELAY - 4.0e-6 - WFG_START_DELAY - pwcosed
           			- WFG_STOP_DELAY - 100.2e-6 - 2.0*GRADIENT_DELAY - gt12
	   			- POWER_DELAY - 4.0e-6 - WFG_START_DELAY)*sw1*2.0) +1);
        psg_abort(1);
    }

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y'))
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

    if(d_rbhp > 62) {
         text_error("d_rbhp is too high;< 62\n");
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
	|| gt7 > 15e-3 || gt8 > 15e-3 || gt9 > 15e-3 || gt10 > 15.0e-3 
	|| gt11>15.0e-3)  
    {
       text_error("gti values must be < 15e-3\n");
       psg_abort(1);
    } 

    if(tpwrsl > 30) {
       text_error("tpwrsl must be less than 25\n");
       psg_abort(1);
    }

    if( pwDlvl > 59) {
       text_error("pwDlvl too high. Set pwDlvl <= 50\n");
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

    if (fc180[A] =='y' && ni2 > 1.0) {
       text_error("must set fc180='n' to allow C' evolution (ni2>1)\n");
       psg_abort(1);
   }



/*  Phase incrementation for hypercomplex 2D data */

    if (phase == 2) tsadd(t4,1,4);

    if (phase2 == 2) tsadd(t7,1,4);

    if (phase3 == 2) { tsadd(t3,2,4); icosel = 1; }
      else icosel = -1;


/*  Set up f1180  tau1 = t1               */

      tau1 = d2;
      if((f1180[A] == 'y') && (ni>1)){
          tau1 += ( 1.0 / (2.0*sw1) );
          if(tau1 < 0.2e-6) tau1 = 2.0e-6;
      }
        tau1 = tau1/4.0;

/*  Set up f2180  tau2 = t2               */

    tau2 = d3;
    if((f2180[A] == 'y') && (ni2>1)){
        tau2 += ( 1.0 / (2.0*sw2) - 8.0e-6 - 2.0*POWER_DELAY - 4.0*pwc90/PI
		 - 2.0*pwN - WFG_START_DELAY - pwca180dec - WFG_STOP_DELAY); 
    }

    if(f2180[A] == 'n') 
        tau2 = ( tau2 - 8.0e-6 - 2.0*POWER_DELAY - 4.0*pwc90/PI
		 - 2.0*pwN - WFG_START_DELAY - pwca180dec - WFG_STOP_DELAY); 

        if(tau2 < 0.2e-6) tau2 = 0.2e-6;
        tau2 = tau2/2.0;

/*  Set up f2180  tau3 = t3               */
 
    tau3 = d4;
    if((f3180[A] == 'y') && (ni3>1)){
        tau3 += ( 1.0 / (2.0*sw3) );
        if(tau3 < 0.2e-6) tau3 = 0.2e-6;
    }
        tau3 = tau3/2.0;
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
      tsadd(t7,2,4);  
      tsadd(t6,2,4);    
    }

   if( ix == 1) d4_init = d4 ;
   t3_counter = (int) ( (d4-d4_init)*sw3 + 0.5 );
   if(t3_counter % 2) {
      tsadd(t1,2,4);
      tsadd(t6,2,4);
    }
 
/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tsatpwr);     /* Set transmitter power for 1H presaturation */
   decpower(d_c90);       /* Set Dec1 power to high power          */
   dec2power(pwNlvl);     /* Set Dec2 power for 15N hard pulses         */
   decoffset(dof);


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

   initval(1.0,v2);
   obsstepsize(phase_sl);
   xmtrphase(v2);
 
   /* shaped pulse */
   obspower(tpwrsl);
   shaped_pulse(shp_sl,pw_sl,one,4.0e-6,0.0);
   xmtrphase(zero);
   obspower(tpwr);  txphase(zero);
   delay(4.0e-6);
   /* shaped pulse */

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */

   delay(0.2e-6);
   zgradpulse(gzlvl5,gt5);
   delay(2.0e-6);

   delay(taua - gt5 - 2.2e-6);   /* taua <= 1/4JNH */ 

   sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);

   txphase(three); dec2phase(zero); decphase(zero); 

   delay(taua - gt5 - 200.2e-6 - 2.0e-6); 

   delay(0.2e-6);
   zgradpulse(gzlvl5,gt5);
   delay(200.0e-6);

   if (sel_flg[A] == 'n') 
     {
      rgpulse(pw,three,2.0e-6,0.0);

      decpower(d_c180);

      delay(0.2e-6);
      zgradpulse(gzlvl3,gt3);
      delay(200.0e-6);

      dec2rgpulse(pwN,zero,0.0,0.0);

      delay(tauc);
 
      dec2rgpulse(2*pwN,zero,0.0,0.0);
      decrgpulse(pwca180,zero,0.0,0.0);
      dec2phase(one); 

      delay(tauc - pwca180);

      dec2rgpulse(pwN,one,0.0,0.0);
     }
    else 
     {
      rgpulse(pw,one,2.0e-6,0.0);

      decpower(d_c180);

      initval(1.0,v5);
      dec2stepsize(45.0);
      dcplr2phase(v5);

      delay(0.2e-6);
      zgradpulse(gzlvl3,gt3);
      delay(200.0e-6);

      dec2rgpulse(pwN,zero,0.0,0.0);
      dcplr2phase(zero);

      delay(1.34e-3 - SAPS_DELAY - 2.0*pw);

      rgpulse(pw,one,0.0,0.0);
      rgpulse(2.0*pw,zero,0.0,0.0);
      rgpulse(pw,one,0.0,0.0);

      delay(tauc - 1.34e-3 - 2.0*pw);

      dec2rgpulse(2*pwN,zero,0.0,0.0);
      decrgpulse(pwca180,zero,0.0,0.0);
      dec2phase(one); 

      delay(tauc - pwca180);

      dec2rgpulse(pwN,one,0.0,0.0);
     }

   decphase(t4);

   decpower(d_c90);

   delay(0.2e-6);
   zgradpulse(gzlvl8,gt8);
   delay(200.0e-6);

/* Cay to CaxC'z  and constant t1 period */
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

/* CONSTANT TIME Calfa EVOLUTION BEGINS */
   if (composite_flg[A] == 'y' && jab_evolve[A] == 'n') 
     {
      decrgpulse(pwc90,t4,2.0e-6,0.0);
      
      decpower(pwClvl);
      
      delay(bigTC - tau1 - POWER_DELAY);
      decrgpulse(pwC,one,0.0,0.0);
      decrgpulse(2*pwC,zero,0.0,0.0);
      decrgpulse(pwC,one,0.0,0.0);
      decpower(d_csed);
      
      delay(0.2e-6);
      zgradpulse(gzlvl12,gt12);
      delay(100.0e-6);
      
      delay(bigTC + tau1 - POWER_DELAY - 100.2e-6 - 2.0*GRADIENT_DELAY
      		- gt12 - WFG_START_DELAY - pwcosed - WFG_STOP_DELAY
      		- POWER_DELAY - 4.0e-6 - WFG_START_DELAY);
      
      decshaped_pulse(spcosed,pwcosed,zero,0.0,0.0);
      
      decpower(d_creb); decphase(t5);
      initval(1.0,v3);
      decstepsize(sphase);
      dcplrphase(v3);
      	
             
      decshaped_pulse(spcareb,pwcareb,t5,4.0e-6,0.0);
      
      dcplrphase(zero);
      decpower(pwClvl);
      
      delay(bigTC - tau1 - WFG_STOP_DELAY - POWER_DELAY - 2.0e-6);
      
      decrgpulse(pwC,one,2.0e-6,0.0);
      decrgpulse(2*pwC,zero,0.0,0.0);
      decrgpulse(pwC,one,0.0,0.0);
      
      decpower(d_c90); decphase(one);
      
      delay(0.2e-6);
      zgradpulse(gzlvl12,gt12);
      delay(100.0e-6);
      
      delay(bigTC + tau1 - POWER_DELAY - 100.2e-6
      		- gt12 - 2.0*GRADIENT_DELAY);
                     
      decrgpulse(pwc90,one,0.0,0.0);
     }
    else if (composite_flg[A] == 'n' && jab_evolve[A] == 'n')
     {
      decrgpulse(pwc90,t4,2.0e-6,0.0);
      
      decpower(d_csed); 
      
      delay(bigTC - tau1 - POWER_DELAY
      - WFG_START_DELAY - pwcosed - WFG_STOP_DELAY
      - POWER_DELAY - 4.0e-6 - WFG_START_DELAY);
      
      decshaped_pulse(spcosed,pwcosed,zero,0.0,0.0); /* delta/sq(3) on C' */
                      
      decpower(d_rbhp); 
      decshaped_pulse(shrbhp,pwrbhp,zero,4.0e-6,0.0); /* high power reburp */
                      
      decpower(d_csed);
      
      delay(0.2e-6);
      zgradpulse(gzlvl12,gt12);
      delay(100.0e-6);
      
      delay(bigTC + tau1 - WFG_STOP_DELAY 
      - POWER_DELAY - 100.2e-6 - 2.0*GRADIENT_DELAY
      - gt12 - WFG_START_DELAY - pwcosed - WFG_STOP_DELAY
      - POWER_DELAY - 4.0e-6 - WFG_START_DELAY);
      
      decshaped_pulse(spcosed,pwcosed,zero,0.0,0.0);
      
      decpower(d_creb); decphase(t5);
      initval(1.0,v3);
      decstepsize(sphase);
      dcplrphase(v3);
      	
      decshaped_pulse(spcareb,pwcareb,t5,4.0e-6,0.0);
      
      dcplrphase(zero);
      decpower(d_rbhp);
      
      delay(bigTC - tau1 - WFG_STOP_DELAY - POWER_DELAY - 2.0e-6
      - WFG_START_DELAY);
      
      decshaped_pulse(shrbhp,pwrbhp,zero,2.0e-6,0.0); /* high power reburp */
      
      decpower(d_csed); 
      decshaped_pulse(spcosed,pwcosed,zero,4.0e-6,0.0); /* delta/sq(3) on C' */
      
      decpower(d_c90); decphase(one);
      
      delay(0.2e-6);
      zgradpulse(-gzlvl12,gt12);
      delay(100.0e-6);
      
      delay(bigTC + tau1 - WFG_STOP_DELAY - POWER_DELAY - 4.0e-6 
      - WFG_START_DELAY - pwcosed - WFG_STOP_DELAY
      - POWER_DELAY
      - 100.2e-6
      - gt12 - 2.0*GRADIENT_DELAY);
                     
      decrgpulse(pwc90,one,0.0,0.0);
     }
    else if (jab_evolve[A] == 'y')
     {
      decrgpulse(pwc90,t4,2.0e-6,0.0);
      
      decpower(d_csed);
      decshaped_pulse(spcosed,pwcosed,zero,4.0e-6,0.0);
      
      
      delay(0.2e-6);
      zgradpulse(gzlvl12,gt12);
      delay(100.0e-6);
      
      delay(2.0*bigTC - 2.0*tau1 
      - POWER_DELAY - 4.0e-6 - WFG_START_DELAY - pwcosed
      - WFG_STOP_DELAY
      - 100.2e-6 - 2.0*GRADIENT_DELAY
      - gt12
      - POWER_DELAY - 4.0e-6 - WFG_START_DELAY);
      
      decpower(d_creb); decphase(t5);
      initval(1.0,v3);
      decstepsize(sphase);
      dcplrphase(v3);
             
      decshaped_pulse(spcareb,pwcareb,t5,4.0e-6,0.0);
      dcplrphase(zero);
      
      decpower(d_csed);
      decshaped_pulse(spcosed,pwcosed,zero,4.0e-6,0.0);
      
      delay(0.2e-6);
      zgradpulse(gzlvl12,gt12);
      delay(100.0e-6);
      
      delay(2.0*bigTC + 2.0*tau1 - WFG_STOP_DELAY 
      - POWER_DELAY - 4.0e-6 - WFG_START_DELAY - pwcosed - WFG_STOP_DELAY  
      - gt12 - 100.2e-6 - 2.0*GRADIENT_DELAY 
      - POWER_DELAY - 2.0e-6);
      
      decpower(d_c90); decphase(one);
                     
      decrgpulse(pwc90,one,2.0e-6,0.0);
     }
/* CONSTANT TIME Calfa EVOLUTION ENDS */

   /* Turn off D decoupling */
   setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
   dec3blank(); 
   dec3phase(three);
   dec3power(pwDlvl);
   dec3rgpulse(pwD,three,4.0e-6,0.0);
   /* Turn off D decoupling */
 
   decoffset(dof +(174.0-56.0)*dfrq);   /* change Dec1 carrier to Co  */
 
   delay(2.0e-7);
   zgradpulse(gzlvl4,gt4);
   delay(100.0e-6);

   delay(9.3e-3 -PRG_STOP_DELAY -POWER_DELAY -pwD -4.0e-6 -gt4 -100.2e-6);

/*  t2 period for C' chemical shift evolution; Ca 180 and N 180 are used to decouple  */

   decrgpulse(pwc90,t7,2.0e-6,0.0);
/* t2 EVOLUTION BEGINS */
   if (fc180[A]=='n')
     {
      decpower(d_c180);
      delay(tau2);
      decshaped_pulse(spca180,pwca180dec,zero,4.0e-6,0.0);
      dec2rgpulse(2*pwN,zero,0.0,0.0);
      delay(tau2);
      decpower(d_c90);
     }
    else
      decrgpulse(2*pwc90,zero,0.0,0.0);
/* t2 EVOLUTION ENDS */
   decrgpulse(pwc90,zero,4.0e-6,0.0);
 
   decoffset(dof);  /* set carrier to Ca */

   delay(2.0e-7);
   zgradpulse(gzlvl9,gt9);
   delay(100.0e-6);

/*  Refocusing  CayC'z to Cax  */

   /* Turn on D decoupling using the third decoupler */
   dec3phase(one);
   dec3power(pwDlvl);
   dec3rgpulse(pwD,one,4.0e-6,0.0);
   dec3phase(zero);
   dec3power(dpwr3);
   dec3unblank();
   setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
   /* Turn on D decoupling */

   decrgpulse(pwc90,zero,0.0e-6,0.0);
   decpower(d_csed);
   delay(taud - POWER_DELAY 
		- PRG_STOP_DELAY - POWER_DELAY
		- 4.0e-6 - pwD - 100.2e-6 - gt10
		- WFG_START_DELAY - pwcosed - WFG_STOP_DELAY
		- POWER_DELAY - 4.0e-6 - WFG_START_DELAY); 
 
   /* Turn off D dec */
   setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
   dec3blank();
   dec3phase(three);
   dec3power(pwDlvl);
   dec3rgpulse(pwD,three,4.0e-6,0.0);
   /* Turn off D decoupling */

   delay(2.0e-7);
   zgradpulse(gzlvl10,gt10);
   delay(100.0e-6);

   decshaped_pulse(spcosed,pwcosed,zero,0.0e-6,0.0);

   decpower(d_creb);
   initval(1.0,v4);
   decstepsize(sphase1);
   dcplrphase(v4);

   decshaped_pulse(spcareb,pwcareb,zero,4.0e-6,0.0);
   dcplrphase(zero);
 
   decpower(d_csed);
   delay(2.0e-7);
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

   delay(taud - WFG_STOP_DELAY - POWER_DELAY - 100.2e-6 - gt10
		- POWER_DELAY - 4.0e-6 - pwD - POWER_DELAY - PRG_START_DELAY
		- WFG_START_DELAY - pwcosed - WFG_STOP_DELAY - POWER_DELAY 
		- 4.0e-6);

   decshaped_pulse(spcosed,pwcosed,zero,0.0e-6,0.0);  /* BS */
   decpower(d_c90);
   decrgpulse(pwc90,one,4.0e-6,0.0);

   /* Turn off D decoupling */
   setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
   dec3blank();  
   dec3phase(three);
   dec3power(pwDlvl);
   dec3rgpulse(pwD,three,4.0e-6,0.0);
   /* Turn off D decoupling */

   decpower(d_c180);
   txphase(zero);

   delay(2.0e-7);
   zgradpulse(gzlvl11,gt11);
   delay(100.0e-6);

/* Constant t3 period  */
   dec2rgpulse(pwN,t1,2.0e-6,0.0);

   dec2phase(t2);

   delay(bigTN - tau3 + pwca180);

   dec2rgpulse(2*pwN,t2,0.0,0.0);
   decrgpulse(pwca180,zero,0.0,0.0);

   decpower(d_csed);
   delay(bigTN - gt1 - 600.2e-6 - 2.0*GRADIENT_DELAY - POWER_DELAY
         - WFG_START_DELAY - pwcosed - WFG_STOP_DELAY);

   delay(2.0e-7);
   zgradpulse(gzlvl1,gt1);
   delay(600.0e-6);

   decshaped_pulse(spcosed,pwcosed,zero,0.0,0.0); 

   delay(tau3);

   sim3pulse(pw,0.0e-6,pwN,zero,zero,t3,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl6,gt6);
   delay(2.0e-6);
 
   dec2phase(zero);
   delay(taub - gt6 - 2.2e-6);
 
   sim3pulse(2*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);
 
   delay(0.2e-6);
   zgradpulse(gzlvl6,gt6);
   delay(200.0e-6);
   
   delay(taub - gt6 - 200.2e-6);
   txphase(one);
   dec2phase(one);
 
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
 
   delay(gstab +gt2 -0.5*(pwN -pw) -2.0*pw/PI);
 
   rgpulse(2*pw,zero,0.0,0.0);
 
   delay(2.0e-6);
   zgradpulse(icosel*gzlvl2,gt2);
   decpower(dpwr);
   dec2power(dpwr2);
   delay(gstab -2.0e-6 -2.0*GRADIENT_DELAY -2.0*POWER_DELAY);

   lk_sample();
status(C);
   setreceiver(t6);

}
