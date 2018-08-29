/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  rna_gnoesyChsqcA.c

    3D C13 edited noesy with separation via the carbon of the destination site
         recorded on a water sample 

    NOTE: cnoesy_3c_sed_pfg(1).c 3D C13 edited NOESYs with separation via the
          carbon of the origination site. (D2O experiments)

    Uses three channels:
         1)  1H             - carrier @ water  
         2) 13C             - carrier @ 110 ppm
         3) 15N             - carrier @ 200 ppm for D2O sample
           or
         3) 15N             - carrier @ 80 ppm for H2O sample

    Set dm = 'nnny', [13C decoupling during acquisition].
    Set dm2 = 'nyny', [15N dec during t1 and acq] 

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI
    acquisition in t1 [H]  and t2 [C].

    Set f1180 = 'y' and f2180 = 'y' for (90, -180) in F1 and (90, -180) in  F2.    
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
    Auto-calibrated version, E.Kupce, 27.08.2002.
    Modified from gnoesyChsqcA.c   GG Nov 2002

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

     AUTOCAL and CHECKOFS.
       The autocal flag is generated automatically in Pbox_bio.h
       If this flag does not exist in the parameter set, it is automatically 
       set to 'y' - yes. In order to change the default value, create the  
       flag in your parameter set and change as required. 
       For the autocal flag the available options are: 'y' (yes - by default), 
       'q' (quiet mode - suppress Pbox output), 'r' (read from file, no new  
       shapes are created), 's' (semi-automatic mode - allows access to user  
       defined parameters) and 'n' (no - use full manual setup, equivalent to 
       the original code).
*/

#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */


