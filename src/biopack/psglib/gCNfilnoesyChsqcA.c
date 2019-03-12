/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gCNfilnoesyChsqcA.c

    3D C13,N15 filtered, C13 edited noesy with separation via the carbon
     of the destination site
     recorded on a water sample 


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
       
       Added CN filtering   Marco Tonelli, NMRFAM
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
static int
            phi1[2]  = {0,2},
            phi2[4]  = {0,0,2,2},
            phi3[8]  = {0,0,0,0, 2,2,2,2},
            phi6[16] = {0,0,0,0, 0,0,0,0, 2,2,2,2, 2,2,2,2},
            phi7[32] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
                        2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2},
            rec[32]  = {0,2,2,0, 2,0,0,2, 2,0,0,2, 0,2,2,0,
                        2,0,0,2, 0,2,2,0, 0,2,2,0, 2,0,0,2};

                    
static double d2_init=0.0, d3_init=0.0;
static double   H1ofs=4.7, C13ofs=43.0, N15ofs=120.0, H2ofs=0.0;

static shape offC10, stC30, stC200;

void pulsesequence()
{
/* DECLARE VARIABLES */

 char     
	    aliph[MAXSTR],	 		 /* aliphatic CHn groups only */
	    arom[MAXSTR], 			  /* aromatic CHn groups only */
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell             */
            wet[MAXSTR],      /* Flag to select optional WET water suppression */
            satmode[MAXSTR],   /* Flag to select optional water presaturation */
  	    C13refoc[MAXSTR],
  	    Cfil[MAXSTR],     /* in between aliphatic/aromatic for decoupling in t1 */
  	    CNrefoc[MAXSTR];	/* flag for refocusing 15N during indirect H1 evolution */

 int         
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter;   /* used for states tppi in t2           */ 

double    

  taue = 1.0/(4.0*getval("JCH1")),	/* JCH1 = 129 */
  taud = 1.0/(4.0*getval("JCH2")),	/* JCH2 = 145 */
  tauNH = 1.0/(4.0*getval("JNH")),	/* JNH  = 93 */

  dofALL = getval("dofALL"),    /* in between aliphatic/aromatic for decoupling during t1 */
  dofa = 0.0,                   /* actual 13C offset (depends on aliph and arom)*/
  rfst = 0.0,                  /* fine power level for adiabatic pulse initialized*/
  rffil = 0.0,                 /* fine power level for adiabatic pulse */

                                 /* temporary Pbox parameters */
  bw, pws, ofs, ppm, nst,  /* bandwidth, pulsewidth, offset, ppm, # steps */

            pwC10 = getval("pwC10"),    /* 180 degree selective sinc pulse on CO(174ppm) */
            rf7,	                /* fine power for the pwC10 ("offC10") pulse */
            rf0,                        /* full fine power */
            compC = getval("compC"),         /* adjustment for C13 amplifier compression */
            compN = getval("compN"),         /* adjustment for C13 amplifier compression */
             tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             corr,         /*  correction for t2  */
             jch,          /*  CH coupling constant */
             pwN,          /* PW90 for 15N pulse              */
             pwC,          /* PW90 for c nucleus @ pwClvl         */
             pwC180,       /* PW180 for c nucleus in INEPT transfers */
             pwClvl,        /* power level for 13C pulses on dec1  */
             pwNlvl,       /* high dec2 pwr for 15N hard pulses    */


  pwClw=0.0,
  pwNlw=0.0,
  pwZlw=0.0,			/* largest of pwNlw and 2.0*pwClw */


             mix,          /* noesy mix time                       */
             sw1,          /* spectral width in t1 (H)             */
             sw2,          /* spectral width in t2 (C) (3D only)   */
             dz,wetpw,gtw,gswet,gswet2,gstab,
             gt0,
             gt1,
             gt2,
             gt3,
             gt4,
             gt5,
             gt6,
             gt7,
             gt8,
             gt9,
             gzcal,        /* dac to G/cm conversion factor */
             gzlvl0, 
             gzlvl1, 
             gzlvl2, 
             gzlvl3, 
             gzlvl4, 
             gzlvl5,
             gzlvl6,
             gzlvl7,
             gzlvl8,
             gzlvl9;


/* LOAD VARIABLES */

  getstr("aliph",aliph);
  getstr("arom",arom);
  getstr("wet",wet);
  getstr("satmode",satmode);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("CNrefoc",CNrefoc);
  getstr("C13refoc",C13refoc);
  getstr("Cfil",Cfil);

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
  gt6 = getval("gt6");
  gt7 = getval("gt7");
  gt8 = getval("gt8");
  gt9 = getval("gt9");
  gzlvl0 = getval("gzlvl0");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl6 = getval("gzlvl6");
  gzlvl7 = getval("gzlvl7");
  gzlvl8 = getval("gzlvl8");
  gzlvl9 = getval("gzlvl9");


/* LOAD PHASE TABLE */

  settable(t1,2,phi1);
  settable(t2,4,phi2);
  settable(t3,8,phi3);

  settable(t6,16,phi6);
  settable(t7,32,phi7);

  settable(t4,32,rec);

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


/*  Phase incrementation for hypercomplex 2D data */

    if (phase1 == 2) tsadd(t2,1,4);
    if (phase2 == 2) tsadd(t1,1,4);

/*  Set up f1180  tau1 = t1               */
   
    tau1 = d2;
    if(f1180[A] == 'y') {
        tau1 += 1.0 / (2.0*sw1);
    }
    if(tau1 < 0.2e-6) tau1 = 4.0e-7;
    tau1 = tau1/2.0;

/*  Set up f2180  tau2 = t2               */
    if (aliph[A] == 'y')
     corr = (4.0/PI)*pwC + pwC10 +WFG2_START_DELAY +4.0e-6;
    else
     corr = (4.0/PI)*pwC + WFG2_START_DELAY +4.0e-6;

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
   if(t1_counter % 2) 
     { tsadd(t2,2,4); tsadd(t4,2,4);}

   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) 
     { tsadd(t1,2,4);  tsadd(t4,2,4); }

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
         bw = 200.0*ppm; pws = 0.001; ofs = 0.0; nst = 1000.0;    
         stC200 = pbox_makeA("stC200", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
         ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
     }
     rf7 = offC10.pwrf;  pwC10 = offC10.pw;
     rffil=stC200.pwrf;
     if (arom[A]=='y') 
      rfst=stC30.pwrf;
     if (aliph[A]=='y') 
      {
       if (pwC180>3.0*pwC)
         rfst = stC200.pwrf;
       else
         rfst = 4095.0;
      }
   }

   if (arom[A]=='y')  
   {
     dofa=dof+(125-43)*dfrq;
   }

   if (aliph[A]=='y')
   {
     dofa=dof;
   }


