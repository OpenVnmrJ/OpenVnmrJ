/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gtocsyChsqcA.c   (aliphatic carbons)

    3D C13 edited tocsy with separation via the carbon of the destination site
         recorded on a water sample 

    NOTE: cnoesy_3c_sed_pfg(1).c 3D C13 edited NOESYs with separation via the
          carbon of the origination site. (D2O experiments)

    Uses three channels:
         1)  1H             - carrier @ water  
         2) 13C             - carrier @ 43 ppm (CA/CB)
         3) 15N             - carrier @ 118 ppm

    Set dm = 'nnny', [13C decoupling during acquisition].
    Set dm2 = 'nynn', [15N dec during t1 ] 
    (C13 is decoupled in t1 by a 180 degree composite pulse)

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI
    acquisition in t1 [H]  and t2 [C].

    Set f1180 = 'y' and f2180 = 'y' for (90, -180) in F1 and (90, -180) in  F2.    
    Note: Zero order phase correct may not be exactly +90 in F2 due to seduce.

    First several points along ni2 may have intensity distortions leading to
    curving baseline, even though lp2=0 or -180. For better baselines, linear
    predict the first several points (e.g. setlp2(128,64,3) for extending from
    64 to 128 points and predicting the first 3).

    Modified by L. E. Kay to allow for simult N, C acq   July 19, 1993
    original code: noesyc_pfg_h2o_NC_dps.c
    Modified for dps and magic angle gradients D.Mattiello, Varian, 6/8/94
    Modified for vnmr5.2 (new power statements, use status control in t1)
      GG. Palo Alto  16jan96
    Modified to use only z-gradients and only C13 editing (use wCNnoesy for
     simultaneous editing of N15 and C13
    Modified to permit magic-angle gradients
    Modified to use BioPack-style of C=O decoupling
    Auto-calibrated version, E.Kupce, 27.08.2002.

    Added SEAD,gstab  G.G. 6may2004
       Sideband elimination by adiabatic defocussing (SEAD) option and J-compensated
       adiabatic pulses 
       The decoupling power level can be adjusted by changing seadpw. The later should 
       not exceed 2 ms. The efficiency of sideband suppression can be verified by
       setting sead = 'n' and dseq='wusead' (use macro pxset('wusead.DEC') for
       convenience.
       Ref. E.Kupce, J.Magn.Reson., vol. 129, pp. 219-221 (1997).
    
    Derived from gnoesyChsqc.c proteinpack sequence [Krish]
    
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

     AUTOCAL and CHECKOFS.
       The autocal and checkofs flags are generated automatically in Pbox_bio.h
       If these flags do not exist in the parameter set, they are automatically 
       set to 'y' - yes. In order to change their default values, create the  
       flag(s) in your parameter set and change them as required. 
       The available options for the checkofs flag are: 'y' (yes) and 'n' (no). 
       The offset (tof, dof, dof2 and dof3) checks can be switched off  
       individually by setting the corresponding argument to zero (0.0).
       For the autocal flag the available options are: 'y' (yes - by default), 
       'q' (quiet mode - suppress Pbox output), 'r' (read from file, no new  
       shapes are created), 's' (semi-automatic mode - allows access to user  
       defined parameters) and 'n' (no - use full manual setup, equivalent to 
       the original code).
*/

#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */

/* Chess - CHEmical Shift Selective Suppression */
void Chess(pulsepower,pulseshape,duration,phase,rx1,rx2,gzlvlw,gtw,gswet)
double pulsepower,duration,rx1,rx2,gzlvlw,gtw,gswet;
  codeint phase;
  char* pulseshape;
{
  obspwrf(pulsepower);
  shaped_pulse(pulseshape,duration,phase,rx1,rx2);
  zgradpulse(gzlvlw,gtw);
  delay(gswet);
}

/* Wet4 - Water Elimination */
void Wet4(phaseA,phaseB)
  codeint phaseA,phaseB;
{
  double finepwr,gzlvlw,gtw,gswet,gswet2,wetpwr,wetpw,dz;
  char   wetshape[MAXSTR];
  getstr("wetshape",wetshape);    /* Selective pulse shape (base)  */
  wetpwr=getval("wetpwr");        /* User enters power for 90 deg. */
  wetpw=getval("wetpw");        /* User enters power for 90 deg. */
  dz=getval("dz");
  finepwr=wetpwr-(int)wetpwr;     /* Adjust power to 152 deg. pulse*/
  wetpwr=(double)((int)wetpwr);
  if (finepwr==0.0) {wetpwr=wetpwr+5; finepwr=4095.0; }
  else {wetpwr=wetpwr+6; finepwr=4095.0*(1-((1.0-finepwr)*0.12)); }
  rcvroff();
  obspower(wetpwr);         /* Set to low power level        */
  gzlvlw=getval("gzlvlw");      /* Z-Gradient level              */
  gtw=getval("gtw");            /* Z-Gradient duration           */
  gswet=getval("gswet");        /* Post-gradient stability delay */
  gswet2=getval("gswet2");        /* Post-gradient stability delay */
  Chess(finepwr*0.5059,wetshape,wetpw,phaseA,20.0e-6,rof2,gzlvlw,gtw,gswet);
  Chess(finepwr*0.6298,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/2.0,gtw,gswet);
  Chess(finepwr*0.4304,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/4.0,gtw,gswet);
  Chess(finepwr*1.00,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/8.0,gtw,gswet2);
  obspower(tpwr); obspwrf(tpwrf);     /* Reset to normal power level   */
  rcvron();
  delay(dz);
}
void dipsi(phse1,phse2)
codeint phse1,phse2;
{
        double slpw5;
        slpw5 = 1.0/(4.0*getval("strength")*18.0);

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

static int  phi1[8]  = {0,0,0,0,2,2,2,2},
            phi2[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
            phi3[8]  = {0,0,0,0,2,2,2,2},
            rec[16]  = {0,3,2,1,2,1,0,3,2,1,0,3,0,3,2,1},
            phi5[4]  = {0,1,2,3},
            phi6[2]  = {0,2},
            phi7[4]  = {1,1,3,3},
            phi8[8]  = {2,2,2,2,0,0,0,0},
            phi9[8]  = {0,0,0,0,2,2,2,2};
                    
static double d2_init=0.0, d3_init=0.0;
static double   H1ofs=4.7, C13ofs=46.0, N15ofs=120.0, H2ofs=0.0;

static shape offC10, stC80;

void pulsesequence()
{
/* DECLARE VARIABLES */

 char     
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            mag_flg[MAXSTR],  /* magic angle gradient                     */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell             */
            wet[MAXSTR],      /* Flag to select optional WET water suppression */
            C180[MAXSTR],     /* C13 inversion pulse name */
            STUD[MAXSTR],     /* Flag to select adiabatic decoupling      */
            sead[MAXSTR],           /* Flag to eliminate decoupling sidebands */
            seadpat[MAXSTR],                                    /* SEAD shape */
            seaddseq[MAXSTR],                     /* SEAD decoupling sequence */
            stCdec[MAXSTR];   /* contains name of adiabatic decoupling shape */

 int         
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter;   /* used for states tppi in t2           */ 

double    
            stdmf = getval("dmf80"),     /* dmf for 80 ppm of STUD decoupling */
            rf80 = getval("rf80"),       	  /* rf in Hz for 80ppm STUD+ */
            rfst,                        /* fine power level for adiabatic pulse */
            studlvl,	                         /* coarse power for STUD+ decoupling */
            pwC10 = getval("pwC10"),    /* 180 degree selective sinc pulse on CO(174ppm) */
            idel, tcorr1, tcorr2,
            nsteps = 4.0,
            seadpw = getval("seadpw"),
            seadpwrf = getval("seadpwrf"),          
            rf7,	                /* fine power for the pwC10 ("offC10") pulse */
            rf0,                        /* full fine power */
            compC = getval("compC"),         /* adjustment for C13 amplifier compression */
                               /* temporary Pbox parameters */
             bw, pws, ofs, ppm, nst,  /* bandwidth, pulsewidth, offset, ppm, # steps */
             tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             corr,         /*  correction for t2  */
             jch,          /*  CH coupling constant */
             pwN,          /* PW90 for 15N pulse              */
             pwC,          /* PW90 for c nucleus @ pwClvl         */
             pwC180,       /* PW180 for c nucleus in INEPT transfers */
             pwClvl,        /* power level for 13C pulses on dec1  */
             pwNlvl,       /* high dec2 pwr for 15N hard pulses    */
             mix,          /* noesy mix time                       */
             sw1,          /* spectral width in t1 (H)             */
             sw2,          /* spectral width in t2 (C) (3D only)   */
             slpwr,
             slpw,
             slpw5,
             strength,
             compH,
             cycles,
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

  getstr("wet",wet);
  getstr("mag_flg",mag_flg);
  getstr("C180",C180);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("STUD",STUD);
  getstr("sead",sead);
  getstr("seadpat",seadpat);
  getstr("seaddseq", seaddseq);

    mix  = getval("mix");
    sw1  = getval("sw1");
    sw2  = getval("sw2");
    strength = getval("strength");
    compH = getval("compH");
  jch = getval("jch"); 
  pwC = getval("pwC");
  pwC180 = getval("pwC180");  
  pwN = getval("pwN");
  pwClvl = getval("pwClvl");
  pwNlvl = getval("pwNlvl");
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

/* LOAD PHASE TABLE */
  settable(t1,8,phi1);
  settable(t2,16,phi2);
  settable(t3,8,phi3);
  settable(t4,16,rec);
  settable(t5,4,phi5);
  settable(t6,2,phi6);
  settable(t7,4,phi7);
  settable(t8,8,phi8);
  settable(t9,8,phi9);

/*  SEAD delays and cycles */

     idel = seadpw/nsteps;
     tcorr2 = (nsteps-1.0)*idel;
     
     if (pwC180>3.0*pwC)
       tcorr1 = tcorr2 - (seadpw - pwC180);
     else
       tcorr1 = tcorr2 - (seadpw - 2.0*pwC);
       
     initval(nsteps, v11);
     initval(nsteps-1.0, v8);
     modn(ct, v11, v12);		/* v12 = 0,1,2,3 ... N-1 */
     add(v8, v12, v13);			/* v13 = 3,4,5,6 ... N-1 */
     sub(v8, v12, v6); 	        	/* v6  = N-1 ... 3,2,1,0 */

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

   if( gzlvl3*gzlvl4 > 0.0 ) 
    {
        printf("gt3 and gt4 must be of opposite sign for optimal water suppression\n");
        psg_abort(1);
     }

/*  Phase incrementation for hypercomplex 2D data */

    if (phase1 == 2)
      tsadd(t1,1,4);
    if (phase2 == 2)
      tsadd(t2,1,4);

/*  Set up f1180  tau1 = t1               */
   
    tau1 = d2;
    if(f1180[A] == 'y') {
        tau1 += ( 1.0 / (2.0*sw1) - 4.0/PI*pw - 4.0*pwC - 6.0e-6);
    }

    else tau1 = tau1 - (4.0/PI*pw + 4.0*pwC + 6.0e-6);

    if(tau1 < 0.2e-6) tau1 = 4.0e-7;

    tau1 = tau1/2.0;

/*  Set up f2180  tau2 = t2               */
    corr = (4.0/PI)*pwC + pwC10 +WFG2_START_DELAY +4.0e-6;

    tau2 = d3;
    if(f2180[A] == 'y')
     {
      tau2 += ( 1.0 / (2.0*sw2) -corr); 
     }

    else
     tau2 = tau2 - corr; 
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

    /* maximum fine power for pwC pulses */
	rf0 = 4095.0;

      setautocal();                        /* activate auto-calibration flags */ 
        
      if (autocal[0] == 'n') 
      {
        /* "offC10": 180 degree one-lobe sinc pulse on CO, null at Ca 139ppm away */
	  rf7 = (compC*4095.0*pwC*2.0*1.65)/pwC10;     /* needs 1.65 times more     */
	  rf7 = (int) (rf7 + 0.5);		           /* power than a square pulse */

        if (pwC180>3.0*pwC)   /*80ppm sech/tanh "stC80" inversion pulse*/
        {
          rfst = (compC*4095.0*pwC*4000.0*sqrt((12.07*sfrq/600+3.85)/0.35));
          rfst = (int) (rfst + 0.5);
        }
        else rfst=4095.0;

        if( pwC > (24.0e-6*600.0/sfrq) )
	  { printf("Increase pwClvl so that pwC < 24*600/sfrq");
	    psg_abort(1); 
        }
      }
      else        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
      {
        if(FIRST_FID)                                            /* call Pbox */
        {
          ppm = getval("dfrq"); 
          bw = 118.0*ppm; ofs = 139.0*ppm;
          offC10 = pbox_make("offC10", "sinc180n", bw, ofs, compC*pwC, pwClvl);   
          bw = 80.0*ppm; pws = 0.001; ofs = 0.0; nst = 1000.0;    
          stC80 = pbox_makeA("stC80", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
          ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
        }
        rf7 = offC10.pwrf;  pwC10 = offC10.pw;
        if (pwC180>3.0*pwC) rfst = stC80.pwrf;
        else rfst=4095.0;
      }

   /* 80 ppm STUD+ decoupling */
	strcpy(stCdec, "stCdec80");
	studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf80);
	studlvl = (int) (studlvl + 0.5);

/* BEGIN ACTUAL PULSE SEQUENCE */


status(A);
   decpower(pwClvl);  /* Set Dec1 power for hard 13C pulses         */
   dec2power(dpwr2);       /* Set Dec2 to low power for decoupling */
   delay(d1);

  if (wet[A] == 'y') {
     Wet4(zero,one);
  }

   obspower(tpwr);           /* Set transmitter power for hard 1H pulses */
   rcvroff();
status(B);
   rgpulse(pw,t1,1.0e-6,0.0);                  /* 90 deg 1H pulse */
if (d2>0.0)
 {
   delay(tau1);
   decrgpulse(pwC,zero,0.0,0.0);            /* composite 180 on 13C */ 
   decrgpulse(2.0*pwC,one,2.0e-6,0.0);
   decrgpulse(pwC,zero,2.0e-6,0.0);
   delay(tau1);
 }
status(C);
   rgpulse(pw,t8,2.0e-6,0.0);             /*  2nd 1H 90 pulse   */

      obspower(slpwr);
      zgradpulse(gzlvl0,gt0);
      delay(150.0e-6);
     if (cycles > 1.0)
      {
         rcvroff();
         starthardloop(v9);
             dipsi(one,three); dipsi(three,one); 
             dipsi(three,one); dipsi(one,three);
         endhardloop();
       }
      decrgpulse(pwC,zero,2.0e-6,2.0e-6); 
      zgradpulse(-gzlvl0,gt0);
      delay(500.0e-6);
      obspower(tpwr);

   rgpulse(pw,t9,0.0,2.0e-6);
   zgradpulse(gzlvl2, gt2);       /* g3 in paper   */
   decpwrf(rfst);
   delay(1/(4.0*jch) - gt2 - 2.0e-6 -pwC180/2.0);
   if (pwC180>3.0*pwC)
    simshaped_pulse("",C180,2*pw,pwC180,zero,zero,0.0,2.0e-6);
   else
    simpulse(2.0*pw,2.0*pwC,zero,zero,0.0,2.0e-6);
   zgradpulse(gzlvl2, gt2);       /* g4 in paper  */
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
   decrgpulse(pwC,t2,150.0e-6,2.0e-6);   

   if( tau2 > 0.0 ) 
    {
        decphase(zero);
	decpwrf(rf7);
	delay(tau2);
	simshaped_pulse("","offC10",2.0*pw, pwC10,zero, zero, 0.0, 0.0);
        decpwrf(4095.0);
        delay(tau2);
    }
   else
     simpulse(2*pw,2*pwC,zero,zero,2.0e-6,2.0e-6);  
   decrgpulse(pwC,zero,2.0e-6,2.0e-6);
                if (mag_flg[A] == 'y')
                {
                   magradpulse(gzcal*gzlvl4, gt4);
                }
                else
                {
                   zgradpulse(gzlvl4, gt4);
                }
   rgpulse(pw,t5,152.0e-6,2.0e-6);
   zgradpulse(gzlvl5, gt5);


 if(sead[A] == 'y')
  {   
   if(nsteps < 2.0) nsteps = 2.0;   /* minimum number of steps */
   decpwrf(rfst);  
   delay(1/(4.0*jch) - tcorr1 - gt5 - POWER_DELAY); 
   loop(v13, v10); delay(idel); endloop(v10); /* 3,4,5,6 ... N-1 idel  */
   
   if (pwC180>3.0*pwC)
     decshaped_pulse("stC200", pwC180, one, 2.0e-6, 2.0e-6);
   else
     decrgpulse(2.0*pwC, one, 2.0e-6, 2.0e-6);
   loop(v6, v10); delay(idel); endloop(v10);  /* N-1 ... 3,2,1,0 idel  */
   rgpulse(2.0*pw, zero, 2.0e-6, 2.0e-6);
   zgradpulse(gzlvl5, gt5);
   decpwrf(4095.0);
   delay(1/(4.0*jch) - tcorr2 - gt5 - pwC - 4.0e-6 - 4.0*POWER_DELAY - OFFSET_DELAY);
   loop(v6, v10); delay(2.0*idel); endloop(v10); /* N-1 ... 3,2,1,0 2*idel */ 
  /* decrgpulse(pwC, zero, 2.0e-6, 2.0e-6); */                       /* purging */
   decrgpulse(pwC, t6, 2.0e-6, 2.0e-6); 
   decpwrf(seadpwrf);
   decshaped_pulse(seadpat, seadpw, zero, 2.0e-6, 2.0e-6);   /* sead pulse */
   decpwrf(rf0);decpower(getval("seaddpwr"));  
 
   decprgon(seaddseq, 1.0/getval("seaddmf"), getval("seaddres")); decon();
   loop(v12, v10); delay(2.0*idel); endloop(v10); /* 0,1,2,3 ... N-1 2*idel */
 /*  rgpulse(pw,t5,1.0e-6,rof2); */
    delay(pw+1.0e-6+rof2);

   setreceiver(t4);
  }
   else
  {
     if (pwC180>3.0*pwC)
     {
      decpwrf(rfst);
      delay(1/(4.0*jch) - gt5 - 2.0e-6 -pwC180/2.0);
      simshaped_pulse("",C180,2*pw,pwC180,zero,zero,0.0,2.0e-6);
      decpwrf(4095.0);
      zgradpulse(gzlvl5, gt5);
      delay(1/(4.0*jch) -  gt5 -2.0*pwC - 6.0e-6 -pwC180/2.0);
     }
     else
     {
      decpwrf(4095.0);
      delay(1/(4.0*jch) - gt5 - 2.0e-6 -pwC);
      simpulse(2.0*pw,2.0*pwC,zero,zero,0.0,2.0e-6);
      zgradpulse(gzlvl5, gt5);
      delay(1/(4.0*jch) -  gt5 -2.0*pwC - 6.0e-6 -pwC);
     }     
     decphase(zero);
     decrgpulse(pwC,zero,0.0,0.0);    
     decrgpulse(pwC,t3,2.0e-6,0.0);     
     rgpulse(pw,t5,1.0e-6,rof2);                        /* flip-back pulse  */
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





}