/* Chess - CHEmical Shift Selective Suppression */
Chess(pulsepower,pulseshape,duration,phase,rx1,rx2,gzlvlw,gtw,gswet)
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
Wet4(phaseA,phaseB)
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
static int  phi1[8]  = {0,0,0,0,2,2,2,2},
            phi2[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
            phi3[8]  = {0,0,0,0,2,2,2,2},
            rec[16]  = {0,3,2,1,2,1,0,3,2,1,0,3,0,3,2,1},
            phi5[4]  = {0,1,2,3},
            phi6[2]  = {0,2},
            phi7[4]  = {1,1,3,3};
                    
static double d2_init=0.0, d3_init=0.0;

static shape  stC30, stC80;

pulsesequence()
{
/* DECLARE VARIABLES */

 char     
	    aliph[MAXSTR],	 		 /* aliphatic CHn groups only */
	    arom[MAXSTR], 			  /* aromatic CHn groups only */

            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            mag_flg[MAXSTR],  /* magic angle gradient                     */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell             */
            wet[MAXSTR],      /* Flag to select optional WET water suppression */
            stCshape[MAXSTR], /* C13 inversion pulse shape name */
            STUD[MAXSTR],     /* Flag to select adiabatic decoupling      */
            stCdec[MAXSTR];   /* contains name of adiabatic decoupling shape */

 int         
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter;   /* used for states tppi in t2           */ 

double    

  dofa = 0.0,                   /* actual 13C offset (depends on aliph and arom)*/
  rf80 = getval("rf80"), 	/* rf in Hz for 80ppm STUD+ */
  dmf80 = getval("dmf80"),     /* dmf for 80ppm STUD+ */
  rf30 = getval("rf30"),       /* rf in Hz for 30ppm STUD+ */
  dmf30 = getval("dmf30"),     /* dmf for 30ppm STUD+ */

  stdmf = 1.0,                 /* dmf for STUD decoupling initialized */ 
  studlvl = 0.0,	       /* coarse power for STUD+ decoupling initialized */

  rfst = 0.0,                  /* fine power level for adiabatic pulse initialized*/

                                 /* temporary Pbox parameters */
  bw, pws, ofs, nst,  /* bandwidth, pulsewidth, offset, ppm, # steps */

            rf0,                        /* full fine power */
            compC = getval("compC"),         /* adjustment for C13 amplifier compression */
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
             dz,wetpw,gtw,gswet,gswet2,
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

  getstr("wet",wet);
  getstr("mag_flg",mag_flg);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("STUD",STUD);

  wetpw=getval("wetpw");
  gtw=getval("gtw");
  gswet=getval("gswet");
  gswet2=getval("gswet2");
  dz=getval("dz");

  mix  = getval("mix");
  sw1  = getval("sw1");
  sw2  = getval("sw2");
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
    if (aliph[A] == 'y')
     corr = (4.0/PI)*pwC + 4.0e-6;
    else
     corr = (4.0/PI)*pwC +4.0e-6;

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

     if (arom[A]=='y')  /* AROMATIC spectrum */
     {
       /* 30ppm sech/tanh inversion */
       rfst = (compC*4095.0*pwC*4000.0*sqrt((4.5*sfrq/600.0+3.85)/0.41));   
       rfst = (int) (rfst + 0.5);
     }
  
     if (aliph[A]=='y')  /* ALIPHATIC spectrum */
     {
       /* 80ppm sech/tanh inversion pulse */
       if (pwC180>3.0*pwC) 
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
   }
   else        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
   {
     if(FIRST_FID)                                            /* call Pbox */
     {
       if (arom[A]=='y')  /* AROMATIC spectrum */
       {
         bw = 30.0*dfrq; pws = 0.001; ofs = 0.0; nst = 500.0;    
         stC30 = pbox_makeA("stC30", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
       }
       if (aliph[A]=='y')
       {
         bw = 80.0*dfrq; pws = 0.001; ofs = 0.0; nst = 1000.0;    
         stC80 = pbox_makeA("stC80", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
       }
     }
     if (arom[A]=='y')  rfst = stC30.pwrf;
     if (aliph[A]=='y') 
     {
      if (pwC180>3.0*pwC) rfst = stC80.pwrf;
      else rfst = 4095.0;
     }
   }

   if (arom[A]=='y')  
   {
     strcpy(stCshape, "stC30");
     /* 30 ppm STUD+ decoupling */
     strcpy(stCdec, "stCdec30");             
     stdmf = dmf30;
     studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf30);
     studlvl = (int) (studlvl + 0.5);
   }

   if (aliph[A]=='y')
   {
     strcpy(stCshape, "stC80");
     /* 80 ppm STUD+ decoupling */
     strcpy(stCdec, "stCdec80");
     stdmf = dmf80;
     studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf80);
     studlvl = (int) (studlvl + 0.5);
   }

/* BEGIN ACTUAL PULSE SEQUENCE */


status(A);
   if (aliph[A] == 'y') 
    dofa=dof-24*dfrq;  /* ribose region  */
   else
     dofa=dof+15*dfrq;  /* aromatic region */

   decoffset(dofa);
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
if (tau1>1.0e-6)
 {
   delay(tau1);
   decrgpulse(pwC,zero,0.0,0.0);            /* composite 180 on 13C */ 
   decrgpulse(2.0*pwC,one,2.0e-6,0.0);
   decrgpulse(pwC,zero,2.0e-6,0.0);
   delay(tau1);
 }
status(C);
   rgpulse(pw,zero,2.0e-6,0.0);             /*  2nd 1H 90 pulse   */

   if (wet[B] == 'y')
     delay(mix - 10.0e-3 -4.0*wetpw -4.0*gtw -3.0*gswet -gswet2 -dz);    /* mixing time */
   else
     delay(mix - 10.0e-3 ); 

   zgradpulse(gzlvl0, gt0);
   decrgpulse(pwC,zero,102.0e-6,2.0e-6); 
   decpwrf(rfst);                           /* power for inversion pulse */
   zgradpulse(gzlvl1, gt1);
   delay(10.0e-3 - gt1 - gt0 - 8.0e-6);
   delay(150.0e-6);

   if (wet[B] == 'y') {
     Wet4(zero,one);
   }

   rgpulse(pw,zero,0.0,2.0e-6);
   zgradpulse(gzlvl2, gt2);       /* g3 in paper   */
   delay(1/(4.0*jch) - gt2 - 2.0e-6 -pwC180/2.0);
   if (pwC180>3.0*pwC)
    simshaped_pulse("",stCshape,2*pw,pwC180,zero,zero,0.0,2.0e-6);
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

   if( tau2 > 1.0e-6 ) 
    {
        decphase(zero);
	delay(tau2);
        rgpulse(2.0*pw,zero,0.0,0.0);
        delay(tau2);
    }
   else
     rgpulse(2*pw,zero,2.0e-6,2.0e-6);  
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
   decpwrf(rfst);
   zgradpulse(gzlvl5, gt5);
   delay(1/(4.0*jch) - gt5 - 2.0e-6 -pwC180/2.0);
   if (pwC180>3.0*pwC)
    simshaped_pulse("",stCshape,2*pw,pwC180,zero,zero,0.0,2.0e-6);
   else
    simpulse(2.0*pw,2.0*pwC,zero,zero,0.0,2.0e-6);
   decpwrf(4095.0);
   zgradpulse(gzlvl5, gt5);
   delay(1/(4.0*jch) -  gt5 -2.0*pwC - 6.0e-6 -pwC180/2.0);
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
