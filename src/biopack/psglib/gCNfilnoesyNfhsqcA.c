/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gCNfilnoesyNfhsqcA.c    

   program coded by Marco Tonelli and Klaas Hallenga, NMRFAM, January 2004

   for regular NOESY N15-HSQC minimum step cycling is 4
   for TROSY NOESY N15-HSQC minimum step cycling is 8
   
   Modified by Eriks Kupce :
   the TROSY phase cycle corrected to pick the correct component and to use the 
   sensitivity enhanced version. The minimum phase cycle for TROSY reduced to 4 steps.
   Corrected the d2 timing in the TROSY version. Added soft watergate option via
   wtg3919 flag.   
   Use f2coef = '1 0 -1 0 0 1 0 1' for TROSY

   tau1 timing corrected for regular experiment (4*pwN/PI correction added)
   tau1 timing corrected for TROSY experiment
   f1180 flag added for starting t1 at half-dwell
   C13shape flag added for chosing betweem adiabatic or composite 13C refocussing pulse in t1 
   (Marco Tonelli and Klaas Hallenga, NMRFAM, Univ. Wisconsin).

   Auto-calibrated version, E.Kupce, 27.08.2002.

   The autocal and checkofs flags are generated automatically in Pbox_bio.h
   If these flags do not exist in the parameter set, they are automatically 
   set to 'y' - yes. In order to change their default values, create the flag(s) 
   in your parameter set and change them as required. 
   The available options for the checkofs flag are: 'y' (yes) and 'n' (no). 
   The offset (tof, dof, dof2 and dof3) checks can be switched off individually 
   by setting the corresponding argument to zero (0.0).
   For the autocal flag the available options are: 'y' (yes - by default), 
   'q' (quiet mode - suppress Pbox output), 'r' (read from file, no new shapes 
   are created), 's' (semi-automatic mode - allows access to user defined 
   parameters) and 'n' (no - use full manual setup, equivalent to the original code).

   Timing fixed in H1 t1 dimension. Marco@NMRAFAM (Nov.2004).


Notes for noesy N15 hsqc:

   -mix is the mixing time
 
   -rp1 (zero order phase correction for H1 indirect dimension) should be 135 degrees

   WATERGATE OPTIONS:
   -tauWG is the delay for the 3919 watergate pulse train (wtg3919='y')
   -for using watergate with shaped pulses set wtg3919='n', set all the parameters 
    (important are pw, tpwr, pwHs) and fine powers and 
    small angle phase corrections for up and down flipback pulses (phincr_d, tpwrsf_d 
    and phincr_u, tpwrsf_u respectively).

   -to control water magnetization during H1 indirect evolution weak +/- gradients can
    be used. gt6 is the fixed length for these gradients, gzlvl6 is the power level.
    gstab is the recovery delay after the second gradient.
    gt6 should be not shorter than 500us and the power level, gzlvl6, should be low (400).
    Note. Using these gradients is not recomended on coldprobes as it can result in 
    distorion of baseline in H1 indirect dimension. To turn off these gradients set 
    gzlvl6 to zero.

*/


#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */

static int phx[1]={0}, phy[1]={1}, ph_y[1]={3},

	   phi1[8]  = {1,1,1,1, 3,3,3,3},
           phi2[4]  = {0,0,2,2},

           phi6[2]  = {0,2},
           /*rec1[8]  = {2,0,0,2, 0,2,2,0},*/

	   phi8[16] = {0,0,0,0, 0,0,0,0, 2,2,2,2, 2,2,2,2},
	   phi9[32] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
		       2,2,2,2, 2,2,2,2 ,2,2,2,2, 2,2,2,2},

           rec2[32]  = {2,0,0,2, 0,2,2,0, 0,2,2,0, 2,0,0,2,
                        0,2,2,0, 2,0,0,2, 2,0,0,2, 0,2,2,0},

/* phase cycling for TROSY */
           phT2[4] = {1,3,0,2},                     
           phT6[8] = {0,0,0,0, 2,2,2,2},
           phT7[4] = {1,3,2,0},
           recT[8] = {1,3,2,0, 3,1,0,2};   /* NOTE: minimum nt = 8 */

