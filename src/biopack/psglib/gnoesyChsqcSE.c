/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gnoesyChsqcSE.c

    3D C13 edited noesy with separation via the carbon of the destination site
    recorded on a water sample with Sensitivity-Enhancement in 13C dimension. 

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
       For simultaneous N15/C13 decoupling (CNrefoc='y') the N15 and C13 power
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
       
     PROJECTION-RECONSTRUCTION option:  
       Projection-Reconstruction experiments are enabled by setting the projection 
       angle, pra to values between 0 and 90 degrees, 0 < pra < 90. Note that for 
       these experiments axis='ph', ni>1, ni2=0, phase=1,2 and phase2=1,2 must be used. 
       Processing: use wft2dx macro for positive tilt angles and wft2dy for negative 
       tilt angles. 
       wft2dx = wft2d(1,0,-1,0,0,1,0,1,0,1,0,1,-1,0,1,0)
       wft2dy = wft2d(1,0,-1,0,0,-1,0,-1,0,1,0,1,1,0,-1,0)
       The following relationships can be used to inter-convert the frequencies (in Hz) 
       between the tilted, F1(+)F3, F1(-)F3 and the orthogonal, F1F3, F2F3 planes:              
         F1(+) = F1*cos(pra) + F2*sin(pra)  
         F1(-) = F1*cos(pra) - F2*sin(pra)
         F1 = 0.5*[F1(+) + F1(-)]/cos(pra)
         F2 = 0.5*[F1(+) - F1(-)]/sin(pra)
       References: 
       E.Kupce and R.Freeman, J. Amer. Chem. Soc., vol. 125, pp. 13958-13959 (2003).
       E.Kupce and R.Freeman, J. Amer. Chem. Soc., vol. 126, pp. 6429-6440 (2004).
       Related:
       S.Kim and T.Szyperski, J. Amer. Chem. Soc., vol. 125, pp. 1385-1393 (2003).
       Eriks Kupce, Oxford, 26.08.2004.
*/

#include <standard.h>

static int  phi1[2]  = {0,2},
            phi2[4]  = {0,0,2,2},
            phi3[8]  = {0,0,0,0,2,2,2,2},
            rec[4]   = {0,2,2,0},
            phi5[1]  = {0};
                    
static double d2_init=0.0, d3_init=0.0;

void pulsesequence()
{
/* DECLARE VARIABLES */

char     
  aliph[MAXSTR],	/* aliphatic CHn groups only */
  arom[MAXSTR],		/* aromatic CHn groups only */
  CNrefoc[MAXSTR],	/* flag for refocusing 15N during indirect H1 evolution */
  SBSUPR[MAXSTR],	/* flag for side-band suppression (use 8 step phase cycle) */

  f1180[MAXSTR],	/* Flag to start t1 @ halfdwell */
  mag_flg[MAXSTR],	/* magic angle gradient */
  f2180[MAXSTR],	/* Flag to start t2 @ halfdwell */
  stCshape[MAXSTR],	/* C13 inversion pulse shape name */
  STUD[MAXSTR],		/* Flag to select adiabatic decoupling */
  stCdec[MAXSTR];	/* contains name of adiabatic decoupling shape */

int         
  icosel,		/* used to get n and p type */
  PRexp,                /* projection-reconstruction flag */
  t1_counter,		/* used for states tppi in t1 */ 
  t2_counter;		/* used for states tppi in t2 */ 

double    
  ni2 = getval("ni2"),
  dofa = 0.0,			/* actual 13C offset (depends on aliph and arom)*/
  rf80 = getval("rf80"),	/* rf in Hz for 80ppm STUD+ */
  dmf80 = getval("dmf80"),	/* dmf for 80ppm STUD+ */
  rf30 = getval("rf30"),	/* rf in Hz for 30ppm STUD+ */
  dmf30 = getval("dmf30"),	/* dmf for 30ppm STUD+ */

  stdmf = 1.0,			/* dmf for STUD decoupling initialized */ 
  studlvl = 0.0,		/* coarse power for STUD+ decoupling initialized */

  rfst = 0.0,			/* fine power level for adiabatic pulse initialized */
  rf0,				/* full fine power */

  /*compH = getval("compH"),       adjustment for H1  amplifier compression */
  compC = getval("compC"),      /* adjustment for C13 amplifier compression */
  compN = getval("compN"),      /* adjustment for N15 amplifier compression */

  tau1,				/*  t1 delay */
  tau2,				/*  t2 delay */
  csa, sna,
  pra = M_PI*getval("pra")/180.0,
  jch = getval("jch"),		/*  CH coupling constant */

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
  gt4 = getval("gt4"),
  gt5 = getval("gt5"),
  gt6 = getval("gt6"),
  gzlvl0 = getval("gzlvl0"),
  gzlvl1 = getval("gzlvl1"),
  gzlvl2 = getval("gzlvl2"),
  gzlvl3 = getval("gzlvl3"),
  gzlvl4 = getval("gzlvl4"),
  gzlvl5 = getval("gzlvl5"),
  gzlvl6 = getval("gzlvl6");

  csa = cos(pra);
  sna = sin(pra);

/* LOAD VARIABLES */

  getstr("aliph",aliph);
  getstr("arom",arom);
  getstr("CNrefoc",CNrefoc);

  getstr("mag_flg",mag_flg);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("STUD",STUD);
  getstr("SBSUPR",SBSUPR);


/* LOAD PHASE TABLE */
  settable(t1,2,phi1);
  settable(t2,4,phi2);
  settable(t3,8,phi3);
  settable(t4,4,rec);
  settable(t5,1,phi5);


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


/*  Set phases for States acquisition in the indirect dimensions */

    if (phase1 == 2)
      tsadd(t1,1,4);

    if (phase2 == 1)  {tsadd(t5,2,4);  icosel = +1;}
        else 			       icosel = -1;    

/*  Set up f1180  tau1 = t1               */
   
    PRexp = 0;
    if((pra > 0.0) && (pra < 90.0)) PRexp = 1;
    
    if(PRexp)                /* set up Projection-Reconstruction experiment */
      tau1 = d2*csa;
    else
      tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) tau1 += 1.0/(2.0*sw1);
    if(tau1 < 0.2e-6) tau1 = 4.0e-7;
    tau1 = tau1/2.0;

