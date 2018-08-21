/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef LINT
#endif
/* 
 */
/* Dbppste_ghsqcse 
	Sensitivity enhanced phase sensitive gradient selected HSQC
         with a bppste diffusion filter
Literature reference:
	S. Rajagopalan, C. Chow, V. Vinodhkumar, C. G. Fry and S. Cavagnero:
          J. Biomol. NMR. 29. 505-516 2004.

Parameters:
        d1      - relaxation delay
        pw      - 90 degrees 1H pulse
        tpwr    - 1H pulse power
        pwx i   - 90degrees X pulse
        pwxlvl  - X pulse power level
        j1xh    - 1JXH in Hz (140 for 1H-13C)
        xhn     - '2','1' or '3' flag for signal selection in reverse INEPT
                   sensitivity enhancement factors for different
                   X-multiplicities against normal gHSQC:
	            '1': CH(enh):2.0     CH2(enh):1.0     CH3(enh):1.0
                    '2': CH(enh):1.71    CH2(enh):1.41    CH3(enh):1.21 best for all
                    '3': CH(enh):1.5     CH2(enh):1.37    CH3(enh):1.25
        sspul   - flag for a GRD-90-GRD homospoil block
        gzlvlhs - gradient level for sspul
        hsgt    - gradient duration for sspul
        gtE     - gradient time for coherence selection in seconds
        gzlvlE  - gradient amplitude for coherence selection
        EDratio - Encode/Decode ratio
        gstab   - delay for stability (~ 0.0003 seconds)
        edit    - 'y' makes multiplicity selection (CH & CH3 same sign
                         CH2s opposite sign)
        f1180   - flag to set inital delay for t1 for phase (-90,180)
        satmode - 'y' or 'n' turns presat on or off
        satfrq  - transmitter frequency for presat
        satpwr  - transmitter power for presat
        satdly  - duration of presaturation in seconds
        wet     - flag for optional wet solvent suppression
        del     - diffusion delay
        gzlvl1  - gradient level for diffusion
        gt1     - gradient duration for gzlvl1
        alt_grd - flag to invert diffusion gradient sign on alternating scans
                        (default = 'n')
     lkgate_flg - flag to gate the lock sampling off  during
                              gradient pulses
        nugflag - 'n' uses simple mono- or multi-exponential fitting to
                        estimate diffusion coefficients
                  'y' uses a modified Stejskal-Tanner equation in which the
                        exponent is replaced by a power series
   nugcal_[1-5] - a 5 membered parameter array summarising the results of a
                      calibration of non-uniform field gradients. Used if
                      nugcal is set to 'y'
                      requires a preliminary NUG-calibration by the 
                      Doneshot_nugmap sequence
     dosy3Dproc - 'y' calls dosy with 3D option for phase sensitive 
                        experiments (phase=1,2; set automatically)
         probe_ - stores the probe name used for the dosy experiment

Run the phase sensive 2D HSQC spectra in the same experiment.
  set array as: array='gzlvl1,phase'

peter sandor 25. nov. 2005 */

#include <standard.h>
#include <chempack.h>

static double d2_init = 0.0;

static int phi1[16] = {0,0,0,0,0,0,0,0,3,3,3,3,3,3,3,3},
           phi2[2]  = {0,2},
           phi3[8]  = {0,0,1,1,2,2,3,3},
           phi4[1]  = {2},
           rec[4]   = {0,2,2,0}; 