static double   d2_init = 0.0, d3_init = 0.0;
static double   H1ofs=4.7, C13ofs=0.0, N15ofs=120.0, H2ofs=0.0;

static shape H2Osinc, stC200;

void pulsesequence()
{
  int       t1_counter,
            t2_counter,
	    ni2=getval("ni2");

  char	    C13refoc[MAXSTR],	      /* C13 refocussing pulse in middle of t1 */
  	    N15refoc[MAXSTR],	      /* C13 refocussing pulse in middle of t1 */
	    C13shape[MAXSTR],   /* choose between sech/tanh or composite pulse */
            TROSY[MAXSTR],
            wtg3919[MAXSTR],
	    f1180[MAXSTR],   		       /* Flag to start t1 @ halfdwell */
	    f2180[MAXSTR];   		       /* Flag to start t2 @ halfdwell */

  double    


            tauxh, tau1, tau2, corr,
	    tauWG=getval("tauWG"),
	    mix=getval("mix"),
            pwNt = 0.0,               /* pulse only active in the TROSY option */
            gsign = 1.0,
            gzlvl3=getval("gzlvl3"),
            gt3=getval("gt3"),

            gzlvl6=getval("gzlvl6"),	/* gradients during H1 indirect evolution, tau1 */
            gt6=getval("gt6"),		/* set gzlvl6 to zero for no gradients */

            gstab=getval("gstab"),			/* gradient recovery delay */
            JNH = getval("JNH"),
            pwN = getval("pwN"),
            pwNlvl = getval("pwNlvl"),  

            pwHs, tpwrs=0.0, compH=1.0,          /* H1 90 degree pulse length at tpwrs */               
	    tpwrsf_u = getval("tpwrsf_u"),   /* fine power and small angle phase correction */
	    phincr_u = getval("phincr_u"),   /* for up (u) and down (d) H2O flipback pulses */
	    tpwrsf_d = getval("tpwrsf_d"),   /* of watergate pulse */
	    phincr_d = getval("phincr_d"),

  	    pwClw=getval("pwClw"),
  	    pwNlw=getval("pwNlw"),
  	    pwZlw = 0.0,                     /* largest of pwNlw and 2*pwClw */

            sw1 = getval("sw1"),
            sw2 = getval("sw2"),
                               /* temporary Pbox parameters */
            bw, pws, ofs, ppm, nst,  /* bandwidth, pulsewidth, offset, ppm, # steps */
            pwClvl = getval("pwClvl"), 	         /* coarse power for C13 pulse */
            pwC = getval("pwC"),       /* C13 90 degree pulse length at pwClvl */
            rfst = 4095.0,	            /* fine power for the stCall pulse */
            compC = getval("compC"),   /* adjustment for C13 amplifier compr-n */


  CNfil=getval("CNfil"),  /* flag for selecting the type of Cfilter to be used */
  JCH1 = getval("JCH1"),        /* smallest coupling that you wish to purge */
  JCH2 = getval("JCH2"),        /* largest coupling that you wish to purge */
  tauCH1 = 0.0,                         /* 1/(2JCH1)   */
  tauCH2 = 0.0,                         /* 1/(2JCH2)   */

/* N15 purging */
  tauNH  = 1/(4.0*JNH),               /* HN coupling constant */

  rffil = 0.0,			/* fine power level for 200ppm adiabatic pulse */
  rf0,				/* full fine power */

  gt14 = getval("gt14"),
  gt17 = getval("gt17"),
  gt8 = getval("gt8"),
  gt9 = getval("gt9"),

  gzlvl14 = getval("gzlvl14"),
  gzlvl17 = getval("gzlvl17"),
  gzlvl8 = getval("gzlvl8"),
  gzlvl9 = getval("gzlvl9");



    getstr("C13refoc",C13refoc);
    getstr("N15refoc",N15refoc);
    getstr("C13shape",C13shape);
    getstr("TROSY",TROSY);
    getstr("wtg3919",wtg3919);
    getstr("f1180",f1180);
    getstr("f2180",f2180);
    
/* check validity of parameter range */

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
    { text_error("incorrect Dec1 decoupler flags!  ");          psg_abort(1); } 

    if((dm2[A] == 'y' || dm2[B] == 'y') )
    { text_error("incorrect Dec2 decoupler flags!  ");          psg_abort(1); } 

    if( dpwr > 0 )
    { text_error("don't fry the probe, dpwr too large!  ");     psg_abort(1); }

    if( dpwr2 > 48 )
    { text_error("don't fry the probe, dpwr2 too large!  ");    psg_abort(1); }

    if (TROSY[A]=='y') 
    { if(dm2[C] == 'y') { text_error("Choose either TROSY='n' or dm2='n' ! "); psg_abort(1); }
      /*if(getval("nt")<8) { text_error("WARNING: use nt = 8*n "); psg_abort(1); }*/
    }

/* INITIALIZE VARIABLES */
    
    if(wtg3919[0] != 'y')      /* selective H20 one-lobe sinc pulse needs 1.69  */
    {                                   /* times more power than a square pulse */
      pwHs = getval("pwHs");            
      compH = getval("compH");
    }
    else 
      pwHs = pw*2.385+7.0*rof1+tauWG*2.5; 

    tauxh = ((JNH != 0.0) ? 1/(4*(JNH)) : 2.25e-3);

    setautocal();                        /* activate auto-calibration flags */ 
        
    if (autocal[0] == 'n') 
    {
      if ( (C13refoc[A]=='y') && (C13shape[A]=='y') )
      {
        /* 180 degree adiabatic C13 pulse from 0 to 200 ppm */
        rfst = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)); 
        rfst = (int) (rfst + 0.5);
        if ( 1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n", 
	     (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) );    psg_abort(1); }
      }

      if(wtg3919[0] != 'y')      /* selective H20 one-lobe sinc pulse needs 1.69  */
      {                                   /* times more power than a square pulse */
        if (pwHs > 1e-6) tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));  
        else tpwrs = 0.0;
        tpwrs = (int) (tpwrs); 
      }	  
    }
    else        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
    {
      if(FIRST_FID)                                            /* call Pbox */
      {
        if ( (C13refoc[A]=='y') && (C13shape[A]=='y') )
        {
          ppm = getval("dfrq"); ofs = 0.0;   pws = 0.001;  /* 1 ms long pulse */
          bw = 200.0*ppm;       nst = 1000;          /* nst - number of steps */
          stC200 = pbox_makeA("stC200", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
          C13ofs = 100.0;
        }
        if(wtg3919[0] != 'y')
          H2Osinc = pbox_Rsh("H2Osinc", "sinc90", pwHs, 0.0, compH*pw, tpwr);
        ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
      }
      if ( (C13refoc[A]=='y') && (C13shape[A]=='y') ) rfst = stC200.pwrf;
      if (wtg3919[0] != 'y') 
      { 
        pwHs = H2Osinc.pw; tpwrs = H2Osinc.pwr-1.0;  /* 1dB correction applied */ 
      }
    }


/* calculate 3db lower power hard pulses for simultaneous CN decoupling during
   indirect H1 evoluion */

  if (C13refoc[A] == 'y')
    {
     if (pwNlw==0.0) pwNlw = pwN*exp(3.0*2.303/20.0);
     if (pwClw==0.0) pwClw = pwC*exp(3.0*2.303/20.0);
     if (pwNlw > 2.0*pwClw)
         pwZlw=pwNlw;
      else
         pwZlw=2.0*pwClw;
     if (d2==0.0 && d3==0.0) printf(" pwClw = %.2f ; pwNlw = %.2f\n", pwClw*1e6,pwNlw*1e6);
    }

/* LOAD VARIABLES */

    if(ix == 1) d2_init = d2;
    t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);

    if(ix == 1) d3_init = d3;
    t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5);

    
