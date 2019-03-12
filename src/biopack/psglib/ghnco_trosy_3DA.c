/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghnco_trosy_3DA.c - auto-calibrated version

    3D HNCO with trosy(enhanced sensitivity PFG)
    with selective pulses to minimize excitation of water
                       F1      CO 
                       F2      15N + JNH/2
                       F3(acq) 1H (NH) - JNH/2


    Pulse scheme uses TROSY principle in 15N and NH with transfer of
    magnetization achieved using the enhanced sensitivity pfg approach,

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

    See Figure 3 in Yang and Kay, J. Biomol NMR, Jan 1999, 13, 3-9. 

    This sequence uses the standard three channel configuration
         1)  1H             - carrier (tof) @ 4.7 ppm [H2O]
         2) 13C             - carrier (dof) @ 174 ppm [CO]
         3) 15N             - carrier (dof2)@ 120 ppm [centre of amide 15N]  
    
    Set dm = 'nnn', dmm = 'ccc' 
    Set dm2 = 'nnn', dmm2 = 'ccc'

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in t1
    [CO] and t2 [N].
    For non-VNMR software: The fids must be manipulated (add/subtract) with 
    'grad_sort_nd' program (or equivalent) prior to regular processing.
    
    Flags
        fsat            'y' for presaturation of H2O
        fscuba          'y' for apply scuba pulse after presaturation of H2O
        f1180           'y' for 180 deg linear phase correction in F1
                            otherwise 0 deg linear phase correction
        f2180           'y' for 180 deg linear phase coreection in F2
                            otherwise 0 deg
        fc180           'y' for C180 at t2/2 when checking 15N/NH 2D

        sel_flg         'y' for small proteins or fields less than 750
                        'n' for high fields and/or larger proteins 
        shared_flg      'y' allows acquisition in F2 for longer than 2bigTN
                            using the shared constant time principle
        nietl_flg       'y' active suppression of anti-trosy with no loss in s/n
                        (D.Nietlispach,J.Biomol.NMR, 31,161(2005))

	Standard Settings
        fsat='n',fscuba='n',f1180='y',f2180='n',fc180='n'

    
    Set f1180 to y for (-90,180) in C' and f2180 to n for (0,0) in N
    Set the carbon carrier on the C' and use the waveform to pulse the
        c-alpha

    Written by Daiwen Yang on June 1998 and modified by L.E.Kay on Oct. 28, 1998

    References: Pervushin et al. PNAS, 94, 12366-12371 (1997)
                Pervushin et al. J. Biomol. NMR Aug 1998
                Yang and Kay, J. Biomol. NMR, Jan 1999, 13, 3-9.
                Yang and Kay, J. Am. Chem. Soc., 1999, In Press.
    Modified from hnco_tydw.c

    Modified by L.E.Kay on March 3, 1999 to insert C' 90 purge after 15N
    evolution to ensure in case where the N-C' refocussing is not complete
    (2*bigTN < 1/2JNC') that the remaining NxCz' magnetization which
    becomes NHxCz' is not present. This will remove a dispersive contribution
    to the F3 lineshape

  Modified by L.E.Kay on January 30, 2002 to improve the shared constant time

  Modified by L.E.Kay to include the Nietlispach trick
    so that the anti-trosy component is eliminated without setting sel_flg to y

   Usage: nietl_flg == y 

  Calibrate pw_sl, tpwrsl unless sel_flg == y
  pw_sl1, tpwrsl1 used only with nietl_flg == y

  Modified by E.Kupce, Jan 2005 from  hnco_trosy_dwlek_500.c for auto-calibrated
  version of the original sequence

  Modified for BioPack by G.Gray, Feb 2005
*/

#include <standard.h>
#include "Pbox_bio.h"

#define CA180     "square180n 118p -118p"       /* hard 180 on CA, at 58 ppm 118 ppm away  */
#define CO90      "square90n 118p"               /* hard 90 on C', at 176 ppm on resonance */
#define CA180ps   "-s 0.2 -attn d"                                     /* shape parameters */

static shape   ca180, co90;

static int  phi1[4]  = {0,0,2,2},
            phi2[2]  = {1,3},
            phi3[8]  = {0,0,0,0,2,2,2,2},
            phi4[1]  = {0},
            rec[4]   =  {0,2,2,0};

static double d2_init=0.0, d3_init=0.0;
            
void pulsesequence()
{
/* DECLARE VARIABLES */

 char       autocal[MAXSTR],  /* auto-calibration flag */
            fsat[MAXSTR],
	    fscuba[MAXSTR],
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell             */
            spca180[MAXSTR],  /* string for the waveform 180 */
            fc180[MAXSTR], 
            shp_sl[MAXSTR],   /* string for shape of water pulse */
            sel_flg[MAXSTR],
            shared_CT[MAXSTR],
            nietl_flg[MAXSTR];

 int         phase, phase2, ni2, icosel, /* icosel changes sign with gds  */ 
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter;   /* used for states tppi in t2           */ 

 double      tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             taua,         /*  ~ 1/4JNH =  2.25 ms */
             taub,         /*  ~ 1/4JNH =  2.25 ms */
             zeta,         /* time for C'-N to refocus set to 0.5*24.0 ms */
             bigTN,        /* nitrogen T period */
             pwco90,       /* PW90 for co nucleus @ dhpwr         */
             pwca180h,     /* PW180 for ca at dvhpwr               */
             tsatpwr,      /* low level 1H trans.power for presat  */
             dhpwr,        /* power level for 13C pulses on dec1 - 64 us 
                              90 for part a of the sequence  */
             dvhpwr,       /* power level for 180 13C pulses at 54 ppm
                                using a 55.6 us 180 so that get null in
                                co at 178 ppm */

             pwN,          /* PW90 for 15N pulse              */
             pwNlvl,       /* high dec2 pwr for 15N hard pulses    */

             sw1,          /* sweep width in f1                    */             
             sw2,          /* sweep width in f2                    */             
             pw_sl,        /* pw90 for H selective pulse on water ~ 2ms */
             phase_sl,        /* pw90 for H selective pulse on water ~ 2ms */
             tpwrsl,       /* power level for square pw_sl       */

             compC,       /* C-13 RF calibration parameters */
             pwC,
             pwClvl,

             pw_sl1,        
             tpwrsl1,   

             gt0,
             gt1,
             gt2,
             gt3,
             gt4,
             gt5,
             gt6,
             gt7,
             gt8,

             gstab = getval("gstab"),

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
  getstr("fc180",fc180);
  getstr("shp_sl",shp_sl);
  getstr("sel_flg",sel_flg);

  getstr("shared_CT",shared_CT);

  getstr("nietl_flg",nietl_flg);

  taua   = getval("taua"); 
  taub   = getval("taub"); 
  zeta  = getval("zeta");
  bigTN = getval("bigTN");

  pwN = getval("pwN");
  pwNlvl = getval("pwNlvl");

  tpwr = getval("tpwr");
  tsatpwr = getval("tsatpwr");
  dpwr = getval("dpwr");
  phase = (int) ( getval("phase") + 0.5);
  phase2 = (int) ( getval("phase2") + 0.5);
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  ni = getval("ni");
  ni2 = getval("ni2");
  pw_sl = getval("pw_sl");
  phase_sl = getval("phase_sl");
  tpwrsl = getval("tpwrsl");

  pw_sl1 = getval("pw_sl1");
  tpwrsl1 = getval("tpwrsl1");

  gt0 = getval("gt0");
  gt1 = getval("gt1");
  if (getval("gt2") > 0) gt2=getval("gt2");
    else gt2=gt1*0.1;
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");
  gt6 = getval("gt6");
  gt7 = getval("gt7");
  gt8 = getval("gt8");

  gzlvl0 = getval("gzlvl0");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl6 = getval("gzlvl6");
  gzlvl7 = getval("gzlvl7");
  gzlvl8 = getval("gzlvl8");

  if(autocal[0]=='n')
  {     
    getstr("spca180",spca180);
    pwca180h = getval("pwca180h");
    dvhpwr = getval("dvhpwr");
    pwco90 = getval("pwco90");
    dhpwr = getval("dhpwr");
  }
  else
  {        
    strcpy(spca180,"Phard_-118p");    
    if (FIRST_FID)  
    {
      compC = getval("compC");
      pwC = getval("pwC");
      pwClvl = getval("pwClvl");
      ca180 = pbox(spca180, CA180, CA180ps, dfrq, compC*pwC, pwClvl);        
      co90 = pbox("Phard90", CO90, CA180ps, dfrq, compC*pwC, pwClvl);  
    }    
    pwca180h = ca180.pw;
    dvhpwr = ca180.pwr;
    pwco90 = co90.pw;
    dhpwr = co90.pwr;
  }   

/* LOAD PHASE TABLE */

  settable(t1,4,phi1);
  settable(t2,2,phi2);
  settable(t3,8,phi3);
  settable(t4,1,phi4);
  settable(t6,4,rec);

/* CHECK VALIDITY OF PARAMETER RANGES */


   if(shared_CT[A] == 'n')
    if( bigTN - ((ni2-1)*0.5/sw2) < 0.2e-6 )
    {
        printf(" ni2 is too big\n");
        psg_abort(1);
    }

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y'))
    {
        printf("incorrect dec1 decoupler flags!  ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y'))
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

    if ( ni> 1 && f2180[A] == 'y')

    {
        printf("f2180 may set wrong ! ");
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

    if(nietl_flg[A] == 'y' && sel_flg[A] == 'y') {
        printf("Both nietl_flg and sel_flg cannot be y\n");
        psg_abort(1);
     }

    if (fc180[A] =='y' && ni > 1.0) {
       text_error("must set fc180='n' to allow Calfa evolution (ni>1)\n");
       psg_abort(1);
   }


/*  Phase incrementation for hypercomplex 2D data */

    if (phase == 2) tsadd(t1,1,4);

    if (shared_CT[A] == 'n') 
      {
       if (phase2 == 2) { tsadd(t4, 2, 4); icosel = 1; }
         else icosel = -1;
      }
     else 
      {
       if (phase2 == 2) { tsadd(t4, 2, 4); icosel = -1; }
         else icosel = 1;
      }

    if (nietl_flg[A] == 'y') icosel = -1*icosel;

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
   decpower(dhpwr);        /* Set Dec1 power for hard 13C pulses         */
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
   delay(20.0e-6);

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */

   delay(0.2e-6);
   zgradpulse(gzlvl5,gt5);
   delay(2.0e-6);

   delay(taua -gt5 -2.2e-6);   /* taua <= 1/4JNH */ 

   sim3pulse(2*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);

   dec2phase(zero); decphase(zero); 

   delay(0.2e-6);
   zgradpulse(gzlvl5,gt5);
   delay(200.0e-6);

   delay(taua -gt5 -200.2e-6); 

   if (sel_flg[A] == 'y')	/* active suppression of one of the two components */
     {
      rgpulse(pw,one,4.0e-6,0.0);

      initval(1.0,v2);
      obsstepsize(phase_sl);
      xmtrphase(v2);

      /* shaped pulse  */
      obspower(tpwrsl);
      shaped_pulse(shp_sl,pw_sl,two,2.0e-6,0.0);
      xmtrphase(zero);
      delay(2.0e-6);
      obspower(tpwr);
      txphase(zero);
      /* shaped pulse  */

      initval(1.0,v3);
      dec2stepsize(45.0);   
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

      delay( zeta - 1.34e-3 - 2.0*pw + 2*pwco90 );
     }
    else 
     {
      rgpulse(pw,three,4.0e-6,0.0);

      initval(1.0,v2);
      obsstepsize(phase_sl);
      xmtrphase(v2);
   
      /* shaped pulse  */
      obspower(tpwrsl);
      shaped_pulse(shp_sl,pw_sl,zero,2.0e-6,0.0);
      xmtrphase(zero);
      delay(2.0e-6);
      obspower(tpwr);
      txphase(zero);
      /* shaped pulse  */

      delay(0.2e-6);
      zgradpulse(gzlvl3,gt3);
      delay(gstab);

      dec2rgpulse(pwN,zero,0.0,0.0);

      delay( zeta + 2*pwco90 );
     }
  
   dec2rgpulse(2*pwN,zero,0.0,0.0);
   decrgpulse(2*pwco90,zero,0.0,0.0);

   delay(zeta - 2.0e-6);

   dec2rgpulse(pwN,one,2.0e-6,0.0);

   dec2phase(zero); decphase(t1);

   delay(0.2e-6);
   zgradpulse(gzlvl7,gt7);
   delay(gstab);

   decrgpulse(pwco90,t1,2.0e-6,0.0);

   if ( fc180[A] == 'n' ) 
     {
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

   delay(0.2e-6);
   zgradpulse(gzlvl4,gt4);
   delay(gstab);

   if (shared_CT[A] == 'n') 
     {
      dec2rgpulse(pwN,t2,2.0e-6,0.0);

      dec2phase(t3);
      delay(bigTN - tau2 + 2.0*pwco90);

      dec2rgpulse(2*pwN,t3,0.0,0.0);
      decrgpulse(2*pwco90,zero,0.0,0.0);

       txphase(zero);
       dec2phase(t4);

       delay(0.2e-6);
       zgradpulse(gzlvl1,gt1);
       delay(500.0e-6);

       delay(bigTN - gt1 - 500.2e-6 - 2.0*GRADIENT_DELAY - POWER_DELAY
	      - 4.0e-6 - WFG_START_DELAY - pwca180h - WFG_STOP_DELAY
	      - POWER_DELAY);

       decpower(dvhpwr);
       decshaped_pulse(spca180,pwca180h,zero,4.0e-6,0.0);
       decpower(dhpwr);

       delay(tau2);

       sim3pulse(pw,0.0e-6,pwN,zero,zero,t4,0.0,0.0);
      }
    if (shared_CT[A] == 'y') 
      {
       dec2rgpulse(pwN,t2,2.0e-6,2.0e-6);
       dec2phase(t3);

       if (bigTN - tau2 > 0.2e-6) 
         {
          delay(tau2);

          decpower(dvhpwr);
          decshaped_pulse(spca180,pwca180h,zero,4.0e-6,0.0);
          decpower(dhpwr);

          delay(0.2e-6);
          zgradpulse(gzlvl1,gt1);
          delay(1000.0e-6);

          delay(bigTN - POWER_DELAY
                - 4.0e-6 - WFG_START_DELAY - pwca180h
                - WFG_STOP_DELAY - POWER_DELAY
                - gt1 - 1000.2e-6
                - 2.0*GRADIENT_DELAY - 2.0*pwco90);

          decrgpulse(2*pwco90,zero,0.0,0.0);
          dec2rgpulse(2*pwN,t3,0.0,0.0);
          txphase(zero);
          dec2phase(t4);

          delay(bigTN - tau2);
         }
        else 
         {
          delay(tau2);

          decpower(dvhpwr);
          decshaped_pulse(spca180,pwca180h,zero,4.0e-6,0.0);
          decpower(dhpwr);

          delay(0.2e-6);
          zgradpulse(gzlvl1,gt1);
          delay(1000.0e-6);

          delay(bigTN - POWER_DELAY - 4.0e-6 - WFG_START_DELAY
                - pwca180h - WFG_STOP_DELAY - POWER_DELAY
                - gt1 - 1000.2e-6 - 2.0*GRADIENT_DELAY
                - 2.0*pwco90);

          decrgpulse(2*pwco90,zero,0.0,0.0);
 
          delay(tau2 - bigTN);
          dec2rgpulse(2*pwN,t3,0.0,0.0);
         }
      
       sim3pulse(pw,0.0e-6,pwN,zero,zero,t4,2.0e-6,0.0);
      }  
/* end of shared_CT   */

    if (nietl_flg[A] == 'n') 
      {
       decpower(dhpwr);
       decrgpulse(pwco90,zero,4.0e-6,0.0);

       delay(0.2e-6);
       zgradpulse(gzlvl6,gt6);
       delay(2.0e-6);

       dec2phase(zero);
       delay(taub - POWER_DELAY - 4.0e-6 
              - pwco90 - gt6 - 2.2e-6);

       sim3pulse(2*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);

       delay(0.2e-6);
       zgradpulse(gzlvl6,gt6);
       delay(200.0e-6);
   
       txphase(one);
       dec2phase(one);

       delay(taub - gt6 - 200.2e-6);

       sim3pulse(pw,0.0e-6,pwN,one,zero,one,0.0,0.0);

       delay(0.2e-6);
       zgradpulse(1.33*gzlvl6,gt6);
       delay(2.0e-6);
 
       txphase(zero);
       dec2phase(zero);

       delay(taub -gt6 -2.2e-6);

       sim3pulse(2*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);

       delay(0.2e-6);
       zgradpulse(1.33*gzlvl6,gt6);
       delay(200.0e-6);

       delay(taub -gt6 -200.2e-6);

       sim3pulse(pw,0.0e-6,pwN,zero,zero,zero,0.0,0.0);
      }
     else 
      {
       /* shaped pulse  */
       obspower(tpwrsl1);
       shaped_pulse(shp_sl,pw_sl1,zero,2.0e-6,0.0);
       delay(2.0e-6);
       obspower(tpwr);
       txphase(zero);
       /* shaped pulse  */

       decpower(dhpwr);
       decrgpulse(pwco90,zero,4.0e-6,0.0);

       delay(0.2e-6);
       zgradpulse(gzlvl6,gt6);
       delay(2.0e-6);

       dec2phase(zero);
       delay(taub - POWER_DELAY - 2.0e-6 - WFG_START_DELAY - pw_sl1
              - WFG_STOP_DELAY - 2.0e-6 - POWER_DELAY 
              - POWER_DELAY - 4.0e-6 
              - pwco90 - gt6 - 2.2e-6);

       sim3pulse(2*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);

       delay(0.2e-6);
       zgradpulse(gzlvl6,gt6);
       delay(200.0e-6);
   
       txphase(one);
       dec2phase(zero);

       delay(taub -gt6 -200.2e-6);

       sim3pulse(pw,0.0e-6,pwN,one,zero,zero,0.0,0.0);

       delay(0.2e-6);
       zgradpulse(1.33*gzlvl6,gt6);
       delay(2.0e-6);
 
       txphase(zero);
       dec2phase(zero);

       delay(taub -gt6 -2.2e-6);

       sim3pulse(2*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);
       txphase(one);
       dec2phase(one);

       delay(0.2e-6);
       zgradpulse(1.33*gzlvl6,gt6);
       delay(200.0e-6);

       delay(taub -gt6 -200.2e-6);

       sim3pulse(pw,0.0e-6,pwN,one,zero,one,0.0,0.0);
       txphase(zero);
      }

    delay(gt2 +2.0*GRADIENT_DELAY +50.2e-6 +gstab +2.0*POWER_DELAY
    	   -0.5*(pwN -pw) -2.0*pw/PI);

    rgpulse(2*pw,zero,0.0,0.0);

    delay(0.2e-6);
    zgradpulse(icosel*gzlvl2,gt2);
    delay(50.0e-6);

    delay(gstab);
    dec2power(dpwr2);
    decpower(dpwr);
status(C);
         setreceiver(t6);

}
