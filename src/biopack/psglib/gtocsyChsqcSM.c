/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gtocsyChsqcSM.c - BioPack pulse sequence adapted for Small Molecules 
                      Eriks Kupce, Oxford, 2005 

    3D C13 TOCSY-HSQC with sensitivity-enhancement in 13C dimension. 

    Uses three channels:
         1)  1H             - carrier @ water  
         2) 13C             - carrier @ 43 ppm  (for aliphatic C-s)
                            - carrier @ 125 ppm (for aromatic C-s)
         3) 15N (optional)  - carrier @ 118 ppm

    Set dm = 'nnny', [13C decoupling during acquisition].

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI
    acquisition in t1 [H]  and t2 [C].

    Set f1180 = 'y' and f2180 = 'y' for (90, -180) in F1 and (90, -180) in  F2.    

          	  DETAILED INSTRUCTIONS FOR USE OF gnoesyChsqcSM
         
    1. Centre H1, C13 and N15 frequencies as required for your sample.

    2. Ensure that H1, C13 and N15 pulse widths (pw, pwC, pwN), power levels 
       (tpwr, pwClvl, pwNlvl) and compression factors (compH, compC, compN) are
       correct. All waveforms are generated and calibrated automatically based 
       on these numbers. 
       
    3. Make sure the gzlvl2 coherence gradient is fine tuned for maximum signal.

    4. N15/C13 decoupling in t1:
       For simultaneous N15/C13 decoupling (CNrefoc='y') the N15 and C13 power
       levels should be reduced by 3dB (parameters pwNlw and pwClw). 
       - pwNlvl and pwClvl can be set manually by the user
       OR
       - if pwNlvl and pwClvl are set to 0, the pulse sequence will estimate their 
	 value from the corresponding hard pulses.

    5. LP in t2:
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
    6. ADIABATIC (WURST) SPINLOCK. 
       Setting the flag wumix='y' uses an adiabatic spinlock. The waveform  
       is generated automatically from within the pulse sequence by Pbox.  
       
    7. WURST DECOUPLING. 
       If the required decoupling bandwidth can not be covered by conventional    
       decoupling schemes, setting the flag wudec='y' overrides the decoupling 
       parameters listed in dg2 and applies WURST decoupling instead. The waveform 
       is generated automatically from within the pulse sequence by Pbox.  
       
    8. PROJECTION-RECONSTRUCTION and TILT experiments:  
       PR and TILT experiments are enabled by setting the projection angle, pra 
       to values between 0 and 90 degrees (0 < pra < 90). Note, that for these 
       experiments axis='ph', ni>1, ni2=0, phase=1,2 and phase2=1,2 must be used. 
       Processing: use wft2dx macro to obtain positive tilt angles and wft2dy for  
       negative tilt angles. For array='phase,phase2' the macros correspond to:
       wft2dx = wft2d(1,0,-1,0,0,-1,0,-1,0,-1,0,-1,-1,0,1,0)
       wft2dy = wft2d(1,0,-1,0,0,1,0,1,0,1,0,1,-1,0,1,0)
       The following relationships can be used to inter-convert the frequencies (in Hz) 
       between the tilted, F1(+)F3, F1(-)F3 and the orthogonal, F1F3, F2F3 planes:       
         F1(+) = F1*cos(pra) + F2*sin(pra)  
         F1(-) = F1*cos(pra) - F2*sin(pra)
         F1 = 0.5*[F1(+) + F1(-)]/cos(pra)
         F2 = 0.5*[F1(+) - F1(-)]/sin(pra)
       References: 
       PROJECTION-RECONSTRUCTION:
       E.Kupce and R.Freeman, J. Amer. Chem. Soc., vol. 125, pp. 13958-13959 (2003).
       E.Kupce and R.Freeman, J. Amer. Chem. Soc., vol. 126, pp. 6429-6440 (2004).
       TILT in small molecules:
       E.Kupce and R.Freeman, J. Magn. Reson., vol. 172, pp. 329-332 (2005).
       E.Kupce, et al, Magn. Reson. Chem., vol. 43, pp. 791-794 (2005).
       
       Eriks Kupce, Oxford, 26.08.2004.       