/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;


/*  Set up f2180  */
   
    tau2 = d3;
    if((f2180[A] == 'y') && (ni2 > 1.0))
	{ tau2 += ( 1.0 / (2.0*sw2) ); if(tau2 < 0.2e-6) tau2 = 0.0; }
    tau2 = tau2/2.0;


/* LOAD PHASE TABLES */

      settable(t5,  1, phx);
    if (TROSY[A] == 'y')
    { gsign = -1.0;
      pwNt = pwN;
      assign(zero,v7); 
      assign(two,v8);
      settable(t1,  1, phy);
      settable(t2,  4, phT2);
      settable(t3,  1, ph_y); 
      settable(t4,  1, ph_y);
      settable(t6,  8, phT6);
      settable(t7,  4, phT7); 
      settable(t11, 8, recT); }
    else
    { 
      assign(one,v7); 
      assign(three,v8);
      settable(t1,  8, phi1);
      settable(t2,  4, phi2);
      settable(t3,  1, phx);
      settable(t4,  1, phx);
      settable(t6,  2, phi6);

      settable(t8, 16, phi8);
      settable(t9, 32, phi9);

      settable(t11, 32, rec2); 
/***********************************************
      if (CNfil==1) settable(t11, 32, rec2); 
        else settable(t11, 8, rec1); 
************************************************/
     } 

      if ( phase1 == 2 ) tsadd(t5, 1, 4);     /* Hypercomplex in t1 */
       
      if ( phase2 == 2 )                      /* Hypercomplex in t2 */
       { 
        if (TROSY[A] == 'y')          
         { tsadd(t3, 2, 4); tsadd(t11, 2, 4); }                      
        else tsadd(t2, 1, 4); 
       }
                                   
    if(t1_counter %2)          /* calculate modification to phases based on */
    { tsadd(t5,2,4); tsadd(t11,2,4); }   		/* current t1 values */

    if(t2_counter %2)          /* calculate modification to phases based on */
    { tsadd(t2,2,4); tsadd(t11,2,4); if (TROSY[A]=='y') tsadd(t7,2,4); }   /* current t2 values */

    if(wtg3919[0] != 'y') 
    { add(one,v7,v7); add(one,v8,v8); }


    /* make small angle phase correction always positive */
    if (phincr_d < 0.0) phincr_d=phincr_d+360.0;
    if (phincr_u < 0.0) phincr_u=phincr_u+360.0;


    if(JCH1 != 0.0) tauCH1 = 1.0/(4.0*JCH1);
    if(JCH2 != 0.0) tauCH2 = 1.0/(4.0*JCH2);


    if (CNfil > 0)
    {
      /* 200ppm sech/tanh inversion = stC200 */
      rffil = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35));
      rffil = (int) (rffil + 0.5);
      if ( 1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
         { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n",
           (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) );    psg_abort(1); }
    }


