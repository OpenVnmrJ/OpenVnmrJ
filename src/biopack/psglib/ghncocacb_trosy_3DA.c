/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghncocacb_trosy_3DA.c


Notes from NMRFAM:

-make sure that the SE gradients gt1 and gt2 are matched. The values
 that we use here at NMRFAM are:
	gt1=.003 gzlvl1=21500
	gt2=.0005 gzlvl2=13073
 run a 1D array changing gzlvl5 +/-200 units and choose value that returns maximum signal
 (if your protein doesn't give much signal in a 1D experiment you may want to use a 
  different stronger sample ).
 
-for running a 1D or 2D:N15-H1 experiment use:
	fCT='n'
	fc180='y'

-for running constant time 2D:C13-H1 or 3D experiment use:
	fCT='y'

-for running non-constant time 2D:C13-H1 or 3D experiment use:
	fCT='n'
	fc180='n'

-to turn on H2 decoupling:
	dn3='H2'	(lock level should drop some when dn3 is set to 'H2')
        pwDlvl and pwD	power and pulse width for hard 90 pulse on H2
        dpwr3 and dmf3	power and dmf for H2 waltz decoupling

-to turn off H2 decoupling:
	dn3=''

-the flags:
	sel_flg
	nietl_flg
 can be turned on separately (one at the time) for actively suppressing the 
 unwanted TROSY components 
 for larger deuterated proteins however, this may not be necessary, hence set:
	sel_flg='n'
        nietl_flg='n'

 
    This pulse sequence will allow one to perform the following experiment:

     3D hncocacb (Hn,N,Cb) with deuterium decoupling
     and either CT or non-CT (fCT == y,n) for 13Cb acquisition, 
     It is optimized for fully (100 %) labeled proteins.

     F1    Cbeta (CT if fCT = y, i-1)
     F2    N + JNH/2 (i)
     F-acq HN - JNH/2 (i)

    This sequence uses the standard four channel configuration
         1)  1H             - carrier @ 4.7 ppm [H2O]
         2) 13C             - carrier @ 43 ppm (CA/CB)
         3) 15N             - carrier @ 120 ppm  
	 4)  2H		    - carrier @ 3 ppm  

    Standard flag settings
	f1180 =   'n'  for (0,0) phase correction in f1.
	f2180 =   'n'  for (0,0) phase correction in f2
	fsat =    'n' fscuba = 'n'. don't saturate water
        sel_flg = 'y' for active purging of the fast component
                = 'n' no active purging; relaxation attenuation of the
                   fast relaxation component
        fCT    = 'y' for CT evolution of Cb chemical shift 
        fc180= 'n' 
        cal_sphase='y' only for calibration of sphase.
                    set sphase to sphase - 45 check to get no signal.
                    (array for zero signal and add 45o).
                    set eta to a small value, gt12=gt13=gt14=0. 
        shared_CT 'y' for extending the acq in t2 (N15)
	dm = 'nnn'
	dm2 = 'nnn' dmm2 = 'ccc' [NO 15N decoupling during acquisition]
        dm3 = 'nnn' dmm3 = 'ccc'
        ampmode = 'dddp'
	
    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
    t1 [Cb]  and t2 [N]. [The fids must be manipulated (add/subtract) with
    'grad_sort_nd' program (or equivalent) before regular non-VNMR processing.]
    
    Use ampmode = 'dddp'

    Pulse scheme is optimized for completely deuterated samples
 
    Pulse scheme uses TROSY principle in 15N and NH with transfer of
    magnetization achieved using the enhanced sensitivity pfg approach,
 
    Yang and Kay, J. Biomol. NMR, January 1999, 13, 3-9.
    Yang and Kay, J. Am. Chem. Soc., 1999
 
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
          Yang and Kay, J. Am. Chem. Soc., 1999. 

    Written by L.E.Kay on Oct. 28, 1998

   Setup: See Shan et al. JACS, 1996 for details. Note that the expt. can
          be run as non-CT in Cb dimension (see Yamazaki et al. JACS 116,
          11655, 1994 for details).

          The "a" part of the sequence (pulses during this period have
           the suffex a) has the carrier at 174 (C') ppm.
          The "b" part of the sequence (pulses during this period have
           the suffex b) has the carrier at 43 (Ca/Cb) ppm. 

       Define delta as the distance in Hz between 174 (C') and 56 (Ca) ppm
       Define delta1 as the distance in Hz between 174 (C') and 43 (Ca/Cb) ppm

          pwco90a:  C' 90, applied with a stength of delta/sq(15), on res (174).
          pwco180a: C' 180, (seduce), on res (174 ppm).
          pwca180a: Ca 180, strength of delta/sq(3), off resonance
          pwca90b:  Ca/Cb 90,  strength of delta1/sq(15), on res (43 ppm).
          pwca180b: Ca/Cb 180, strength of delta1/sq(3), on res (43 ppm).
            (used only for calibration of sphase).
          pwcreb:   C 180 (reburp), on res (43 ppm).
          pwcgcob:  g3 pulse, inverts C' and Cg of aromatics, off res. 
          pwcgcobi: g3 pulse, inverts C' and Cg of aromatics, off res.
             (this is a time inverted g3 pulse ---> can use pwcgcob instead). 

    Based on hncocacb_CT_D_sel_pfg_600.c

    Modified by L.E.Kay on March 3, 1999 to insert C' 90 purge after 15N
    evolution to ensure in case where the N-C' refocussing is not complete
    (2*bigTN < 1/2JNC') that the remaining NxCz' magnetization which
    becomes NHxCz' is not present. This will remove a dispersive contribution
    to the F3 lineshape   

  Modified by L.E.Kay on Nov. 22, 2001 to include shared CT
  Modified by L.E.Kay on Dec. 6, 2001 to extend phase cycle

  Modified by L.E.Kay to add a nietl_flg so that the anti-trosy
  component is eliminated in an active manner with no less in s/n.
                        (D.Nietlispach,J.Biomol.NMR, 31,161(2005))

  pw_sl1, tpwrsl1 used only if nietl_flg == y
	
  Modified by E.Kupce from  hncocacb_D_trosy_lek_500a.c for autocalibration
  Modified by G.Gray, Feb2005, for BioPack
*/

#include <standard.h>
#include "Pbox_bio.h"
#define DELAY_BLANK 0.0e-6
#define max2(x,y) ((x)>(y)?(x):(y))

#define CREB180   "reburp 80p"                 /* RE-BURP 180 on Cab at 43 ppm, on resonance */
#define CREB180ps "-s 1.0 -attn i"                           /* RE-BURP 180 shape parameters */
#define G3CGCOB   "g3 80p 107p"                     /* G3 180 on C', at 150 ppm 107 ppm away */
#define G3CGCOBi  "g3 80p 107p 1"     /* time inverted G3 180 on C', at 150 ppm 107 ppm away */
#define CA180     "square180n 118p -118p"          /* hard 180 on CA, at 56 ppm 13 ppm away  */
#define CA180b    "square180n 133p"              /* hard 180 on CaCb, at 43 ppm on resonance */
#define CA90b     "square90n 133p"                /* hard 90 on CaCb, at 43 ppm on resonance */
#define CO90a     "square90n 118p"                 /* hard 90 on C', at 174 ppm on resonance */
#define CO180     "square180n 118p"               /* hard 180 on C', at 174 ppm 133 ppm away */
#define CA180ps   "-s 0.2 -attn d"                                  /* hard shape parameters */

static shape   cgcob, cgcoib, creb, ca180, ca180b, ca90b, co90a, co180;

static int  
            phi1[2]  = {0,2},
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
	    shca180a[MAXSTR],
	    shco180a[MAXSTR],
   
            shcreb[MAXSTR],  /* reburp shape for center of t1 period */ 
            shcgcob[MAXSTR], /* g3 inversion at 156.9 ppm (350 us) */
            shcgcoib[MAXSTR], /* g3 time inversion at 154 ppm (350 us) */
            sel_flg[MAXSTR],  /* flg for selecting active suppression of
                                 fast relaxation peak (y)  */
            fCT[MAXSTR],       /* set to y for constant time Cb acqn  */
            fc180[MAXSTR],
            cal_sphase[MAXSTR],
            shared_CT[MAXSTR],
            nietl_flg[MAXSTR];

 int         phase, phase2, ni, ni2, icosel, 
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter;   /* used for states tppi in t2           */ 

 double      tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             taua,         /*  ~ 1/4JNH =  2.25 ms */
             del1,       /* time for C'-N to refocuss set to 0.5*24.0 ms */
             bigTN,        /* nitrogen T period */
             bigTC,        /* carbon T period */

             pwN,          /* PW90 for 15N pulse              */
             pwNlvl,       /* high dec2 pwr for 15N hard pulses    */

             tsatpwr,      /* low level 1H trans.power for presat  */

             sw1,          /* sweep width in f1                    */             
             sw2,          /* sweep width in f2                    */             

             tauf,         /* 1/2J NH value                     */
             pw_sl,        /* selective pulse on water      */
             phase_sl,     /* phase for the selective pulse on water      */
             tpwrsl,       /* power for pw_sl               */
             at,
             zeta,
             eta,
	     pwco180a,
	     d_co180a,
	     pwco90a,
	     d_co90a,
	     pwca180a,
	     d_ca180a,
	     pwca90b,
	     d_ca90b,
	     pwca180b,
	     d_ca180b,

             d_cgcob,       /* power level for g3 pulses at 156.9 ppm */
             d_creb,        /* power level for reburp 180 at center of t1 */
             pwcgcob,       /* g3 ~ 350 us 180 pulse */
             pwcreb,        /* reburp ~ 400 us 180 pulse */

             pwDlvl,      /* power level 2H 90 pulse */
             pwD,         /* 2H 90 pulse, about 125 us */

             pw_sl1,
             tpwrsl1,

             compC,       /* C-13 RF calibration parameters */
             pwC,
             pwClvl,

             sphase,

	     gstab,	/* gradient recovery delay */

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
	     gt13,
	     gt14,
	     gt15,
	     gt16,

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
             gzlvl12, 
	     gzlvl13,
	     gzlvl14,
	     gzlvl15,
	     gzlvl16;
            
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
  pwN = getval("pwN");
  tpwr = getval("tpwr");
  tsatpwr = getval("tsatpwr");
  dpwr = getval("dpwr");
  pwNlvl = getval("pwNlvl");
  phase = (int) ( getval("phase") + 0.5);
  phase2 = (int) ( getval("phase2") + 0.5);
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  ni2 = getval("ni2");
  ni = getval("ni");
  tauf = getval("tauf");
  pw_sl = getval("pw_sl");
  phase_sl = getval("phase_sl");
  tpwrsl = getval("tpwrsl");
  at = getval("at");
  zeta = getval("zeta");
  eta = getval("eta");
  pwD = getval("pwD");
  pwDlvl = getval("pwDlvl");
  sphase = getval("sphase");
  tpwrsl1 = getval("tpwrsl1");
  pw_sl1 = getval("pw_sl1");

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
  gt11 = getval("gt11");
  gt12 = getval("gt12");
  gt13 = getval("gt13");
  gt14 = getval("gt14");
  gt15 = getval("gt15");
  gt16 = getval("gt16");
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
  gzlvl13 = getval("gzlvl13");
  gzlvl14 = getval("gzlvl14");
  gzlvl15 = getval("gzlvl15");
  gzlvl16 = getval("gzlvl16");

  if(autocal[0]=='n')
  {     
    getstr("shco180a",shco180a);
    getstr("shca180a",shca180a);
    getstr("shcgcob",shcgcob);
    getstr("shcgcoib",shcgcoib);
    getstr("shcreb",shcreb);

    pwco90a = getval("pwco90a");
    pwca90b = getval("pwca90b");
    pwco180a = getval("pwco180a");
    pwca180a = getval("pwca180a");
    pwca180b = getval("pwca180b");
    pwcgcob = getval("pwcgcob");
    pwcreb = getval("pwcreb");

    d_co90a = getval("d_co90a");
    d_ca90b = getval("d_ca90b");
    d_co180a = getval("d_co180a");
    d_ca180a = getval("d_ca180a");
    d_ca180b = getval("d_ca180b");
    d_cgcob = getval("d_cgcob");
    d_creb = getval("d_creb");
  }
  else
  {        
    strcpy(shco180a,"Phard");    
    strcpy(shca180a,"Phard_-118p"); 
    strcpy(shcgcob,"Pg3_107p");    
    strcpy(shcgcoib,"Pg3i_107p");    
    strcpy(shcreb,"Preb_on");    
    if (FIRST_FID)  
    {
      compC = getval("compC");
      pwC = getval("pwC");
      pwClvl = getval("pwClvl");
      cgcob = pbox(shcgcob, G3CGCOB, CREB180ps, dfrq, compC*pwC, pwClvl);
      cgcoib = pbox(shcgcoib, G3CGCOBi, CREB180ps, dfrq, compC*pwC, pwClvl);                   
      creb = pbox(shcreb, CREB180, CREB180ps, dfrq, compC*pwC, pwClvl);  
      co90a = pbox("Phard90", CO90a, CA180ps, dfrq, compC*pwC, pwClvl);
      ca90b = pbox("Phard90", CA90b, CA180ps, dfrq, compC*pwC, pwClvl);
      ca180b = pbox("Phard180", CA180b, CA180ps, dfrq, compC*pwC, pwClvl);                  
      ca180 = pbox(shca180a, CA180, CA180ps, dfrq, compC*pwC, pwClvl);    
      co180 = pbox(shco180a, CO180, CA180ps, dfrq, compC*pwC, pwClvl);  
    }  
    pwco180a = co180.pw;
    d_co180a = co180.pwr;
    pwca180a = ca180.pw;
    d_ca180a = ca180.pwr;
    pwca180b = ca180b.pw;
    d_ca180b = ca180b.pwr;  
    pwca90b = ca90b.pw;
    d_ca90b = ca90b.pwr;
    pwco90a = co90a.pw;
    d_co90a = co90a.pwr;
    pwcgcob = cgcob.pw;
    d_cgcob = cgcob.pwr;
    pwcreb = creb.pw;
    d_creb = creb.pwr;
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
    if(bigTN - 0.5*(ni2 - 1)/sw2 + pwco180a - POWER_DELAY       
       + WFG_START_DELAY + WFG_STOP_DELAY < 0.2e-6)
    {
        text_error(" ni2 is too big\n");
        text_error(" please set ni2 smaller or equal to %d\n",
    			(int) ((bigTN +pwco180a -POWER_DELAY       
       				+WFG_START_DELAY +WFG_STOP_DELAY)*sw2*2.0) +1 );
        psg_abort(1);
    }


   if(fCT[A] == 'y')
    if (bigTC - 0.5*(ni-1)/sw1 - WFG_STOP_DELAY - gt16 - 102.0e-6
         - POWER_DELAY - 4.0e-6 - pwD - POWER_DELAY - PRG_START_DELAY
         - POWER_DELAY - 4.0e-6 - WFG_START_DELAY - pwcgcob - WFG_STOP_DELAY
         - POWER_DELAY  < 0.2e-6) {
        text_error(" ni is too big\n");
        text_error(" please set ni smaller or equal to %d\n",
    			(int) ((bigTC -WFG_STOP_DELAY -gt16 -102.0e-6
         			-POWER_DELAY -4.0e-6 -pwD -POWER_DELAY -PRG_START_DELAY
         			-POWER_DELAY -4.0e-6 -WFG_START_DELAY -pwcgcob -WFG_STOP_DELAY
         			-POWER_DELAY)*sw1*2.0) +1 );
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

    if( pwNlvl > 63 )
    {
        text_error("don't fry the probe, DHPWR2 too large!  ");
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

    if( pwca90b > 200.0e-6 )
    {
        text_error("dont fry the probe, pwca90b too high ! ");
        psg_abort(1);
    } 


    if( f1180[A] != 'n' && f2180[A] != 'n' ) {
        text_error("flags may be set wrong: set f1180=n and f2180=n for 3d\n");
        psg_abort(1);
    }


    if( gt1 > 15e-3 || gt2 > 15e-3 || gt3 > 15e-3 
	|| gt4 > 15e-3 || gt5 > 15e-3 || gt6 > 15e-3 
	|| gt7 > 15e-3 || gt8 > 15e-3 || gt9 > 15e-3 
	|| gt10 > 15e-3 || gt11 > 15e-3 || gt12 > 15e-3 
	|| gt13 > 15e-3 || gt14 > 15e-3 || gt15 > 15e-3  
        || gt16 > 15e-3)
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

    if( at > 0.1 && dm2[C] == 'y') {
       text_error("too long at with dec2\n");
       psg_abort(1);
    }
    if (pwco180a > 1e-3) {
	text_error("pwco180a too long\n");
	psg_abort(1);
    }
    if (d_co180a > 58) {
	text_error("d_co180a too high\n");
	psg_abort(1);
    }
    if (pwco90a > 1e-3) {
	text_error("pwco90a too long\n");
	psg_abort(1);
    }
    if (d_co90a > 51) {
	text_error("d_co90a too high\n");
	psg_abort(1);
    }
    if (pwca180a > 1e-3) {
	text_error("pwca180a too long\n");
	psg_abort(1);
    }
    if (d_ca180a > 58) {
	text_error("d_ca180a too high\n");
	psg_abort(1);
    }
    if (pwca90b > 1e-3) {
	text_error("pwca90b too long\n");
	psg_abort(1);
    }
    if (d_ca90b > 52) {
	text_error("d_ca90b too high\n");
	psg_abort(1);
    }
    if (pwca180b > 1e-3) {
	text_error("pwca180b too long\n");
	psg_abort(1);
    }
    if (d_ca180b > 59) {
	text_error("d_ca180b too high\n");
	psg_abort(1);
    }
    if ( ni/sw1 > 0.1) {
	text_error("too long t1 with decoupling\n");
	psg_abort(1);
    }
    if (eta > 0.1) {
	text_error("too long eta \n");
	psg_abort(1);
    }

    if(pwDlvl > 59) {
        text_error("pwDlvl is too high; < 59\n");
        psg_abort(1);
    }

    if(d_creb > 62) {
        text_error("d_creb is too high; <= 62\n");
        psg_abort(1);
    }

    if(d_cgcob > 60) {
        text_error("d_gcob is too high; <= 60\n");
        psg_abort(1);
    }

   if(cal_sphase[A] == 'y') {
 text_error("Use only to calibrate sphase\n");
 text_error("Set eta to < 600 us, gt12=gt13=gt14=gt15=0, fCT=y,fc180=n\n");
    } 

   if(sel_flg[A] == 'y' && nietl_flg[A] == 'y') {
    text_error("both sel_flg and nietl_flg cannot be yes\n");
    psg_abort(1);
   }

   if(fCT[A]=='n' && fc180[A] == 'y' && ni > 1) {
    text_error("must set fc180='n' to allow Cbeta evolution (ni > 1).\n");
    psg_abort(1);
   }

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );

/*  Phase incrementation for hypercomplex 2D data */

    /* 1 to 3 so that spectrum rev. not needed */
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

/*  Set up f1180  tau1 = t1       */

    tau1 = d2;

    if(f1180[A] == 'y' && fCT[A] == 'y' && ni>1) 
          tau1 += ( 1.0 / (2.0*sw1) );
  
    if(f1180[A] == 'y' && fCT[A] == 'n' && ni>1)
          tau1 += (1.0 / (2.0*sw1) -4.0/PI*pwca90b -POWER_DELAY 
                    -4.0e-6);

    if(f1180[A] == 'n' && fCT[A] == 'n' && ni>1)
          tau1 = (tau1 -4.0/PI*pwca90b -POWER_DELAY 
                    -4.0e-6);

    if(tau1 < 0.0 && fc180[A]=='n') { 
            tau1 = 2.0e-6;
            text_error("tau1 is < 0.2e-6, there may be a problem\n");
    } 

    tau1 = tau1/2.0;

/*  Set up f2180  tau2 = t2               */

    tau2 = d3;
    if(f2180[A] == 'y' && ni2>1) {
        tau2 += ( 1.0 / (2.0*sw2) ); 
        if(tau2 < 0.2e-6) tau2 = 0.2e-6;
    }
    tau2 = tau2/2.0;


/* Calculate modifications to phases for States-TPPI acquisition          */

   if(t1_counter % 2) {
      tsadd(t2,2,4);     
      tsadd(t8,2,4);    
    }

   if(t2_counter % 2) {
      tsadd(t5,2,4);  
      tsadd(t8,2,4);    
    }

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tsatpwr);     /* Set transmitter power for 1H presaturation */
   decpower(d_co90a);     /* Set Dec1 power for 13C' 90 pulses          */
   dec2power(pwNlvl);     /* Set Dec2 power for 15N hard pulses         */
   dec3power(pwDlvl);      /* Set Dec3 power for 2H hard pulses */

/* Presaturation Period */

   if (fsat[0] == 'y')
     {
      delay(2.0e-5);
      rgpulse(d1,zero,2.0e-6,2.0e-6);
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
   decoffset(dof+(174-43)*dfrq);
   delay(20.0e-6);

   decrgpulse(pwco90a,zero,4.0e-6,4.0e-6);

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */

   delay(0.2e-6);
   zgradpulse(gzlvl5,gt5);
   delay(2.0e-6);

   delay(taua -gt5 -2.2e-6);   /* taua <= 1/4JNH */ 

   sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);

   dec2phase(zero); decphase(zero); 

   delay(taua -gt5 -200.2e-6); 

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

      dec2rgpulse(pwN,zero,0.0,0.0);
      dcplr2phase(zero);
   
      delay(1.34e-3 - SAPS_DELAY);

      rgpulse(pw,one,0.0,0.0);
      rgpulse(2.0*pw,zero,2.0e-6,0.0);
      rgpulse(pw,one,2.0e-6,0.0);

      delay(del1 - 1.34e-3 - 4.0*pw - 4.0e-6);
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

      dec2rgpulse(pwN,zero,0.0,0.0);

      delay(del1);
     }


   dec2rgpulse(2*pwN,zero,0.0,0.0);
   decpower(d_co180a);
   decshaped_pulse(shco180a,pwco180a,zero,4.0e-6,0.0);

   dec2phase(one);

   delay(del1
		- POWER_DELAY - 4.0e-6 - pwco180a 
                - WFG_START_DELAY - WFG_STOP_DELAY);

   dec2rgpulse(pwN,one,0.0,0.0);

   zgradpulse(gzlvl10,gt10);
   delay(200e-6);

   decphase(t1);
   decpower(d_co90a);
   decrgpulse(pwco90a,t1,4.0e-6,0.0);

   decphase(zero);
   decpower(d_ca180a);
   decshaped_pulse(shca180a,pwca180a,zero,4.0e-6,0.0);

   zgradpulse(gzlvl14,gt14);
   delay(200e-6);

   delay(zeta 
	- POWER_DELAY - WFG_START_DELAY - 4.0e-6 
        - pwca180a - WFG_STOP_DELAY
	- gt14 - 200e-6
	- POWER_DELAY - 4.0e-6 - WFG_START_DELAY);

   decphase(zero);
   decpower(d_co180a);
   decshaped_pulse(shco180a,pwco180a,zero,4.0e-6,0.0);

   decphase(zero);
   decpower(d_ca180a);
   decshaped_pulse(shca180a,pwca180a,zero,4.0e-6,0.0);

   zgradpulse(gzlvl14,gt14);
   delay(200e-6);

   delay(zeta 
	- WFG_STOP_DELAY
	- gt14 - 200e-6
	- POWER_DELAY - WFG_START_DELAY - 4.0e-6 
        - pwca180a - WFG_STOP_DELAY
	- POWER_DELAY - 4.0e-6);

   decphase(one);
   decpower(d_co90a);
   decrgpulse(pwco90a,one,4.0e-6,0.0);

   decphase(t2);

   decoffset(dof);

   delay(0.2e-6);
   zgradpulse(gzlvl4,gt4);
   delay(200.0e-6);

   dec2phase(t5); 

   /* Turn on D decoupling using the third decoupler */
   dec3power(pwDlvl);
   dec3rgpulse(pwD,one,4.0e-6,0.0);
   dec3phase(zero);
   dec3power(dpwr3);
   dec3unblank();
   setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
   /* Turn on D decoupling */

   decpower(d_ca90b);
   decrgpulse(pwca90b,t2,4.0e-6,0.0);

   delay(eta 
	- PRG_STOP_DELAY - DELAY_BLANK - POWER_DELAY
	- 4.0e-6 - pwD 
	- gt15 - 100.0e-6
	- POWER_DELAY - WFG_START_DELAY);

   /* Turn off D decoupling */
   setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
   dec3blank();
   dec3power(pwDlvl);
   dec3phase(three);
   dec3rgpulse(pwD,three,4.0e-6,0.0);
   /* Turn off D decoupling */

   zgradpulse(gzlvl15,gt15);
   delay(100.0e-6);

   if (cal_sphase[A] == 'y') 
     {
      decpower(d_ca180b);
      decshaped_pulse("hard",pwca180b,zero,4.0e-6,4.0e-6);
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
   
   zgradpulse(gzlvl15,gt15);
   delay(100.0e-6);

   /* Turn on D decoupling using the third decoupler */
   dec3power(pwDlvl);
   dec3phase(one);
   dec3rgpulse(pwD,one,4.0e-6,0.0);
   dec3phase(zero);
   dec3power(dpwr3);
   dec3unblank(); 
   setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
   /* Turn on D decoupling */

   delay(eta - WFG_STOP_DELAY
	- gt15 - 100.0e-6
	- POWER_DELAY - 4.0e-6 - pwD
	- POWER_DELAY - DELAY_BLANK - PRG_START_DELAY
	- POWER_DELAY - 4.0e-6);

   decphase(t3);
   decpower(d_ca90b);

   decrgpulse(pwca90b,t3,4.0e-6,0.0);
/* Cbeta EVOLUTION BEGINS */

   if (fCT[A] == 'y') 	/* CONSTANT TIME Cbeta EVOLUTION */
     {
      delay(tau1); 
      decpower(d_cgcob);
      decshaped_pulse(shcgcob,pwcgcob,zero,4.0e-6,0.0);

      delay(bigTC 
         -POWER_DELAY -WFG_START_DELAY
         -4.0e-6
         -pwcgcob -WFG_STOP_DELAY
         -102.0e-6 -gt16 -POWER_DELAY 
         -4.0e-6
         -WFG_START_DELAY -PRG_STOP_DELAY -DELAY_BLANK
         -POWER_DELAY -4.0e-6 -pwD);
 
      /* Turn off D decoupling */
      setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
      dec3blank();
      dec3phase(three);
      dec3power(pwDlvl);
      dec3rgpulse(pwD,three,4.0e-6,0.0);
      /* Turn off D decoupling */
    
      initval(1.0,v2);
      decstepsize(sphase);  
      dcplrphase(v2);

      delay(2.0e-6);
      zgradpulse(gzlvl16,gt16);
      delay(100.0e-6);
  
      decpower(d_creb);
      decshaped_pulse(shcreb,pwcreb,zero,4.0e-6,0.0);

      dcplrphase(zero);
    
      delay(2.0e-6);
      zgradpulse(gzlvl16,gt16);
      delay(100.0e-6);

      /* Turn on D decoupling using the third decoupler */
      dec3power(pwDlvl);
      dec3phase(one);
      dec3rgpulse(pwD,one,4.0e-6,0.0);
      dec3phase(zero);
      dec3power(dpwr3);
      dec3unblank(); 
      setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
      /* Turn on D decoupling */

      delay(bigTC -tau1 -WFG_STOP_DELAY -gt16
         	-102.0e-6 -POWER_DELAY -4.0e-6 -pwD -POWER_DELAY
         	-PRG_START_DELAY -POWER_DELAY -4.0e-6 -WFG_START_DELAY
         	-pwcgcob -WFG_STOP_DELAY -POWER_DELAY -4.0e-6); 
	
      decpower(d_cgcob);
      decshaped_pulse(shcgcoib,pwcgcob,zero,4.0e-6,0.0);
      decphase(t4);
     }
    else if (fCT[A] =='n' && fc180[A] =='n')
     {
      delay(tau1);
      delay(tau1);
     }
    else if (fCT[A] =='n' && fc180[A] =='y')  
     {
      initval(1.0,v2);
      decstepsize(sphase);  
      dcplrphase(v2);
      decpower(d_creb);

      decshaped_pulse(shcreb,pwcreb,zero,4.0e-6,0.0);

      dcplrphase(zero);
     }
   decpower(d_ca90b);

/* Cbeta EVOLUTION ENDS */
   decrgpulse(pwca90b,t4,4.0e-6,0.0);

   delay(eta 
	-PRG_STOP_DELAY -DELAY_BLANK -POWER_DELAY
	-4.0e-6 -pwD
	-gt12 -100.0e-6
	-POWER_DELAY - WFG_START_DELAY);

   /* Turn off D decoupling */
   setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
   dec3blank();
   dec3phase(three);
   dec3power(pwDlvl);
   dec3rgpulse(pwD,three,4.0e-6,0.0);
   /* Turn off D decoupling */

   zgradpulse(gzlvl12,gt12);
   delay(100.0e-6);

   if (cal_sphase[A] == 'y') 
     {
      decpower(d_ca180b);
      decshaped_pulse("hard",pwca180b,zero,4.0e-6,4.0e-6);
     }
   else 
     {
      initval(1.0,v4);
      decstepsize(sphase);
      dcplrphase(v4);

      decpower(d_creb);
      decshaped_pulse(shcreb,pwcreb,zero,4.0e-6,4.0e-6);

      dcplrphase(zero);
     }
   
   zgradpulse(gzlvl12,gt12);
   delay(100.0e-6);

   /* Turn on D decoupling using the third decoupler */
   dec3power(pwDlvl);
   dec3phase(one);
   dec3rgpulse(pwD,one,4.0e-6,0.0);
   dec3phase(zero);
   dec3power(dpwr3);
   dec3unblank();
   setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
   /* Turn on D decoupling */

   delay(eta - WFG_STOP_DELAY
	- gt12 - 100.0e-6
	- POWER_DELAY - 4.0e-6 - pwD 
	- POWER_DELAY - DELAY_BLANK - PRG_START_DELAY
	- POWER_DELAY - 4.0e-6);

   decphase(zero);
   decpower(d_ca90b);
   decrgpulse(pwca90b,zero,4.0e-6,0.0);

   /* Turn off D decoupling */
   setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
   dec3blank();
   dec3phase(three);
   dec3power(pwDlvl);
   dec3rgpulse(pwD,three,4.0e-6,0.0);
   /* Turn off D decoupling */

   decoffset(dof+(174-43)*dfrq);

   zgradpulse(gzlvl11,gt11);
   delay(200e-6);

   decphase(one);
   decpower(d_co90a);
   decrgpulse(pwco90a,one,4.0e-6,0.0);

   zgradpulse(gzlvl13,gt13);
   delay(200e-6);

   delay(zeta 
	- POWER_DELAY - WFG_START_DELAY - 4.0e-6 
        - pwca180a - WFG_STOP_DELAY
	- gt13 - 200e-6
	- POWER_DELAY - WFG_START_DELAY - 4.0e-6);

   decphase(zero);
   decpower(d_ca180a);
   decshaped_pulse(shca180a,pwca180a,zero,4.0e-6,0.0);

   decphase(zero);
   decpower(d_co180a);
   decshaped_pulse(shco180a,pwco180a,zero,4.0e-6,0.0);

   zgradpulse(gzlvl13,gt13);
   delay(200e-6);

   delay(zeta 
	- WFG_STOP_DELAY
	- gt13 - 200e-6
	- POWER_DELAY - WFG_START_DELAY - 4.0e-6 
        - pwca180a - WFG_STOP_DELAY
	- POWER_DELAY - 4.0e-6);

   decphase(zero);
   decpower(d_ca180a);
   decshaped_pulse(shca180a,pwca180a,zero,4.0e-6,0.0);

   decphase(zero);
   decpower(d_co90a);
   decrgpulse(pwco90a,zero,4.0e-6,0.0);
   decphase(zero);

   delay(0.2e-6);
   zgradpulse(gzlvl9,gt9);
   delay(200.0e-6);

   if (shared_CT[A] == 'n') 
     {
      dec2rgpulse(pwN,t5,2.0e-6,0.0);

      dec2phase(t6); decphase(zero);

      delay(bigTN - tau2 
		+ pwco180a - POWER_DELAY + WFG_START_DELAY + WFG_STOP_DELAY);

      decpower(d_co180a);
      dec2rgpulse(2*pwN,t6,0.0,0.0);
      decshaped_pulse(shco180a,pwco180a,zero,0.0,0.0);

      dec2phase(t7);
 
      delay(bigTN - gt1 - 2.0*GRADIENT_DELAY - 500.2e-6
           	- POWER_DELAY - 4.0e-6 - WFG_START_DELAY
           	- pwca180a - WFG_STOP_DELAY); 

      delay(0.2e-6);
      zgradpulse(gzlvl1,gt1);
      delay(500.0e-6);

      decpower(d_ca180a);
      decshaped_pulse(shca180a,pwca180a,zero,4.0e-6,0.0);

      delay(tau2);

      sim3pulse(pw,0.0,pwN,zero,zero,t7,0.0,0.0);
     }
   else if (shared_CT[A] == 'y') 
     {
      if (bigTN -tau2 > 0.2e-6) 
        {
         dec2rgpulse(pwN,t5,2.0e-6,0.0);
         dec2phase(t6); decphase(zero);

         delay(tau2);
         decpower(d_ca180a);
         decshaped_pulse(shca180a,pwca180a,zero,4.0e-6,0.0);

         delay(bigTN
           	- POWER_DELAY - 4.0e-6 - WFG_START_DELAY
           	- pwca180a - WFG_STOP_DELAY
           	- POWER_DELAY
           	- gt1 - 500.2e-6 - 2.0*GRADIENT_DELAY
           	- WFG_START_DELAY - pwco180a - WFG_STOP_DELAY);

         decpower(d_co180a);

         delay(0.2e-6);
         zgradpulse(gzlvl1,gt1);
         delay(500.0e-6);

         decshaped_pulse(shco180a,pwco180a,zero,0.0,0.0);
         dec2rgpulse(2*pwN,t6,0.0,0.0);

         delay(bigTN - tau2);
         sim3pulse(pw,0.0,pwN,zero,zero,t7,0.0,0.0);
        }
       else    /* bigTN - tau2 <= 0.2e-6  */
        {
         dec2rgpulse(pwN,t5,2.0e-6,0.0);
         dec2phase(t6); decphase(zero);

         delay(tau2);
         decpower(d_ca180a);
         decshaped_pulse(shca180a,pwca180a,zero,4.0e-6,0.0);

         delay(bigTN
           	- POWER_DELAY - 4.0e-6 - WFG_START_DELAY
           	- pwca180a - WFG_STOP_DELAY
           	- POWER_DELAY
           	- gt1 - 500.2e-6 - 2.0*GRADIENT_DELAY
           	- WFG_START_DELAY - pwco180a - WFG_STOP_DELAY);

         decpower(d_co180a);

         delay(0.2e-6);
         zgradpulse(gzlvl1,gt1);
         delay(500.0e-6);

         decshaped_pulse(shco180a,pwco180a,zero,0.0,0.0);
         delay(tau2 - bigTN);
  
         dec2rgpulse(2*pwN,t6,0.0,0.0);

         sim3pulse(pw,0.0,pwN,zero,zero,t7,0.0,0.0);
        }
     }

   if (nietl_flg[A] == 'n') 
     {
      decpower(d_co90a);
      decrgpulse(pwco90a,zero,4.0e-6,0.0);

      delay(0.2e-6);
      zgradpulse(gzlvl6,gt6);
      delay(2.0e-6);

      dec2phase(zero);
      delay(tauf -POWER_DELAY -4.0e-6 
              -pwco90a -gt6 -2.2e-6);

      sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);

      txphase(one);
      dec2phase(one);

      delay(tauf -gt6 -200.2e-6);

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

      delay(tauf -gt7 -200.2e-6);

      delay(0.2e-6);
      zgradpulse(gzlvl7,gt7);
      delay(200.0e-6);
   
      sim3pulse(pw,0.0e-6,pwN,zero,zero,zero,0.0,0.0);
     }
   else 
     {
      /* shaped pulse */
      obspower(tpwrsl1);
      shaped_pulse(shp_sl,pw_sl1,zero,2.0e-6,0.0);
      delay(2.0e-6);
      obspower(tpwr);
      /* shaped pulse */

      decpower(d_co90a);
      decrgpulse(pwco90a,zero,4.0e-6,0.0);
   
      delay(0.2e-6);
      zgradpulse(gzlvl6,gt6);
      delay(2.0e-6);

      dec2phase(zero);
      delay(tauf 
              - POWER_DELAY - 2.0e-6 - WFG_START_DELAY - pw_sl1 - WFG_STOP_DELAY
              - 2.0e-6 - POWER_DELAY
              - POWER_DELAY - 4.0e-6 
              - pwco90a - gt6 - 2.2e-6);

      sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);

      txphase(one);
      dec2phase(zero);

      delay(tauf -gt6 -200.2e-6);

      delay(0.2e-6);
      zgradpulse(gzlvl6,gt6);
      delay(200.0e-6);

      sim3pulse(pw,0.0,pwN,one,zero,zero,0.0,0.0);
   
      delay(0.2e-6);
      zgradpulse(gzlvl7,gt7);
      delay(2.0e-6);
 
      txphase(zero);
      dec2phase(zero);
      delay(tauf -gt7 -2.2e-6);

      sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);
      txphase(one);
      dec2phase(one);
   
      delay(tauf -gt7 -200.2e-6);

      delay(0.2e-6);
      zgradpulse(gzlvl7,gt7);
      delay(200.0e-6);
      
      sim3pulse(pw,0.0e-6,pwN,one,zero,one,0.0,0.0);
      txphase(zero);
     }

   delay(gt2 +gstab -0.5*(pwN -pw) -2.0/PI*pw);

   rgpulse(2*pw,zero,0.0,0.0);

   delay(2.0e-6);
   zgradpulse(icosel*gzlvl2,gt2);
   decpower(dpwr);
   dec2power(dpwr2);
   delay(gstab -2.0e-6 -2.0*GRADIENT_DELAY -2.0*POWER_DELAY);
    
   lk_sample();
/* BEGIN ACQUISITION */
status(C);
   setreceiver(t8);
}