*/

#include <standard.h>
#include "Pbox_bio.h"

void dipsi(phse1,phse2)
codeint phse1,phse2;
{
        double slpw5, 
               strength=getval("strength");

        if(strength < 0.1)
          strength = 10.0*getval("sfrq");       
        slpw5 = 1.0/(4.0*strength*18.0);

        rgpulse(64*slpw5,phse1,0.0,0.0);
        rgpulse(82*slpw5,phse2,0.0,0.0);
        rgpulse(58*slpw5,phse1,0.0,0.0);
        rgpulse(57*slpw5,phse2,0.0,0.0);
        rgpulse(6*slpw5,phse1,0.0,0.0);
        rgpulse(49*slpw5,phse2,0.0,0.0);
        rgpulse(75*slpw5,phse1,0.0,0.0);
        rgpulse(53*slpw5,phse2,0.0,0.0);
        rgpulse(74*slpw5,phse1,0.0,0.0);

}

static int  phi1[2]  = {0,2},
            phi2[4]  = {0,0,2,2},
            phi3[8]  = {0,0,0,0,2,2,2,2},
            rec[4]   = {0,2,2,0},
            phi5[1]  = {0};
                    
static double d2_init=0.0, d3_init=0.0;

static shape  adC180, wuCdec, wuHmix;