/* maximum fine power for pwC pulses */
        rf0 = 4095.0;


                           /* sequence starts!! */
   status(A);
     
     obspower(tpwr);
     dec2power(pwNlvl);
     decpower(pwClvl);
     if (C13shape[A]=='y') 
	decpwrf(rfst);
     else
	decpwrf(4095.0);

/* small angle phase correction for H2O watergate flipback pulses */
     initval(phincr_d,v2);
     initval(phincr_u,v3);

     initval(135.0,v1);
     obsstepsize(1.0);

     delay(d1);
     
   status(B);


if (CNfil == 1)
   {
      txphase(t8);
      rgpulse(pw, t8, rof1, 0.0);                  /* 90 deg 1H pulse */
/* BEGIN 1st FILTER */
      txphase(zero);

      zgradpulse(gzlvl8,gt8);
      delay(tauNH -gt8 -2.0*GRADIENT_DELAY);

      sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

      decpwrf(rffil);
      delay(tauNH -tauCH1 -0.5e-3 -WFG_START_DELAY -PWRF_DELAY);

      decshaped_pulse("stC200", 1.0e-3, zero, 0.0, 0.0);

      zgradpulse(gzlvl8,gt8);
      decpwrf(rf0);
      delay(tauCH1 -gt8 -2.0*GRADIENT_DELAY -0.5e-3 -PWRF_DELAY);

      rgpulse(pw, zero, 0.0, 0.0);
      zgradpulse(0.7*gzlvl14,0.5*gt14);
      delay(0.5*gstab);

      sim3pulse(0.0, pwC, pwN, zero, zero, zero, 0.0, 0.0);
      zgradpulse(gzlvl14,0.5*gt14);
      txphase(t9);
      delay(gstab);

      rgpulse(pw, t9, 0.0, 0.0);
/* BEGIN 2nd FILTER */

      zgradpulse(gzlvl9,gt9);
      delay(tauNH -gt9 -2.0*GRADIENT_DELAY);

      sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

      decpwrf(rffil);
      delay(tauNH -tauCH2 -0.5e-3 -WFG_START_DELAY -PWRF_DELAY);

      decshaped_pulse("stC200", 1.0e-3, zero, 0.0, 0.0);

      zgradpulse(gzlvl9,gt9);
      decpwrf(rf0);
      delay(tauCH2 -gt9 -2.0*GRADIENT_DELAY -0.5e-3 -PWRF_DELAY);

      rgpulse(pw, zero, 0.0, 0.0);
      zgradpulse(0.7*gzlvl17,0.5*gt17);
      delay(0.5*gstab);

      sim3pulse(0.0, pwC, pwN, zero, zero, zero, 0.0, 0.0);
      zgradpulse(gzlvl17,0.5*gt17);
      txphase(t5); xmtrphase(v1);
      delay(gstab);

      rgpulse(pw, t5, 0.0, 0.0);
      txphase(t6); xmtrphase(zero);
   }