pulsesequence()
{
 char    f1180[MAXSTR],sspul[MAXSTR],edit[MAXSTR],satmode[MAXSTR],
         alt_grd[MAXSTR],lkgate_flg[MAXSTR],arraystring[MAXSTR];
 int	 phase,icosel,t1_counter,satmove,xhn;
 double  tauxh,       /* 2nd refocus delay (1/4J(XH)) */
         tauxh2,      /* 1st refocus delay to be optimized for HX,H2X,H3X */
         j1xh=getval("j1xh"),           /* coupling for XH           */
         tau1, 	      /* t1/2 */
         gt1 = getval("gt1"),
         gstab = getval("gstab"),
         gzlvlE = getval("gzlvlE"),
         gtE = getval("gtE"),
         EDratio = getval("EDratio"),
         gzlvl1 = getval("gzlvl1"),
         gzlvlhs = getval("gzlvlhs"),
         hsgt = getval("hsgt"),
         del = getval("del"),
         satpwr = getval("satpwr"),
         satfrq = getval("satfrq"),
         satdly = getval("satdly"),
         Dtau, Ddelta, dosytimecubed, dosyfrq, d2corr;

       //synchronize gradients to srate for probetype='nano'
       //   Preserve gradient "area"
          gt1 = syncGradTime("gt1","gzlvl1",0.5);
          gzlvl1 = syncGradLvl("gt1","gzlvl1",0.5);
          gtE = syncGradTime("gtE","gzlvlE",0.5);
          gzlvlE = syncGradLvl("gtE","gzlvlE",0.5);

/* LOAD VARIABLES */
  sw1 = getval("sw1");
  pwx = getval("pwx"); /* PW90 for Channel 2 */
  pwxlvl = getval("pwxlvl");
  phase = (int) (getval("phase") + 0.5);
  xhn = (int) (getval("xhn") + 0.5);

  getstr("satmode",satmode); 
  getstr("f1180",f1180); 
  getstr("sspul",sspul);
  getstr("edit",edit);
  getstr("alt_grd",alt_grd);
  getstr("lkgate_flg",lkgate_flg);
  getstr("array",arraystring);

/* INITIALIZE VARIABLES */
  satmove = ( fabs(tof - satfrq) >= 0.1 );
  if (j1xh == 0.0) j1xh = 140.0;
     tauxh = 1.0/(4.0*j1xh); tauxh2=tauxh/2.0;
     if (xhn == 1) tauxh2=tauxh;
     if (xhn == 2) tauxh2=tauxh/2.0;
     if (xhn == 3) tauxh2=tauxh/3.0;

if (strcmp(arraystring,"gzlvl1,phase")!=0)
            fprintf(stdout,"Warning:  array should be 'gzlvl1,phase' for this experiment");
   Ddelta=gt1;
   Dtau=2.0*pw+rof1+gstab+gt1/2.0;
   dosyfrq = getval("sfrq");
   dosytimecubed=Ddelta*Ddelta*(del-(Ddelta/3.0)-(Dtau/2.0));
   putCmd("makedosyparams(%e,%e)\n",dosytimecubed,dosyfrq);
if (ni>1) putCmd("dosy3Dproc=\'y\'\n");
else putCmd("dosy3Dproc=\'n\'\n");

   if (rof1>2.0e-6) rof1=2.0e-6;

/* In pulse sequence, minimum del=4.0*pw+3*rof1+gt1+2.0*gstab   */
   if((del-(4*pw+3.0*rof1+2.0*tauxh)) < 0.0)
        {abort_message("Warning: 'del' is too short!"); }

/* check validity of parameter range */
    if(hsgt > 3.0e-3 || gt1/2 > tauxh-0.0002 || gtE > 3.0e-3 )
      {  abort_message("gti must be less than 3 ms or 1/4J "); }

/* SET PHASES */
  sub(ct,ssctr,v12); /* Steady-state phase cycling */
  settable(t1,16, phi1); getelem(t1,v12,v1);
  settable(t2, 2, phi2); getelem(t2,v12,v2);
  settable(t3, 8, phi3); getelem(t3,v12,v3);
  settable(t4, 1, phi4); getelem(t4,v12,v4);
  settable(t5, 4, rec);  getelem(t5,v12,oph);

/* Phase incrementation for hypercomplex data */
   if ( phase == 2 )     /* Hypercomplex in t1 */
      { add(v4, two, v4); 
        icosel = 1; /*  and reverse the sign of the gradient  */ }
   else icosel = -1;

/* calculate modification to phases based on current t1 values
   to achieve States-TPPI acquisition */
   if(ix == 1)
      d2_init = d2;
      t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);
      if(t1_counter %2) 
      { add(v2, two, v2); add(oph, two, oph); }

/* set up so that get (-90,180) phase corrects in F1 if f1180 flag is y */
   tau1 = d2;
   if (f1180[A] == 'y')  tau1 += ( 1.0/(2.0*sw1) );
   tau1 = tau1/2.0;
   
/* BEGIN ACTUAL PULSE SEQUENCE */
   mod2(ct,v10);        /* diffusion gradients change sign on every odd scan*/

status(A);
   decpower(pwxlvl);          /* Set decoupler1 power to pwxlvl */

/* Presaturation Period */
 if (sspul[A]=='y')
   {  
         zgradpulse(gzlvlhs,hsgt);
         rgpulse(pw,zero,rof1,rof1);
         zgradpulse(gzlvlhs,hsgt);
   }
 if(satmode[A] == 'y')
   {
     obspower(satpwr);
     if (satmove) obsoffset(satfrq);
     if (d1-satdly > 0.0) delay(d1-satdly);
     rgpulse(satdly,zero,rof1,rof1);  /* Presaturation using transmitter */
     obspower(tpwr);                /* Set power for hard pulses  */
     if (satmove) obsoffset(tof);
   }
   else 
   delay(d1); 

 if (getflag("wet")) wet4(zero,one);

status(B);
  txphase(zero); decphase(zero);