void pulsesequence()
{
/* DECLARE VARIABLES */

  char  aliph[MAXSTR],	        /* aliphatic CHn groups only */
        arom[MAXSTR],		/* aromatic CHn groups only */
        wudec[MAXSTR],		/* automatic WURST decoupling */
        wumix[MAXSTR],		/* automatic WURST mixing */
        CNrefoc[MAXSTR],	/* flag for refocusing 15N during indirect H1 evolution */
        SBSUPR[MAXSTR],	        /* flag for side-band suppression (use 8 step phase cycle) */

        f1180[MAXSTR],	        /* Flag to start t1 @ halfdwell */
        mag_flg[MAXSTR],	/* magic angle gradient */
        f2180[MAXSTR];	        /* Flag to start t2 @ halfdwell */

  int   icosel,		        /* used to get n and p type */
        PRexp,                  /* projection-reconstruction flag */
        t1_counter,		/* used for states tppi in t1 */ 
        t2_counter;		/* used for states tppi in t2 */ 

  double csa, sna, tau1, tau2,	/*  t1 and t2 delays */   
         bw, ofs, ppm, pwd, nst,
         slpw, slpw5, slpwr, cycles,    /* spinlock calculations */

        rfst = 0.0,			/* fine power level for adiabatic pulse initialized */
        strength = getval("strength"),  /* spinlock B1 field strength, in Hz */

        compH = getval("compH"),        /* adjustment for H1  amplifier compression */
        compC = getval("compC"),        /* adjustment for C13 amplifier compression */
        compN = getval("compN"),        /* adjustment for N15 amplifier compression */

        pra = M_PI*getval("pra")/180.0,
        jch = getval("jch"),		/*  CH coupling constant */
        ni2 = getval("ni2"),

        pwC = getval("pwC"),		/* PW90 for 13C nucleus @ pwClvl */
        pwClvl = getval("pwClvl"),	/* high power for 13C hard pulses on dec1  */
        pwC180 = getval("pwC180"),	/* PW180 for 13C nucleus in INEPT transfers */
        pwN = getval("pwN"),		/* PW90 for 15N nucleus @ pwNlvl */
        pwNlvl = getval("pwNlvl"),	/* high power for 15N hard pulses on dec2 */

        pwClw=getval("pwClw"), 
        pwNlw=getval("pwNlw"),
        pwZlw=0.0,			/* largest of pwNlw and 2*pwClw */

        mix  = getval("mix"),		/* tocsy mix time */
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

/* LOAD VARIABLES */

  getstr("aliph",aliph);
  getstr("arom",arom);
  getstr("wudec",wudec);
  getstr("wumix",wumix);
  getstr("CNrefoc",CNrefoc);

  getstr("mag_flg",mag_flg);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("SBSUPR",SBSUPR);


/* LOAD PHASE TABLE */
  settable(t1,2,phi1);
  settable(t2,4,phi2);
  settable(t3,8,phi3);
  settable(t4,4,rec);
  settable(t5,1,phi5);

  if(strength < 0.1)
    strength = 10.0*getval("sfrq");       

  slpw = 1/(4.0 * strength) ;           /* spinlock field strength  */
  slpw5 = slpw/18;
  slpwr = tpwr - 20.0*log10(slpw/(compH*pw));
  slpwr = (int) (slpwr + 0.5);
  cycles = (mix/(2072*slpw5));
  cycles = 2.0*(double)(int)(cycles/2.0);
  initval(cycles, v9);                      /* V9 is the MIX loop count */

/* CHECK VALIDITY OF PARAMETER RANGES */

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

    if( slpwr > 49.0 )
    {
        printf("dont fry the probe, spinlock strength too high ! ");
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

   if( gzlvl3*gzlvl4 > 0.0 ) 

    if (phase1 == 2)
      tsadd(t1,1,4);

    if (phase2 == 1)  {tsadd(t5,2,4);  icosel = +1;}
        else 			       icosel = -1;    

/* set up Projection-Reconstruction experiment */
   
    PRexp = 0;      
    if((pra > 0.0) && (pra < 90.0)) PRexp = 1;

    csa = cos(pra);
    sna = sin(pra);
    
    if(PRexp)
    {
      tau1 = d2*csa;
      tau2 = d2*sna;
    }
    else
    {
      tau1 = d2;
      tau2 = d3;
    }

    if((f1180[A] == 'y') && (ni > 1.0))    /*  Set up f1180  tau1 = t1 */
      tau1 += 1.0/(2.0*sw1);
    tau1 = tau1/2.0;

    if((PRexp == 0) && (f2180[A] == 'y') && (ni2 > 1.0)) /*  Set up f2180  tau2 = t2 */
      tau2 += 1.0/(2.0*sw2);     
    tau2 = tau2/2.0;

    if(tau1 < 0.2e-7) tau1 = 2.0e-7;

/* Calculate modifications to phases for States-TPPI acquisition */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ((d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) { tsadd(t1,2,4); tsadd(t4,2,4); }

   if(PRexp==0)
   {
     if( ix == 1) d3_init = d3 ;
     t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
     if(t2_counter % 2) { tsadd(t2,2,4); tsadd(t4,2,4); } 
   }
    

/* calculate 3db lower power hard pulses for simultaneous CN decoupling during
   indirect H1 evoluion pwNlw and pwClw should be calculated by the macro that 
   calls the experiment. */

   if (CNrefoc[A] == 'y')
   {
     if (pwNlw==0.0) pwNlw = compN*pwN*exp(3.0*2.303/20.0);
     if (pwClw==0.0) pwClw = compC*pwC*exp(3.0*2.303/20.0);
     if (pwNlw > 2.0*pwClw) pwZlw=pwNlw;
     else  pwZlw=2.0*pwClw;
   }


/* make sure gt3 and gt1 have always opposite sign to help dephasing H2O */

   if (gzlvl3*gzlvl1 > 0.0) gsign=-1.0;
     else gsign=1.0; 

   ppm = getval("dfrq"); ofs = 0.0; nst = 1000;     /* number of steps */       

   if(arom[A]=='y')                /* AROMATIC spectrum only */
     bw = 40.0*ppm;
   else if(aliph[A]=='y')          /* ALIPHATIC spectrum only */
     bw = 80.0*ppm;
   else    
   {
     bw = 0.1/(pwC*compC);       /* maximum bandwidth */
     bw = pwC180*bw*bw;  
   } 
     
   if(FIRST_FID)       
     adC180 = pbox_makeA("adC180", "wurst2i", bw, pwC180, ofs, compC*pwC, pwClvl, nst);        
   rfst = adC180.pwrf;

   if(wudec[A]=='y') 
   {
     pwd = 0.0013;
     if(FIRST_FID)
       wuCdec = pbox_Adec("wurstC", "WURST40", bw, pwd, ofs, compC*pwC, pwClvl);
   }  

   if(wumix[A]=='y') 
   {
     pwd = 0.0004; bw=10000.0;
     if(FIRST_FID)
       wuHmix = pbox_Adec("wuHmix", "WURST2m", bw, pwd, ofs, compH*pw, tpwr);
   }  

/* BEGIN ACTUAL PULSE SEQUENCE */


status(A);

   presat();
   obspower(tpwr);		/* Set transmitter power for hard 1H pulses */
   decpower(pwClvl);		/* Set Dec1 power for hard 13C pulses */
   dec2power(pwNlvl);		/* Set Dec2 power for decoupling during tau1 */
   decpwrf(4095.0);       
   dec2pwrf(4095.0);       

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

   zgradpulse(gzlvl0,gt0);
   delay(gstab);
   
   if(wumix[A] == 'y')
     pbox_spinlock(&wuHmix, mix, zero);
   else
   {
     obspower(slpwr);                        /* mixing period */
     if (cycles > 1.0)
     {
       rcvroff();
       starthardloop(v9);
         dipsi(one,three); dipsi(three,one); 
         dipsi(three,one); dipsi(one,three);
       endhardloop();
     }
   }
   decrgpulse(pwC,zero,2.0e-6,2.0e-6); 
   zgradpulse(-gzlvl0,gt0);
   obspower(tpwr);
   decpwrf(rfst);                           /* fine power for inversion pulse */
   delay(gstab);

/* FIRST HSQC INEPT TRANSFER */
   rgpulse(pw,zero,0.0,0.0);
   zgradpulse(gzlvl4, gt4);
   delay(1/(4.0*jch) -gt4 -2.0*GRADIENT_DELAY -WFG2_START_DELAY -pwC180*0.45);

   simshaped_pulse("","adC180",2*pw,pwC180,zero,zero,0.0,0.0);
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
   delay(gt1 +2.0*GRADIENT_DELAY +gstab -2.0*pw -SAPS_DELAY);
   decrgpulse(2*pwC,zero,0.0,0.0);

   if (mag_flg[A] == 'y')  magradpulse(gzcal*gzlvl1, gt1);
     else  zgradpulse(gzlvl1, gt1);
   decphase(t5);
   delay(gstab);

   decrgpulse(pwC,t5,0.0,0.0);
   delay(pw);
   rgpulse(pw,zero,0.0,0.0);

   zgradpulse(gzlvl5, gt5);
   decphase(zero);
   delay(1/(8.0*jch) -gt5 -SAPS_DELAY -2.0*GRADIENT_DELAY);		/* d3 = 1/8*Jch */

   decrgpulse(2.0*pwC,zero,0.0,2.0e-6);
   rgpulse(2.0*pw,zero,0.0,0.0);

   zgradpulse(gzlvl5, gt5);
   decphase(one);
   txphase(one);
   delay(1/(8.0*jch) -gt5 -2.0*SAPS_DELAY -2.0*GRADIENT_DELAY);		/* d3 = 1/8*Jch */

   delay(pwC);
   decrgpulse(pwC,one,0.0,2.0e-6);
   rgpulse(pw,one,0.0,0.0);

   zgradpulse(gzlvl6, gt6);
   decpwrf(rfst);                           /* fine power for inversion pulse */
   decphase(zero);
   txphase(zero);
   delay(1/(4.0*jch) -gt6 -pwC180*0.45 -PWRF_DELAY 
		-WFG2_START_DELAY -2.0*SAPS_DELAY -2.0*GRADIENT_DELAY);	/* d2 = 1/4*Jch */

   simshaped_pulse("","adC180",2*pw,pwC180,zero,zero,0.0,0.0);
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
   if ((wudec[A]=='y') && (dm[D] == 'y'))
   {
     decpower(wuCdec.pwr+3.0);
     decprgon("wurstC", 1.0/wuCdec.dmf, wuCdec.dres);
     decon();
   }
   else	
   { 
     decpower(dpwr);
     status(D);
   }
}