else if (CNfil == 2)
   {
      txphase(t8);
      rgpulse(pw, t8, rof1, 0.0);                  /* 90 deg 1H pulse */
/* BEGIN 1st FILTER */
      txphase(zero);

      zgradpulse(gzlvl8,gt8);
      delay(tauNH -gt8 -2.0*GRADIENT_DELAY);

      sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

      decpwrf(rffil);
      delay(tauNH -tauCH1 -0.5e-3 -WFG_START_DELAY -PWRF_DELAY);

      decshaped_pulse("stC200", 1.0e-3, zero, 0.0, 0.0);

      zgradpulse(gzlvl8,gt8);
      decpwrf(rf0);
      delay(tauCH1 -gt8 -2.0*GRADIENT_DELAY -0.5e-3 -PWRF_DELAY);

      sim3pulse(pw, pwC, pwN, zero, zero, zero, 0.0, 0.0);
      zgradpulse(gzlvl14,gt14);
      txphase(t9);
      delay(gstab);

      rgpulse(pw, t9, 0.0, 0.0);
/* BEGIN 2nd FILTER */

      zgradpulse(gzlvl9,gt9);
      delay(tauNH -gt9 -2.0*GRADIENT_DELAY);

      sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

      decpwrf(rffil);
      delay(tauNH -tauCH2 -0.5e-3 -WFG_START_DELAY -PWRF_DELAY);

      decshaped_pulse("stC200", 1.0e-3, zero, 0.0, 0.0);

      zgradpulse(gzlvl9,gt9);
      decpwrf(rf0);
      delay(tauCH2 -gt9 -2.0*GRADIENT_DELAY -0.5e-3 -PWRF_DELAY);

      sim3pulse(pw, pwC, pwN, zero, zero, zero, 0.0, 0.0);
      zgradpulse(gzlvl17,gt17);
      txphase(t5); xmtrphase(v1);
      delay(gstab);

      rgpulse(pw, t5, 0.0, 0.0);
      txphase(t6); xmtrphase(zero);
   }

/* H1 EVOLUTION BEGINS */

     if ((C13refoc[A]=='y') && (N15refoc[A]=='y') && (tau1 > pwZlw +2.0*pw/PI +3.0*SAPS_DELAY +2.0*POWER_DELAY +2.0*rof1))
       {
        decpower(pwClvl-3.0); dec2power(pwNlvl-3.0);

        corr = pwZlw +2.0*pw/PI +SAPS_DELAY +2.0*POWER_DELAY +2.0*rof1;
	if ( ((tau1 -corr -gt6 -gstab) > 0.0) && (gzlvl6 > 0.0))
	  {
	   zgradpulse(-gzlvl6, gt6);
           delay(tau1 -corr -2.0*SAPS_DELAY -gt6 -2.0*GRADIENT_DELAY);
	  }
        else
          delay(tau1 -corr -2.0*SAPS_DELAY);

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
	if ( ((tau1 -corr -gt6 -gstab) > 0.0) && (gzlvl6 > 0.0))
	  {
           delay(tau1 -corr -gt6 -gstab);
	   zgradpulse(gzlvl6, gt6);
           delay(gstab -2.0*GRADIENT_DELAY);
	  }
        else
          delay(tau1 -corr);
       }    
     else if ((N15refoc[A]=='y') && (tau1 > (pwN +2.0*pw/PI +2.0*SAPS_DELAY +2.0*rof1)))
       {
	corr =  pwN +2.0*pw/PI +2.0*rof1;
	if ( ((tau1 -corr -gt6 -gstab) > 0.0) && (gzlvl6 > 0.0))
	  {
	   zgradpulse(-gzlvl6, gt6);
           delay(tau1 -corr -2.0*SAPS_DELAY -gt6 -2.0*GRADIENT_DELAY);
 	   dec2rgpulse(2.0*pwN, zero, rof1, rof1);
           delay(tau1 -corr -gt6 -gstab);
	   zgradpulse(gzlvl6, gt6);
           delay(gstab -2.0*GRADIENT_DELAY);
	  }
 	else
	  {
	   delay(tau1 -corr -2.0*SAPS_DELAY);
 	   dec2rgpulse(2.0*pwN, zero, rof1, rof1);
	   delay(tau1 -corr);
          }
       }
     else if (tau1 > 2.0*pw/PI +SAPS_DELAY)
	delay(2.0*tau1 -2.0*rof1 -4.0*pw/PI -2.0*SAPS_DELAY);