/*  Set up f2180  tau2 = t2               */
    
    if(PRexp)
      tau2 = d2*sna;
    else
    {
      tau2 = d3;
      if((f2180[A] == 'y') && (ni2 > 1.0)) tau2 += ( 1.0 / (2.0*sw2) ); 
    }
    tau2 = tau2/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) {
      tsadd(t1,2,4);     
      tsadd(t4,2,4);    
    }

   if(PRexp==0)
   {
     if( ix == 1) d3_init = d3 ;
     t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
     if(t2_counter % 2) {
        tsadd(t2,2,4);  
        tsadd(t4,2,4);    
     } 
   }
    

/* calculate 3db lower power hard pulses for simultaneous CN decoupling during
   indirect H1 evoluion pwNlw and pwClw should be calculated by the macro that 
   calls the experiment. */

  if (CNrefoc[A] == 'y')
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


/* make sure gt3 and gt1 have always opposite sign to help dephasing H2O */
   if (gzlvl3*gzlvl1 > 0.0) gsign=-1.0;
     else gsign=1.0; 


/* AROMATIC spectrum */
   if (arom[A]=='y')
     {
       /* 30 ppm STUD+ decoupling */
       strcpy(stCdec, "stCdec30");
       stdmf = dmf30;
       studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf30);
       studlvl = (int) (studlvl + 0.5);

       /* 30ppm sech/tanh inversion */
       strcpy(stCshape, "stC30");
       rfst = (compC*4095.0*pwC*4000.0*sqrt((4.5*sfrq/600.0+3.85)/0.41));   
       rfst = (int) (rfst + 0.5);
     }


/* ALIPHATIC spectrum */
   if (aliph[A]=='y')
     {
       /* 80 ppm STUD+ decoupling */
       strcpy(stCdec, "stCdec80");
       stdmf = dmf80;
       studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf80);
       studlvl = (int) (studlvl + 0.5);

       /* 80ppm sech/tanh inversion pulse */
       strcpy(stCshape, "stC80");
       if (pwC180>3.0*pwC) 
	 {
	   rfst = (compC*4095.0*pwC*4000.0*sqrt((12.07*sfrq/600+3.85)/0.35));
	   rfst = (int) (rfst + 0.5);
         }
       else rfst=4095.0;
     }

   if( pwC > (24.0e-6*600.0/sfrq) )
	{ printf("Increase pwClvl so that pwC < 24*600/sfrq");
	  psg_abort(1); }

