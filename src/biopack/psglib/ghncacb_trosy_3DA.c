/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghncacb_trosy_3DA.c - auto-calibrated version of the original sequence

    This pulse sequence will allow one to perform the following experiment:

    3D hncacb with deuterium decoupling - Optimized for fully deuterated samples
	F1	Cbeta  (constant-time if fCT='y')
	F2	N + JNH/2
	F-acq	HN - JNH/2

   If fCT == 'y' this is the TROSY version of hncacb_CT_D_sel_pfg_600.c 
    Shan et al., JACS 118, 6570 (1996).
   If fCT == 'n' this is the TROSY version of hncacb_D_sel_pfg_600_v2.c
    Yamazaki et al. JACS 116, 11655 (1994).
 
    This sequence uses the four channel configuration:
         1)  1H             - carrier @ 4.7 ppm [H2O]
         2) 13C             - carrier @ 43 ppm (CA/CB)
         3) 15N             - carrier @ 120 ppm  
         4)  2H             - carrier @ 3 ppm

    Set dm = 'nnn', dmm = 'ccc' 
    Set dm2 = 'nnn', dmm2 = 'ccc' [NO 15N decoupling during acquition !!]
    Set dm3 = 'nnn', dmm3 = 'ccc'

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
    t1 [Cb]  and t2 [N]. [The fids must be manipulated (add/subtract) with
    'grad_sort_nd' program (or equivalent) prior to regular processing.]
    
    Flags
        fsat            'y' for presaturation of H2O
        fscuba          'y' for apply scuba pulse after presaturation of H2O
        f1180           'y' for 180 deg linear phase correction in F1
                        otherwise 0 deg linear phase correction
        f2180           'y' for 180 deg                         in F2
                        otherwise 0 deg
        sel_flg         'y' for active purging of the fast relaxing component
                        'n' no active purging; relaxation attenuation of
                            fast relaxing component 
        fCT             'y' for CT evolution of Cb chemical shift  
        fc180	        'n' for the 3D 
        cal_sphase      'y' only for calibration of sphase.
                         set sphase to sphase -45 check to get no signal
                         (array for zero signal and add 45o)
                         set zeta to a small value, gt11=gt13=0
                         for callibration of sphase
        shared_CT        'y' for shared CT in 15N dimension (if t2 acq > CT period
                         'n' no shared CT

	standard settings
        fsat='n',fscuba='n',f1180='n',f2180='n',fCT='y'  


    Pulse scheme is optimized for completely deuterated samples
	

    Pulse scheme uses TROSY principle in 15N and NH with transfer of
    magnetization achieved using the enhanced sensitivity pfg approach,

    Yang and Kay, J. Biomol. NMR, January 1999, 13, 3-9.
    Yang and Kay, J. Am. Chem. Soc., J. Am. Chem. Soc. 1999.

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

    See Figure 3 in Yang and Kay, J. Biomol NMR, Jan 1999.


    Ref:  Shan et. al. J. Am. Chem. Soc. 118, 6570-6579, (1996)
          Pervushin et al. PNAS, 94, 12366-12371 (1997)
          Pervushin et al. J. Biomol. NMR Aug 1998
          Yang and Kay, J. Biomol. NMR, Jan 1999
          Yang and Kay, J. Am. Chem. Soc., 1999, In Press.

    Setup: Define Delta as the difference in Hz. between C' (174 ppm) and
               Ca (56 ppm).
           Define Delta1 as the difference in Hz. between C' (174 ppm) and
               Ca/Cb (43 ppm).
           pwca90: a hard 90 (pwC) is used 12 us C 90 or shorter, on res, (43 ppm).
           pwcreb: Reburp pulse, on res, (43 ppm).
           pwcgcob: G3 inversion at 150 ppm to inverted aromatics and C', off
                    res.
                    shcgcob is the waveform for the non-time inverted pulse.
                    shcgcobi is the waveform for the time inverted pulse.
                    (can both be the same).
           pwca180: Ca 180 pulse using a power of delta/sq(3), off res and
                    centered at 58 ppm (15 ppm shift from carrier at 43 ppm).
           pwco180: C' 180 pulse, phase modulated by 133 ppm so that
                    excitation is at 176 ppm, but with null at 58 ppm. NB:
                    Use a delta/sq(3) pulse width !!!, not a delta1/sq(3), 
                    since the null is at 58 ppm.

     Modified by L.E.Kay on March 3, 1999 to include Ca purge immediately
     after the 15N CT period

     Modified by L.E.Kay on November 7, 2001 to include shared_CT

     Modified by L.E.Kay on November 30, 2001 to extend the phase cycle to include
      cycle of 15N 90 prior to t2. Can still use an 8 step cycle

     Modified by L.E.Kay to include a nietl_flg to suppress the anti-TROSY
      component so that there is no need to use sel_flg == y
                        (D.Nietlispach, J.Biomol.NMR,31,161(2005))

     Modified by E.Kupce for autocalibration based on  hncacb_D_trosy_lek_500a.c
     Modified by G.Gray  for BioPack, Feb 2005

     Calibrate pw_sl, tpwrsl  unless sel_flg == y
     Calibrate pw_sl1, tpwrsl1 - used only if nietl_flg == y
    
*/

#include <standard.h>
#include "Pbox_bio.h"
#define DELAY_BLANK 0.0e-6

#define CREB180  "reburp 80p"                  /* RE-BURP 180 on Cab at 43 ppm, on resonance */
#define CAB180ps  "-s 1.0 -attn i"                           /* RE-BURP 180 shape parameters */
#define G3CGCOB   "g3 80p 107p"                     /* G3 180 on C', at 150 ppm 107 ppm away */
#define G3CGCOBi  "g3 80p 107p 1"     /* time inverted G3 180 on C', at 150 ppm 107 ppm away */
#define CA180     "square180n 118p 13p"         /* hard 180 on CA, at 56 ppm 15 ppm away  */
#define CO180     "square180n 118p 133p"        /* hard 180 on C', at 176 ppm 133 ppm away */
#define CA180ps   "-s 0.2 -attn d"                           /* RE-BURP 180 shape parameters */

static shape   cgcob, cgcoib, creb, ca180, co180;

static int  phi1[2]  = {0,2},
            phi2[4]  = {0,0,2,2},
            phi3[8]  = {1,1,1,1,3,3,3,3},
            phi4[1]  = {1},
            phi5[16] = {1,1,1,1,1,1,1,1,3,3,3,3,3,3,3,3},
            phi6[8]  = {0,0,0,0,2,2,2,2},
            phi7[1]  = {0},
            rec[16]  = {0,2,2,0,0,2,2,0,2,0,0,2,2,0,0,2};

static double d2_init=0.0, d3_init=0.0;

pulsesequence()

{

/* DECLARE VARIABLES */

 char       autocal[MAXSTR],  /* auto-calibration flag */
            fsat[MAXSTR],
	    fscuba[MAXSTR],
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell             */
            ddseq[MAXSTR],    /* deuterium decoupling sequence */
            shp_sl[MAXSTR],

            shcreb[MAXSTR],  /* reburp shape for center of t1 period */
            shcgcob[MAXSTR], /* g3 inversion at 154 ppm (350 us) */
            shcgcoib[MAXSTR],  /* g3 time inversion at 154 ppm (350 us) */
            shca180[MAXSTR],   /* Ca 180 [D/sq(3)] during 15N CT */
            shco180[MAXSTR],   /* Co 180 [D/sq(15)] during 15N CT */
            sel_flg[MAXSTR],   /* active/passive purging of undesired 
                                  component  */ 
            fCT[MAXSTR],	       /* Flag for constant time C13 evolution */
            fc180[MAXSTR],
            cal_sphase[MAXSTR],
            shared_CT[MAXSTR],
            nietl_flg[MAXSTR];

 int         phase, phase2, ni2, icosel, 
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter;   /* used for states tppi in t2           */ 

 double      tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             taua,         /*  ~ 1/4JNH =  2.25 ms */
             del1,       /* time for C'-N to refocus set to 0.5*24.0 ms */
             bigTN,        /* nitrogen T period */
             bigTC,        /* carbon T period */
             zeta,         /* delay for transfer from ca to cb = 3.5 ms */
             tsatpwr,      /* low level 1H trans.power for presat  */
             sw1,          /* sweep width in f1                    */             
             sw2,          /* sweep width in f2                    */             
             tauf,         /* 1/2J NH value                     */
             pw_sl,        /* selective pulse on water      */
             phase_sl,     /* phase on water      */
             tpwrsl,       /* power for pw_sl               */
             at,

             d_cgcob,     /* power level for g3 pulses at 154 ppm */
             d_creb,      /* power level for reburp 180 at center of t1 */
             pwcgcob,     /* g3 ~ 35o us 180 pulse */
             pwcreb,      /* reburp ~ 400us 180 pulse */ 
 
             pwD,        /* 2H 90 pulse, about 125 us */
             pwDlvl,        /* 2H 90 pulse, about 125 us */

             pwca180,     /* Ca 180 during N CT at d_ca180 */
             pwco180,     /* Co 180 during N CT at d_co180 */

             d_ca180,
             d_co180,

             compC = getval("compC"),	/* C-13 RF calibration parameters */
             pwC = getval("pwC"),
             pwClvl = getval("pwClvl"),

             pwN,
             pwNlvl,

             sphase,

             pw_sl1,
             tpwrsl1,

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
             gt11,
             gt13,
             gt14,

             gzlvl1,
             gzlvl2,
             gzlvl3,
             gzlvl4,
             gzlvl5,
             gzlvl6,
             gzlvl7,
             gzlvl8,
             gzlvl9,
             gzlvl11,
             gzlvl13,
             gzlvl14;
            
/*  variables commented out are already defined by the system      */


/* LOAD VARIABLES */


  getstr("autocal",autocal);
  getstr("fsat",fsat);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("fscuba",fscuba);
  getstr("ddseq",ddseq);
  getstr("shp_sl",shp_sl);

  getstr("sel_flg",sel_flg);
  
  getstr("fCT",fCT);
  getstr("fc180",fc180);
  getstr("cal_sphase",cal_sphase);

  getstr("shared_CT",shared_CT);

  getstr("nietl_flg",nietl_flg);

  taua   = getval("taua"); 
  del1  = getval("del1");
  bigTN = getval("bigTN");
  bigTC = getval("bigTC");
  zeta = getval("zeta");
  pwN = getval("pwN");
  tpwr = getval("tpwr");
  tsatpwr = getval("tsatpwr");
  dpwr = getval("dpwr");
  pwNlvl = getval("pwNlvl");
  pwD = getval("pwD");
  pwDlvl = getval("pwDlvl");
  phase = (int) ( getval("phase") + 0.5);
  phase2 = (int) ( getval("phase2") + 0.5);
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  ni2 = getval("ni2");
  tauf = getval("tauf");
  pw_sl = getval("pw_sl");
  phase_sl = getval("phase_sl");
  tpwrsl = getval("tpwrsl");
  at = getval("at");

  sphase = getval("sphase");

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
  gt11 = getval("gt11");
  gt13 = getval("gt13");
  gt14 = getval("gt14");

  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl6 = getval("gzlvl6");
  gzlvl7 = getval("gzlvl7");
  gzlvl8 = getval("gzlvl8");
  gzlvl9 = getval("gzlvl9");
  gzlvl11 = getval("gzlvl11");
  gzlvl13 = getval("gzlvl13");
  gzlvl14 = getval("gzlvl14");

  if(autocal[0]=='n')
  {     
    getstr("shcgcob",shcgcob);
    getstr("shcgcoib",shcgcoib);
    getstr("shcreb",shcreb);
    getstr("shca180",shca180);
    getstr("shco180",shco180);
    
    d_ca180 = getval("d_ca180");
    d_co180 = getval("d_co180");
    d_cgcob = getval("d_cgcob");
    d_creb = getval("d_creb");
    pwca180 = getval("pwca180");
    pwco180 = getval("pwco180");
    pwcgcob = getval("pwcgcob");
    pwcreb = getval("pwcreb");
  }
  else
  {        
    strcpy(shcgcob,"Pg3_107p");    
    strcpy(shcgcoib,"Pg3i_107p");    
    strcpy(shcreb,"Preb_on");    
    strcpy(shca180,"Phard_15p");    
    strcpy(shco180,"Phard_133p");    
    if (FIRST_FID)  
    {
      cgcob = pbox(shcgcob, G3CGCOB, CAB180ps, dfrq, compC*pwC, pwClvl);
      cgcoib = pbox(shcgcoib, G3CGCOBi, CAB180ps, dfrq, compC*pwC, pwClvl);  
      creb = pbox(shcreb, CREB180, CAB180ps, dfrq, compC*pwC, pwClvl);      
      ca180 = pbox(shca180, CA180, CA180ps, dfrq, compC*pwC, pwClvl);        
      co180 = pbox(shco180, CO180, CA180ps, dfrq, compC*pwC, pwClvl);  
    }   
    d_ca180 = ca180.pwr;
    d_co180 = co180.pwr;
    d_cgcob = cgcob.pwr;
    d_creb = creb.pwr;
    pwca180 = ca180.pw;
    pwco180 = co180.pw;
    pwcgcob = cgcob.pw;
    pwcreb = creb.pw;
  }   

/* LOAD PHASE TABLE */

  settable(t1,2,phi1);
  settable(t2,4,phi2);
  settable(t3,8,phi3);
  settable(t4,1,phi4);
  settable(t5,16,phi5);
  settable(t6,8,phi6);
  settable(t7,1,phi7);
  settable(t8,16,rec); 

/* CHECK VALIDITY OF PARAMETER RANGES */

   if(shared_CT[A] == 'n')
    if(bigTN - 0.5*(ni2 -1)/sw2 - POWER_DELAY < 0.2e-6)
    {
        text_error(" ni2 is too big\n");
        text_error(" please set ni2 smaller or equal to %d\n",
    			(int) ((bigTN -POWER_DELAY)*sw2*2.0) +1 );
        psg_abort(1);
    }

   if(fCT[A] == 'y')
    if(bigTC - 0.5*(ni-1)/sw1 - WFG_STOP_DELAY - gt14 - 102.0e-6
        - POWER_DELAY - 4.0e-6 - pwD - POWER_DELAY - PRG_START_DELAY
        - POWER_DELAY - WFG_START_DELAY - 4.0e-6 - pwcgcob - WFG_STOP_DELAY
        - POWER_DELAY - 4.0e-6 < 0.2e-6) {

          text_error("ni is too big\n");
          text_error(" please set ni smaller or equal to %d\n",
    			(int) ((bigTC - WFG_STOP_DELAY - gt14 - 102.0e-6
        			- POWER_DELAY - 4.0e-6 - pwD - POWER_DELAY - PRG_START_DELAY
        			- POWER_DELAY - WFG_START_DELAY - 4.0e-6 - pwcgcob - WFG_STOP_DELAY
        			- POWER_DELAY - 4.0e-6)*sw1*2.0) +1 );
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

    if( pwClvl > 63 )
    {
        text_error("don't fry the probe, pwClvl too large!  ");
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
    if( pwC > 200.0e-6 )
    {
        text_error("dont fry the probe, pwC too high ! ");
        psg_abort(1);
    } 

    if( f1180[A] != 'n' && f2180[A] != 'n' ) {
        text_error("flags may be set wrong: set f1180=n and f2180=n for 3d\n");
        psg_abort(1);
    }

    if(d_ca180 > 58) 
    {
        text_error("dont fry the probe, d_ca180 too high ! ");
        psg_abort(1);
    }

    if(d_co180 > 58) 
    {
        text_error("dont fry the probe, d_ca180 too high ! ");
        psg_abort(1);
    }

    if( gt1 > 15e-3 || gt2 > 15e-3 || gt3 > 15e-3 
        || gt4 > 15e-3 || gt5 > 15e-3 || gt6 > 15e-3 
        || gt7 > 15e-3 || gt8 > 15e-3 || gt9 > 15e-3 
        || gt11 > 15e-3 || gt13 > 15e-3  
        || gt14 > 15e-3)
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

    if( dpwr3 > 50) {
       text_error("dpwr3 too high\n");
       psg_abort(1);
    }
    if( del1 > 0.1 ) {
       text_error("too long del1\n");
       psg_abort(1);
    }
    if( zeta > 0.1 ) {
       text_error("too long zeta\n");
       psg_abort(1);
    }
    if( bigTN > 0.1) {
       text_error("too long bigTN\n");
       psg_abort(1);
    }
    if( bigTC > 0.1) {
       text_error("too long bigTC\n");
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
    if( at > 0.1 && dm2[D] == 'y') {
       text_error("too long at with dec2\n");
       psg_abort(1);
    }

    if(pwDlvl > 59) {
        text_error("pwDlvl is too high; <= 59\n");
        psg_abort(1);
    }

    if(d_creb > 62) {
        text_error("d_creb is too high; <= 62\n");
        psg_abort(1);
    }

    if(d_cgcob > 60) {
        text_error("d_cgcob is too high; <=60\n");
        psg_abort(1);
    }

    if(cal_sphase[A] == 'y') {
      text_error("Use only to calibrate sphase\n");
      text_error("Set zeta to 600 us, gt11=gt13=0, fCT=y, fc180=n\n");
    }

    if(nietl_flg[A] == 'y' && sel_flg[A] == 'y') {
       text_error("Both nietl_flg and sel_flg cannot by y\n");
       psg_abort(1);
    }

    if (fCT[A] == 'n' && fc180[A] =='y' && ni > 1.0) {
       text_error("must set fc180='n' to allow Calfa/Cbeta evolution (ni>1)\n");
       psg_abort(1);
   }


/*  Phase incrementation for hypercomplex 2D data */

    /* changed from 1 to 3; spect. rev. not needed */
    if (phase == 2) { tsadd(t2,3,4); tsadd(t3,3,4); }

    if (shared_CT[A] == 'n') 
      {
       if (phase2 == 2) { tsadd(t7,2,4); icosel = 1; }
         else icosel = -1;
      }
     else 
      {
       if (phase2 == 2) { tsadd(t7,2,4); icosel = -1; }
         else icosel = 1;
      }

    if (nietl_flg[A] == 'y') icosel = -1*icosel;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) {
      tsadd(t2,2,4);     
      tsadd(t8,2,4);    
    }

   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) {
      tsadd(t5,2,4);  
      tsadd(t8,2,4);    
    }

/*  Set up f1180  tau1 = t1         */

      tau1 = d2;
      if(f1180[A] == 'y' && fCT[A] == 'y') 
          tau1 += ( 1.0 / (2.0*sw1) );

      if(f1180[A] == 'y' && fCT[A] == 'n') 
          tau1 += (1.0 / (2.0*sw1) - 4.0/PI*pwC - POWER_DELAY
                    - 4.0e-6);

      if(f1180[A] == 'n' && fCT[A] == 'n') 
          tau1 = (tau1 - 4.0/PI*pwC - POWER_DELAY
                    - 4.0e-6);

      if(tau1 < 0.2e-6) tau1 = 4.0e-7; 
      tau1 = tau1/2.0;

/*  Set up f2180  tau2 = t2         */

    tau2 = d3;
    if(f2180[A] == 'y') {
        tau2 += ( 1.0 / (2.0*sw2) ); 
        if(tau2 < 0.2e-6) tau2 = 0.2e-6;
    }
        tau2 = tau2/2.0;

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tsatpwr);     /* Set transmitter power for 1H presaturation */
   decpower(pwClvl);      /* Set Dec1 power to high power          */
   dec2power(pwNlvl);     /* Set Dec2 power for 15N hard pulses         */
   dec3power(pwDlvl);     /* Set Dec3 for 2H hard pulses */

/* Presaturation Period */

   if (fsat[0] == 'y')
     {
      delay(2.0e-5);
      rgpulse(d1,zero,0.0,2.0e-6);
      obspower(tpwr);      /* Set transmitter power for hard 1H pulses */
      delay(2.0e-5);
      if (fscuba[0] == 'y')
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

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */

   delay(0.2e-6);
   zgradpulse(gzlvl5,gt5);
   delay(2.0e-6);

   delay(taua - gt5 - 2.2e-6);   /* taua <= 1/4JNH */ 

   sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);

   dec2phase(t1); decphase(zero); 

   delay(taua - gt5 - 200.2e-6); 

   delay(0.2e-6);
   zgradpulse(gzlvl5,gt5);
   delay(200.0e-6);

   if (sel_flg[A] == 'y') 
     {
      rgpulse(pw,one,4.0e-6,0.0);

      initval(1.0,v2);
      obsstepsize(phase_sl);
      xmtrphase(v2);

      /* shaped pulse */
      obspower(tpwrsl);
      shaped_pulse(shp_sl,pw_sl,two,2.0e-6,0.0);
      xmtrphase(zero);
      delay(2.0e-6);
      obspower(tpwr);
      /* shaped pulse */

      initval(1.0,v6);
      dec2stepsize(45.0);
      dcplr2phase(v6);

      delay(0.2e-6);
      zgradpulse(gzlvl3,gt3);
      delay(200.0e-6);

      dec2rgpulse(pwN,t1,0.0,0.0);
      dcplr2phase(zero);

      delay(1.34e-3 - SAPS_DELAY);
   
      rgpulse(pw,zero,0.0,0.0);
      rgpulse(2.0*pw,one,2.0e-6,0.0);
      rgpulse(pw,zero,2.0e-6,0.0);

      decpower(d_ca180);

        dec2phase(zero);
   
        delay(del1 - 1.34e-3 - 4.0*pw - 4.0e-6 
              - POWER_DELAY + WFG_START_DELAY + pwca180 + WFG_STOP_DELAY);
     }
    else  
     {
      rgpulse(pw,three,4.0e-6,0.0);

      initval(1.0,v2);
      obsstepsize(phase_sl);
      xmtrphase(v2);
   
      /* shaped pulse */
      obspower(tpwrsl);
      shaped_pulse(shp_sl,pw_sl,zero,2.0e-6,0.0);
      xmtrphase(zero);
      delay(2.0e-6);
      obspower(tpwr);
      /* shaped pulse */
   
      delay(0.2e-6);
      zgradpulse(gzlvl3,gt3);
      delay(200.0e-6);

      dec2rgpulse(pwN,t1,0.0,0.0);
      dec2phase(zero);

      decpower(d_ca180);

      delay(del1 - POWER_DELAY + WFG_START_DELAY
      		+ pwca180 + WFG_STOP_DELAY);
     }

   decphase(zero);
   dec2rgpulse(2*pwN,zero,0.0,0.0);
   decshaped_pulse(shca180,pwca180,zero,0.0,0.0);

   dec2phase(one);

   delay(del1);

   dec2rgpulse(pwN,one,0.0,0.0);

   decpower(pwClvl);

   decphase(t2); 

   delay(0.2e-6);
   zgradpulse(gzlvl4,gt4);
   delay(200.0e-6);

   dec2phase(t5); 

   /* Turn on D decoupling using the third decoupler */
   dec3phase(one);
   dec3power(pwDlvl);
   dec3rgpulse(pwD,one,4.0e-6,0.0);
   dec3phase(zero);
   dec3power(dpwr3);
   dec3unblank();
   setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
   /* Turn on D decoupling */

   decrgpulse(pwC,t2,0.0,0.0);

   delay(zeta
	- PRG_STOP_DELAY - DELAY_BLANK - POWER_DELAY - 4.0e-6
        - pwD
        - gt11 - 102.0e-6 - POWER_DELAY - WFG_START_DELAY); 
      
   /* Turn off D decoupling */
   setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
   dec3blank(); 
   dec3phase(three);
   dec3power(pwDlvl);
   dec3rgpulse(pwD,three,4.0e-6,0.0);
   /* Turn off D decoupling */

    decphase(zero);

   delay(2.0e-6);
   zgradpulse(gzlvl11,gt11);
   delay(100.0e-6);

   if (cal_sphase[A] == 'y') 
     {
      decpower(pwClvl);
      decshaped_pulse("hard",2.0*pwC,zero,4.0e-6,4.0e-6);
     }
    else 
     {
      initval(1.0,v3);
      decstepsize(sphase);
      dcplrphase(v3);
      decpower(d_creb);
      decshaped_pulse(shcreb,pwcreb,zero,4.0e-6,4.0e-6);
      dcplrphase(zero);
     }

   delay(2.0e-6);
   zgradpulse(gzlvl11,gt11);
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

   delay(zeta - WFG_STOP_DELAY - gt11 - 102.0e-6 
	- POWER_DELAY - 4.0e-6 - pwD - POWER_DELAY - PRG_START_DELAY
        - DELAY_BLANK - POWER_DELAY - 4.0e-6);

   decpower(pwClvl);
   decrgpulse(pwC,t3,4.0e-6,0.0);

   if (fCT[A] == 'y') 
     {
      delay(tau1);

      decpower(d_cgcob);
      decshaped_pulse(shcgcob,pwcgcob,zero,4.0e-6,0.0);

      delay(bigTC - POWER_DELAY - WFG_START_DELAY - 4.0e-6
            - pwcgcob - WFG_STOP_DELAY 
            - 102.0e-6 - gt14 
            - PRG_STOP_DELAY - DELAY_BLANK
            - POWER_DELAY - 4.0e-6 - pwD - POWER_DELAY - WFG_START_DELAY);
 
      /* Turn off D decoupling */
      setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
      dec3blank();
      dec3phase(three);
      dec3power(pwDlvl);
      dec3rgpulse(pwD,three,4.0e-6,0.0);
      /* Turn off D decoupling */

      delay(2.0e-6);
      zgradpulse(gzlvl14,gt14);
      delay(100.0e-6);

      initval(1.0,v4);
      decstepsize(sphase);
      dcplrphase(v4);
      decpower(d_creb);
      decshaped_pulse(shcreb,pwcreb,zero,4.0e-6,4.0e-6);
      dcplrphase(zero);
    
      delay(2.0e-6);
      zgradpulse(gzlvl14,gt14);
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


      delay(bigTC - tau1 - WFG_STOP_DELAY - gt14
            - 102.0e-6 - POWER_DELAY - 4.0e-6 - pwD - POWER_DELAY
            - PRG_START_DELAY - POWER_DELAY - WFG_START_DELAY
            - 4.0e-6 - pwcgcob - WFG_STOP_DELAY - POWER_DELAY 
            - 4.0e-6);

      decpower(d_cgcob);
      decshaped_pulse(shcgcoib,pwcgcob,zero,4.0e-6,0.0);
      decphase(t4);
     }
    else if(fCT[A] == 'n' && fc180[A] == 'n') 
     {
      delay(tau1);
      delay(tau1);
     }
    else if(fCT[A] == 'n' && fc180[A] == 'y') 
     {
      initval(1.0,v4);
      decstepsize(sphase);
      dcplrphase(v4);

      decpower(d_creb);
      decshaped_pulse(shcreb,pwcreb,zero,4.0e-6,0.0);

      dcplrphase(zero);
     }

   decpower(pwClvl);
   decrgpulse(pwC,t4,4.0e-6,0.0);

   delay(zeta - POWER_DELAY - 4.0e-6
          - pwD - PRG_STOP_DELAY - DELAY_BLANK
          - gt13
          - 102.0e-6 - POWER_DELAY - WFG_START_DELAY); 

   /* Turn off D decoupling */
   setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
   dec3blank(); 
   dec3power(pwDlvl);
   dec3rgpulse(pwD,three,4.0e-6,0.0);
   /* Turn off D decoupling */

   delay(2.0e-6);
   zgradpulse(gzlvl13,gt13);
   delay(100.0e-6);

   if (cal_sphase[A] == 'y') 
     {
      decpower(pwClvl);
      decshaped_pulse("hard",2.0*pwC,zero,4.0e-6,4.0e-6);
     }
    else 
     {
      initval(1.0,v5);
      decstepsize(sphase);
      dcplrphase(v5);
      decpower(d_creb);
      decshaped_pulse(shcreb,pwcreb,zero,4.0e-6,4.0e-6);
      dcplrphase(zero);
     }

   delay(2.0e-6);
   zgradpulse(gzlvl13,gt13);
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


   delay(zeta - WFG_STOP_DELAY - gt13 - 102.0e-6  
        - POWER_DELAY - 4.0e-6 - pwD - POWER_DELAY
        - PRG_START_DELAY - DELAY_BLANK
        - POWER_DELAY - 4.0e-6);

   decpower(pwClvl);
   decrgpulse(pwC,zero,4.0e-6,0.0);

   /* Turn off D decoupling */
   setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
   dec3blank();
   dec3phase(three);
   dec3power(pwDlvl);
   dec3rgpulse(pwD,three,4.0e-6,0.0);
   /* Turn off D decoupling */

   delay(0.2e-6);
   zgradpulse(gzlvl9,gt9);
   delay(200.0e-6);

   if (shared_CT[A] == 'n') 
     {
      dec2rgpulse(pwN,t5,2.0e-6,0.0);

      decpower(d_ca180);

      dec2phase(t6); 

      delay(bigTN - tau2 - POWER_DELAY);

      dec2rgpulse(2*pwN,t6,0.0,0.0);
      decshaped_pulse(shca180,pwca180,zero,0.0,0.0);
      dec2phase(t7);

      delay(bigTN - WFG_START_DELAY - pwca180 - WFG_STOP_DELAY
            - gt1 - 2.0*GRADIENT_DELAY - 500.2e-6 
            - POWER_DELAY - 4.0e-6 - WFG_START_DELAY
            - pwco180 - WFG_STOP_DELAY);

      delay(0.2e-6);
      zgradpulse(gzlvl1,gt1);
      delay(500.0e-6);

      decpower(d_co180);
      decshaped_pulse(shco180,pwco180,zero,4.0e-6,0.0);

      delay(tau2);
   
      sim3pulse(pw,0.0,pwN,zero,zero,t7,0.0,0.0);
     }
    else if (shared_CT[A] == 'y') 
     {
      dec2rgpulse(pwN,t5,2.0e-6,0.0);

      decpower(d_co180);
      dec2phase(t6); 

      if (bigTN - tau2 >= 0.2e-6) 
        {
         delay(tau2);

         decshaped_pulse(shco180,pwco180,zero,4.0e-6,0.0);
         decpower(d_ca180);

         delay(0.2e-6);
         zgradpulse(gzlvl1,gt1);
         delay(500.0e-6);

         delay(bigTN - 4.0e-6 - WFG_START_DELAY - pwco180 - WFG_STOP_DELAY
               - POWER_DELAY - gt1 - 500.2e-6 - 2.0*GRADIENT_DELAY
               - WFG_START_DELAY - pwca180 - WFG_STOP_DELAY);

         decshaped_pulse(shca180,pwca180,zero,0.0,0.0);
         dec2rgpulse(2*pwN,t6,0.0,0.0);

         delay(bigTN - tau2);
        }
       else 
        {
         delay(tau2);
         decshaped_pulse(shco180,pwco180,zero,4.0e-6,0.0);

         delay(0.2e-6);
         zgradpulse(gzlvl1,gt1);
         delay(500.0e-6);
     
         decpower(d_ca180);
         delay(bigTN - 4.0e-6 - WFG_START_DELAY - pwco180
               - WFG_STOP_DELAY - gt1 - 500.2e-6 - 2.0*GRADIENT_DELAY
               - POWER_DELAY - WFG_START_DELAY - pwca180 - WFG_STOP_DELAY); 
   
         decshaped_pulse(shca180,pwca180,zero,0.0,0.0);
    
         delay(tau2 - bigTN);
         dec2rgpulse(2.0*pwN,t6,0.0,0.0);
        }
      sim3pulse(pw,0.0,pwN,zero,zero,t7,0.0,0.0);
     }
/* end of shared_CT */

   if (nietl_flg[A] == 'n') 
     {
      decpower(pwClvl);
      decrgpulse(pwC,zero,4.0e-6,0.0);

      delay(0.2e-6);
      zgradpulse(gzlvl6,gt6);
      delay(2.0e-6);

      dec2phase(zero);
      delay(tauf - POWER_DELAY - 4.0e-6 
                 - pwC - gt6 - 2.2e-6);

      sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);

      txphase(one);
      dec2phase(one);
   
      delay(tauf - gt6 - 200.2e-6);

      delay(0.2e-6);
      zgradpulse(gzlvl6,gt6);
      delay(200.0e-6);

      sim3pulse(pw,0.0,pwN,one,zero,one,0.0,0.0);
      
      delay(0.2e-6);
      zgradpulse(gzlvl7,gt7);
      delay(2.0e-6);
 
      txphase(zero);
      dec2phase(zero);
      delay(tauf - gt7 - 2.2e-6);

      sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);

      delay(tauf - gt7 - 200.2e-6);

      delay(0.2e-6);
      zgradpulse(gzlvl7,gt7);
      delay(200.0e-6);
   
      sim3pulse(pw,0.0e-6,pwN,zero,zero,zero,0.0,0.0);
     }
    else  
     {   /* nietl_flg == y */
      /* shaped pulse */
      obspower(tpwrsl1);
      shaped_pulse(shp_sl,pw_sl1,zero,2.0e-6,0.0);
      delay(2.0e-6);
      obspower(tpwr);
      /* shaped pulse */

      decpower(pwClvl);
      decrgpulse(pwC,zero,4.0e-6,0.0);

      delay(0.2e-6);
      zgradpulse(gzlvl6,gt6);
      delay(2.0e-6);
   
      dec2phase(zero);
      delay(tauf 
                 - POWER_DELAY - 2.0e-6 - WFG_START_DELAY
                 - pw_sl1 - WFG_STOP_DELAY - 2.0e-6 - POWER_DELAY
                 - POWER_DELAY - 4.0e-6 
                 - pwC - gt6 - 2.2e-6);

      sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);

      txphase(one);
      dec2phase(zero);

      delay(tauf - gt6 - 200.2e-6);

      delay(0.2e-6);
      zgradpulse(gzlvl6,gt6);
      delay(200.0e-6);

      sim3pulse(pw,0.0,pwN,one,zero,zero,0.0,0.0);
   
      delay(0.2e-6);
      zgradpulse(gzlvl7,gt7);
      delay(2.0e-6);
 
      txphase(zero);
      dec2phase(zero);
      delay(tauf - gt7 - 2.2e-6);

      sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);
      txphase(one);
      dec2phase(one);
   
      delay(tauf - gt7 - 200.2e-6);

      delay(0.2e-6);
      zgradpulse(gzlvl7,gt7);
      delay(200.0e-6);
   
      sim3pulse(pw,0.0e-6,pwN,one,zero,one,0.0,0.0);
      txphase(zero);
     }  /* end of nietl_flg == y  */

   delay(gt2 +gstab -0.5*(pwN-pw) -2.0*pw/PI);

   rgpulse(2*pw,zero,0.0,0.0);

   delay(2.0e-6);
   zgradpulse(icosel*gzlvl2,gt2);
   decpower(dpwr);    /* NO  13C decoupling */
   dec2power(dpwr2);  /* NO  15N decoupling */
   delay(gstab -2.0e-6 -2.0*GRADIENT_DELAY -2.0*POWER_DELAY);

   lk_sample();
/* BEGIN ACQUISITION */
status(C);
   setreceiver(t8);

}
