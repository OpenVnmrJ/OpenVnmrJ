/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gnoesyChsqc_wg

    3D C13 edited noesy with separation via the carbon of the destination site
         recorded on a water sample with watergate 3919 for water suppression.

    NOTE: cnoesy_3c_sed_pfg(1).c 3D C13 edited NOESYs with separation via the
          carbon of the origination site. (D2O experiments)

    Uses three channels:
         1)  1H             - carrier @ water  
         2) 13C             - carrier @ 35 ppm
         3) 15N             - carrier @ 118 ppm

    Set dm = 'nnny', [13C decoupling during acquisition].
    Set dm2 = 'nyny', [15N dec during t1 and acq] 

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI
    acquisition in t1 [H]  and t2 [C].

    Set f1180 = 'y' and f2180 = 'y' for (90, -180) in F1 and (90, -180) in  F2.    
    c180_flg='y' for HH 2D (ni) experiment (fixed d3 (t2) in this case) 
    c180_flg='n' for carbonyl decoupling using wfg during t2 (3D only)
    Note: Zero order phase correct may not be exactly +90 in F2 due to seduce.


    Modified by L. E. Kay to allow for simult N, C acq   July 19, 1993
    original code: noesyc_pfg_h2o_NC_dps.c
    Modified for dps and magic angle gradients D.Mattiello, Varian, 6/8/94
    Modified for vnmr5.2 (new power statements, use status control in t1)
      GG. Palo Alto  16jan96
    Modified to use only z-gradients and only C13 editing (use gnoeysCNhsqc for
     simultaneous editing of N15 and C13
    Modified to permit magic-angle gradients
    Modified to use BioPack-style of C=O decoupling
     
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

static int  phi1[8]  = {0,0,0,0,2,2,2,2},
            phi2[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
            phi3[8]  = {0,0,0,0,2,2,2,2},
            rec[16]  = {0,3,2,1,2,1,0,3,2,1,0,3,0,3,2,1},
            phi5[4]  = {0,1,2,3},
            phi6[2]  = {0,2},
            phi7[4]  = {1,1,3,3};
                    
static double d2_init=0.0, d3_init=0.0;

void pulsesequence()
{
/* DECLARE VARIABLES */

 char     
	    aliph[MAXSTR],	 		 /* aliphatic CHn groups only */
	    arom[MAXSTR], 			  /* aromatic CHn groups only */
            allC[MAXSTR],

            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            mag_flg[MAXSTR],  /* magic angle gradient                     */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell             */
            stCshape[MAXSTR], /* C13 inversion pulse shape name */
            STUD[MAXSTR],     /* Flag to select adiabatic decoupling      */
            stCdec[MAXSTR],   /* contains name of adiabatic decoupling shape */
            CNrefoc[MAXSTR];    /* flag for refocusing 15N during indirect H1 evolution */

 int         
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter;   /* used for states tppi in t2           */ 

double    

  dofa = 0.0,                   /* actual 13C offset (depends on aliph and arom)*/
  rf80 = getval("rf80"), 	/* rf in Hz for 80ppm STUD+ */
  dmf80 = getval("dmf80"),     /* dmf for 80ppm STUD+ */
  rf30 = getval("rf30"),       /* rf in Hz for 30ppm STUD+ */
  dmf30 = getval("dmf30"),     /* dmf for 30ppm STUD+ */
  rf140 = getval("rf140"),
  dmf140= getval("dmf140"),

  stdmf = 1.0,                 /* dmf for STUD decoupling initialized */ 
  studlvl = 0.0,	       /* coarse power for STUD+ decoupling initialized */

  rfst = 0.0,                  /* fine power level for adiabatic pulse initialized*/

            pwC10 = getval("pwC10"),    /* 180 degree selective sinc pulse on CO(174ppm) */
            rf7,	                /* fine power for the pwC10 ("offC10") pulse */
            rf0,                        /* full fine power */
            compC = getval("compC"),         /* adjustment for C13 amplifier compression */
             compN = getval("compN"),         /* adjustment for C13 amplifier compression */
             pwClw=getval("pwClw"),           /* pwC at 3db lower power */
             pwNlw=getval("pwNlw"),           /* pwN at 3db lower power */
             pwZlw=0.0,                       /* largest of pwNlw and 2*pwClw */

	     tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             tauwg,        /* delay for watergate */
             tofwg,        /* offset for watergate */
             jch,          /*  CH coupling constant */
             pwN,          /* PW90 for 15N pulse              */
             pwC,          /* PW90 for c nucleus @ pwClvl         */
             pwC180,       /* PW180 for c nucleus in INEPT transfers */
             pwClvl,        /* power level for 13C pulses on dec1  */
             pwNlvl,       /* high dec2 pwr for 15N hard pulses    */
             mix,          /* noesy mix time                       */
             flippwr,
             tpwrsf_d,
             flippw,
             compH,
             sw1,          /* spectral width in t1 (H)             */
             sw2,          /* spectral width in t2 (C) (3D only)   */
             gstab,
             gt0,
             gt1,
             gt2,
             gt3,
             gt4,
             gt5,
             gt6,
             gt7,
             gzcal,        /* dac to G/cm conversion factor */
             gzlvl0, 
             gzlvl1, 
             gzlvl2, 
             gzlvl3, 
             gzlvl4, 
             gzlvl5,
             gzlvl6,
             gzlvl7; 


/* LOAD VARIABLES */

  getstr("aliph",aliph);
  getstr("arom",arom);
  getstr("CNrefoc",CNrefoc);
  getstr("allC",allC);
  getstr("mag_flg",mag_flg);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("STUD",STUD);

  gstab=getval("gstab");

  mix  = getval("mix");
  sw1  = getval("sw1");
  sw2  = getval("sw2");
  jch = getval("jch"); 
  pwC = getval("pwC");
  pwC180 = getval("pwC180");  
  pwN = getval("pwN");
  pwClvl = getval("pwClvl");
  pwNlvl = getval("pwNlvl");
  tpwrsf_d = getval("tpwrsf_d");
  flippw = getval("flippw");
  compH = getval("compH");
  gzcal = getval("gzcal");
  gt0 = getval("gt0");
  gt1 = getval("gt1");
  gt2 = getval("gt2");
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");
  gt6 = getval("gt6");
  gt7 = getval("gt7");
  gzlvl0 = getval("gzlvl0");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl6 = getval("gzlvl6");
  gzlvl7 = getval("gzlvl7");
  tauwg = getval("tauwg");
  tofwg = getval("tofwg");

/* LOAD PHASE TABLE */
  settable(t1,8,phi1);
  settable(t2,16,phi2);
  settable(t3,8,phi3);
  settable(t4,16,rec);
  settable(t5,4,phi5);
  settable(t6,2,phi6);
  settable(t7,4,phi7);

/* CHECK VALIDITY OF PARAMETER RANGES */

    if ( ((arom[A]=='n') && (aliph[A]=='n') && (allC[A]=='n')) ||
	 ((arom[A]=='y') && (aliph[A]=='y') && (allC[A]=='y')) )
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

    if( dpwr > 50 )
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

    if( dpwr2 > 50 )
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

    if( gt0 > 15e-3 || gt1 > 15e-3 || gt2 > 15e-3 || gt3 > 15e-3 || gt4 > 15e-3 || gt5 > 15e-3 || gt6 > 15e-3 || gt7 > 15e-3 ) 
    {
        printf("gti values < 15e-3\n");
        psg_abort(1);
    } 

/*   if( gzlvl3*gzlvl4 > 0.0 ) 
    {
        printf("gt3 and gt4 must be of opposite sign for optimal water suppression\n");
        psg_abort(1);
     }
*/

/*  Phase incrementation for hypercomplex 2D data */

    if (phase1 == 2)
      tsadd(t1,1,4);
    if (phase2 == 2)
      tsadd(t2,1,4);

/*  Set up f1180  tau1 = t1               */

    tau1 = d2;
    if(f1180[A] == 'y') tau1 += 1.0 / (2.0*sw1);
    if(tau1 < 0.2e-6) tau1 = 4.0e-7;
    tau1 = tau1/2.0;

/*  Set up f2180  tau2 = t2               */
    tau2 = d3;
    if (f2180[A] == 'y') tau2 += 1.0 / (2.0*sw2);
    tau2 = tau2/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) {
      tsadd(t1,2,4);     
      tsadd(t4,2,4);    
    }

   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) {
      tsadd(t2,2,4);  
      tsadd(t4,2,4);    
    }


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
       dofa=dof+90.0*dfrq;		/* dof at 125ppm */
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
       dofa=dof;			/* dof at 35 ppm */
       if (pwC180>3.0*pwC) 
	 {
	   rfst = (compC*4095.0*pwC*4000.0*sqrt((12.07*sfrq/600+3.85)/0.35));
	   rfst = (int) (rfst + 0.5);
         }
       else rfst=4095.0;
     }

