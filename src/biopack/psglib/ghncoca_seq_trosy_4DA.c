/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* ghncoca_seq_trosy_4DA.c - auto-calibrated version of the original sequence

    This pulse sequence will allow one to perform the following experiment:

    4D hncoca with trosy
                       F1      CO(i-1)
                       F2      CA(i)
                       F3      15N + JNH/2
                       F4(acq) 1H (NH) - JNH/2

    This sequence uses the standard three channel configuration
         1)  1H             - carrier (tof) @ 4.7 ppm [H2O]
         2) 13C             - carrier (dof) @ 174 ppm [CO] 
         3) 15N             - carrier (dof2)@ 119 ppm [centre of amide 15N]  
    
    Set dm = 'nnn', dmm = 'ccc' 
    Set dm2 = 'nnn', dmm2 = 'ccc'

    Must set phase = 1,2 , phase2 = 1,2, and phase3 = 1,2 
    for States-TPPI acquisition in t1[Co], t2[Ca], and t3[N].
    [The fids must be manipulated (add/subtract) with 
    'grad_sort_nd' program (or equivalent) prior to regular processing.]
    
    Flags
        fsat            'y' for presaturation of H2O
        fscuba          'y' for apply scuba pulse after presaturation of H2O
        f1180           'y' for 180 deg linear phase correction in F1
                            otherwise 0 deg linear phase correction
        f2180           'y' for 180 deg linear phase coreection in F2
                            otherwise 0 deg
        sel_flg         'y' for active suppression of the anti-TROSY component
        sel_flg         'n' for relaxation suppression of the anti-TROSY component
        fCT		'y' for CT on Ca dimension 

	Standard Settings
        fsat='n',fscuba='n',f1180='y',f2180='y',f3180='n'

    
    Set f2180 to y for (-90,180) in Ca and f3180 to n for (0,0) in N
    Set the carbon carrier on the C' and use the waveform to pulse the
        c-alpha

    Written By Daiwen Yang on September 16, 1998.
    Modified by L. E. Kay on Sept8, 1999 to include sel_flg
    Modified for Autocalibrate with Pbox, E.Kupce, Jan 2005
     based on Kay and Yang's hncoca_4d_trosy_ydw.c
    Modified for BioPack, G.Gray Feb 2005

*/

#include <standard.h>
#include "Pbox_bio.h"
    
#define CA180reb "reburp 22p -2p"                /* RE-BURP 180 on Ca at 54 ppm, 2 ppm away */
#define CA180ps  "-s 1.0 -attn i"                           /* RE-BURP 180 shape parameters */
#define CO180    "square180n 118p 118p"          /* hard 180 on C', at 174 ppm 118 ppm away */
#define CA180    "square180n 118p -118p"         /* hard 180 on CA, at 56 ppm 118 ppm away  */
#define C90      "square90n 118p"                            /* hard  90 on C, on resonance */
#define C180     "square180n 118p"                           /* hard  90 on C, on resonance */
#define N180     "square180"                                           /* hard 180 on N-15  */
#define CO180ps  "-s 0.2 -attn d"                              /* hard 180 shape parameters */

static shape c90, ca180, ca180reb, co180, c180;