/* First section of the Bppste sequence   */
    rgpulse(pw, zero, rof1, 0.0);             /* first 90 */
         if (lkgate_flg[0] == 'y') lk_hold();   /* turn lock sampling off */
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
               zgradpulse(gzlvl1,gt1/2.0);
            elsenz(v10);
               zgradpulse(-1.0*gzlvl1,gt1/2.0);
            endif(v10);
         }
         else zgradpulse(gzlvl1,gt1/2.0);
    delay(tauxh-gt1/2.0);
    rgpulse(pw*2.0, zero, rof1, 0.0);         /* first 180 */
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
               zgradpulse(-1.0*gzlvl1,gt1/2.0);
            elsenz(v10);
               zgradpulse(gzlvl1,gt1/2.0);
            endif(v10);
         }
         else zgradpulse(-1.0*gzlvl1,gt1/2.0);
    delay(tauxh-gt1/2.0);
    rgpulse(pw, v1, rof1, 0.0);             /* second 90 */
/* Diffusion delay */
   if (satmode[1] == 'y')
     {
        obspower(satpwr);
        if (satfrq != tof) obsoffset(satfrq);
        rgpulse(del-4.0*pw-3.0*rof1-2.0*tauxh,zero,rof1,rof1);
        if (satfrq != tof) obsoffset(tof);
        obspower(tpwr);
     }
     else delay(del-4.0*pw-3.0*rof1-2.0*tauxh); /*diffusion delay */
/* Second half of Bppste */
    rgpulse(pw, v1, rof1, 0.0);             /* third 90 */
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
               zgradpulse(gzlvl1,gt1/2.0);
            elsenz(v10);
               zgradpulse(-1.0*gzlvl1,gt1/2.0);
            endif(v10);
         }
         else zgradpulse(gzlvl1,gt1/2.0);
    txphase(zero); decphase(zero);
    delay(tauxh-gt1/2.0);
    simpulse(pw*2.0, 2*pwx, zero, zero, rof1, rof1);  /* second 180 */
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
               zgradpulse(-1.0*gzlvl1,gt1/2.0);
            elsenz(v10);
               zgradpulse(gzlvl1,gt1/2.0);
            endif(v10);
         }
         else zgradpulse(-1.0*gzlvl1,gt1/2.0);
    txphase(one); decphase(v2);
    delay(tauxh-gt1/2.0);

  rgpulse(pw,one,0.0,0.0); 
  zgradpulse(gzlvlhs,hsgt);   /* homospoil gradient for zz/state selection */
  delay(gstab);
  decrgpulse(pwx,v2,0.0,0.0);
  txphase(zero); decphase(v3); 
  d2corr=pw+2.0*pwx/3.1416;
  if (tau1 > d2corr) delay(tau1-d2corr);  /* delay=t1/2 */
  else delay(tau1);  /* just to dps the evolution period */
  rgpulse(2*pw,zero,0.0,0.0);
  txphase(zero);
  if (tau1 > pw) delay(tau1-pw);
  else delay(tau1);  /* just to dps the evolution period */
  if (edit[A] == 'y')
   { delay(2.0*tauxh  -  gtE - gstab - 2.0*GRADIENT_DELAY);
     zgradpulse(icosel*gzlvlE,gtE);
     delay(gstab);
     simpulse(2.0*pw,2.0*pwx,zero,v3,0.0,0.0);
     delay(2.0*tauxh - 2.0*GRADIENT_DELAY-2.0*pwx/3.1416);
   }
  else
   {
     zgradpulse(icosel*gzlvlE,gtE);
     delay(gstab );
     decrgpulse(2*pwx,v3,0.0,0.0);
     delay(gtE+gstab-2.0*pwx/3.1416);
   }
  decphase(v4); 
  simpulse(pw,pwx,zero,v4,0.0,0.0); /*  X read pulse */
  txphase(zero); decphase(zero); 
  delay(tauxh2 );                /* delay=1/4J (XH)  */
  simpulse(2*pw,2*pwx,zero,zero,0.0,0.0);  
  txphase(one); decphase(one); 
  delay(tauxh2 );                /* delay=1/4J (XH)  */
  simpulse(pw,pwx,one,one,0.0,0.0);
  decphase(zero); txphase(zero);
  delay(tauxh); /* delay=1/4J (XH)  */
  simpulse(2*pw,2*pwx,zero,zero,0.0,0.0);
   delay(tauxh );  /* delay=1/4J(XH)   */
  rgpulse(pw,zero,0.0,0.0);
  decpower(dpwr);
  delay(gtE/2.0 + gstab + 2.0*GRADIENT_DELAY - POWER_DELAY);
  rgpulse(2*pw,zero,0.0,rof2);
  zgradpulse(2.0*gzlvlE/EDratio,gtE/2.0);
  delay(gstab);
  if (lkgate_flg[0] == 'y') lk_sample();     /* turn lock sampling on */
status(C);
}