/*  TOTAL CARBON spectrum, centered on 70ppm */

if (allC[A]=='y')
   {/* 140 ppm STUD+ decoupling */
        strcpy(stCdec, "stCdec140");
        stdmf = dmf140;
        studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf140);
        studlvl = (int) (studlvl + 0.5);

    /* 200ppm sech/tanh inversion */
        rfst = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35));
        rfst = (int) (rfst + 0.5);
        dofa = dof + 35.0*dfrq;       /* dof at 70 ppm */
        strcpy(stCshape, "stC200");
        if ( 1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n",
            (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) ); psg_abort(1); } 
  
     else rfst=4095.0;
   }


   if( pwC > (24.0e-6*600.0/sfrq) )
	{ printf("Increase pwClvl so that pwC < 24*600/sfrq");
	  psg_abort(1); }

    /* maximum fine power for pwC pulses */
	rf0 = 4095.0;

    /* "offC10": 180 degree one-lobe sinc pulse on CO, null at Ca 139ppm away */
	rf7 = (compC*4095.0*pwC*2.0*1.65)/pwC10;	/* needs 1.65 times more     */
	rf7 = (int) (rf7 + 0.5);		/* power than a square pulse */

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


    /* H2Osinc_d.RF or H2Osinc_u.RF flipdown pulse */
         flippwr = tpwr - 20.0*log10(flippw/(pw*compH*1.69));
         flippwr = (int) (flippwr +0.5);
    
