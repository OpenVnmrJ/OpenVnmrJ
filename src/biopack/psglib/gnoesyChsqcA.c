/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gnoesyChsqcA.c

    3D C13 edited noesy with separation via the carbon of the destination site
         recorded on a water sample 

    NOTE: cnoesy_3c_sed_pfg(1).c 3D C13 edited NOESYs with separation via the
          carbon of the origination site. (D2O experiments)

    Uses three channels:
         1)  1H             - carrier @ water  
         2) 13C             - carrier @ 43 ppm
         3) 15N             - carrier @ 118 ppm

    Set dm = 'nnny', [13C decoupling during acquisition].
    Set dm2 = 'nyny', [15N dec during t1 and acq] 

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI
    acquisition in t1 [H]  and t2 [C].

    Set f1180 = 'y' and f2180 = 'y' for (90, -180) in F1 and (90, -180) in  F2.    
    c180_flg='y' for HH 2D (ni) experiment (fixed d3 (t2) in this case) 
    c180_flg='n' for carbonyl decoupling using wfg during t2 (3D only)
    Note: Zero order phase correct may not be exactly +90 in F2 due to seduce.

    If you set the C13 carrier to a value other than 43 ppm (such as 35 ppm),
    change the line
        dofa=dof+(125-43)*dfrq;
    below to
        dofa=dof+(125-35)*dfrq;

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
    Added SEAD,gstab  E.K. and G.G. 24,10.2003

    Modified H1 indirect evolution: 
    	- N15 decoupling done using hard 180 used instead of dm2; 
	- use CNrefoc flag to decouple both C13 and N15 (3db lower power 
	  used for both)
    Marco@NMRFAM 2006

    Modified C13 indirect evolution: 
	- flag for optional CO refocusing added (COrefoc)
    Marco@NMRFAM 2006

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
       resolution and less distortion from truncation. The macro "BPsetlp2" can be
       used in the format "BPsetlp2(desired ni2, acquired ni2, #fixed)". Set
       "desired ni2" to be the final extended data size, "acquired ni2" to be the
       total # of increments to be used as a basis (it may be less than ni2, for
       example if the experiment is running), and "#fixed" to the number of
       initial points in t2 to be predicted (typically 2-4). Try this with a 2D
       data set for varying numbers of fixed points until the baseline is sufficiently
       flat in F2.

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
       
       Sideband elimination by adiabatic defocussing (SEAD) option and J-compensated
       adiabatic pulses added by E.K.
       The decoupling power level can be adjusted by changing seadpw. The later should 
       not exceed 2 ms. The efficiency of sideband suppression can be verified by
       setting sead = 'n' and dseq='wusead' (use macro pxset('wusead.DEC') for
       convenience.
       Ref. E.Kupce, J.Magn.Reson., vol. 129, pp. 219-221 (1997).
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
static double   H1ofs=4.7, C13ofs=43.0, N15ofs=120.0, H2ofs=0.0;

static shape offC10, stC30, stC200;

pulsesequence()
{
/* DECLARE VARIABLES */

 char     
	    aliph[MAXSTR],	 		 /* aliphatic CHn groups only */
	    arom[MAXSTR], 			  /* aromatic CHn groups only */
	    COrefoc[MAXSTR],   /* flag for CO decoupling during C13 evolution */
            sead[MAXSTR],           /* Flag to eliminate decoupling sidebands */
            seadpat[MAXSTR],                                    /* SEAD shape */
            seaddseq[MAXSTR],                     /* SEAD decoupling sequence */
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            mag_flg[MAXSTR],  /* magic angle gradient                     */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell             */
            wet[MAXSTR],      /* Flag to select optional WET water suppression */
            satmode[MAXSTR],   /* Flag to select optional water presaturation */
            stCshape[MAXSTR], /* C13 inversion pulse shape name */
            STUD[MAXSTR],     /* Flag to select adiabatic decoupling      */
            stCdec[MAXSTR],   /* contains name of adiabatic decoupling shape */
  	    CNrefoc[MAXSTR];	/* flag for refocusing 15N during indirect H1 evolution */

 int         
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter;   /* used for states tppi in t2           */ 

double    

  dofa = 0.0,                   /* actual 13C offset (depends on aliph and arom)*/
  rf200 = getval("rf200"), 	/* rf in Hz for 200ppm STUD+ */
  dmf200 = getval("dmf200"),     /* dmf for 200ppm STUD+ */
  rf30 = getval("rf30"),       /* rf in Hz for 30ppm STUD+ */
  dmf30 = getval("dmf30"),     /* dmf for 30ppm STUD+ */

  stdmf = 1.0,                 /* dmf for STUD decoupling initialized */ 
  studlvl = 0.0,	       /* coarse power for STUD+ decoupling initialized */

  rfst = 0.0,                  /* fine power level for adiabatic pulse initialized*/

                                 /* temporary Pbox parameters */
  bw, pws, ofs, ppm, nst,  /* bandwidth, pulsewidth, offset, ppm, # steps */

            pwC10 = getval("pwC10"),    /* 180 degree selective sinc pulse on CO(174ppm) */
            rf7,	                /* fine power for the pwC10 ("offC10") pulse */
            rf0,                        /* full fine power */
             tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             jch,          /*  CH coupling constant */
             pwN,          /* PW90 for 15N pulse              */
             pwC,          /* PW90 for c nucleus @ pwClvl         */
             pwC180,       /* PW180 for c nucleus in INEPT transfers */
             pwClvl,        /* power level for 13C pulses on dec1  */
             pwNlvl,       /* high dec2 pwr for 15N hard pulses    */

  compC = getval("compC"),         /* adjustment for C13 amplifier compression */
  compN = getval("compN"),         /* adjustment for C13 amplifier compression */
  pwClw=getval("pwClw"), 	   /* pwC at 3db lower power */
  pwNlw=getval("pwNlw"),	   /* pwN at 3db lower power */
  pwZlw=0.0,			   /* largest of pwNlw and 2*pwClw */

             mix,          /* noesy mix time                       */
             sw1,          /* spectral width in t1 (H)             */
             sw2,          /* spectral width in t2 (C) (3D only)   */
             idel, tcorr1, tcorr2,
             nsteps = 8.0,
             seadpw = getval("seadpw"),
             seadpwrf = getval("seadpwrf"),          
             dz,wetpw,gtw,gswet,gswet2,gstab,
             gt0,
             gt1,
             gt2,
             gt3,
             gt4,
             gt5,
             gzcal,        /* dac to G/cm conversion factor */
             gzlvl0, 
             gzlvl1, 
             gzlvl2, 
             gzlvl3, 
             gzlvl4, 
             gzlvl5;


/* LOAD VARIABLES */

  getstr("aliph",aliph);
  getstr("arom",arom);
  getstr("COrefoc",COrefoc);
  getstr("CNrefoc",CNrefoc);

  getstr("sead",sead);
  getstr("seadpat",seadpat);
  getstr("seaddseq", seaddseq);
  getstr("wet",wet);
  getstr("satmode",satmode);
  getstr("mag_flg",mag_flg);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("STUD",STUD);

  wetpw=getval("wetpw");
  gtw=getval("gtw");
  gstab=getval("gstab");
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
  gzlvl0 = getval("gzlvl0");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");

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

    if (aliph[A]=='n' && COrefoc[A]=='y')
      { 
	printf("CO refocusing only meaningful for aliphatic spectra ");
	psg_abort(1); 
      }

    if((dm[A] == 'y' || dm[C] == 'y' ))
    {
        printf("incorrect 13C decoupler flags! dm='nnnn' or 'nnny' only  ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' ))
    {
        printf("incorrect 15N decoupler flags! No dm2 decoupling needed for N15 ");
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

    if( gt0 > 15e-3 || gt1 > 15e-3 || gt2 > 15e-3 || gt3 > 15e-3 || gt4 > 15e-3 || gt5 > 15e-3 ) 
    {
        printf("gti values < 15e-3\n");
        psg_abort(1);
    } 

   if( gzlvl3*gzlvl4 > 0.0 ) 
    {
        printf("gt3 and gt4 must be of opposite sign for optimal water suppression\n");
        psg_abort(1);
     }

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
     sub(v8, v12, v9); 	        	/* v9  = N-1 ... 3,2,1,0 */

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

/* maximum fine power for pwC pulses */
   rf0 = 4095.0;

   setautocal();                        /* activate auto-calibration flags */ 
        
   if (autocal[0] == 'n') 
   {
    /* "offC10": 180 degree one-lobe sinc pulse on CO, null at Ca 139ppm away */
	rf7 = (compC*4095.0*pwC*2.0*1.65)/pwC10;	/* needs 1.65 times more     */
	rf7 = (int) (rf7 + 0.5);		/* power than a square pulse */

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
       ppm = getval("dfrq"); 
       bw = 118.0*ppm; ofs = 139.0*ppm;
       offC10 = pbox_make("offC10", "sinc180n", bw, ofs, compC*pwC, pwClvl);         
       if (arom[A]=='y')  /* AROMATIC spectrum */
       {
         bw = 30.0*ppm; pws = 0.001; ofs = 0.0; nst = 500.0;    
         stC30 = pbox_makeA("stC30", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
       }
       if (aliph[A]=='y')
       {
         bw = 200.0*ppm; pws = 0.001; ofs = 0.0; nst = 1000.0;    
         stC200 = pbox_makeA("stC200", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
       }
       ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
     }
     rf7 = offC10.pwrf;  pwC10 = offC10.pw;
     if (arom[A]=='y')  rfst = stC30.pwrf;
     if (aliph[A]=='y') 
     {
      if (pwC180>3.0*pwC) rfst = stC200.pwrf;
      else rfst = 4095.0;
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


/* BEGIN ACTUAL PULSE SEQUENCE */


status(A);

   decoffset(dofa);
   decpower(pwClvl);  /* Set Dec1 power for hard 13C pulses         */

  if (satmode[A] == 'y')
  {
    obspower(getval("satpwr"));
    rgpulse(getval("d1"),zero,2.0e-6,2.0e-6); 
  }
  else
     delay(d1);
  if (wet[A] == 'y') 
  {
     Wet4(zero,one);
  }

   obspower(tpwr);           /* Set transmitter power for hard 1H pulses */
   rcvroff();
status(B);
   rgpulse(pw,t1,rof1,rof1);                  /* 90 deg 1H pulse */
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
   rgpulse(pw,zero,rof1,rof1);             /*  2nd 1H 90 pulse   */
status(C);
   dec2power(dpwr2);
   if (wet[B] == 'y')
   {
     delay(mix - 10.0e-3 -4.0*wetpw -4.0*gtw -3.0*gswet -gswet2 -dz);    /* mixing time */
     zgradpulse(gzlvl0, gt0);
     decrgpulse(pwC,zero,102.0e-6,2.0e-6); 
     decpwrf(rfst);                           /* power for inversion pulse */
     zgradpulse(gzlvl1, gt1);
     delay(10.0e-3 - gt1 - gt0);
     Wet4(zero,one);
   }
   else
   {     
     if(satmode[B] == 'y')
     {
       obspower(getval("satpwr"));
       xmtron();
     }
     delay(mix - 5.0e-3 ); 
     if(satmode[B] == 'y') 
     {
       xmtroff();
       obspower(tpwr);
     }
     zgradpulse(gzlvl0, gt0);
     delay(gstab);
     decrgpulse(pwC,zero,0.0,0.0); 
     decpwrf(rfst);                           /* power for inversion pulse */
     zgradpulse(gzlvl1, gt1);
     delay(5.0e-3 - gt1 - gt0);
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
   delay(gstab);
   decrgpulse(pwC,t2,rof1,0.0);   
/* C13 EVOLUTION TIME BEGINS */
   decphase(zero);
   if (COrefoc[A]=='y' &&  
	(tau2 -(2.0/PI)*pwC -SAPS_DELAY -PWRF_DELAY -0.5*pwC10 -WFG2_START_DELAY -2.0e-6) > 0.2e-6)
     {
	decpwrf(rf7);
        delay(tau2 -(2.0/PI)*pwC -SAPS_DELAY -PWRF_DELAY -0.5*pwC10 -WFG2_START_DELAY -2.0e-6);

	simshaped_pulse("","offC10",2.0*pw, pwC10,zero, zero, 2.0e-6, 0.0);

        decpwrf(4095.0);
        delay(tau2 -(2.0/PI)*pwC -PWRF_DELAY -0.5*pwC10 -2.0e-6);
    }
   else if (tau2 -(2.0/PI)*pwC -SAPS_DELAY -pw -2.0e-6 > 0.2e-6)
    {
     delay(tau2 -(2.0/PI)*pwC -SAPS_DELAY -pw -2.0e-6);
     rgpulse(2.0*pw, zero, 2.0e-6, 0.0);  
     delay(tau2 -(2.0/PI)*pwC -pw -2.0e-6);
    }
   else
     rgpulse(2.0*pw, zero, 2.0e-6, 0.0);  
/* C13 EVOLUTION TIME ENDS */
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
   loop(v9, v10); delay(idel); endloop(v10);  /* N-1 ... 3,2,1,0 idel  */
   rgpulse(2.0*pw, zero, 2.0e-6, 2.0e-6);
   zgradpulse(gzlvl5, gt5);
   decoffset(dofa);
   decpwrf(rf0);
   delay(1/(4.0*jch) - tcorr2 - gt5 - pwC - 4.0e-6 - 4.0*POWER_DELAY - OFFSET_DELAY);
   loop(v9, v10); delay(2.0*idel); endloop(v10); /* N-1 ... 3,2,1,0 2*idel */ 
  /* decrgpulse(pwC, zero, 2.0e-6, 2.0e-6); */                       /* purging */
   decrgpulse(pwC, t6, 2.0e-6, 2.0e-6); 
   decpwrf(seadpwrf);
   decshaped_pulse(seadpat, seadpw, zero, 2.0e-6, 2.0e-6);   /* sead pulse */
   decpwrf(rf0);decpower(getval("seaddpwr"));  
 
   decprgon(seaddseq, 1.0/getval("seaddmf"), getval("seaddres")); decon();
    delay(pw+1.0e-6+rof2);
    initval(1.0e6*2.0*seadpw/nsteps,v17);
    mult(v17,v12,v18);
    vdelay(USEC,v18);
    setreceiver(t4);
    startacq(alfa);
    acquire(np,1.0/sw);
    endacq();
    decprgoff(); decoff(); decblank();
 }
   else
   {
     if (pwC180>3.0*pwC)
     {
      decpwrf(rfst);
      delay(1/(4.0*jch) - gt5 - 2.0e-6 -pwC180/2.0);
      simshaped_pulse("",stCshape,2*pw,pwC180,zero,zero,0.0,2.0e-6);
      decpwrf(4095.0);
      zgradpulse(gzlvl5, gt5);
      delay(1/(4.0*jch) -  gt5 -2.0*pwC - 6.0e-6 -pwC180/2.0);
    }
    else
    {
      delay(1/(4.0*jch) - gt5 - 2.0e-6 -pwC);
      simpulse(2.0*pw,2.0*pwC,zero,zero,0.0,2.0e-6);
      zgradpulse(gzlvl5, gt5);
      delay(1/(4.0*jch) -  gt5 -2.0*pwC - 6.0e-6 -pwC);
    }     
     decrgpulse(pwC,zero,0.0,0.0);    
     decrgpulse(pwC,t3,2.0e-6,0.0);     
     rgpulse(pw,t5,1.0e-6,rof2);                        /* flip-back pulse  */
     rcvron();
     setreceiver(t4);
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
}
