/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghncoca_seq_trosy_3DA.c - auto-calibrated version of the original sequence
 
    This pulse sequence will allow one to perform the following experiment:

    3D hncoca (Hn,N,Ca) (enhanced sensitivity PFG) with deuterium decoupling
                        F1       C_alpha     (i-1)	(constant-time with fCT='y')
                        F2       N  + JNH/2  (i)
                        F3(acq)  NH - JNH/2  (i)

    This sequence is designed for highly deuterated samples and makes use
     of TROSY principles to enhance sensitivity in N and HN dimensions.

    This sequence uses the standard four channel configuration
         1)  1H             - carrier @ 4.7 ppm [H2O]
         2) 13C             - carrier @ 56 ppm (CA)
         3) 15N             - carrier @ 120 ppm  
	 4)  2H		    - carrier @ 4.5 ppm  

    Standard flag setting
	fCT = 'y'  for constant time Ca chemical shift evol. period
	fc180 = 'n' for Ca chemical shift refocussing (only when fCT='n')
	f1180 = 'n'  for (0,0) phase correction in f1
	f2180 = 'n'  for (0,0) phase correction in f2
	fsat = 'n' fscuba = 'n'. don't saturate water
        sel_flg = 'y' for active suppression of the fast relaxing component
                  'n' for relaxation suppression of the fast relaxing
                      component 
	dm = 'nnn'
	dm2 = 'nnn' dmm2 = 'ccc' [NO 15N decoupling during acquisition]
	dm3 = 'nnn' dmm3 = 'ccc'
	
    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI
    acquisition in t1 [Ca]  and t2 [N]. [The fids must be manipulated (add/
    subtract) with 'grad_sort_nd' program (or equivalent) before regular
    processing with non-VNMR processing software]
    
    This scheme uses TROSY principle in 15N and NH with transfer of
    magnetization achieved using the enhanced sensitivity pfg approach,

    Yang and Kay, J. Biomol. NMR, January 1999, 13, 3-9.
    Yang and Kay, J. Am. Chem. Soc., 1999.

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

    Use ampmode='dddp'