/* BEGIN ACTUAL PULSE SEQUENCE */


status(A);
   decoffset(dofa);
   decpower(pwClvl);  /* Set Dec1 power for hard 13C pulses         */
   dec2power(dpwr2);       /* Set Dec2 to low power for decoupling */
   delay(d1);


   obspower(tpwr);           /* Set transmitter power for hard 1H pulses */
   rcvroff();
status(B);
   obsstepsize(45.0);
   initval(7.0,v7);
   xmtrphase(v7);
   delay(.0001);

   getelem(t1,ct,v1);
   add(v1,two,v1);
   obspower(flippwr);
   if (tpwrsf_d<4095.0) flippwr=flippwr+6.0;
   obspower(flippwr); obspwrf(tpwrsf_d);
   shaped_pulse("H2Osinc_d",flippw,v1,20e-6,rof1);
   obspwrf(4095.0); obspower(tpwr);
   rgpulse(pw,t1,1.0e-6,0.0);                  /* 90 deg 1H pulse */
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

       decpower(pwClvl);
       delay(tau1 -pwZlw -2.0*pw/PI -SAPS_DELAY -POWER_DELAY -2.0*rof1);
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


status(C);
   rgpulse(pw,zero,2.0e-6,0.0);             /*  2nd 1H 90 pulse   */
   obspwrf(tpwrsf_d); obspower(flippwr);
   shaped_pulse("H2Osinc_d",flippw,two,20e-6,rof1);
   obspwrf(4095.0); obspower(tpwr);


     delay(mix - 10.0e-3-flippw ); 

   zgradpulse(gzlvl0, gt0);
   delay(gstab);
   decrgpulse(pwC,zero,102.0e-6,2.0e-6); 
   decpwrf(rfst);                           /* power for inversion pulse */
   zgradpulse(gzlvl1, gt1);
   delay(10.0e-3 - gt1 - gt0 - 8.0e-6i-gstab);
   delay(150.0e-6);

  /* obspwrf(tpwrsf_d); obspower(flippwr);
   shaped_pulse("H2Osinc_d",flippw,zero,20e-6,rof1);
   obspwrf(4095.0); obspower(tpwr);*/
   rgpulse(pw,zero,0.0,2.0e-6);
   zgradpulse(gzlvl2, gt2);       /* g3 in paper   */
   delay(1/(4.0*jch) - gt2 - 2.0e-6 -pwC180/2.0);
   if (pwC180>3.0*pwC)
    simshaped_pulse("",stCshape,2*pw,pwC180,zero,zero,0.0,2.0e-6);
   else
    simpulse(2.0*pw,2.0*pwC,zero,zero,0.0,2.0e-6);
   zgradpulse(gzlvl2, gt2);       /* g4 in paper  */
    decphase(zero);
   decpwrf(4095.0);
   delay(1/(4.0*jch) - gt2 - 2.0e-6 -pwC180/2.0);
   rgpulse(pw,one,1.0e-6,2.0e-6);
                if (mag_flg[A] == 'y')
                {
                   magradpulse(gzcal*gzlvl3, gt3);
                }
                else
                {
                   zgradpulse(gzlvl3, gt3);
                }
   delay(gstab);
   decrgpulse(pwC,t2,rof1,2.0e-6);   

   if( tau2 > 0.0 ) 
    {
        decphase(zero);
	decpwrf(rf7);
	delay(tau2);
        if (aliph[A] == 'y')
        simshaped_pulse("","offC10",2.0*pw, pwC10,zero, zero, 0.0, 0.0);
        else
         rgpulse(2.0*pw,zero,0.0,0.0);
        decpwrf(4095.0);
        decphase(zero);
        delay(tau2);
    }
   else
   {delay(tau2*2);
   /*rgpulse(2*pw,zero,2.0e-6,2.0e-6);*/
   }  
   decrgpulse(pwC,zero,2.0e-6,2.0e-6);
                if (mag_flg[A] == 'y')
                {
                   magradpulse(gzcal*gzlvl4, gt4);
                }
                else
                {
                   zgradpulse(gzlvl4, gt4);
                }
   delay(gstab);
   rgpulse(pw,t5,rof1,2.0e-6);
   zgradpulse(gzlvl5, gt5);
   if (pwC180>3.0*pwC)
    {
    obsoffset(tofwg);
    decpwrf(rfst);
    delay(1/(4.0*jch) - gt5 - 2.0e-6 -pwC180/2.0 - 2.385*pw - 2.5*tauwg + 70.0e-6);
    rgpulse(pw*0.231,v2,0.0,0.0);
    delay(tauwg);
    rgpulse(pw*0.692,v2,0.0,0.0);
    delay(tauwg);
    rgpulse(pw*1.462,v2,0.0,0.0);
    delay(tauwg/2.0);
    simshaped_pulse("",stCshape,0.0,pwC180,zero,zero,0.0,2.0e-6);
    decphase(zero);
    delay(tauwg/2.0);
    rgpulse(pw*1.462,v3,0.0,0.0);
    delay(tauwg);
    rgpulse(pw*0.692,v3,0.0,0.0);
    delay(tauwg);
    rgpulse(pw*0.231,v3,0.0,0.0);
    decpwrf(4095.0);
    obsoffset(tof);
    zgradpulse(gzlvl5, gt5);
    delay(1/(4.0*jch) -  gt5 - 2.0e-6 -pwC180/2.0 - 2.385*pw - 2.5*tauwg + 70.0e-6);
    }
   else
    {
    delay(1/(4.0*jch) - gt5 - 2.0e-6 -pwC);
    simpulse(2.0*pw,2.0*pwC,zero,zero,0.0,2.0e-6);
    zgradpulse(gzlvl5, gt5);
    delay(1/(4.0*jch) -  gt5 -2.0*pwC - 6.0e-6 -pwC);
    }
 /*  decrgpulse(pwC,zero,0.0,0.0);     
   decrgpulse(pwC,t3,2.0e-6,0.0);     
   rgpulse(pw,t5,1.0e-6,rof2);  */                      /* flip-back pulse  */
   rcvron();
   setreceiver(t4);
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