static int  phi1[1]  = {0},
            phi2[1]  = {1},
            phi3[4]  = {0,0,2,2},
            phi4[1]  = {0},
            phi5[1]  = {0},
            phi7[4]  = {0,0,2,2},
            phi8[4]  = {0,1,2,3},
            rec[4]   = {0,2,2,0};

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
            fc180[MAXSTR],    /* Flag for checking sequence              */
            spca180[MAXSTR],  /* string for the waveform Ca 180 */
            spco180[MAXSTR],  /* string for the waveform Co 180 */
            spcareb[MAXSTR],  /* string for the waveform reburp 180 */
            ddseq[MAXSTR],    /* 2H decoupling seqfile */
            fCT[MAXSTR],       /* Flag for constant time Ca evolution */
            shp_sl[MAXSTR],   /* string for seduce shape */
            sel_flg[MAXSTR];

 int         phase, phase2, phase3, ni2, ni3, icosel,
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter,   /* used for states tppi in t2           */ 
             t3_counter;   /* used for states tppi in t3           */ 

 double      tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             tau3,         /*  t2 delay */
             taua,         /*  ~ 1/4JNH =  2.25 ms */
             taub,         /*  ~ 1/4JNH =  2.25 ms */
             tauc,         /*  ~ 1/4JCaC' =  4 ms */
             taud,         /*  ~ 1/4JCaC' =  4.5 ms if bigTCo can be set to be
				less than 4.5ms and then taud can be smaller*/
             zeta,        /* time for C'-N to refocuss set to 0.5*24.0 ms */
             bigTCa,      /* Ca T period */
             bigTCo,      /* Co T period */
             bigTN,       /* nitrogen T period */
             pwc90,       /* PW90 for co nucleus @ d_c90         */
             pwc180on,     /* PW180 at @ d_c180         */
             pwcareb,     /* PW90 for co nucleus @ d_creb         */
             pwc180off,     /* PW180 at d_c180 + pad              */
             tsatpwr,     /* low level 1H trans.power for presat  */
             d_c90,       /* power level for 13C pulses(pwc90 = sqrt(15)/4delta)
                             delta is the separation between Ca and Co  */
             d_c180,      /* power level for 180 13C pulses
				(pwc180on=sqrt(3)/2delta   */
             sw1,          /* sweep width in f1                    */             
             sw2,          /* sweep width in f2                    */             
             sw3,          /* sweep width in f3                    */             
             pw_sl,        /* pw90 for H selective pulse on water ~ 2ms */
             phase_sl,     /* phase for pw_sl */
             tpwrsl,       /* power level for square pw_sl       */
	     d_creb,

	     pwDlvl,	   /* power for D flank pulse */
	     pwD,	   /* pw90 at pwDlvl  */

	     sphase,       /* small angle phase shift */
	     sphase1,
	     sphase2,      /* used only for constant t2 period */

             compC,       /* C-13 RF calibration parameters */
             pwC,
             pwClvl,

             pwN,         /* PW90 for 15N pulse              */
             pwNlvl,       /* high dec2 pwr for 15N hard pulses    */

             gstab,       /* delay to compensate for gradient gt5 */

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
             gzlvl11; 

/* LOAD VARIABLES */


  getstr("autocal",autocal);
  getstr("fsat",fsat);
  getstr("fc180",fc180);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("f3180",f3180);
  getstr("fscuba",fscuba);
  getstr("ddseq",ddseq);
  getstr("fCT",fCT);
  getstr("shp_sl",shp_sl);

  getstr("sel_flg",sel_flg);

  taua   = getval("taua"); 
  taub   = getval("taub"); 
  tauc   = getval("tauc"); 
  taud   = getval("taud"); 
  zeta  = getval("zeta");
  bigTCa = getval("bigTCa");
  bigTCo = getval("bigTCo");
  bigTN = getval("bigTN");
  tpwr = getval("tpwr");
  tsatpwr = getval("tsatpwr");
  dpwr = getval("dpwr");
  pwN = getval("pwN");
  pwNlvl = getval("pwNlvl");
  pwD = getval("pwD");
  pwDlvl = getval("pwDlvl");
  phase = (int) ( getval("phase") + 0.5);
  phase2 = (int) ( getval("phase2") + 0.5);
  phase3 = (int) ( getval("phase3") + 0.5);
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  sw3 = getval("sw3");
  ni2 = getval("ni2");
  ni3 = getval("ni3");
  pw_sl = getval("pw_sl");
  phase_sl = getval("phase_sl");
  sphase = getval("sphase");
  sphase1 = getval("sphase1");
  sphase2 = getval("sphase2");
  tpwrsl = getval("tpwrsl");

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

  if(autocal[0]=='n')
  {     
    getstr("spca180",spca180);
    getstr("spco180",spco180);
    getstr("spcareb",spcareb);  
    pwc180off = getval("pwc180off");
    pwc90 = getval("pwc90");
    pwc180on = getval("pwc180on");
    pwcareb = getval("pwcareb");
    d_c90 = getval("d_c90");
    d_c180 = getval("d_c180");
    d_creb = getval("d_creb");
  }
  else
  {        
    strcpy(spca180,"Phard_-118p");    
    strcpy(spco180,"Phard_118p");    
    strcpy(spcareb,"PrebCa_on");    
    if (FIRST_FID)
    {
      compC = getval("compC");
      pwC = getval("pwC");
      pwClvl = getval("pwClvl");
      ca180reb = pbox(spcareb, CA180reb, CA180ps, dfrq, compC*pwC, pwClvl);                  
      c180 = pbox("Phard180", C180, CO180ps, dfrq, compC*pwC, pwClvl);            
      co180 = pbox(spco180, CO180, CO180ps, dfrq, compC*pwC, pwClvl);      
      ca180 = pbox(spca180, CA180, CO180ps, dfrq, compC*pwC, pwClvl);                  
      c90 = pbox("Phard90", C90, CO180ps, dfrq, compC*pwC, pwClvl);  
    }    
    pwc180off = co180.pw;
    pwc90 = c90.pw;
    pwc180on = c180.pw;
    pwcareb = ca180reb.pw;
    d_c90 = c90.pwr;
    d_c180 = c180.pwr;
    d_creb = ca180reb.pwr;
  }   

