/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gCNfilnoesyChsqcSE

    3D 13C,15N F1-filtered C13 edited noesy 
    recorded on a water sample 

    Uses three channels:
         1)  1H             - carrier @ water  
         2) 13C             - carrier @ 43 ppm
         3) 15N             - carrier @ 118 ppm

    Set dm = 'nnny', [13C decoupling during acquisition].

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI
    acquisition in t1 [H]  and t2 [C].

    Set f1180 = 'y' and f2180 = 'y' for (90, -180) in F1 and (90, -180) in  F2.    

    If you set the C13 carrier to a value other than 43 ppm (such as 35 ppm),
    change the line
        dofa=dof+(125-43)*dfrq;
    below to
        dofa=dof+(125-35)*dfrq;

    Coded to use sensitivity enhancement in the 13C dimension for better water
    suppression in coldprobes. Marco Tonelli, NMRFAM 2003.
     
    STUD DECOUPLING.   SET STUD='y':
       Setting the flag STUD='y' overrides the decoupling parameters listed in
       dg2 and applies STUD+ decoupling instead.  In consequence is is easy
       to swap between the decoupling scheme you have been using to STUD+ for
       an easy comparison.  The STUD+ waveforms are calculated for your field
       strength at the time of BioPack installation and RF power is 
       calculated within the pulse sequence.  The calculations are for the most 
       peaks being greater than 90% of ideal. If you wish to compare different 
       decoupling schemes, the power level used for STUD+ can be obtained from 
       dps - subtract 3dB from that level to compare to decoupling schemes at
       a continuous RF level such as GARP.  The value of 90% has
       been set to limit sample heating as much as possible.  If you wish to 
       change STUD parameters, for example to increase the quality of decoupling
       (and RF heating) change the 95% level for the centerband
       by changing the relevant number in the macro makeSTUDpp and 
       rerun the macro (don't use 100% !!).  (At the time of writing STUD has
       been coded to use the coarse attenuator, because the fine attenuator
       is not presently included in the fail-safe calculation of decoupling 
       power which aborts an experiment if the power is too high - this may
       lower STUD efficiency a little).

     N15/C13 decoupling in t1:
       For simultaneous N15/C13 decoupling (N15refoc='y') the N15 and C13 power
       levels should be reduced by 3dB (parameters pwNlw and pwClw). 
       - pwNlvl and pwClvl are calculated by the macro that calls the sequence 
	 using parameters read from the probefile
       OR
       - pwNlvl and pwClvl can also be set manually by the user
       OR
       - if pwNlvl and pwClvl are set to 0, the pulse sequence will estimate their 
	 value from the corresponding hard pulses

     LP in t2:
       The finite delays necessary during 13C evolution make the first few data
       points in t2 distorted in intensity. The timing is correct so that lp2
       may be set to zero, but the intensity distortion, particularly of the
       second complex point, lead to a "dish" aspect of the baseline. This is not
       due to the presence of a first-order phase correction (lp2), so adjustment
       of the timing of the pulse sequence events is not needed.
   
       One solution is to use a smaller sw2 with intentional folding. This can make
       the second d2 value large enough so there is enough time for the C=O
       refocusing pulse to be executed. For larger sw2's there is not enough time.
 
       A solution to this is to use linear prediction in t2, the 13C dimension.
       In VNMR you can both "fix up" the first few points using the rest of the
       data table as the basis set, as well as extend the data set for better F2
       resolution and less distortion from truncation. The macro "setlp2" can be
       used in the format "setlp2(desired ni2, acquired ni2, #fixed)". Set
       "desired ni2" to be the final extended data size, "acquired ni2" to be the
       total # of increments to be used as a basis (it may be less than ni2, for
       example if the experiment is running), and "#fixed" to the number of
       initial points in t2 to be predicted (typically 2-4). Try this with a 2D
       data set for varying numbers of fixed points until the baseline is sufficiently
       flat in F2.
*/

#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */  

