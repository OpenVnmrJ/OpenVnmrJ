// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/*  ATCNnoesy (gnoesyCNhsqc from ProteinPack with power reduction eliminated)

    3D C13,N15 edited noesy with separation via the carbon of the destination site
         recorded on a water sample 
    includes optional "WET" water suppression in relaxation delay
    includes optional magic-angle gradients


    Uses three channels:
         1)  1H             - carrier @ water  
         2) 13C             - carrier @ 43 ppm (CA/CB)
         3) 15N             - carrier @ 118 ppm

    Set dm = 'nnny', [13C decoupling during acquisition].
    (13C decoupling in t1 is done by a composite pulse)
    Set dm2 = 'nynn', [15N dec during t1] 
    Set dm2 = 'nyny', [15N dec during t1 and acquisition] 

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI
    acquisition in t1 [H]  and t2 [C].

    Set f1180 = 'y' and f2180 = 'y' for (-90,180) in F1 and (-90,180) in  F2.    
    Use linear prediction in t2 for the first few points to flatten baseline
      The macro setlp2 could be used, e.g. setlp2(128,32,2) for extending
      to 128 complex points in t2 from 32 acquired complex points, with the
      first two complex points in t2 linear predicted


   "setlp2(new ni2,ni2 used,frontpoints)"
    parlp(2)
    if $#>2 then
     lpopt2='b','f' strtlp2=$3,$2
     lpfilt2=8 lpnupts2=$2-$3,$2 lpext2=$3,$1-$2
     strtext2=$3,$2+1
     setsb2(2*$1) fn2=2*$1 proc2='lp'
    else
     lpopt2='f' strtlp2=$2
     lpfilt2=8 lpnupts2=$2 lpext2=$1-$2
     strtext2=$2+1
     setsb2(2*$1) fn2=2*$1 proc2='lp'
    endif


    Note: Zero order phase correct may not be exactly -90 in F2 due to C=O 
          decoupling pulse.

    Modified by L. E. Kay to allow for simult N, C acq   July 19, 1993
    original code: noesyc_pfg_h2o_NC_dps.c
    Modified for dps and magic angle gradients D.Mattiello, Varian, 6/8/94
    Modified for vnmr5.2 (new power statements, use status control in t1)
      GG. Palo Alto  16jan96
    Modified to use only z-gradients 
    Modified to use pwC/pwClvl/pwN/pwNlvl/compC as in other ProteinPack/
     sequences. The parameter "compN" is added to permit accurate lower power
     N15 pulses. This parameter should be calibrated by determining the pw90
     for N15 at the normal pwNlvl(pw1) and 3db lower(pw2). Then,      
     compN=pw2/(2.0*pw1). Use ghn_co or gNhsqc for these calibrations.

    Modified to use ProteinPack-style of C=O decoupling
     
    STUD DECOUPLING.   SET STUD='y':
       Setting the flag STUD='y' overrides the decoupling parameters listed in
       dg2 and applies STUD+ decoupling instead.  In consequence is is easy
       to swap between the decoupling scheme you have been using to STUD+ for
       an easy comparison.  The STUD+ waveforms are calculated for your field
       strength at the time of ProteinPack installation and RF power is 
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

       Ref: Pascal et.al.,J.Magn.Reson. B103,197-201(1994)
       GG, palo alto, 7 july 1998
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

/* ATchess - CHEmical Shift Selective Suppression */
static void ATchess(double pulsepower, char *pulseshape, double duration, codeint phase,
                    double rx1, double rx2, double gzlvlw, double gtw, double gswet)
{
  obspwrf(pulsepower);
  shaped_pulse(pulseshape,duration,phase,rx1,rx2);
  zgradpulse(gzlvlw,gtw);
  delay(gswet);
}

/* wet4 - Water Elimination */
static void ATwet4(codeint phaseA, codeint phaseB)
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
  ATchess(finepwr*0.5059,wetshape,wetpw,phaseA,20.0e-6,rof2,gzlvlw,gtw,gswet);
  ATchess(finepwr*0.6298,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/2.0,gtw,gswet);
  ATchess(finepwr*0.4304,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/4.0,gtw,gswet);
  ATchess(finepwr*1.00,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/8.0,gtw,gswet2);
  obspower(tpwr); obspwrf(tpwrf);     /* Reset to normal power level   */
  rcvron();
  delay(dz);
}