/* calculate 3db lower power hard pulses for simultaneous CN decoupling during
   indirect H1 evolution. pwNlw and pwClw should be calculated by the macro that 
   calls the experiment. */

  if (CNrefoc[A] == 'y')
    {
     if(find("pwClw")>1) pwClw=getval("pwClw");
     if(find("pwNlw")>1) pwNlw=getval("pwNlw");

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

   decoffset(dofALL);
   decpower(pwClvl);  /* Set Dec1 power for hard 13C pulses         */
   dec2power(pwNlvl);       /* Set Dec2 to low power for decoupling */

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



if (Cfil[A] == 'y')
   {
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
      zgradpulse(gzlvl6,gt6);
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
      zgradpulse(gzlvl7,gt7);
      txphase(t3); 
      delay(gstab);
   }

status(B);
   rgpulse(pw,t3,rof1,rof1);                  /* 90 deg 1H pulse */
   txphase(t2); 

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
	  sim3pulse(0.0,2.0*pwClw,2.0*pwClw,zero,one,zero,0.0,0.0);
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
     else if ((tau1 > 2.0*pwC +2.0*pw/PI +3.0*SAPS_DELAY +2.0*rof1) && (C13refoc[A]=='y'))
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
   rgpulse(pw,t2,rof1,rof1);             /*  2nd 1H 90 pulse   */

   decoffset(dofa);
   dec2power(dpwr2);
   decpwrf(rfst);                           /* power for inversion pulse */

   if (wet[B] == 'y')
   {
     delay(mix - 10.0e-3 -4.0*wetpw -4.0*gtw -3.0*gswet -gswet2 -dz);    /* mixing time */
     zgradpulse(gzlvl0, gt0);
     decrgpulse(pwC,zero,102.0e-6,2.0e-6); 
     zgradpulse(gzlvl1, gt1);
     delay(10.0e-3 - gt1 - gt0);
     Wet4(zero,one);
   }

  if (satmode[B] == 'y')
  {
    obspower(getval("satpwr"));
    rgpulse(mix-5.0e-3,zero,2.0e-6,2.0e-6); 
    obspower(tpwr);
  }

  if ((satmode[B]=='n') && (wet[B]=='n'))
     delay(mix - 5.0e-3 ); 

     zgradpulse(gzlvl0, gt0);
     delay(gstab);

     decrgpulse(pwC,zero,0.0,0.0); 
     decpwrf(rfst);                           /* power for inversion pulse */
     zgradpulse(gzlvl1, gt1);
     delay(5.0e-3 - gt1 - gt0);

   rgpulse(pw,zero,0.0,2.0e-6);

   zgradpulse(gzlvl2, gt2);       /* g3 in paper   */
   delay(1/(4.0*jch) - gt2 - 2.0e-6 -pwC180/2.0);

   if (pwC180>3.0*pwC)
   {
    if (arom[A]=='y') 
     simshaped_pulse("","stC30",2.0*pw,pwC180,zero,zero,0.0,2.0e-6);
    else
     simshaped_pulse("","stC200",2.0*pw,pwC180,zero,zero,0.0,2.0e-6);
   }
   else
    simpulse(2.0*pw,2.0*pwC,zero,zero,0.0,2.0e-6);

   zgradpulse(gzlvl2, gt2);       /* g4 in paper  */
   decpwrf(4095.0);
   delay(1/(4.0*jch) - gt2 - 2.0e-6 -pwC180/2.0);

   rgpulse(pw,one,1.0e-6,2.0e-6);

   zgradpulse(gzlvl3, gt3);
   decphase(t1);
   delay(gstab);

   decrgpulse(pwC,t1,rof1,2.0e-6);   

   if ( tau2 > 0.0 ) 
     {
      decphase(zero);
      decpwrf(rf7);
      delay(tau2);
      rgpulse(2.0*pw,zero,0.0,0.0);
      decpwrf(4095.0);
      delay(tau2);
     }
    else
     rgpulse(2.0*pw,zero,2.0e-6,2.0e-6);  

   decrgpulse(pwC,zero,2.0e-6,2.0e-6);

   zgradpulse(gzlvl4, gt4);
   delay(gstab);

   rgpulse(pw, zero, 0.0, 0.0);

   zgradpulse(gzlvl5, gt5);


   if (pwC180>3.0*pwC)
     {
      decpwrf(rfst);
      delay(1/(4.0*jch) - gt5 - 2.0e-6 -pwC180/2.0);
      if (arom[A]=='y')
       simshaped_pulse("","stC30",2.0*pw,pwC180,zero,zero,0.0,2.0e-6);
      else
       simshaped_pulse("","stC200",2.0*pw,pwC180,zero,zero,0.0,2.0e-6);
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

   rgpulse(pw,zero,1.0e-6,rof2);                        /* flip-back pulse  */
   decpower(dpwr);

  status(D);
   setreceiver(t4);
}

