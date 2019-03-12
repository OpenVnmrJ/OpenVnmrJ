/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* PR42_intra_ghncacbP_TROSY.c 

Ref: (4,2)D Projection-Reconstruction Experiemnts for Protein Backbone
Assignment:  Application to Human Carbonic Anhydrase II and Calbindin
D28K.  Venters, R.A., Coggins, B.E., Kojetin, D., Cavanagh, J. and
Zhou, P. JACS 127(24), 8785-8795 (2005)
Modified for BioPack from  PR42_INTRA_HNCACB_TROSY.c 

To obtain reconstruction software package, please visit
http://zhoulab.biochem.duke.edu/software/pr-calc

 
Converted by Pei Zhou to intra-hnca sequence on 7/6/04
Reference:  JACS 124, 11199-11207, 2002

 set tauNCa = 26 ms; tauNCo =16.5 ms, tauCaCo=4.2 ms, timeTN=15.8 ms
     tauCaCb=7.0 ms

     S/N=13.3 with 0.9 mM HCAII at 25C, 16 scans (passive suppression)

Modified on 10/07/04 by R. Venters to include proper tilts
and to increment Ca evolution.

Corrected by R. Venters on 12/07/04.  Added active suppression 
and glycine inversion pulse.
*/


#include <standard.h>
#include "bionmr.h"
#include "Pbox_bio.h"
  
static shape gly90;
static int  
       phx[1]={0},   phy[1]={1},
       phi3[2]  = {0,2},
       phi5[4]  = {0,0,2,2},
       phi9[8]  = {0,0,0,0,2,2,2,2},
       rec[4]   = {0,2,2,0};