void pulsesequence()
{
/* DECLARE VARIABLES */

 char     
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell             */
            mag_flg[MAXSTR],  /* magic-angle gradients                    */
            C180[MAXSTR],     /* adiabatic 13C inversion pulse            */
            wet[MAXSTR],      /* Flag to select optional WET water suppression */
            STUD[MAXSTR],     /* Flag to select adiabatic decoupling      */
            dmm2[MAXSTR],
            stCdec[MAXSTR];   /* contains name of adiabatic decoupling shape */

 int         
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter;   /* used for states tppi in t2           */ 

 double     stdmf = getval("dmf80"),     /* dmf for 80 ppm of STUD decoupling */
            rf80 = getval("rf80"),       	  /* rf in Hz for 80ppm STUD+ */
            pwC180=getval("pwC180"),     /* adiabatic 13C pw180 */
            rfst,                        /* fine power level for adiabatic pulse*/
            studlvl,	  
                        /* coarse power for STUD+ decoupling */
            pwC10 = getval("pwC10"), 
                        /* 180 degree selective sinc pulse on CO(174ppm) */
            rf7,
  	                /* fine power for the pwC10 ("offC10") pulse */
            rf0,                        /* full fine power */
            compC = getval("compC"), 
                        /* adjustment for C13 amplifier compression */
             tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             corr,         /*  correction for t2  */
             jch,          /*  CH coupling constant */
             pwN,          /* PW90 for 15N pulse              */
             pwC,          /* PW90 for c nucleus @ pwClvl         */
             pwClvl,        /* power level for 13C pulses on dec1  */
             pwNlvl,       /* high dec2 pwr for 15N hard pulses    */
             mix,          /* noesy mix time                       */
             sw1,          /* spectral width in t1 (H)             */
             sw2,          /* spectral width in t2 (C) (3D only)   */
             gt0,
             gt1,
             gt2,
             gzcal,
             gt3,
             gt4,
             gt5,
             gt6,
             gt7,
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
  getstr("dmm2",dmm2);

    mix  = getval("mix");
    sw1  = getval("sw1");
    sw2  = getval("sw2");
  jch = getval("jch"); 
  pwC = getval("pwC");
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

    if((dm[A] == 'y') || (dm[B] == 'y') || (dm[C] == 'y' ))
    {
        printf("incorrect dec1 decoupler flags! Should be 'nnny' 13C is decoupled in t1 by pulse ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[C] == 'y' ))
    {
        printf("incorrect dec2 decoupler flags! Should be 'nyny' ");
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
        printf("dont fry the probe, pw too long. Check value ! ");
        psg_abort(1);
    } 


    if( pwN > 200.0e-6 )
    {
        printf("dont fry the probe, pwN too long. Check value ! ");
        psg_abort(1);
    } 

    if( pwC > 200.0e-6 )
    {
        printf("dont fry the probe, pwC too long. Check value ! ");
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


   /* 80 ppm STUD+ decoupling */
	strcpy(stCdec, "stCdec80");
	studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf80);
	studlvl = (int) (studlvl + 0.5);

    /* maximum fine power for pwC pulses */
	rf0 = 4095.0;


    /* 180 degree one-lobe sinc pulse on CO, null at Ca 139ppm away */
    /* 13C offset (dof) should be at 35ppm to properly decouple C=O */
	rf7 = (compC*4095.0*pwC*2.0*1.65)/pwC10;	/* needs 1.65 times more     */
        rf7 = 1.4*rf7;                           /* to account for pwClvl lower by 3dB */
	rf7 = (int) (rf7 + 0.5);		/* power than a square pulse */

    /* 80ppm sech/tanh ("stC80") inversion */
	rfst = (compC*4095.0*pwC*4000.0*sqrt((12.07*sfrq/600+3.85)/0.35));
        rfst = 1.4*rfst;                           /* to account for pwClvl lower by 3dB */
	rfst = (int) (rfst + 0.5);

   /*  Set up f1180  tau1 = t1       */
    tau1 = d2;
    if(f1180[A] == 'y') 
        tau1 += ( 1.0 / (2.0*sw1) - 4.0/PI*pw - 4.0*pwC - 6.0e-6);
    else
        tau1 = tau1 - (4.0/PI*pw + 4.0*pwC + 6.0e-6);
    if(tau1 < 0.2e-6) tau1 = 4.0e-7;
    tau1 = tau1/2.0;

   /*  Set up f2180  tau2 = t2   */
    corr = (4.0/PI)*pwN + pwC10 +WFG2_START_DELAY +2.0*POWER_DELAY+4.0e-6;
    tau2 = d3;
    if(f2180[A] == 'y')
      tau2 += ( 1.0 / (2.0*sw2)); 
    tau2 = (tau2 - corr)/2.0; 

/* BEGIN ACTUAL PULSE SEQUENCE */


status(A);
   decpower(pwClvl);  /* Set Dec1 power for hard 13C pulses         */
   dec2power(dpwr2);       /* Set Dec2 to low power for decoupling */
   delay(d1);
   if (wet[A] == 'y') 
    {
     obsoffset(tof);
     ATwet4(zero,one);             /* WET solvent suppression */
     delay(1.0e-4);
    }
   obspower(tpwr);           /* Set transmitter power for hard 1H pulses */
   rcvroff();
status(B);
   rgpulse(pw,t1,1.0e-4,0.0);                  /* 90 deg 1H pulse */

   if (d2>0.0)
    {
      delay(tau1);
      decrgpulse(pwC,zero,0.0,0.0);            /* composite 180 on 13C */ 
      decrgpulse(2.0*pwC,one,2.0e-6,0.0);
      decrgpulse(pwC,zero,2.0e-6,0.0);
      delay(tau1);
    }
status(C);
   rgpulse(pw,zero,2.0e-6,0.0);             /*  2nd 1H 90 pulse   */
   dec2power(pwNlvl);
   delay(mix - 10.0e-3);                    /*  mixing time     */
   zgradpulse(gzlvl0, gt0);
   decrgpulse(pwC,zero,102.0e-6,2.0e-6); 
   zgradpulse(gzlvl1, gt1);
   delay(10.0e-3 - gt1 - gt0 - 8.0e-6);
   delay(150.0e-6);
   rgpulse(pw,zero,0.0,2.0e-6);
   zgradpulse(gzlvl2, gt2);       /* g3 in paper   */
   if (pwC180 > 2.0*pwC)          /* allows using normal pulses */
    {
     delay(1/(4.0*jch) - gt2 - 2.0e-6 -pwC180/2.0);
     decpwrf(rfst);
     sim3shaped_pulse("",C180,"",2*pw,pwC180,2.0*pwN,zero,zero,zero,0.0,2.0e-6);
     decpwrf(4095.0);
     txphase(one); decphase(t2); dec2phase(t2);
     zgradpulse(gzlvl2, gt2);       /* g4 in paper  */
     delay(1/(4.0*jch) - gt2 - 2.0e-6 -pwC180/2.0);
    }
   else
    {
     delay(1/(4.0*jch) - gt2 - 2.0e-6 -pwC);
     sim3pulse(2*pw,2.0*pwC,2.0*pwN,zero,zero,zero,0.0,2.0e-6);
     txphase(one); decphase(t2); dec2phase(t2);
     zgradpulse(gzlvl2, gt2);       /* g4 in paper  */
     delay(1/(4.0*jch) - gt2 - 2.0e-6 -pwC);
    }
   rgpulse(pw,one,1.0e-6,2.0e-6);
                if (mag_flg[A] == 'y')
                {
                   magradpulse(gzcal*gzlvl3, gt3);
                }
                else
                {
                   zgradpulse(gzlvl3, gt3);
                }

   dec2rgpulse((PI-2.0)/PI*(pwN-pwC),t2,150.0e-6,0.0);
   sim3pulse(0.0,pwC,pwC,zero,t2,t2,0.0,2.0e-6);  
   dec2rgpulse((2.0/PI)*(pwN-pwC),t2,0.0,0.0);

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
     sim3pulse(2*pw,2*pwC,2.0*pwN,zero,zero,zero,2.0e-6,2.0e-6); 

   dec2rgpulse((2.0/PI)*(pwN-pwC),zero,0.0,0.0);
   sim3pulse(0.0,pwC,pwC,zero,zero,zero,0.0,2.0e-6);  
   dec2rgpulse((PI-2.0)/PI*(pwN-pwC),zero,0.0,0.0);
                if (mag_flg[A] == 'y')
                {
                   magradpulse(gzcal*gzlvl4, gt4);
                }
                else
                {
                   zgradpulse(gzlvl4, gt4);
                }

   rgpulse(pw,t5,152.0e-6,2.0e-6);
   txphase(zero); decphase(zero); dec2phase(zero);
   zgradpulse(gzlvl5, gt5);
   if (pwC180 > 3.0*pwC)
    {
     decpwrf(rfst);
     delay(1/(4.0*jch) - gt5 - 2.0e-6 -pwC180/2.0);
     sim3shaped_pulse("",C180,"",2*pw,pwC180,2.0*pwN,zero,zero,zero,0.0,0.0e-6);
     delay(2.0e-6);
     zgradpulse(gzlvl5, gt5);
     decpwrf(4095.0);
     delay(1/(4.0*jch) -  gt5 - 3.0e-6 -2.0*pwN -pwC180/2.0);
    }
   else
    {
    delay(1/(4.0*jch) - gt5 - 2.0e-6 -pwC);
    sim3pulse(2*pw,2.0*pwC,2.0*pwN,zero,zero,zero,0.0,0.0e-6);
    delay(2.0e-6);
    zgradpulse(gzlvl5, gt5);
    delay(1/(4.0*jch) -  gt5 - 3.0e-6 -pwC -2.0*pwN);
    } 

   sim3pulse(0.0,pwC,pwN,t5,zero,zero,0.0,0.0);     
   sim3pulse(0.0,pwC,pwN,t5,t3,t3,2.0e-6,0.0);     
   rgpulse(pw,t5,1.0e-6,rof2);                        /* flip-back pulse  */
   rcvron();
   dec2power(dpwr2);
   decpower(dpwr);
   setreceiver(t4);
   if ((STUD[A] == 'y') && (dm[D] == 'y'))
    {
     setstatus(DECch, FALSE, 'c', FALSE, dmf);     /* override status mode */
     decpower(studlvl);
     decprgon(stCdec, 1.0/stdmf, 1.0);
     decon();
     if(dm2[D] == 'y')		
      {
       setstatus(DEC2ch, TRUE, dmm2[D], FALSE, dmf2); 
      }
    }
   else
    status(D);
}                 /* end of pulse sequence */