/* H1 EVOLUTION ENDS */
     rgpulse(pw, t6, rof1, rof1);
     txphase(zero);

     delay(mix -gt3 -gstab);
     dec2rgpulse(2.0*pwN, zero, rof1, rof1);
     zgradpulse(0.7*gzlvl3,gt3);
     delay(gstab);

     rgpulse(pw, zero, rof1, rof1);
     
     zgradpulse(0.3*gzlvl3,gt3);
     txphase(zero);
     dec2phase(zero);
     delay(tauxh-gt3);               /* delay=1/4J(XH)   */

     sim3pulse(2*pw,0.0,2*pwN,t4,zero,zero,rof1,rof1);

     zgradpulse(0.3*gzlvl3,gt3);
     dec2phase(t2);
     delay(tauxh-gt3 );               /* delay=1/4J(XH)   */
  
     rgpulse(pw, t1, rof1, rof1);

     zgradpulse(0.5*gsign*gzlvl3,gt3);
     delay(gstab); 
     decphase(zero);
            
     if (TROSY[A] == 'y')
     { 
       txphase(t3);       
       if ( phase2 == 2 ) 
         dec2rgpulse(pwN, t7, rof1, rof1);
       else 
         dec2rgpulse(pwN, t2, rof1, rof1);              

       if ( (C13shape[A]=='y') && (C13refoc[A]=='y') 
	   && (tau2 > 0.5e-3 +WFG2_START_DELAY +rof1) )
         {
         delay(tau2 -0.5e-3 -WFG2_START_DELAY -rof1);     
         decshaped_pulse("stC200", 1.0e-3, zero, 0.0, 0.0);
         delay(tau2 -0.5e-3 -WFG2_STOP_DELAY -rof1);
         }
       else if ( (C13shape[A]!='y') && (C13refoc[A]=='y') 
                && (tau2 > 2.0*pwC +SAPS_DELAY +2.0*rof1) )
         {
         delay(tau2 -2.0*pwC -SAPS_DELAY -2.0*rof1); 
	 decrgpulse(pwC, zero, rof1, 0.0);
	 decphase(one);
         decrgpulse(2.0*pwC, one, 0.0, 0.0);  
	 decphase(zero);
	 decrgpulse(pwC, zero, 0.0, rof1);
         delay(tau2 -2.0*pwC -SAPS_DELAY -2.0*rof1); 
         }
       else if (tau2 > rof1)
         delay(2.0*tau2 -2.0*rof1);

       rgpulse(pw, t3, rof1, rof1);         
       zgradpulse(gzlvl3,gt3);
       delay(tauxh -gt3);
       
       sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,rof1,rof1);
       
       zgradpulse(gzlvl3,gt3);
       delay(tauxh-gt3 );       
       sim3pulse(pw,0.0,pwN,zero,zero,t3,rof1,rof1);
     }
     else
     {         
       txphase(t4);      
       dec2rgpulse(pwN, t2, rof1, rof1);
       dec2phase(t3);

       if ( (C13shape[A]=='y') && (C13refoc[A]=='y') 
           && (tau2 > 0.5e-3 +WFG2_START_DELAY +2.0*pwN/PI +SAPS_DELAY +rof1) )
         {
         delay(tau2 -0.5e-3 -WFG2_START_DELAY -2.0*pwN/PI -SAPS_DELAY -rof1); 
         simshaped_pulse("", "stC200", 2.0*pw, 1.0e-3, t4, zero, 0.0, 0.0);  
         delay(tau2 -0.5e-3 -WFG2_STOP_DELAY -2.0*pwN/PI -SAPS_DELAY -rof1);
         }
       else if ( (C13shape[A]!='y') && (C13refoc[A]=='y') 
                && (tau2 > 2.0*pwN/PI +2.0*pwC +2.0*SAPS_DELAY +2.0*rof1) )
         {
         delay(tau2 -2.0*pwN/PI -2.0*pwC -2.0*SAPS_DELAY -2.0*rof1); 
	 decrgpulse(pwC, zero, rof1, 0.0);
	 decphase(one);
         simpulse(2.0*pw, 2.0*pwC, t4, one, 0.0, 0.0);  
	 decphase(zero);
	 decrgpulse(pwC, zero, 0.0, rof1);
         delay(tau2 -2.0*pwN/PI -2.0*pwC -SAPS_DELAY -2.0*rof1); 
         }
       else if (tau2 > pw +2.0*pwN/PI +SAPS_DELAY +2.0*rof1) 
         {
         delay(tau2 -pw -2.0*pwN/PI -SAPS_DELAY -2.0*rof1);
         rgpulse(2.0*pw, t4, rof1, rof1);
         delay(tau2 -2.0*pwN/PI -pw -2.0*rof1);
         }
       else
         rgpulse(2.0*pw, t4, rof1, rof1);
       
       dec2rgpulse(pwN, t3, rof1, rof1);
       
       zgradpulse(0.5*gzlvl3,gt3);
       delay(gstab);
       rgpulse(pw, two, rof1, rof1);
     } 
     
     zgradpulse(gzlvl3,gt3);
     txphase(v7); dec2phase(zero);
     delay(tauxh-gt3-pwHs-rof1);
     
     if(wtg3919[0] == 'y')
     {     	
       rgpulse(pw*0.231,v7,rof1,rof1);     
       delay(tauWG);
       rgpulse(pw*0.692,v7,rof1,rof1);
       delay(tauWG);
       rgpulse(pw*1.462,v7,rof1,rof1);

       delay(tauWG/2-pwN);
       dec2rgpulse(2*pwN, zero, rof1, rof1);
       txphase(v8);
       delay(tauWG/2-pwN);

       rgpulse(pw*1.462,v8,rof1,rof1);
       delay(tauWG);
       rgpulse(pw*0.692,v8,rof1,rof1);
       delay(tauWG);
       rgpulse(pw*0.231,v8,rof1,rof1); 
     }
     else
     {
       xmtrphase(v2);
       if (tpwrsf_d < 4095.0)
	 {obspower(tpwrs+6.0); obspwrf(tpwrsf_d);}
       else	
          obspower(tpwrs); 
       shaped_pulse("H2Osinc", pwHs, v7, rof1, 0.0);

       xmtrphase(zero);
       obspower(tpwr); if (tpwrsf_d < 4095.0) obspwrf(4095.0);
       sim3pulse(2.0*pw, 0.0, 2.0*pwN, v8, zero, zero, 0.0, 0.0);

       xmtrphase(v3);
       if (tpwrsf_d < 4095.0)
         {obspower(tpwrs+6.0); obspwrf(tpwrsf_u);}
       else
          obspower(tpwrs);
       shaped_pulse("H2Osinc", pwHs, v7, rof1, 0.0);
       xmtrphase(zero);
       obspower(tpwr); if (tpwrsf_d < 4095.0) obspwrf(4095.0);
     } 
        
     zgradpulse(gzlvl3,gt3);   
     delay(tauxh-gt3-pwHs-rof1-pwNt-POWER_DELAY); 
     dec2rgpulse(pwNt, zero, rof1, rof2); 
     dec2power(dpwr2);

   status(C);
     setreceiver(t11);   
}