/* LOAD PHASE TABLE */

  settable(t1,1,phi1);
  settable(t2,1,phi2);
  settable(t3,4,phi3);
  settable(t4,1,phi4);
  settable(t5,1,phi5);
  settable(t7,4,phi7);
  settable(t8,4,phi8);
  settable(t6,4,rec);

/* CHECK VALIDITY OF PARAMETER RANGES */


    if( bigTN - (ni3-1)*0.5/sw3 + pwc180on < 0.2e-6 )
    {
        text_error(" ni3 is too big\n");
        text_error(" please make ni3 equal or smaller than %d \n", 
			(int) (((bigTN +pwc180on)*sw3/0.5)+1.0) );
        psg_abort(1);
    }

  if (fCT[A]=='y')
  {

   if(bigTCa - 0.5*(ni2-1)/sw2 - WFG_STOP_DELAY 
	- POWER_DELAY - gt11 - 50.2e-6 < 0.2e-6)
    {
        text_error(" ni2 is too big\n");
        text_error(" please make ni2 equal or smaller than %d \n", 
			(int) (((bigTCa -WFG_STOP_DELAY -POWER_DELAY -gt11 -50.2e-6)*sw2/0.5)+1.0) );
        psg_abort(1);
    }
  }
  else
   {
     if ((ni2-1)/sw2>12.0e-3)
    {
        text_error(" ni2 is too big\n");
        text_error(" please make ni2 equal or smaller than %d \n", 
			(int) ((6.0e-3*sw2) +1.0) );
        psg_abort(1);
    }
   }

   if (bigTCo - 0.5*(ni-1)/sw1 - 4.0e-6 - POWER_DELAY < 0.2e-6)
     {
        text_error(" ni is too big\n");
        text_error(" please make ni equal or smaller than %d \n", 
			(int) (((bigTCo -4.0e-6 -POWER_DELAY)*sw1/0.5)+1.0) );
        psg_abort(1);
     }

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' || dm[D] == 'y' ))
    {
        text_error("incorrect dec1 decoupler flags!  ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' || dm2[D] == 'y'))
    {
        text_error("incorrect dec2 decoupler flags! Should be 'nnnn' ");
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

    if( dpwr2 > 46 )
    {
        text_error("don't fry the probe, DPWR2 too large!  ");
        psg_abort(1);
    }

    if( dpwr3 > 50 )
    {
        text_error("don't fry the probe, dpwr3 too large!  ");
        psg_abort(1);
    }

    if( d_c90 > 62 )
    {
        text_error("don't fry the probe, DHPWR too large!  ");
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
    if( pwc90 > 200.0e-6 )
    {
        text_error("dont fry the probe, pwc90 too high ! ");
        psg_abort(1);
    } 
    if( pwc180off > 200.0e-6 )
    {
        text_error("dont fry the probe, pwc180 too high ! ");
        psg_abort(1);
    } 

    if( gt3 > 2.5e-3 ) 
    {
        text_error("gt3 is too long\n");
        psg_abort(1);
    }
    if( gt1 > 10.0e-3 || gt2 > 10.0e-3 || gt4 > 10.0e-3 || gt5 > 10.0e-3
        || gt6 > 10.0e-3 || gt7 > 10.0e-3 || gt8 > 10.0e-3
	|| gt9 > 10.0e-3 || gt10 > 10.0e-3 || gt11 > 200.0e-6)
    {
        text_error("gt values are too long. Must be < 10.0e-3 or gt11=200us\n");
        psg_abort(1);
    } 

    if (fCT[A] == 'n' && fc180[A] =='y' && ni2 > 1.0) {
       text_error("must set fc180='n' to allow Calfa evolution (ni2>1)\n");
       psg_abort(1);
   }


/*  Phase incrementation for hypercomplex 2D data */

    if (phase == 2) tsadd(t1,1,4);

    if (phase2 == 2) tsadd(t5,1,4);

    if (phase3 == 2) { tsadd(t4, 2, 4); icosel = 1; }
      else icosel = -1;

/*  Set up f1180  tau1 = t1               */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni>1)){
        tau1 += (1.0/(2.0*sw1));
        if(tau1 < 0.2e-6) tau1 = 0.4e-6;
    }
        tau1 = tau1/2.0;

/*  Set up f2180  tau2 = t2               */

    tau2 = d3;
    if ((f2180[A] == 'y') && (ni2>1)) 
      {
       if (fCT[A]=='y')
         {
          tau2 += ( 1.0 / (2.0*sw2) ); 
	  if(tau2 < 0.2e-6) tau2 = 0.4e-6;
         }
       else
         { 
	  if (pwc180off > 2.0*pwN)
	    {
             tau2 += ( 1.0 / (2.0*sw2) - 4.0*pwc90/PI - 4.0e-6
		    - 2.0*POWER_DELAY
		    - WFG3_START_DELAY - pwc180off - WFG3_STOP_DELAY);
	    }
	  else
	   {
            tau2 += ( 1.0 / (2.0*sw2) - 4.0*pwc90/PI - 4.0e-6
                   - 2.0*POWER_DELAY
                   - WFG3_START_DELAY - 2.0*pwN - WFG3_STOP_DELAY);
	   }
          if(tau2 < 0.2e-6) tau2 = 0.4e-6; 
         }
      }
    tau2 = tau2/2.0;



/*  Set up f3180  tau3 = t3               */
 
    tau3 = d4;
    if((f3180[A] == 'y') && (ni3>1)){
        tau3 += ( 1.0 / (2.0*sw3) );
        if(tau3 < 0.2e-6) tau3 = 0.4e-6;
    }
        tau3 = tau3/2.0;

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
      tsadd(t5,2,4);
      tsadd(t6,2,4);
    }

   if( ix == 1) d4_init = d4 ;
   t3_counter = (int) ( (d4-d4_init)*sw3 + 0.5 );
   if(t3_counter % 2) {
      tsadd(t2,2,4);  
      tsadd(t6,2,4);    
    }

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obsoffset(tof);
   decoffset(dof);		/* set Dec1 carrier at Co		      */
   obspower(tsatpwr);      /* Set transmitter power for 1H presaturation */
   decpower(d_c180);       /* Set Dec1 power for hard 13C pulses         */
   dec2power(pwNlvl);      /* Set Dec2 power for 15N hard pulses         */

/* Presaturation Period */

   if (fsat[0] == 'y')
   {
	delay(2.0e-5);
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

   sim3pulse(2*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);

   txphase(three); dec2phase(zero); decphase(zero); 

   delay(0.2e-6);
   zgradpulse(gzlvl5,gt5);
   delay(200.0e-6);

   delay(taua - gt5 - 200.2e-6 - 2.0e-6); 

   if (sel_flg[A] == 'n') 
     {
      rgpulse(pw,three,2.0e-6,0.0);

      delay(0.2e-6);
      zgradpulse(gzlvl3,gt3);
      delay(200.0e-6);

      dec2rgpulse(pwN,zero,0.0,0.0);

      delay( zeta + pwc180on );
  
      dec2rgpulse(2*pwN,zero,0.0,0.0);
      decrgpulse(pwc180on,zero,0.0,0.0);

      delay(zeta - 2.0e-6);

      dec2rgpulse(pwN,one,2.0e-6,0.0);
     }
    else 
     {
      rgpulse(pw,one,2.0e-6,0.0);

      initval(1.0,v6);
      dec2stepsize(45.0);
      dcplr2phase(v6);

      delay(0.2e-6);
      zgradpulse(gzlvl3,gt3);
      delay(200.0e-6);

      dec2rgpulse(pwN,zero,0.0,0.0);
      dcplr2phase(zero);

      delay(1.34e-3 - SAPS_DELAY - 2.0*pw);

      rgpulse(pw,one,0.0,0.0);
      rgpulse(2.0*pw,zero,0.0,0.0);
      rgpulse(pw,one,0.0,0.0);

      delay( zeta - 1.34e-3 - 2.0*pw + pwc180on );
  
      dec2rgpulse(2*pwN,zero,0.0,0.0);
      decrgpulse(pwc180on,zero,0.0,0.0);

      delay(zeta - 2.0e-6);

      dec2rgpulse(pwN,one,2.0e-6,0.0);
     }

   dec2phase(zero); decphase(t1);
   decpower(d_c90);

   delay(0.2e-6);
   zgradpulse(gzlvl8,gt8);
   delay(200.0e-6);

/* Transfer Coy to CoxCaz and CT period for t1 */
   decrgpulse(pwc90,t1,2.0e-6,0.0);

   delay(tau1);
   dec2rgpulse(pwN,one,0.0,0.0);
   dec2rgpulse(2*pwN,zero,0.0,0.0);
   dec2rgpulse(pwN,one,0.0,0.0);

   decpower(d_c180);
   delay(taud -4.0*pwN -POWER_DELAY -0.5*(WFG_START_DELAY +pwc180off +WFG_STOP_DELAY));

   decshaped_pulse(spca180,pwc180off,zero,0.0,0.0);

   decphase(t8);
   initval(1.0,v4); decstepsize(sphase); dcplrphase(v4);

   delay(bigTCo -taud -0.5*(WFG_START_DELAY +pwc180off +WFG_STOP_DELAY) );

   decrgpulse(pwc180on,t8,0.0,0.0);

   decphase(one); dcplrphase(zero); 
   decpower(d_c90);

   delay(bigTCo - tau1 - POWER_DELAY - 4.0e-6);


   decrgpulse(pwc90,one,4.0e-6,0.0);

   decoffset(dof-(174-56)*dfrq);   /* change Dec1 carrier to Ca (56 ppm) */
   delay(0.2e-6);
   zgradpulse(gzlvl9,gt9);
   delay(150.0e-6);

/*  t2 period  */

 
   /* Turn on D decoupling using the third decoupler */
   dec3phase(one);
   dec3power(pwDlvl);
   dec3rgpulse(pwD,one,4.0e-6,0.0);
   dec3phase(zero);
   dec3power(dpwr3);
   dec3unblank();
   setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
   /* Turn on D decoupling */

   decrgpulse(pwc90,t5,2.0e-6,0.0);


   if (fCT[A]=='y')		/* Constant t2 */
     {
      decpower(d_c180);
      delay(tau2);
      dec2rgpulse(pwN,one,0.0,0.0);
      dec2rgpulse(2*pwN,zero,0.0,0.0);
      dec2rgpulse(pwN,one,0.0,0.0);
      decshaped_pulse(spco180,pwc180off,zero,0.0,0.0);
      decpower(d_creb); 
      decphase(t7);
      initval(1.0,v3);
      decstepsize(sphase2);
      dcplrphase(v3);

      delay(bigTCa - 4.0*pwN - WFG_START_DELAY - pwc180off
	 	- WFG_STOP_DELAY - POWER_DELAY - WFG_START_DELAY - gt11 - 50.2e-6); 

      delay(0.2e-6);
      zgradpulse(gzlvl11,gt11);
      delay(50.0e-6);

      decshaped_pulse(spcareb,pwcareb,zero,0.0,0.0);
      dcplrphase(zero);

      decpower(d_c90); decphase(t7); 
      delay(0.2e-6); 
      zgradpulse(gzlvl11,gt11);
      delay(50.0e-6);   

      delay(bigTCa - tau2 - WFG_STOP_DELAY - POWER_DELAY - gt11 - 50.2e-6); 
      decrgpulse(pwc90,t7,0.0,0.0);
     }
   else				 /* non_constant t2 */
     {
      if (fc180[A]=='n')
        {
         decphase(zero); dec2phase(zero);
         decpower(d_c180); 
         delay(tau2);
         sim3shaped_pulse("",spco180,"",0.0,pwc180off,2.0*pwN,zero,zero,zero,0.0,0.0);
         decpower(d_c90); 
         decphase(t7);
         delay(tau2);
        }
      else			/* for checking sequence */
        {
         decpower(d_c180);
         decrgpulse(pwc180on,zero,4.0e-6,0.0);
         decpower(d_c90);
        }
      decrgpulse(pwc90,t7,4.0e-6,0.0);
     }
 
   /* Turn off D decoupling */
   setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
   dec3blank();
   dec3phase(three);
   dec3power(pwDlvl);
   dec3rgpulse(pwD,three,4.0e-6,0.0);
   /* Turn off D decoupling */

   decoffset(dof);   /* set carrier back to Co */

   delay(0.2e-6);
   zgradpulse(gzlvl10,gt10);
   delay(150.0e-6);


/* refocusing CoyCaz to Cox */
   decrgpulse(pwc90,zero,2.0e-6,0.0);
   decpower(d_c180);
   delay(tauc - POWER_DELAY - WFG_START_DELAY - pwc180off - WFG_STOP_DELAY);

   decshaped_pulse(spca180,pwc180off,zero,0.0,0.0);

   initval(1.0,v5);
   decstepsize(sphase1);
   dcplrphase(v5);
   decrgpulse(pwc180on,zero,0.0e-6,0.0);
   dcplrphase(zero);
 
   delay(tauc - WFG_START_DELAY - pwc180off - WFG_STOP_DELAY
          - POWER_DELAY - 4.0e-6);
   decshaped_pulse(spca180,pwc180off,zero,0.0,0.0);  /* BS */
   decpower(d_c90);
 
   decrgpulse(pwc90,one,4.0e-6,0.0);
 
   decpower(d_c180);
   delay(0.2e-6);
   zgradpulse(gzlvl4,gt4);
   delay(200.0e-6);


/* t3 period */
   dec2rgpulse(pwN,t2,2.0e-6,0.0);

   dec2phase(t3);

   delay(bigTN - tau3 + pwc180on);

   dec2rgpulse(2*pwN,t3,0.0,0.0);
   decrgpulse(pwc180on,zero,0.0,0.0);

   txphase(zero);
   dec2phase(t4);

   delay(0.2e-6);
   zgradpulse(gzlvl1,gt1);
   delay(500.0e-6);

   delay(bigTN - gt1 - 500.2e-6 - 2.0*GRADIENT_DELAY
	 - 4.0e-6 - WFG_START_DELAY - pwc180off - WFG_STOP_DELAY);

   decshaped_pulse(spca180,pwc180off,zero,4.0e-6,0.0);

   delay(tau3);

   sim3pulse(pw,0.0e-6,pwN,zero,zero,t4,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl6,gt6);
   delay(2.0e-6);

   dec2phase(zero);
   delay(taub - gt6 - 2.2e-6);

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

   delay(gt2 +gstab -0.5*(pwN -pw) -2.0*pw/PI);

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