void pulsesequence()
{

char        sel_flg[MAXSTR],
            autocal[MAXSTR],
            glyshp[MAXSTR];

int         icosel, t1_counter, ni = getval("ni");

double
   d2_init=0.0, 
   tau1, tau2, 
   tau3,  
   glypwr,glypwrf,                    /* Power levels for Cgly selective 90 */
   pwgly,                              /* Pulse width for Cgly selective 90 */

   waltzB1  = getval("waltzB1"),     /* 1H decoupling strength (in Hz) */
   timeTN  = getval("timeTN"),     /* constant time for 15N evolution */
   tauCaCb = getval("tauCaCb"),    
   tauNCa  = getval("tauNCa"),
   tauNCo  = getval("tauNCo"),
   tauCaCo = getval("tauCaCo"),
            
   compH = getval("compH"),        /* adjustment for H1 amplifier compression */
   tpwrs,                        /* power for the pwHs ("H2Osinc") pulse */
   bw,ppm,

   pwClvl = getval("pwClvl"),              /* coarse power for C13 pulse */
   compC = getval("compC"),      /* amplifier compression for C13 pulse */
   pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */

   pwNlvl = getval("pwNlvl"),                   /* power for N15 pulses */
   pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
   dpwr2 = getval("dpwr2"),            /* power for N15 decoupling */

   pwCa90,                          /* length of square 90 on Ca */
   pwCa180,
   pwCab90,
   pwCab180,
   phshift,        /*  phase shift induced on Ca by 180 on CO in middle of t1 */
   pwCO180,                                /* length of 180 on CO */
   pwS = getval("pwS"), /* used to change 180 on CO in t1 for 1D calibrations */
   pwZ,                            /* the largest of pwCO180 and 2.0*pwN */
   pwZ1,             /* the largest of pwCO180 and 2.0*pwN for 1D experiments */

   sw1 = getval("sw1"),   
   swCb = getval("swCb"),
   swCa = getval("swCa"),
   swN  = getval("swN"),
   swTilt,                     /* This is the sweep width of the tilt vector */

   cos_N, cos_Ca, cos_Cb,
   angle_N, angle_Ca, angle_Cb,      /* angle_N is calculated automatically */

  gstab = getval("gstab"),
   gt0 = getval("gt0"),             gzlvl0 = getval("gzlvl0"),
   gt1 = getval("gt1"),             gzlvl1 = getval("gzlvl1"),
                                    gzlvl2 = getval("gzlvl2"),
   gt3 = getval("gt3"),             gzlvl3 = getval("gzlvl3"),
   gt4 = getval("gt4"),             gzlvl4 = getval("gzlvl4"),
   gt5 = getval("gt5"),             gzlvl5 = getval("gzlvl5"),
   gt6 = getval("gt6"),             gzlvl6 = getval("gzlvl6"),
   gt7 = getval("gt7"),             gzlvl7 = getval("gzlvl7"),
   gt10= getval("gt10"),            gzlvl10= getval("gzlvl10"),
   gt11= getval("gt11"),            gzlvl11= getval("gzlvl11"),
   gt12= getval("gt12"),            gzlvl12= getval("gzlvl12");

   angle_N = 0;
   glypwr = getval("glypwr");
   pwgly = getval("pwgly");
   tau1 = 0;
   tau2 = 0;
   tau3 = 0;
   cos_N = 0;
   cos_Cb = 0;
   cos_Ca = 0;
   getstr("autocal", autocal);
   getstr("glyshp", glyshp);
   getstr("sel_flg",sel_flg);

   pwHs = getval("pwHs");          /* H1 90 degree pulse length at tpwrs */
 
/*   LOAD PHASE TABLE    */

   settable(t2,1,phy);
   settable(t3,2,phi3);     settable(t4,1,phx);     settable(t5,4,phi5);
   settable(t8,1,phy);      settable(t9,8,phi9);    settable(t10,1,phx);
   settable(t11,1,phy);     settable(t12,4,rec);

/*   INITIALIZE VARIABLES   */

   kappa = 5.4e-3;     lambda = 2.4e-3;

/* selective H20 one-lobe sinc pulse */

   
   tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
   tpwrs = (int) (tpwrs);                        /*power than a square pulse */

   pwHs = 1.7e-3*500.0/sfrq;
   widthHd = 2.861*(waltzB1/sfrq);   /* bandwidth of H1 WALTZ16 decoupling in ppm */
   pwHd = h1dec90pw("WALTZ16", widthHd, 0.0);     /* H1 90 length for WALTZ16 */
 
/* get calculated pulse lengths of shaped C13 pulses */
   pwCa90  = c13pulsepw("ca", "co", "square", 90.0); 
   pwCa180 = c13pulsepw("ca", "co", "square", 180.0);
   pwCO180 = c13pulsepw("co", "cab", "sinc", 180.0); 
   pwCab90 = c13pulsepw("cab","co", "square", 90.0);
   pwCab180= c13pulsepw("cab","co", "square", 180.0);

/* the 180 pulse on CO at the middle of t1 */
   if (pwCO180 > 2.0*pwN) pwZ = pwCO180; else pwZ = 2.0*pwN;
   if ((pwS==0.0) && (pwCO180>2.0*pwN)) pwZ1=pwCO180-2.0*pwN; else pwZ1=0.0;
   if ( ni > 1 )     pwS = 180.0;
   if ( pwS > 0 )   phshift = 320.0;
     else             phshift = 0.0;

/* CHECK VALIDITY OF PARAMETER RANGES */

   if ( dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' )
      { printf("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1);}

   if ( dm2[A] == 'y' || dm2[B] == 'y' )
      { printf("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1);}

   if ( dm3[A] == 'y' || dm3[C] == 'y' )
      { printf("incorrect dec3 decoupler flags! Should be 'nyn' or 'nnn' ");
                                                psg_abort(1);}     
   if ( dpwr2 > 46 )
      { printf("dpwr2 too large! recheck value  ");               psg_abort(1);}

   if ( pw > 20.0e-6 )
      { printf(" pw too long ! recheck value ");                  psg_abort(1);} 
  
   if ( pwN > 100.0e-6 )
      { printf(" pwN too long! recheck value ");                  psg_abort(1);} 


/* PHASES AND INCREMENTED TIMES */

   /* Set up angles and phases */

   angle_Cb=getval("angle_Cb");  cos_Cb=cos(PI*angle_Cb/180.0);
   angle_Ca=getval("angle_Ca");  cos_Ca=cos(PI*angle_Ca/180.0);

   if ( (angle_Cb < 0) || (angle_Cb > 90) )
   {  printf ("angle_Cb must be between 0 and 90 degree.\n"); psg_abort(1); }

   if ( (angle_Ca < 0) || (angle_Ca > 90) )
   {  printf ("angle_Ca must be between 0 and 90 degree.\n"); psg_abort(1); }

   if ( 1.0 < (cos_Cb*cos_Cb + cos_Ca*cos_Ca) )
   {
       printf ("Impossible angles.\n"); psg_abort(1);
   }
   else
   {
           cos_N=sqrt(1.0- (cos_Cb*cos_Cb + cos_Ca*cos_Ca));
           angle_N = 180.0*acos(cos_N)/PI;
   }

   swTilt=swCb*cos_Cb + swCa*cos_Ca + swN*cos_N;

   if (ix ==1)
   {
      printf("\n\nn\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
      printf ("Maximum Sweep Width: \t\t %f Hz\n", swTilt);
      printf ("Angle_Cb:\t%6.2f\n", angle_Cb);
      printf ("Angle_Ca:\t%6.2f\n", angle_Ca);
      printf ("Angle_N :\t%6.2f\n", angle_N );
   }

/* Set up hyper complex */

   /* sw1 is used as symbolic index */
   if ( sw1 < 1000 ) { printf ("Please set sw1 to some value larger than 1000.\n"); psg_abort(1); }

   if (ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if (t1_counter % 2)  { tsadd(t8,2,4); tsadd(t12,2,4); }

   if (phase1 == 1)  { ;}                                                  /* CC */
   else if (phase1 == 2)  { tsadd(t3,3,4); tsadd(t2,3,4);}                 /* SC */
   else if (phase1 == 3)  { tsadd(t5,1,4); }                               /* CS */
   else if (phase1 == 4)  { tsadd(t3,3,4); tsadd(t2,3,4); tsadd(t5,1,4);}  /* SS */
   else { printf ("phase1 can only be 1,2,3,4. \n"); psg_abort(1); }

   if (phase2 == 2)  { tsadd(t10,2,4); icosel = +1; }                      /* N  */
            else                       icosel = -1;

   tau1 = 1.0*t1_counter*cos_Cb/swTilt;
   tau2 = 1.0*t1_counter*cos_Ca/swTilt;
   tau3 = 1.0*t1_counter*cos_N/swTilt;

   tau1 = tau1/2.0;  tau2 = tau2/2.0;  tau3 = tau3/2.0;


/* CHECK VALIDITY OF PARAMETER RANGES */

    if (0.5*ni*(cos_N/swTilt) > timeTN - WFG3_START_DELAY)
       { printf(" ni is too big. Make ni equal to %d or less.\n",
         ((int)((timeTN - WFG3_START_DELAY)*2.0*swTilt/cos_N)));       psg_abort(1);}


/* BEGIN PULSE SEQUENCE */

status(A);
      delay(d1);
      if ( dm3[B] == 'y' ) lk_hold();  

      rcvroff();
      obsoffset(tof);          obspower(tpwr);        obspwrf(4095.0);
      set_c13offset("cab");     decpower(pwClvl);      decpwrf(4095.0);
      dec2power(pwNlvl);

      txphase(zero);           delay(1.0e-5);

      decrgpulse(pwC, zero, 0.0, 0.0);
      zgradpulse(gzlvl0, gt0);
      delay(gstab);

      decrgpulse(pwC, one, 0.0, 0.0);
      zgradpulse(0.7*gzlvl0, gt0);
      delay(gstab);


      txphase(one);      delay(1.0e-5);
      shiftedpulse("sinc", pwHs, 90.0, 0.0, one, 2.0e-6, 0.0);

      txphase(zero);  decphase(zero); dec2phase(zero);
      delay(2.0e-6);

/* pulse sequence starts */


   rgpulse(pw,zero,0.0,0.0);                      /* 1H pulse excitation */
      dec2phase(zero);
      zgradpulse(gzlvl3, gt3);
      delay(lambda - gt3);

   sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
      if (sel_flg[A] == 'n') txphase(three);
          else txphase(one);

      zgradpulse(gzlvl3, gt3);
      delay(lambda - gt3);

if (sel_flg[A] == 'n')
{
   rgpulse(pw, three, 0.0, 0.0);
      txphase(zero);
      zgradpulse(gzlvl4, gt4);                       /* Crush gradient G4 */
      delay(gstab);
                                             /* Begin of N to Ca transfer */
     dec2rgpulse(pwN, one, 0.0, 0.0);
     decphase(zero);      dec2phase(zero);
     delay(tauNCo - pwCO180/2 - 2.0e-6 - WFG3_START_DELAY);
}
else  /* active suppresion */
{
   rgpulse(pw,one,2.0e-6,0.0);

      initval(1.0,v6);   dec2stepsize(45.0);   dcplr2phase(v6);
      zgradpulse(gzlvl4, gt4);                       /* Crush gradient G4 */
      delay(gstab);
                                             /* Begin of N to Ca transfer */
   dec2rgpulse(pwN,one,0.0,0.0);
      dcplr2phase(zero);                                    /* SAPS_DELAY */
      delay(1.34e-3 - SAPS_DELAY - 2.0*pw);

      rgpulse(pw,one,0.0,0.0);
      rgpulse(2.0*pw,zero,0.0,0.0);
      rgpulse(pw,one,0.0,0.0);

      delay(tauNCo - pwCO180/2 - 1.34e-3 - 2.0*pw - WFG3_START_DELAY);
}

/* Begin transfer from HzNz to N(i)zC'(i-1)zCa(i)zCa(i-1)z */

      c13pulse("co", "cab", "sinc", 180.0, zero, 2.0e-6, 0.0);
      delay(tauNCa - tauNCo - pwCO180/2 - WFG3_START_DELAY -
            WFG3_STOP_DELAY - 2.0e-6);

                                     /* WFG3_START_DELAY */
   sim3_c13pulse("", "cab", "co", "square", "", 0.0, 180.0, 2.0*pwN,
                             zero, zero, zero, 2.0e-6, 2.0e-6);
      delay(tauNCa - 2.0e-6 - WFG3_STOP_DELAY);

   dec2rgpulse(pwN, zero, 0.0, 0.0);

/* End transfer from HzNz to N(i)zC'(i-1)zCa(i)zCa(i-1)z */

      zgradpulse(gzlvl5, gt5);
      delay(gstab);

/* Begin removal of Ca(i-1) */

   c13pulse("co", "cab", "sinc", 90.0, zero, 2.0e-6, 2.0e-6);
      zgradpulse(gzlvl6, gt6);
      delay(tauCaCo - gt6 - pwCab180 - pwCO180/2 - 6.0e-6);

      c13pulse("cab","co", "square", 180.0, zero, 2.0e-6, 2.0e-6);
   c13pulse("co","cab", "sinc", 180.0, zero, 2.0e-6, 2.0e-6);

      zgradpulse(gzlvl6, gt6);
      delay(tauCaCo - gt6 - pwCab180 - pwCO180/2 - 6.0e-6);

      c13pulse("cab","co", "square", 180.0, zero, 2.0e-6, 2.0e-6);
   c13pulse("co", "cab", "sinc", 90.0, one, 2.0e-6, 2.0e-6);

/* End removal of Ca(i-1) */

      /* xx Selective glycine pulse xx */
      set_c13offset("gly");
      setautocal();
      if (autocal[A] == 'y')
      {
        if(FIRST_FID)
        {
         ppm = getval("dfrq"); bw=9*ppm;
         gly90 = pbox_make("gly90","eburp1",bw,0.0,compC*pwC,pwClvl);
                               /* Gly selective 90 with null at 50ppm */
        }
        pwgly=gly90.pw; glypwr=gly90.pwr; glypwrf=gly90.pwrf;
        decpwrf(glypwrf);
        decpower(glypwr);                           
        decshaped_pulse("gly90",pwgly,zero,2.0e-6,0.0);
      }
      else
      {
       decpwrf(4095.0);
       decpower(glypwr);                           
       decshaped_pulse(glyshp,pwgly,zero,2.0e-6,0.0);
      }
      /* xx End of glycine selecton xx */

      zgradpulse(gzlvl7, gt7);
      set_c13offset("cab");
      delay(gstab);
      decphase(t3);

      if ( dm3[B] == 'y' )     /* begins optional 2H decoupling */
      {
         dec3unblank();
         dec3rgpulse(1/dmf3,one,10.0e-6,2.0e-6);
         dec3unblank();
         dec3phase(zero);
         delay(2.0e-6);
         setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
      }


/* ========== Ca to Cb transfer =========== */

   c13pulse("cab", "co", "square", 90.0, t3, 2.0e-6, 2.0e-6);
      decphase(zero);
      delay(tauCaCb - 4.0e-6);

   c13pulse("cab", "co", "square", 180.0, zero, 2.0e-6, 2.0e-6);
      decphase(t2);
      delay(tauCaCb - 4.0e-6 );

/*   xxxxxxxxxxxxxxxxxxxxxx   13Cb EVOLUTION        xxxxxxxxxxxxxxxxxx    */

     c13pulse("cab", "co", "square", 90.0, t2, 2.0e-6, 0.0);      /*  pwCa90  */
     decphase(zero);

     if ((ni>1.0) && (tau1>0.0))
      {
         if (tau1 - 2.0*pwCab90/PI - WFG_START_DELAY - pwN - 2.0e-6
                 - PWRF_DELAY - POWER_DELAY > 0.0)
         {
            delay(tau1 - 2.0*pwCab90/PI - pwN - 2.0e-6 );

            dec2rgpulse(2.0*pwN, zero, 2.0e-6, 0.0);
            delay(tau1 - 2.0*pwCab90/PI  - pwN - WFG_START_DELAY
                                - 2.0e-6 - PWRF_DELAY - POWER_DELAY);
         }
         else
         {
            tsadd(t12,2,4);
            delay(2.0*tau1);
            delay(10.0e-6);                                    /* WFG_START_DELAY */
         sim3_c13pulse("", "cab", "co", "square", "", 0.0, 180.0, 2.0*pwN,
                     zero, zero, zero, 2.0e-6, 0.0);
            delay(10.0e-6);
         }
      }
      else
      {
         tsadd(t12,2,4);
         delay(10.0e-6);                                    /* WFG_START_DELAY */
         sim3_c13pulse("", "cab", "co", "square", "", 0.0, 180.0, 2.0*pwN,
                        zero, zero, zero, 2.0e-6, 0.0);
         delay(10.0e-6);
      }

   decphase(one);
   c13pulse("cab", "co", "square", 90.0, one, 2.0e-6, 0.0);      /*  pwCa90  */

/*   xxxxxxxxxxx End of 13Cb EVOLUTION - Start 13Ca EVOLUTION   xxxxxxxxxxxx    */

        decphase(zero);
        delay(tau2);

        sim3_c13pulse("", "co", "cab", "sinc", "", 0.0, 180.0, 2.0*pwN,
                    zero, zero, zero, 2.0e-6, 0.0);
        decphase(zero);

        delay(tauCaCb - 2*pwN - pwCab180/2 - WFG2_START_DELAY - 2.0*PWRF_DELAY -
              2.0*POWER_DELAY - WFG2_STOP_DELAY - 4.0e-6 );
   c13pulse("cab", "co", "square", 180.0, zero, 2.0e-6, 0.0);

        delay(tauCaCb- tau2 - pwCO180 - pwCab180/2 - WFG2_START_DELAY - 2.0*PWRF_DELAY
              -2.0*POWER_DELAY - WFG2_STOP_DELAY - 4.0e-6);

        c13pulse("co", "cab", "sinc", 180.0, zero, 2.0e-6, 0.0);
        decphase(t5);

   c13pulse("cab", "co", "square", 90.0, t5, 2.0e-6, 0.0);

/*   xxxxxxxxxxxxxxxxxxx End of 13Ca EVOLUTION        xxxxxxxxxxxxxxxxxx    */


      if ( dm3[B] == 'y' )   /* turns off 2H decoupling  */
      {
          dec3rgpulse(1/dmf3,three,2.0e-6,2.0e-6);
          dec3blank();
          setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
          dec3blank();
      }
/*  xxxxxxxxxxxxxxxxxxxx  N15 EVOLUTION & SE TRAIN   xxxxxxxxxxxxxxxxxxxxxxx  */     

      dcplrphase(zero);     dec2phase(t8);
      zgradpulse(gzlvl10, gt10);
      delay(gstab);

   dec2rgpulse(pwN, t8, 2.0e-6, 0.0);
      c13pulse("co", "cab", "sinc", 180.0, zero, 2.0e-6, 0.0); /*pwCO180*/
      decphase(zero);     dec2phase(t9);
      delay(timeTN - pwCO180 - WFG3_START_DELAY - tau3 - 4.0e-6);
                                    /* WFG3_START_DELAY  */
   sim3_c13pulse("", "cab", "co", "square", "", 0.0, 180.0, 2.0*pwN, 
                              zero, zero, t9, 2.0e-6, 2.0e-6);
     c13pulse("co", "cab", "sinc", 180.0, zero, 2.0e-6, 0.0); /*pwCO180*/

      dec2phase(t10);
      txphase(t4);

      delay(timeTN - pwCO180 + tau3 - 500.0e-6 - gt1 - 2.0*GRADIENT_DELAY-
             WFG_START_DELAY - WFG_STOP_DELAY );

      delay(0.2e-6);
      zgradpulse(gzlvl1, gt1);
      delay(gstab);

   sim3pulse(pw, 0.0, pwN, t4, zero, t10, 0.0, 0.0);
      txphase(zero);     dec2phase(zero);
      zgradpulse(gzlvl11, gt11);
      delay(lambda - 1.3*pwN - gt11);

   sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
      zgradpulse(gzlvl11, gt11);     txphase(one);
      dec2phase(t11);
      delay(lambda - 1.3*pwN - gt11);

   sim3pulse(pw, 0.0, pwN, one, zero, t11, 0.0, 0.0);
      txphase(zero);     dec2phase(zero);
      zgradpulse(gzlvl12, gt12);
      delay(lambda - 1.3*pwN - gt12);

   sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
      dec2phase(zero);
      zgradpulse(gzlvl12, gt12);
      delay(lambda - 1.3*pwN - gt12);

   sim3pulse(pw, 0.0, pwN, zero, zero, zero, 0.0, 0.0);
      delay((gt1/10.0) + 1.0e-4 + 2.0*GRADIENT_DELAY + POWER_DELAY);

   rgpulse(2.0*pw, zero, 0.0, 0.0);
      dec2power(dpwr2);                           /* POWER_DELAY */
      zgradpulse(icosel*gzlvl2, gt1/10.0);            /* 2.0*GRADIENT_DELAY */

statusdelay(C, 1.0e-4 );
   setreceiver(t12);
   if (dm3[B]=='y') lk_sample();
}