static int  phi1[8]  = {0,0,0,0, 2,2,2,2},
            phi2[4]  = {0,0,2,2},
	    phi3[2]  = {0,2},
            phi5[1]  = {0},

            phi6[16] = {0,0,0,0, 0,0,0,0, 2,2,2,2, 2,2,2,2},
            phi7[32] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
			2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2},

	    rec1[8]  = {0,2,2,0, 2,0,0,2},
            rec2[32] = {0,2,2,0, 2,0,0,2, 2,0,0,2, 0,2,2,0,
			2,0,0,2, 0,2,2,0, 0,2,2,0, 2,0,0,2};


static double d2_init=0.0, d3_init=0.0;
static double H1ofs=4.7, C13ofs=35.0, N15ofs=120.0, H2ofs=0.0;

static shape stC30, stC200;

void pulsesequence()
{
/* DECLARE VARIABLES */

char     
  aliph[MAXSTR],	/* aliphatic CHn groups only */
  arom[MAXSTR],		/* aromatic CHn groups only */
  N15refoc[MAXSTR],	/* flag for refocusing 15N during indirect H1 evolution */

  f1180[MAXSTR],	/* Flag to start t1 @ halfdwell */
  mag_flg[MAXSTR],	/* magic angle gradient */
  f2180[MAXSTR],	/* Flag to start t2 @ halfdwell */
  stCshape[MAXSTR],	/* C13 inversion pulse shape name */
  STUD[MAXSTR],		/* Flag to select adiabatic decoupling */
  stCdec[MAXSTR],	/* contains name of adiabatic decoupling shape */
  auto_dof[MAXSTR];	/* automatically adjust dof for aromatic, aliphatic, all carbon */

int         
  
  icosel,		/* used to get n and p type */
  t1_counter,		/* used for states tppi in t1 */ 
  t2_counter;		/* used for states tppi in t2 */ 

double    

  JCH1 = getval("JCH1"),	/* smallest coupling that you wish to purge */
  JCH2 = getval("JCH2"),	/* largest coupling that you wish to purge */
  taud,				/* 1/(2JCH1)   */
  taue,				/* 1/(2JCH2)   */

/* N15 purging */
  tauNH  = 1/(4.0*getval("JNH")),		/* HN coupling constant */

  gt4 = getval("gt4"),
  gt14 = getval("gt14"),
  gt7 = getval("gt7"),
  gt17 = getval("gt17"),
  gt8 = getval("gt8"),
  gt9 = getval("gt9"),

  gzlvl4 = getval("gzlvl4"),
  gzlvl14 = getval("gzlvl14"),
  gzlvl7 = getval("gzlvl7"),
  gzlvl17 = getval("gzlvl17"),
  gzlvl8 = getval("gzlvl8"),
  gzlvl9 = getval("gzlvl9"),

  bw, pws, ofs, ppm, nst,  /* bandwidth, pulsewidth, offset, ppm, # steps */

  ni2 = getval("ni2"),
  dofa = 0.0,			/* actual 13C offset (depends on aliph and arom)*/
  rf200 = getval("rf200"), 	/* rf in Hz for 200ppm STUD+ */
  dmf200 = getval("dmf200"),     /* dmf for 200ppm STUD+ */
  rf30 = getval("rf30"),	/* rf in Hz for 30ppm STUD+ */
  dmf30 = getval("dmf30"),	/* dmf for 30ppm STUD+ */

  stdmf = 1.0,			/* dmf for STUD decoupling initialized */ 
  studlvl = 0.0,		/* coarse power for STUD+ decoupling initialized */

  rffil = 0.0,			/* fine power level for 200ppm adiabatic pulse */

  rfst = 0.0,			/* fine power level for adiabatic pulse initialized */
  rf0,				/* full fine power */

  /*compH = getval("compH"),       adjustment for H1  amplifier compression */
  compC = getval("compC"),      /* adjustment for C13 amplifier compression */
  compN = getval("compN"),      /* adjustment for N15 amplifier compression */

  tau1,				/*  t1 delay */
  tau2,				/*  t2 delay */

  JCH = getval("JCH"),		/*  CH coupling constant */
  Cfil = getval("Cfil"),		/*  CH coupling constant */

  pwC = getval("pwC"),		/* PW90 for 13C nucleus @ pwClvl */
  pwClvl = getval("pwClvl"),	/* high power for 13C hard pulses on dec1  */
  pwC180 = getval("pwC180"),	/* PW180 for 13C nucleus in INEPT transfers */
  pwN = getval("pwN"),		/* PW90 for 15N nucleus @ pwNlvl */
  pwNlvl = getval("pwNlvl"),	/* high power for 15N hard pulses on dec2 */

  pwClw=getval("pwClw"), 
  pwNlw=getval("pwNlw"),
  pwZlw=0.0,			/* largest of pwNlw and 2*pwClw */

  mix  = getval("mix"),		/* noesy mix time */
  sw1  = getval("sw1"),		/* spectral width in t1 (H) */
  sw2  = getval("sw2"),		/* spectral width in t2 (C) */
  gstab = getval("gstab"),	/* gradient recovery delay (300 us recom.) */
  gsign = 1.0,
  gzcal = getval("gzcal"),	/* dac to G/cm conversion factor */
  gt0 = getval("gt0"),
  gt1 = getval("gt1"),
  gt2 = getval("gt2"),
  gt3 = getval("gt3"),
  gt5 = getval("gt5"),
  gt6 = getval("gt6"),
  gzlvl0 = getval("gzlvl0"),
  gzlvl1 = getval("gzlvl1"),
  gzlvl2 = getval("gzlvl2"),
  gzlvl3 = getval("gzlvl3"),
  gzlvl5 = getval("gzlvl5"),
  gzlvl6 = getval("gzlvl6");


/* LOAD VARIABLES */

  getstr("aliph",aliph);
  getstr("arom",arom);
  getstr("N15refoc",N15refoc);

  getstr("mag_flg",mag_flg);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("STUD",STUD);

  getstr("auto_dof",auto_dof);

/* LOAD PHASE TABLE */
  settable(t1,8,phi1);
  settable(t2,4,phi2);
  settable(t3,2,phi3);
  settable(t5,1,phi5);

  settable(t6,16,phi6);
  settable(t7,32,phi7);

  if (Cfil == 1) settable(t4,8,rec1);
    else settable(t4,32,rec2);

/* CHECK VALIDITY OF PARAMETER RANGES */

    if ( (arom[A]=='n' && aliph[A]=='n') || (arom[A]=='y' && aliph[A]=='y') )
      { 
	printf("You need to select one and only one of arom or aliph options  ");
	psg_abort(1); 
      }

    if((dm[A] == 'y' || dm[C] == 'y' ))
    {
        printf("incorrect 13C decoupler flags! dm='nnnn' or 'nnny' only  ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[C] == 'y' ))
    {
        printf("incorrect 15N decoupler flags! No decoupling in relax or mix periods  ");
        psg_abort(1);
    }

    if( dpwr > 49 )
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

    if( dpwr2 > 49 )
    {
        printf("don't fry the probe, DPWR2 too large!  ");
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

    if( pwC > 200.0e-6 )
    {
        printf("dont fry the probe, pwC too high ! ");
        psg_abort(1);
    } 

    if( gt0 > 15e-3 || gt1 > 15e-3 || gt2 > 15e-3 || gt3 > 15e-3 || gt4 > 15e-3 || gt5 > 15e-3 || gt6 > 15e-3 ) 
    {
        printf("gti values < 15e-3\n");
        psg_abort(1);
    } 

/*   if( gzlvl3*gzlvl4 > 0.0 )*/ 

    if (phase1 == 2)
      tsadd(t3,1,4);

    if (phase2 == 1)  {tsadd(t5,2,4);  icosel = +1;}
        else 			       icosel = -1;    

/*  Set up f1180  tau1 = t1               */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) tau1 += 1.0/(2.0*sw1);
    if(tau1 < 0.2e-6) tau1 = 4.0e-7;
    tau1 = tau1/2.0;

/*  Set up f2180  tau2 = t2               */

    tau2 = d3;
    if((f2180[A] == 'y') && (ni2 > 1.0)) tau2 += ( 1.0 / (2.0*sw2) ); 
    tau2 = tau2/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if (t1_counter % 2) 
     { tsadd(t3,2,4); tsadd(t4,2,4);}

   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if (t2_counter % 2)  
     {tsadd(t2,2,4); tsadd(t4,2,4);}
    

/* calculate 3db lower power hard pulses for simultaneous CN decoupling during
   indirect H1 evoluion pwNlw and pwClw should be calculated by the macro that 
   calls the experiment. */

  if (N15refoc[A] == 'y')
    {
     if (pwNlw==0.0) pwNlw = compN*pwN*exp(3.0*2.303/20.0);
     if (pwClw==0.0) pwClw = compC*pwC*exp(3.0*2.303/20.0);
     if (pwNlw > 2.0*pwClw) 
	 pwZlw=pwNlw;
      else
	 pwZlw=2.0*pwClw;
/* Uncomment to check pwClw and pwNlw
     if (d2==0.0 && d3==0.0) printf(" pwClw = %.2f ; pwNlw = %.2f\n", pwClw*1e6,pwNlw*1e6);
*/
    }


/* make sure that gt3 and gt1 are of opposite sign to help dephasing H2O */
   if (gzlvl3*icosel*gzlvl1 > 0.0) gsign=-1.0;
     else gsign=1.0; 


/* if coupling constants are input by user use them to calculate delays */
   if (Cfil == 1)
     {
      taud = 1.0/(2.0*JCH1);
      taue = 1.0/(2.0*JCH2);
     }
    else
     {
       taud = 1.0/(4.0*JCH1);
       taue = 1.0/(4.0*JCH2);
     }


/* maximum fine power for pwC pulses */
   rf0 = 4095.0;

   setautocal();                        /* activate auto-calibration flags */ 
        
   if (autocal[0] == 'n') 
   {
     if (arom[A]=='y')  /* AROMATIC spectrum */
     {
       /* 30ppm sech/tanh inversion */
       rfst = (compC*4095.0*pwC*4000.0*sqrt((4.5*sfrq/600.0+3.85)/0.41));   
       rfst = (int) (rfst + 0.5);
     }
  
     if (aliph[A]=='y')  /* ALIPHATIC spectrum */
     {
       /* 200ppm sech/tanh inversion pulse */
       if (pwC180>3.0*pwC) 
	 {
	   rfst = (compC*4095.0*pwC*4000.0*sqrt((12.07*sfrq/600+3.85)/0.35));
	   rfst = (int) (rfst + 0.5);
       }
       else rfst=4095.0;

       if( pwC > (20.0e-6*600.0/sfrq) )
	 { printf("Increase pwClvl so that pwC < 20*600/sfrq");
	  psg_abort(1); 
       }
     }

     if (Cfil > 1)  /* 200ppm pulse for C13 filtering */
     {
       /* 200ppm sech/tanh inversion pulse */
       if (pwC180>3.0*pwC) 
	 {
	   rffil = (compC*4095.0*pwC*4000.0*sqrt((12.07*sfrq/600+3.85)/0.35));
	   rffil = (int) (rffil + 0.5);
       }
       else rfst=4095.0;

       if( pwC > (20.0e-6*600.0/sfrq) )
	 { printf("Increase pwClvl so that pwC < 20*600/sfrq");
	  psg_abort(1); 
       }
     }

   }
   else        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
   {
     if(FIRST_FID)                                            /* call Pbox */
     {
       ppm = getval("dfrq"); 
       bw = 118.0*ppm; ofs = 139.0*ppm;
       if (arom[A]=='y')  /* AROMATIC spectrum */
       {
         bw = 30.0*ppm; pws = 0.001; ofs = 0.0; nst = 500.0;    
         stC30 = pbox_makeA("stC30", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
       }
       if ((aliph[A]=='y') || (Cfil > 1))
       {
         bw = 200.0*ppm; pws = 0.001; ofs = 0.0; nst = 1000.0;    
         stC200 = pbox_makeA("stC200", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
       }
       ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
     }
     if (arom[A]=='y')  rfst = stC30.pwrf;
     if (aliph[A]=='y') 
     {
      if (pwC180>3.0*pwC) rfst = stC200.pwrf;
      else rfst = 4095.0;
     }
     if (Cfil > 1)
     {
      if (pwC180>3.0*pwC) rffil = stC200.pwrf;
      else rffil = 4095.0;
     }
   }

   if (arom[A]=='y')  
   {
     dofa=dof+(125-43)*dfrq;

     strcpy(stCshape, "stC30");
     /* 30 ppm STUD+ decoupling */
     strcpy(stCdec, "stCdec30");             
     stdmf = dmf30;
     studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf30);
     studlvl = (int) (studlvl + 0.5);
   }

   if (aliph[A]=='y')
   {
     dofa=dof;
       
     strcpy(stCshape, "stC200");
     /* 200 ppm STUD+ decoupling */
     strcpy(stCdec, "stCdec200");
     stdmf = dmf200;
     studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf200);
     studlvl = (int) (studlvl + 0.5);
   }

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);

   if (auto_dof[A]=='y') decoffset(dofa);

   obspower(tpwr);		/* Set transmitter power for hard 1H pulses */
   decpower(pwClvl);		/* Set Dec1 power for hard 13C pulses */
   dec2power(pwNlvl);		/* Set Dec2 power for decoupling during tau1 */
   dec2pwrf(rf0);       

   initval(135.0,v1);
   obsstepsize(1.0);


   delay(d1);

/* destroy N15 and C13 magnetization */
   if (N15refoc[A] == 'y') dec2rgpulse(pwN, zero, 0.0, 0.0);
   decrgpulse(pwC, zero, 0.0, 0.0);
   zgradpulse(gzlvl0, 0.5e-3);
   delay(gstab);
   if (N15refoc[A] == 'y') dec2rgpulse(pwN, one, 0.0, 0.0);
   decrgpulse(pwC, one, 0.0, 0.0);
   zgradpulse(0.7*gzlvl0, 0.5e-3);

   decphase(zero);       
   dec2phase(zero);       
   rcvroff();
   delay(gstab);


status(B);

if (Cfil == 1) 
  {
   xmtrphase(v1);
   rgpulse(pw, t1, rof1 , 0.0);  
   txphase(zero); 
   xmtrphase(zero); 

/* CN FILTER BEGINS */
      zgradpulse(gzlvl8, gt8);
      txphase(zero); xmtrphase(zero);
      delay(taud -gt8 -2.0*GRADIENT_DELAY -2.0*SAPS_DELAY);

      simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

      zgradpulse(gzlvl8, gt8);
      delay(taue -gt8 -2.0*GRADIENT_DELAY);

      decrgpulse(pwC, zero, 0.0, 0.0);

      delay(taud -taue -pwC);

/* CN FILTER ENDS */

   }
else if (Cfil == 2)
   {
      txphase(t6);
      rgpulse(pw, t6, rof1, 0.0);                  /* 90 deg 1H pulse */
/* BEGIN 1st FILTER */
      txphase(zero);

      zgradpulse(gzlvl8,gt8);
      decpwrf(rffil);
      delay(taud -gt8 -2.0*GRADIENT_DELAY -WFG2_START_DELAY -0.5e-3 +70.0e-6);

      simshaped_pulse("", "stC200", 2.0*pw, pwC180, zero, zero, 0.0, 0.0);

      zgradpulse(gzlvl8,gt8);
      decpwrf(rf0);
      delay(taud -gt8 -2.0*GRADIENT_DELAY -0.5e-3 +70.0e-6);

      simpulse(pw, pwC, zero, zero, 0.0, 0.0);

      zgradpulse(gzlvl4,gt4);
      txphase(t7);
      delay(gstab);

      rgpulse(pw, t7, 0.0, 0.0);
/* BEGIN 2nd FILTER */

      zgradpulse(gzlvl9,gt9);
      decpwrf(rffil);
      delay(taue -gt9 -2.0*GRADIENT_DELAY -WFG2_START_DELAY -0.5e-3 +70.0e-6);

      simshaped_pulse("", "stC200", 2.0*pw, pwC180, zero, zero, 0.0, 0.0);

      zgradpulse(gzlvl9,gt9);
      decpwrf(rf0);
      delay(taue -gt9 -2.0*GRADIENT_DELAY -0.5e-3 +70.0e-6);

      simpulse(pw, pwC, zero, zero, 0.0, 0.0);

      zgradpulse(gzlvl7,gt7);
      txphase(t1); xmtrphase(v1);
      delay(gstab);

      rgpulse(pw, t1, 0.0, 0.0);
      txphase(zero); xmtrphase(zero);
   }
else if (Cfil == 3)
   {
      txphase(t6);
      rgpulse(pw, t6, rof1, 0.0);                  /* 90 deg 1H pulse */
/* BEGIN 1st FILTER */
      txphase(zero);

      zgradpulse(gzlvl8,gt8);
      delay(tauNH -gt8 -2.0*GRADIENT_DELAY);

      sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

      decpwrf(rffil);
      delay(tauNH -taud -0.5e-3 -WFG_START_DELAY -PWRF_DELAY);

      decshaped_pulse("stC200", pwC180, zero, 0.0, 0.0);

      zgradpulse(gzlvl8,gt8);
      decpwrf(rf0);
      delay(taud -gt8 -2.0*GRADIENT_DELAY -0.5e-3 -PWRF_DELAY);

      sim3pulse(pw, pwC, pwN, zero, zero, zero, 0.0, 0.0);
      zgradpulse(gzlvl14,gt14);
      txphase(t7);
      delay(gstab);

      rgpulse(pw, t7, 0.0, 0.0);
/* BEGIN 2nd FILTER */

      zgradpulse(gzlvl9,gt9);
      delay(tauNH -gt9 -2.0*GRADIENT_DELAY);

      sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

      decpwrf(rffil);
      delay(tauNH -taue -0.5e-3 -WFG_START_DELAY -PWRF_DELAY);

      decshaped_pulse("stC200", pwC180, zero, 0.0, 0.0);

      zgradpulse(gzlvl9,gt9);
      decpwrf(rf0);
      delay(taue -gt9 -2.0*GRADIENT_DELAY -0.5e-3 -PWRF_DELAY);

      sim3pulse(pw, pwC, pwN, zero, zero, zero, 0.0, 0.0);
      zgradpulse(gzlvl17,gt17);
      txphase(t1); xmtrphase(v1);
      delay(gstab);

      rgpulse(pw, t1, 0.0, 0.0);
      txphase(zero); xmtrphase(zero);
   }

/* H1 INDIRECT EVOLUTION BEGINS */
   if (ni > 0) 
    txphase(t3);
    {
     if ( (N15refoc[A]=='y') && ((tau1 -pwN -2.0*pw/PI -rof1 -SAPS_DELAY) > 0.0) )
      {
       delay(tau1 -pwN -2.0*pw/PI -SAPS_DELAY);
       dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
       delay(tau1 -pwN -2.0*pw/PI -rof1);
      }     
     else if (tau1 > 2.0*pw/PI +rof1 +SAPS_DELAY)
       delay(2.0*tau1 -4.0*pw/PI -2.0*rof1 -SAPS_DELAY);
    }
/* H1 INDIRECT EVOLUTION ENDS */
   rgpulse(pw, t3, rof1, rof1);             /*  2nd 1H 90 pulse   */

status(C);

   delay(mix -pwC -gt0 -PWRF_DELAY -gstab -2.0*GRADIENT_DELAY); 

   decrgpulse(pwC,zero,0.0,0.0); 
   zgradpulse(gzlvl0, gt0);
   decpwrf(rfst);                           /* fine power for inversion pulse */
   delay(gstab);

/* FIRST HSQC INEPT TRANSFER */
   rgpulse(pw,zero,0.0,0.0);
   zgradpulse(gzlvl4, gt4);
   delay(1/(4.0*JCH) -gt4 -2.0*GRADIENT_DELAY -WFG2_START_DELAY -pwC180*0.45);

   simshaped_pulse("",stCshape,2*pw,pwC180,zero,zero,0.0,0.0);

   zgradpulse(gzlvl4, gt4);
   decpwrf(rf0);
   txphase(one);
   delay(1/(4.0*JCH) -gt4 -2.0*GRADIENT_DELAY -pwC180*0.45 -PWRF_DELAY -SAPS_DELAY);

   rgpulse(pw,one,0.0,0.0);
   zgradpulse(gsign*gzlvl3, gt3);
   txphase(zero);
   delay(gstab);

/* C13 EVOLUTION */
   decrgpulse(pwC,t2,0.0,0.0);   

   delay(tau2);
   rgpulse(2.0*pw,zero,0.0,0.0);
   delay(tau2);

   decphase(zero);
   delay(gt1 +2.0*GRADIENT_DELAY +gstab -2.0*pw -SAPS_DELAY);
   decrgpulse(2*pwC,zero,0.0,0.0);

   if (mag_flg[A] == 'y')  magradpulse(icosel*gzcal*gzlvl1, gt1);
     else  zgradpulse(icosel*gzlvl1, gt1);
   decphase(t5);
   delay(gstab);

   decrgpulse(pwC,t5,0.0,0.0);
   delay(pw);
   rgpulse(pw,zero,0.0,0.0);

   zgradpulse(gzlvl5, gt5);
   decphase(zero);
   delay(1/(8.0*JCH) -gt5 -SAPS_DELAY -2.0*GRADIENT_DELAY);		/* d3 = 1/8*Jch */

   decrgpulse(2.0*pwC,zero,0.0,2.0e-6);
   rgpulse(2.0*pw,zero,0.0,0.0);

   zgradpulse(gzlvl5, gt5);
   decphase(one);
   txphase(one);
   delay(1/(8.0*JCH) -gt5 -2.0*SAPS_DELAY -2.0*GRADIENT_DELAY);		/* d3 = 1/8*Jch */

   delay(pwC);
   decrgpulse(pwC,one,0.0,2.0e-6);
   rgpulse(pw,one,0.0,0.0);

   zgradpulse(gzlvl6, gt6);
   decpwrf(rfst);                           /* fine power for inversion pulse */
   decphase(zero);
   txphase(zero);
   delay(1/(4.0*JCH) -gt6 -pwC180*0.45 -PWRF_DELAY 
		-WFG2_START_DELAY -2.0*SAPS_DELAY -2.0*GRADIENT_DELAY);	/* d2 = 1/4*Jch */

   simshaped_pulse("",stCshape,2*pw,pwC180,zero,zero,0.0,0.0);

   zgradpulse(gzlvl6, gt6);
   decpwrf(rf0);
   delay(1/(4.0*JCH) -gt6 -pwC180*0.45 -PWRF_DELAY -2.0*GRADIENT_DELAY);	/* d2 = 1/4*Jch */

   rgpulse(pw,zero,0.0,0.0);  

   delay(gt2 +gstab +2.0*GRADIENT_DELAY +POWER_DELAY);

   rgpulse(2*pw,zero,0.0,0.0);  

   if (mag_flg[A] == 'y')  magradpulse(gzcal*gzlvl2, gt2);
     else  zgradpulse(gzlvl2, gt2);
   delay(gstab);

  setreceiver(t4);
   rcvron();
   if ((STUD[A]=='y') && (dm[D] == 'y'))
    {
     decpower(studlvl);
     decprgon(stCdec, 1.0/stdmf, 1.0);
     decon();
    }
   else	
    { 
     decpower(dpwr);
     status(D);
    }
}