REF: Yamazaki et. al.   J. Am. Chem. Soc. 116, 11655 (1994)
     Yamazaki et. al.   J. Am. Chem. Soc. 116,  6464 (1994)
     Pervushin et al. PNAS, 94, 12366-12371 (1997)
     Pervushin et al. J. Biomol. NMR Aug 1998
     Yang and Kay, J. Biomol. NMR, Jan 1999
     Yang and Kay, J. Am. Chem. Soc., 1999.


  Setup:  delta is the difference in Hz between C' and Ca 
   At a:  carbon carrier on C' (174ppm).
      b:  carbon carrier on Ca (56 ppm).
          pwco90a:  C' 90 at delta/sq(15), on res.
          pwco180a: C' 180 (seduce), on res.
          pwca180a: Ca 180 at delta/sq(3), off res.
          pwca90b:  Ca 90 at delta/sq(15), on res.
          pwca180b: reburp 180, at 45 ppm. 
          pwco180b: C' 180 (seduce), off res. --> if fCT==y
                    C' 180 at delta/sq(3), off res. --> if fCT==n
          shared_CT: set to y to extend N15 acq. time
     

   Modified by L.E.Kay on March 3, 1999 to insert C' 90 purge after 15N
    evolution to ensure in case where the N-C' refocussing is not complete
    (2*bigTN < 1/2JNC') that the remaining NxCz' magnetization which
    becomes NHxCz' is not present. This will remove a dispersive contribution
    to the F3 lineshape 

   Modified by L.E.Kay on Nov. 22 2001 to include shared CT in the 15N dimension (F2)

   Modified by L.E.Kay to include a nietl_flg to suppress the anti-trosy component
     in an active manner with no sensitivity loss 
                        (D.Nietlispach,J.Biomol.NMR,31,161(2005))

   Modified by E.Kupce for AutoCalibrate, Jan 2005 from hncoca_D_trosy_lek_500.c
   Modified by G.Gray for BioPack, Feb 2005


   pw_sl, tpwrsl unless sel_flg == y
      Used for the first flip back

   pw_sl1, tpwrsl1
      Used only if nietl_flg == y
*/

#include <standard.h>
#include "Pbox_bio.h"

#define DELAY_BLANK 0.0e-6

#define CA180reb  "reburp 80p -13p"             /* RE-BURP 180 on Cab at 43 ppm, 13 ppm away */
#define CA180ps   "-s 1.0 -attn i"                           /* RE-BURP 180 shape parameters */
#define CO180a    "square180n 118p"               /* hard 180 on C', at 174 ppm on resonance */
#define CO180b    "square180n 118p 118p"          /* hard 180 on C', at 174 ppm 118 ppm away */
#define CO180ps   "-s 0.2 -attn d"                              /* hard 180 shape parameters */
#define CA180     "square180n 118p -118p"         /* hard 180 on CA, at 56 ppm 118 ppm away  */
#define CO90a     "square90n 118p"                /* hard  90 on C', at 174 ppm on resonance */

static shape co90a, ca180a, ca180reb, co180a, co180b;

static int  phi7[4]  = {0,0,2,2},
            phi11[1] = {0},
            phi6[8]  = {0,0,0,0,2,2,2,2},
            phi12[2] = {1,3},
            rec[4]   = {0,0,2,2},
            phi13[8] = {0,0,0,0,2,2,2,2},
            phi15[1] = {0},
            phi18[2] = {0,2};

static double d2_init=0.0, d3_init=0.0;

void pulsesequence()

{

/* DECLARE VARIABLES */

 char       autocal[MAXSTR],  /* auto-calibration flag */
            fsat[MAXSTR],
	    fscuba[MAXSTR],
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell             */
            ddseq[MAXSTR],    /* deuterium decoupling sequence */
            shp_sl[MAXSTR],
            fCT[MAXSTR],	      /* Flag for constant time C13 evolution */
            fc180[MAXSTR],
	    shca180a[MAXSTR],
            shca180b[MAXSTR],
	    shco180a[MAXSTR],
	    shco180b[MAXSTR],
            sel_flg[MAXSTR],
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
             tsatpwr,      /* low level 1H trans.power for presat  */

	     pwDlvl,	   /* power for D flank pulse */
	     pwD,	   /* pw90 at pwDlvl  */

             sw1,          /* sweep width in f1                    */             
             sw2,          /* sweep width in f2                    */             
             tauf,         /* 1/2J NH value                     */
             dresD,        /* dres for the deuterium decoupling */
             pw_sl,        /* selective pulse on water      */
             tpwrsl,       /* power for pw_sl               */
             at,
             zeta,
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
	     pwco180b,
	     d_co180b,

             compC,       /* C-13 RF calibration parameters */
             pwC,
             pwClvl,

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
	     gt10,
	     gt11,
             gt12,
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
             gzlvl10,
             gzlvl11,
             gzlvl12,
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
  getstr("fCT",fCT);
  getstr("fc180",fc180);

  getstr("sel_flg",sel_flg);
  getstr("shared_CT",shared_CT);

  getstr("nietl_flg",nietl_flg);

  taua   = getval("taua"); 
  del1  = getval("del1");
  bigTN = getval("bigTN");
  bigTC = getval("bigTC");
  pwca90b = getval("pwca90b");
  pwN = getval("pwN");
  pwD = getval("pwD");
  pwDlvl = getval("pwDlvl");
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
  dresD = getval("dresD");
  pw_sl = getval("pw_sl");
  tpwrsl = getval("tpwrsl");
  at = getval("at");
  zeta = getval("zeta");
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
  gt10 = getval("gt10");
  gt11 = getval("gt11");
  gt12 = getval("gt12");
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
  gzlvl10 = getval("gzlvl10");
  gzlvl11 = getval("gzlvl11");
  gzlvl12 = getval("gzlvl12");
  gzlvl13 = getval("gzlvl13");
  gzlvl14 = getval("gzlvl14");

  if(autocal[0]=='n')
  {     
    getstr("shca180a",shca180a);
    getstr("shco180a",shco180a);
    getstr("shco180b",shco180b);
    getstr("shca180b",shca180b);
    
    pwco180a = getval("pwco180a");
    d_co180a = getval("d_co180a");
    pwco90a = getval("pwco90a");
    d_co90a = getval("d_co90a");
    pwca180a = getval("pwca180a");
    d_ca180a = getval("d_ca180a");
    pwca90b = getval("pwca90b");
    d_ca90b = getval("d_ca90b");
    pwca180b = getval("pwca180b");
    d_ca180b = getval("d_ca180b");
    pwco180b = getval("pwco180b");
    d_co180b = getval("d_co180b");
  }
  else
  {        
    strcpy(shca180b,"Preburp_-15p");    
    strcpy(shco180a,"Phard");    
    strcpy(shco180b,"Phard_118p");    
    strcpy(shca180a,"Phard_-118p");    
    if (FIRST_FID)
    {
      compC = getval("compC");
      pwC = getval("pwC");
      pwClvl = getval("pwClvl");
      ca180reb = pbox(shca180b, CA180reb, CA180ps, dfrq, compC*pwC, pwClvl);                  
      co180a = pbox(shco180a, CO180a, CO180ps, dfrq, compC*pwC, pwClvl);            
      co180b = pbox(shco180b, CO180b, CA180ps, dfrq, compC*pwC, pwClvl);      
      ca180a = pbox(shca180a, CA180, CA180ps, dfrq, compC*pwC, pwClvl);                  
      co90a = pbox("Phard90", CO90a, CO180ps, dfrq, compC*pwC, pwClvl);  
    }    
    pwca180b = ca180reb.pw;
    d_ca180b = ca180reb.pwr;
    pwco180a = co180a.pw;
    d_co180a = co180a.pwr;
    pwco180b = co180b.pw;
    d_co180b = co180b.pwr;
    pwca180a = ca180a.pw;
    d_ca180a = ca180a.pwr;
    pwco90a = co90a.pw;
    d_co90a = co90a.pwr;
    pwca90b = co90a.pw;
    d_ca90b = co90a.pwr;
  }   


/* LOAD PHASE TABLE */

  settable(t7,4, phi7);
  settable(t6,8, phi6);
  settable(t9, 4, rec); 
  settable(t11,1,phi11);
  settable(t12,2,phi12);
  settable(t13,8,phi13);
  settable(t15,1,phi15);
  settable(t18,2,phi18);

/* CHECK VALIDITY OF PARAMETER RANGES */
    if (shared_CT[A] == 'n')
      if ( bigTN - 0.5*(ni2 - 1)/sw2 < 0.2e-6 )
        {
         text_error(" ni2 is too big\n");
         text_error(" please set ni2 smaller or equal to %d\n",
			(int) (bigTN*sw2*2.0) +1 );
         psg_abort(1);
        }

    if (fCT[A] == 'y')
      if (bigTC - 0.5*(ni-1)/sw1 
		- WFG_STOP_DELAY
                - POWER_DELAY - pwD - 4.0e-6
		- POWER_DELAY - DELAY_BLANK - PRG_START_DELAY
		- gt12 - 100.2e-6
		- POWER_DELAY - WFG_START_DELAY 
                - 4.0e-6 - pwco180b - WFG_STOP_DELAY 
		- POWER_DELAY - 4.0e-6 < 2.0e-6)
       {
        text_error(" ni is too big\n");
        text_error(" please set ni smaller or equal to %d\n",
      			(int) ((bigTC - WFG_STOP_DELAY
                		- POWER_DELAY - pwD - 4.0e-6
				- POWER_DELAY - DELAY_BLANK - PRG_START_DELAY
				- gt12 - 100.2e-6
				- POWER_DELAY - WFG_START_DELAY 
                		- 4.0e-6 - pwco180b - WFG_STOP_DELAY 
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
	|| gt13 > 15e-3 || gt14 > 15e-3)  
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

    if( pwDlvl > 59) {
       text_error("pwDlvl too high\n");
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

    if (d_ca90b > 51) {
	text_error("d_ca90b too high\n");
	psg_abort(1);
    }

    if (pwca180b > 1e-3) {
	text_error("pwca180b too long\n");
	psg_abort(1);
    }

    if (d_ca180b > 62) {
	text_error("d_ca180b too high\n");
	psg_abort(1);
    }

    if (pwco180b > 1e-3) {
	text_error("pwco180b too long\n");
	psg_abort(1);
    }

    if (d_co180b > 58) {
	text_error("d_co180b too high\n");
	psg_abort(1);
    }

    if (fCT[0] != 'y' && ni/sw1 > 0.1) {
	text_error("too long t1 with decoupling\n");
	psg_abort(1);
    }

    if(sel_flg[A] == 'y' && nietl_flg[A] == 'y') {
      text_error("both sel_flg and nietl_flg cannot be yes\n");
      psg_abort(1);
   }

    if (fCT[A] == 'n' && fc180[A] =='y' && ni > 1.0) {
       text_error("must set fc180='n' to allow Calfa evolution (ni>1)\n");
       psg_abort(1);
   }


/*  Phase incrementation for hypercomplex 2D data */

    if (phase == 2) tsadd(t7,1,4);

    if (shared_CT[A] == 'n') 
      {
       if (phase2 == 2) { tsadd(t15,2,4); icosel = 1; }
         else icosel = -1;
      }
     else 
      {
       if (phase2 == 2) { tsadd(t15,2,4); icosel = -1; }
         else icosel = 1;
      }

    if (nietl_flg[A] == 'y') icosel = -1*icosel;

/*  Set up f1180  tau1 = t1               */

    tau1 = d2;

    if(fCT[A] == 'y') { 
      if(f1180[A] == 'y') {
          tau1 += ( 1.0 / (2.0*sw1) );
          if(tau1 < 0.2e-6) tau1 = 2.0e-6;
      }
        tau1 = tau1/2.0;
    }

    else { 
       if( f1180[A] == 'y' && fc180[A] == 'n' ) 
           tau1 += (1.0/(2.0*sw1) - (4.0/PI)*pwca90b
                     - POWER_DELAY - 4.0e-6
                     - WFG_START_DELAY - pwco180b - WFG_STOP_DELAY
                     - POWER_DELAY - 4.0e-6); 
       
       if( f1180[A] == 'n' && fc180[A] == 'n' ) 
           tau1  = (tau1 - (4.0/PI)*pwca90b
                     - POWER_DELAY - 4.0e-6
                     - WFG_START_DELAY - pwco180b - WFG_STOP_DELAY
                     - POWER_DELAY - 4.0e-6); 

        if(tau1 < 0.4e-6 && fc180[A] == 'n') { 
            printf("tau1 is %f; adjusted to 0\n",tau1);
            printf("Use a delta/sq(3) pulse for C' 180\n");
            tau1 = 4.0e-7;
        }
        tau1 = tau1/2.0;
    }

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
      tsadd(t11,2,4);     
      tsadd(t9,2,4);    
    }

   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) {
      tsadd(t12,2,4);  
      tsadd(t9,2,4);    
    }

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tsatpwr);     /* Set transmitter power for 1H presaturation */
   decpower(d_co90a);     /* Set Dec1 power for C' 90 pulses       */
   dec2power(pwNlvl);     /* Set Dec2 power for 15N hard pulses         */

/* Presaturation Period */
   if (fsat[0] == 'y')
     {
      delay(2.0e-5);
      rgpulse(d1,zero,2.0e-6,2.0e-6);  /* Presaturation */
      obspower(tpwr);   /* Set transmitter power for hard 1H pulses */
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
   decoffset(dof+(174-56)*dfrq);

   lk_hold();
   delay(20.0e-6);

   decrgpulse(pwco90a,zero,4.0e-6,4.0e-6);

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */

   delay(0.2e-6);
   zgradpulse(gzlvl5,gt5);
   delay(2.0e-6);

   delay(taua - gt5 - 2.2e-6);   /* taua <= 1/4JNH */ 

   sim3pulse(2.0*pw,0.0e-6,2.0*pwN,zero,zero,zero,0.0,0.0);

   dec2phase(zero); decphase(zero); 

   delay(taua - gt5 - 200.2e-6); 

   delay(0.2e-6);
   zgradpulse(gzlvl5,gt5);
   delay(200.0e-6);

   if (sel_flg[A] == 'y') 
     {
      rgpulse(pw,one,4.0e-6,0.0);

      /* shaped pulse */
      obspower(tpwrsl);
      shaped_pulse(shp_sl,pw_sl,two,2.0e-6,0.0);
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

      dcplr2phase(zero); dec2phase(zero);

      delay(1.34e-3 - SAPS_DELAY);

      rgpulse(pw,zero,0.0,0.0);
      rgpulse(2.0*pw,one,2.0e-6,0.0);
      rgpulse(pw,zero,2.0e-6,0.0);

      delay(del1 - 1.34e-3 - 4.0*pw - 4.0e-6); 
     }
   else 
     {
      rgpulse(pw,three,4.0e-6,0.0);

      /* shaped pulse */
      obspower(tpwrsl);
      shaped_pulse(shp_sl,pw_sl,zero,2.0e-6,0.0);
      delay(2.0e-6);
      obspower(tpwr);
      /* shaped pulse */

      delay(0.2e-6);
      zgradpulse(gzlvl3,gt3);
      delay(200.0e-6);

      dec2rgpulse(pwN,zero,0.0,0.0);

      delay(del1); 
     }

   decphase(zero);
   dec2rgpulse(2*pwN,zero,0.0,0.0);

   dec2phase(one); 

   decpower(d_co180a);
   decshaped_pulse(shco180a,pwco180a,zero,4.0e-6,0.0);

   delay(del1 - POWER_DELAY - 4.0e-6 - WFG_START_DELAY
         - pwco180a - WFG_STOP_DELAY);

   dec2rgpulse(pwN,one,0.0,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl4,gt4);
   delay(200e-6);

   decphase(t18);
   decpower(d_co90a);
   decrgpulse(pwco90a,t18,4.0e-6,0.0);

   decphase(zero);
   decpower(d_ca180a);
   decshaped_pulse(shca180a,pwca180a,zero,4.0e-6,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl14,gt14);
   delay(200e-6);

   delay(zeta 
	- POWER_DELAY - WFG_START_DELAY 
        - 4.0e-6 - pwca180a - WFG_STOP_DELAY
	- gt14 - 202e-6
	- POWER_DELAY
	- WFG_START_DELAY - 4.0e-6);

   decphase(zero);
   decpower(d_co180a);
   decshaped_pulse(shco180a,pwco180a,zero,4.0e-6,0.0);

   decphase(zero);
   decpower(d_ca180a);
   decshaped_pulse(shca180a,pwca180a,zero,4.0e-6,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl14,gt14);
   delay(200e-6);

   delay(zeta 
	- WFG_STOP_DELAY
	- gt14 - 202e-6
	- POWER_DELAY - WFG_START_DELAY 
        - 4.0e-6 - pwca180a - WFG_STOP_DELAY
	- POWER_DELAY - 4.0e-6);

   decphase(one);
   decpower(d_co90a);
   decrgpulse(pwco90a,one,4.0e-6,0.0);

   decoffset(dof);

   delay(0.2e-6);
   zgradpulse(gzlvl10,gt10);
   delay(200.0e-6);

   dec2phase(zero); 

   if (fCT[A] == 'y') 
     {
      /* Turn on D decoupling using the third decoupler */
      dec3phase(one);
      dec3power(pwDlvl);
      dec3rgpulse(pwD,one,4.0e-6,0.0);
      dec3phase(zero);
      dec3power(dpwr3);
      dec3unblank();
      setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
      /* Turn on D decoupling */

      decphase(t7);
      decpower(d_ca90b);
      decrgpulse(pwca90b,t7,4.0e-6,0.0);
      
      delay(tau1);
      
      decphase(zero);
      decpower(d_co180b);
      decshaped_pulse(shco180b,pwco180b,zero,4.0e-6,0.0);
      
      dec2rgpulse(pwN,one,0.0,0.0);
      dec2rgpulse(2*pwN,zero,2.0e-6,0.0);
      dec2rgpulse(pwN,one,2.0e-6,0.0);
      	
      delay(bigTC 
      		- POWER_DELAY - WFG_START_DELAY 
      		- 4.0e-6 - pwco180b - WFG_STOP_DELAY
      		- 4.0*pwN - 4.0e-6
      		- PRG_STOP_DELAY - DELAY_BLANK - POWER_DELAY
      		- pwD - 4.0e-6
      		- gt12 - 100.2e-6
      		- POWER_DELAY - 4.0e-6 - WFG_START_DELAY);
      
      /* Turn off D decoupling */
      setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
      dec3blank();
      dec3phase(three);
      dec3power(pwDlvl);
      dec3rgpulse(pwD,three,4.0e-6,0.0);
      /* Turn off D decoupling */
      
      initval(1.0,v3);
      decstepsize(sphase); 
      dcplrphase(v3);
      
      delay(2.0e-7);
      zgradpulse(gzlvl12,gt12);
      delay(100.0e-6);
      
      decpower(d_ca180b);
      decshaped_pulse(shca180b,pwca180b,zero,4.0e-6,0.0);
      dcplrphase(zero);
      
      delay(2.0e-7);
      zgradpulse(gzlvl12,gt12);
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
      
      delay(bigTC - tau1
      		- WFG_STOP_DELAY
      		- gt12 - 100.2e-6
      		- POWER_DELAY - pwD - 4.0e-6
      		- POWER_DELAY - DELAY_BLANK - PRG_START_DELAY
      		- POWER_DELAY - 4.0e-6);
      decphase(t11);
      decpower(d_ca90b);
      decrgpulse(pwca90b,t11,4.0e-6,0.0);
      
      /* Turn off D decoupling */
      setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
      dec3blank(); 
      dec3phase(three);
      dec3power(pwDlvl);
      dec3rgpulse(pwD,three,4.0e-6,0.0);
      /* Turn off D decoupling */
     }  /* END   (fCT[A] == 'y')  */
  else 
     { 	/* BEGIN (fCT[A] == 'n') */

      /* Turn on D decoupling using the third decoupler */
      dec3phase(one);
      dec3power(pwDlvl);
      dec3rgpulse(pwD,one,4.0e-6,0.0);
      dec3phase(zero);
      dec3power(dpwr3);
      dec3unblank();
      setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
      /* Turn on D decoupling */

      if (fc180[A] == 'y') 
	{
	 decphase(t7);
	 decpower(d_ca90b);
    	 decrgpulse(pwca90b,t7,4.0e-6,0.0);

    	 delay(WFG_STOP_DELAY);

         initval(1.0,v3);
         decstepsize(sphase);
         dcplrphase(v3); 

 	 decpower(d_ca180b);
    	 decshaped_pulse(shca180b,pwca180b,zero,4.0e-6,0.0);

         dcplrphase(zero);

         delay(WFG_START_DELAY);

 	 decphase(t11);
 	 decpower(d_ca90b);
    	 decrgpulse(pwca90b,t11,4.0e-6,0.0);
   	}
      else if (fc180[A] == 'n' && (tau1 - pwN) >= 0.2e-6) 
        {
	 decphase(t7);
	 decpower(d_ca90b);
	 decrgpulse(pwca90b,t7,4.0e-6,0.0);

	 delay(tau1 - pwN);  

	 decphase(zero);
	 decpower(d_co180b);
         decshaped_pulse(shco180b,pwco180b,t6,4.0e-6,0.0);
         dec2rgpulse(2.0*pwN,zero,0.0,0.0);

	 delay(tau1 - pwN);  

	 decphase(t11);
	 decpower(d_ca90b);
	 decrgpulse(pwca90b,t11,4.0e-6,0.0);
	}
      else if (fc180[A] == 'n' && (tau1 - pwN) < 0.2e-6) 
        {
	 decphase(t7);
	 decpower(d_ca90b);
	 decrgpulse(pwca90b,t7,4.0e-6,0.0);

	 delay(tau1);  

	 decphase(zero);
	 decpower(d_co180b);
         decshaped_pulse(shco180b,pwco180b,t6,4.0e-6,0.0);

	 delay(tau1);  

	 decphase(t11);
	 decpower(d_ca90b);
 	 decrgpulse(pwca90b,t11,4.0e-6,0.0);
	}

      if (fc180[A] == 'n' && (tau1 - pwN) < 0.2e-6) 
        dec2rgpulse(2.0*pwN,zero,0.0,0.0);

      if (fc180[A] == 'n' && (tau1 - pwN) >= 0.2e-6) 
        delay(2.0*pwN);

      /* Turn off D decoupling */
      setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
      dec3blank();
      dec3phase(three);
      dec3power(pwDlvl);
      dec3rgpulse(pwD,three,4.0e-6,0.0);
      /* Turn off D decoupling */
     }  /* END (fCT[A] == 'n') */

   decoffset(dof+(174-56)*dfrq);

   delay(2.0e-6);
   zgradpulse(gzlvl11,gt11);
   delay(200e-6);

   decphase(one);
   decpower(d_co90a);
   decrgpulse(pwco90a,one,4.0e-6,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl13,gt13);
   delay(200e-6);

   delay(zeta 
	- gt13 - 202e-6
	- POWER_DELAY - WFG_START_DELAY 
        - 4.0e-6 - pwca180a - WFG_STOP_DELAY
	- POWER_DELAY
	- WFG_START_DELAY - 4.0e-6);

   decphase(zero);
   decpower(d_ca180a);
   decshaped_pulse(shca180a,pwca180a,zero,4.0e-6,0.0);

   decphase(zero);
   decpower(d_co180a);
   decshaped_pulse(shco180a,pwco180a,zero,4.0e-6,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl13,gt13);
   delay(200e-6);

   delay(zeta 
	- WFG_STOP_DELAY
	- gt13 - 202e-6
	- POWER_DELAY - WFG_START_DELAY 
        - 4.0e-6 - pwca180a - WFG_STOP_DELAY
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
     dec2rgpulse(pwN,t12,2.0e-6,0.0);

     dec2phase(t13); decphase(zero);

     delay(bigTN - tau2); 

     dec2rgpulse(2*pwN,t13,0.0,0.0);

     decpower(d_co180a);
     decshaped_pulse(shco180a,pwco180a,zero,4.0e-6,0.0);
 
     delay(bigTN - POWER_DELAY - 4.0e-6 - WFG_START_DELAY
          - pwco180a - WFG_STOP_DELAY - 500.2e-6 - 2.0*GRADIENT_DELAY
          - gt1 - POWER_DELAY - 4.0e-6 - WFG_START_DELAY
          - pwca180a - WFG_STOP_DELAY);

     dec2phase(t15);

     delay(0.2e-6);
     zgradpulse(gzlvl1,gt1);
     delay(500.0e-6);

     decpower(d_ca180a);
     decshaped_pulse(shca180a,pwca180a,zero,4.0e-6,0.0);
  
     delay(tau2);
   
     sim3pulse(pw,0.0e-6,pwN,zero,zero,t15,0.0,0.0);
    }
   else if (shared_CT[A] == 'y') 
    {
     if (bigTN - tau2 > 0.2e-6)  
       {
        dec2rgpulse(pwN,t12,2.0e-6,0.0);
        dec2phase(t13); decphase(zero);

        delay(tau2);

        decpower(d_ca180a);
        decshaped_pulse(shca180a,pwca180a,zero,4.0e-6,0.0);

        delay(0.2e-6);
        zgradpulse(gzlvl1,gt1);
        delay(500.0e-6);

        delay(bigTN - POWER_DELAY - 4.0e-6 - WFG_START_DELAY
              - pwca180a - WFG_STOP_DELAY - 500.2e-6 - 2.0*GRADIENT_DELAY
              - gt1 - POWER_DELAY - 4.0e-6 - WFG_START_DELAY
              - pwco180a - WFG_STOP_DELAY);

        decpower(d_co180a);
        decshaped_pulse(shco180a,pwco180a,zero,4.0e-6,0.0);

        dec2rgpulse(2*pwN,t13,0.0,0.0);

        delay(bigTN - tau2);

        sim3pulse(pw,0.0e-6,pwN,zero,zero,t15,0.0,0.0);
       }
      else	/* bigTN - tau2 <= 0.2e-6      */
       {
        dec2rgpulse(pwN,t12,2.0e-6,0.0);
        dec2phase(t13); decphase(zero);

        delay(tau2);

        decpower(d_ca180a);
        decshaped_pulse(shca180a,pwca180a,zero,4.0e-6,0.0);

        delay(0.2e-6);
        zgradpulse(gzlvl1,gt1);
        delay(500.0e-6);

        delay(bigTN - POWER_DELAY - 4.0e-6 - WFG_START_DELAY
              - pwca180a - WFG_STOP_DELAY - 500.2e-6 - 2.0*GRADIENT_DELAY
              - gt1 - POWER_DELAY - 4.0e-6 - WFG_START_DELAY
              - pwco180a - WFG_STOP_DELAY);

        decpower(d_co180a);
        decshaped_pulse(shco180a,pwco180a,zero,4.0e-6,0.0);

        delay(tau2 - bigTN);

        dec2rgpulse(2*pwN,t13,0.0,0.0);
        sim3pulse(pw,0.0e-6,pwN,zero,zero,t15,0.0,0.0);

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
     delay(tauf - POWER_DELAY - 4.0e-6 
                - pwco90a - gt6 - 2.2e-6);

     sim3pulse(2.0*pw,0.0e-6,2.0*pwN,zero,zero,zero,0.0,0.0);

     txphase(one);
     dec2phase(one);

     delay(tauf - gt6 - 200.2e-6);
  
     delay(0.2e-6);
     zgradpulse(gzlvl6,gt6);
     delay(200.0e-6);

     sim3pulse(pw,0.0e-6,pwN,one,zero,one,0.0,0.0);
   
     delay(0.2e-6);
     zgradpulse(gzlvl7,gt7);
     delay(2.0e-6);
 
     txphase(zero);
     dec2phase(zero);
     delay(tauf - gt7 - 2.2e-6);

     sim3pulse(2.0*pw,0.0e-6,2.0*pwN,zero,zero,zero,0.0,0.0);

     delay(tauf - gt7 - 200.2e-6);

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
     delay(tauf - POWER_DELAY - 4.0e-6 
                - pwco90a 
                - POWER_DELAY - 2.0e-6 - WFG_START_DELAY - pw_sl1 - WFG_STOP_DELAY
                - 2.0e-6 - POWER_DELAY
                - gt6 - 2.2e-6);

     sim3pulse(2.0*pw,0.0e-6,2.0*pwN,zero,zero,zero,0.0,0.0);

     txphase(one);
     dec2phase(zero);

     delay(tauf - gt6 - 200.2e-6);

     delay(0.2e-6);
     zgradpulse(gzlvl6,gt6);
     delay(200.0e-6);

     sim3pulse(pw,0.0e-6,pwN,one,zero,zero,0.0,0.0);
   
     delay(0.2e-6);
     zgradpulse(gzlvl7,gt7);
     delay(2.0e-6);
 
     txphase(zero);
     dec2phase(zero);
     delay(tauf - gt7 - 2.2e-6);

     sim3pulse(2.0*pw,0.0e-6,2.0*pwN,zero,zero,zero,0.0,0.0);
     txphase(one); dec2phase(one);
  
     delay(tauf - gt7 - 200.2e-6);
  
     delay(0.2e-6);
     zgradpulse(gzlvl7,gt7);
     delay(200.0e-6);

     sim3pulse(pw,0.0e-6,pwN,one,zero,one,0.0,0.0);
    }

   txphase(zero);
   dec2phase(zero);

   delay(gt2 +gstab -0.5*(pwN -pw) -2.0/PI*pw);

   rgpulse(2*pw,zero,0.0,0.0);

   delay(2.0e-6);
   zgradpulse(icosel*gzlvl2,gt2);
   decpower(dpwr);
   dec2power(dpwr2);  /* set power for 15N decoupling */
   delay(gstab -2.0e-6 -2.0*GRADIENT_DELAY -2.0*POWER_DELAY);

   lk_sample();
/* BEGIN ACQUISITION */
status(C);
    setreceiver(t9);

}