/* maximum fine power for pwC pulses */
	rf0 = 4095.0;

/* BEGIN ACTUAL PULSE SEQUENCE */


status(A);
   if (arom[A] == 'y') 
    dofa=dof+(125-43)*dfrq;
   else
    dofa=dof;
   decoffset(dofa);

   obspower(tpwr);		/* Set transmitter power for hard 1H pulses */
   decpower(pwClvl);		/* Set Dec1 power for hard 13C pulses */
   dec2power(pwNlvl);		/* Set Dec2 power for decoupling during tau1 */
   dec2pwrf(4095.0);       

   initval(135.0,v1);
   obsstepsize(1.0);
   xmtrphase(v1);


   delay(d1);

/* destroy N15 and C13 magnetization */
   if (CNrefoc[A] == 'y') dec2rgpulse(pwN, zero, 0.0, 0.0);
   decrgpulse(pwC, zero, 0.0, 0.0);
   zgradpulse(gzlvl0, 0.5e-3);
   delay(gstab);
   if (CNrefoc[A] == 'y') dec2rgpulse(pwN, one, 0.0, 0.0);
   decrgpulse(pwC, one, 0.0, 0.0);
   zgradpulse(0.7*gzlvl0, 0.5e-3);

   decphase(zero);       
   dec2phase(zero);       
   rcvroff();
   delay(gstab);


status(B);
   rgpulse(pw, t1, rof1 ,rof1);                  /* 90 deg 1H pulse */
   txphase(zero); 
   xmtrphase(zero);

   if (ni > 0) 
    {
     if ((CNrefoc[A]=='y') && (tau1 > pwZlw +2.0*pw/PI +3.0*SAPS_DELAY +2.0*POWER_DELAY +2.0*rof1))
      {
       decpower(pwClvl-3.0); dec2power(pwNlvl-3.0);
       delay(tau1 -pwNlw -2.0*pw/PI -3.0*SAPS_DELAY -2.0*POWER_DELAY -2.0*rof1);

       if (pwNlw > 2.0*pwClw)
         {
	  dec2rgpulse(pwNlw -2.0*pwClw,zero,rof1,0.0);
	  sim3pulse(0.0,pwClw,pwClw,zero,zero,zero,0.0,0.0);
          decphase(one);
	  sim3pulse(0.0,2*pwClw,2*pwClw,zero,one,zero,0.0,0.0);
          decphase(zero);
	  sim3pulse(0.0,pwClw,pwClw,zero,zero,zero,0.0,0.0);
	  dec2rgpulse(pwNlw -2.0*pwClw,zero,0.0,rof1);
         }
	else
         {
	  decrgpulse(2.0*pwClw-pwNlw,zero,rof1,0.0);
	  sim3pulse(0.0,pwNlw-pwClw,pwNlw-pwClw,zero,zero,zero,0.0,0.0);
          decphase(one);
	  sim3pulse(0.0,2.0*pwClw,2.0*pwClw,zero,one,zero,0.0,0.0);
          decphase(zero);
	  sim3pulse(0.0,pwNlw-pwClw,pwNlw-pwClw,zero,zero,zero,0.0,0.0);
	  decrgpulse(2.0*pwClw-pwNlw,zero,0.0,rof1);
         }

       decpower(pwClvl); dec2power(pwNlvl);
       delay(tau1 -pwZlw -2.0*pw/PI -SAPS_DELAY -2.0*POWER_DELAY -2.0*rof1);
      }     
     else if (tau1 > 2.0*pwC +2.0*pw/PI +3.0*SAPS_DELAY +2.0*rof1)
      {
       delay(tau1 -2.0*pwC -2.0*pw/PI -3.0*SAPS_DELAY -2.0*rof1);

       decrgpulse(pwC, zero, rof1, 0.0);
       decphase(one);
       decrgpulse(2.0*pwC, one, 0.0, 0.0);
       decphase(zero);
       decrgpulse(pwC, zero, 0.0, rof1);

       delay(tau1 -2.0*pwC -2.0*pw/PI -SAPS_DELAY -2.0*rof1);
      }
     else if (tau1 > 2.0*pw/PI +2.0*SAPS_DELAY +rof1)
	  delay(2.0*tau1 -4.0*pw/PI -2.0*SAPS_DELAY -2.0*rof1);
    }

   rgpulse(pw, zero, rof1, rof1);             /*  2nd 1H 90 pulse   */
status(C);
   dec2power(dpwr2);

   delay(mix -pwC -gt0 -POWER_DELAY -PWRF_DELAY -gstab -2.0*GRADIENT_DELAY); 

   decrgpulse(pwC,zero,0.0,0.0); 
   zgradpulse(gzlvl0, gt0);
   decpwrf(rfst);                           /* fine power for inversion pulse */
   delay(gstab);

/* FIRST HSQC INEPT TRANSFER */
   rgpulse(pw,zero,0.0,0.0);
   zgradpulse(gzlvl4, gt4);
   delay(1/(4.0*jch) -gt4 -2.0*GRADIENT_DELAY -WFG2_START_DELAY -pwC180*0.45);

   simshaped_pulse("",stCshape,2*pw,pwC180,zero,zero,0.0,0.0);
   decphase(zero);

   zgradpulse(gzlvl4, gt4);
   decpwrf(4095.0);
   txphase(one);
   delay(1/(4.0*jch) -gt4 -2.0*GRADIENT_DELAY -pwC180*0.45 -PWRF_DELAY -SAPS_DELAY);

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
   delay(gt1 +gstab -2.0*pw);

   decrgpulse(2*pwC,zero,0.0,0.0);

   if (mag_flg[A] == 'y')  magradpulse(gzcal*gzlvl1, gt1);
     else  zgradpulse(gzlvl1, gt1);
   decphase(t5);
   delay(gstab -2.0*GRADIENT_DELAY);

   decrgpulse(pwC,t5,0.0,0.0);
   delay(4.0*pwC/PI -pw);
   rgpulse(pw,zero,0.0,0.0);

   zgradpulse(gzlvl5, gt5);
   decphase(zero);
   delay(1/(8.0*jch) -gt5 -SAPS_DELAY -2.0*GRADIENT_DELAY);		/* d3 = 1/8*Jch */

   simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

   zgradpulse(gzlvl5, gt5);
   decphase(one);
   txphase(one);
   delay(1/(8.0*jch) -gt5 -2.0*SAPS_DELAY -2.0*GRADIENT_DELAY);		/* d3 = 1/8*Jch */

   simpulse(pw, pwC, one, one, 0.0, 0.0);

   zgradpulse(gzlvl6, gt6);
   decpwrf(rfst);                           /* fine power for inversion pulse */
   decphase(zero);
   txphase(zero);
   delay(1/(4.0*jch) -gt6 -pwC180*0.45 -PWRF_DELAY 
		-WFG2_START_DELAY -2.0*SAPS_DELAY -2.0*GRADIENT_DELAY);	/* d2 = 1/4*Jch */

   simshaped_pulse("",stCshape,2*pw,pwC180,zero,zero,0.0,0.0);
   decphase(zero);

   zgradpulse(gzlvl6, gt6);
   decpwrf(4095.0);
   delay(1/(4.0*jch) -gt6 -pwC180*0.45 -PWRF_DELAY -2.0*GRADIENT_DELAY);	/* d2 = 1/4*Jch */

   rgpulse(pw,zero,0.0,0.0);  

   if (SBSUPR[A]=='y') delay(gt2 +gstab +2.0*GRADIENT_DELAY +2.0*pwC 
					+SAPS_DELAY +rof2 +POWER_DELAY);
   else delay(gt2 +gstab +2.0*GRADIENT_DELAY +POWER_DELAY);

   rgpulse(2*pw,zero,0.0,0.0);  

   if (mag_flg[A] == 'y')  magradpulse(icosel*gzcal*gzlvl2, gt2);
     else  zgradpulse(icosel*gzlvl2, gt2);
   delay(gstab);

   if (SBSUPR[A]=='y') {
       decrgpulse(pwC,zero,0.0,0.0);     
       decphase(t3);
       decrgpulse(pwC,t3,0.0,rof2);     
      }

   setreceiver(t4);
   rcvron();
   if ((STUD[A]=='y') && (dm[D] == 'y'))
    {
        decpower(studlvl);
        decunblank();
        decon();
        decprgon(stCdec,1/stdmf, 1.0);
        startacq(alfa);
        acquire(np, 1.0/sw);
        decprgoff();
        decoff();
        decblank();
    }
   else	
    { 
     decpower(dpwr);
     status(D);
    }
}
