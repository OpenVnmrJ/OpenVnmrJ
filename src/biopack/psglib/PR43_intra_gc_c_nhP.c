/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* PR43_intra_gc_c_nhP.c 

PR(4,3)D   Non-Trosy Version

coevolving side-chain protons and aliphatic carbons together
with optional CB decoulping, tauCaCO=4.5ms, tauNCO=15ms for GB1

Ling Jiang and Pei Zhou,Duke University.
  (PR43_INTRA_HCCNH.c)
Reference: Ling Jiang, Brian Coggins and Pei Zhou, JMR, 175,170-176(2005).

To obtain reconstruction software package, please visit
http://zhoulab.biochem.duke.edu/software/pr-calc

*/

#include <standard.h>
#include "bionmr.h"

static int   
        phx[1]   = {0},   
        phy[1]   = {1},
        phi3[2]  = {0,2},
        phi9[8]  = {0,0,1,1,2,2,3,3},
        rec[4]   = {0,2,2,0};

void pulsesequence()
{

/* DECLARE AND LOAD VARIABLES; parameters used in the last half of the */
/* sequence are declared and initialized as 0.0 in bionmr.h, and       */
/* reinitialized below  */

char   f2180[MAXSTR],
       cbdec[MAXSTR],
       cbdecseq[MAXSTR];                  /* shape for selective CB inversion */

int    t1_counter,  t2_counter, 
       ni = getval("ni"), ni2 = getval("ni2");

double
   d2_init=0.0, d3_init=0.0,  
   tau1, tau2, tau3,                                        /* t1,t2,t3 delay */
   t1a, t1b, t1c, sheila_1,
   t2a, t2b, t2c, sheila_2,
   tauCH = getval("tauCH"),                              /* 1/4J delay for CH */
   tauCH_1,
   timeTN = getval("timeTN"),   /* ~ 12 ms for N evolution and 1JNCa transfer */
   epsilon = 1.05e-3,                               /* 0.7*1/4J delay for CHn */
   epsilon_1,
   tauCaCO = getval("tauCaCO"),                 /* 1/4J delay for CaCO, 4.5ms */
   tauNCO = getval("tauNCO"),                   /* 1/4J delay for NCO, 17.0ms */

   Hali_offset = getval("Hali_offset"),
   cbpwr,                           /* power level for selective CB inversion */
   cbdmf,                           /* pulse width for selective CB inversion */
   cbres,

   pwClvl = getval("pwClvl"),                   /* coarse power for C13 pulse */
   pwC = getval("pwC"),               /* C13 90 degree pulse length at pwClvl */

   pwN = getval("pwN"),               /* N15 90 degree pulse length at pwNlvl */
   pwNlvl = getval("pwNlvl"),                         /* power for N15 pulses */
   dpwr2 = getval("dpwr2"),                       /* power for N15 decoupling */

   swH = getval("swH"), swC = getval("swC"), swTilt,
   angle_H = getval("angle_H"), angle_C, cos_H, cos_C,

   pwCa90,
   pwCa180,                                     /* length of square 180 on Ca */
   pwCO90,
   pwCO180,                                       /* length of sinc 180 on CO */

   pwZ,
   phi7cal = getval("phi7cal"),     /* small phase correction for 90 CO pulse */
   ncyc = getval("ncyc"),       /* no. of cycles of DIPSI-3 decoupling on Cab */
   waltzB1 = getval("waltzB1"),  /* H1 decoupling strength in Hz for DIPSI-2  */

   sw1 = getval("sw1"),   sw2 = getval("sw2"), 
  gstab= getval("gstab"),
   gt0 = getval("gt0"),     gzlvl0 = getval("gzlvl0"),             
   gt1 = getval("gt1"),     gzlvl1 = getval("gzlvl1"),
                            gzlvl2 = getval("gzlvl2"),
   gt3 = getval("gt3"),     gzlvl3 = getval("gzlvl3"),
   gt4 = getval("gt4"),     gzlvl4 = getval("gzlvl4"),
   gt5 = getval("gt5"),     gzlvl5 = getval("gzlvl5"),
   gt6 = getval("gt6"),     gzlvl6 = getval("gzlvl6"),
   gt7 = getval("gt7"),     gzlvl7 = getval("gzlvl7"),
   gt8 = getval("gt8"),     gzlvl8 = getval("gzlvl8"),
   gt9 = getval("gt9"),     gzlvl9 = getval("gzlvl9");

   getstr("f2180",f2180);
   widthHd = 2.069*(waltzB1/sfrq);          /* produces same B1 as gc_co_nh.c */

   cbpwr = getval("cbpwr");  cbdmf = getval("cbdmf");  cbres = getval("cbres");
   getstr("cbdecseq", cbdecseq);   getstr("cbdec", cbdec);

/*   LOAD PHASE TABLE    */

   settable(t2,1,phx);    settable(t3,2,phi3);    settable(t4,1,phx);
   settable(t8,1,phx);    settable(t9,8,phi9);   settable(t10,1,phx);
   settable(t11,1,phy);   settable(t12,4,rec);

/*   INITIALIZE VARIABLES   */

   kappa = 5.4e-3;   lambda = 2.4e-3;

/* get calculated pulse lengths of shaped C13 pulses */
   pwCa90 = c13pulsepw("ca", "co", "square", 90.0);
   pwCa180 = c13pulsepw("ca", "co", "square", 180.0); 
   pwCO90  = c13pulsepw("co", "ca", "sinc", 90.0);
   pwCO180 = c13pulsepw("co", "ca", "sinc", 180.0); 

/* pwZ: the bigger of pwN*2.0 and pwCa180 */
   if (pwN*2.0 > pwCa180) pwZ=pwN*2.0; else pwZ=pwCa180;

/* CHECK VALIDITY OF PARAMETER RANGES */

    if ( 0.5*ni2*1/(sw2) > timeTN - WFG3_START_DELAY)
       { printf(" ni2 is too big. Make ni2 equal to %d or less.\n", 
      ((int)((timeTN - WFG3_START_DELAY)*2.0*sw2)));                 psg_abort(1);}

    if ( dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' )
       { printf("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1);}

    if ( dm2[A] == 'y' || dm2[B] == 'y' )
       { printf("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1);}

    if ( dm3[A] == 'y' || dm3[B] == 'y' ||  dm3[C] == 'y' )
       { printf("incorrect dec3 decoupler flags! Should be 'nnn' ");
                                                                     psg_abort(1);}
    if ( dpwr2 > 46 )
       { printf("dpwr2 too large! recheck value  ");                 psg_abort(1);}

    if ( pw > 20.0e-6 )
       { printf(" pw too long ! recheck value ");                    psg_abort(1);} 
  
    if ( pwN > 100.0e-6 )
       { printf(" pwN too long! recheck value ");                    psg_abort(1);} 
 
    if ( pwC > 20.0*600.0/sfrq )
       { printf("increase pwClvl so that pwC < 20*600/sfrq");        psg_abort(1);}

   /**********************************************************************/
   /* Calculate t1_counter from sw1 as a generic control of the sequence */
   /* Make sure sw1 is not zero                                          */
   /**********************************************************************/

   angle_C = 90.0 - angle_H;
   if ( (angle_H < 0) || (angle_H > 90) )
   { printf ("angle_H must be between 0 and 90 degree.\n");    psg_abort(1); }

   if ( sw1 < 1.0 )
   { printf ("Please set sw1 to a non-zero value.\n");         psg_abort(1); }

   cos_H = cos (PI*angle_H/180);
   cos_C = cos (PI*angle_C/180);
   swTilt = swH * cos_H + swC * cos_C;

   if (ix ==1)
   {
      printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
      printf ("PR(4,3)D intra_cbd_hccnh\n");
      printf ("Set ni2=1, phase=1,2,3,4 and phase2=1,2 \n");
      printf ("Maximum Sweep Width: \t\t %f Hz\n", swTilt);
      printf ("Angle_H:\t%6.2f degree \t\tAngle_C:\t%f degree\n", angle_H, angle_C);
   }

/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 1) {;}                                               /* CC */
    else if (phase1 == 2)  { tsadd(t2, 1, 4); }                        /* SC */
    else if (phase1 == 3)  { tsadd(t3, 1, 4); }                        /* CS */
    else if (phase1 == 4)  { tsadd(t2, 1, 4); tsadd(t3,1,4); }         /* SS */
 
    if (phase2 == 2)  {tsadd(t10,2,4); icosel = +1;}
            else                       icosel = -1;    
    
/* Calculate modifications to phases for States-TPPI acquisition */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if (t1_counter % 2) { tsadd(t2,2,4); tsadd(t12,2,4); }
   tau1 = 1.0 * t1_counter * cos_H / swTilt;
   tau2 = 1.0 * t1_counter * cos_C / swTilt;

   tau1 = tau1/2.0;   tau2 = tau2/2.0;

   if (ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if (t2_counter % 2) { tsadd(t8,2,4); tsadd(t12,2,4); }

/*  Set up f2180  */
    tau3 = d3;
    if ((f2180[A] == 'y') && (ni2 > 1.0)) 
    { tau3 += ( 1.0 / (2.0*sw2) ); if(tau3 < 0.2e-6) tau3 = 0.0; }
    tau3 = tau3/2.0;

/*  Hyperbolic sheila_1 seems superior */ 

    tauCH_1 = tauCH - gt3 - 2.0*GRADIENT_DELAY - 5.0e-5;

 if ((ni-1)/(2.0*swTilt/cos_H) > 2.0*tauCH_1)
    {
      if (tau1 > 2.0*tauCH_1) sheila_1 = tauCH_1;
      else if (tau1 > 0)      sheila_1 = 1.0/(1.0/tau1+1.0/tauCH_1 - 1.0/(2.0*tauCH_1));
      else                    sheila_1 = 0.0;
    }
 else
    {
      if (tau1 > 0) sheila_1 = 1.0/(1.0/tau1 + 1.0/tauCH_1 - 2.0*swTilt/cos_H/((double)(ni-1)));
      else          sheila_1 = 0.0;
    }

/* The following check fixes the phase distortion of certain tilts */

   if (sheila_1 > tau1) sheila_1 = tau1;
   if (sheila_1 > tauCH_1) sheila_1 =tauCH_1;

    t1a = tau1 + tauCH_1;
    t1b = tau1 - sheila_1;
    t1c = tauCH_1 - sheila_1;

/* subtract unavoidable delays from epsilon */
    epsilon_1 = epsilon - pwCO180 - WFG_START_DELAY - 4.0e-6 - POWER_DELAY 
                - PWRF_DELAY - gt5 - 2.0*GRADIENT_DELAY - 5.0e-5;

 if ((ni-1)/(2.0*swTilt/cos_C) > 2.0*epsilon_1)
    { 
      if (tau2 > 2.0*epsilon_1) sheila_2 = epsilon_1;
      else if (tau2 > 0) sheila_2 = 1.0/(1.0/tau2+1.0/epsilon_1 - 1.0/(2.0*epsilon_1));
      else          sheila_2 = 0.0;
    }
 else
    {    
      if (tau2 > 0) sheila_2 = 1.0/(1.0/tau2 + 1.0/epsilon_1 - 2.0*swTilt/cos_C/((double)(ni-1)));
      else          sheila_2 = 0.0;
    }

/* The following check fixes the phase distortion of certain tilts */

   if (sheila_2 > tau2) sheila_2 = tau2;
   if (sheila_2 > epsilon_1) sheila_2 = epsilon_1;

    t2a = tau2;
    t2b = tau2 - sheila_2;
    t2c = epsilon_1 - sheila_2;

/*   BEGIN PULSE SEQUENCE   */

status(A);
   delay(d1);
   rcvroff();

   obsoffset(tof - Hali_offset);  obspower(tpwr);        obspwrf(4095.0);
   set_c13offset("gly");      decpower(pwClvl);      decpwrf(4095.0);
   dec2offset(dof2);              dec2power(pwNlvl);     dec2pwrf(4095.0);

   txphase(t2);    delay(1.0e-5);

   dec2rgpulse(pwN, zero, 0.0, 0.0);     /*destroy N15 and C13 magnetization*/
   decrgpulse(pwC, zero, 0.0, 0.0);
      zgradpulse(gzlvl0, gt0);
      delay(gstab);

   rgpulse(pw, t2, 0.0, 0.0);                        /* 1H pulse excitation */
                                                                 /* point a */
      txphase(zero);
      decphase(zero);
      zgradpulse(gzlvl3, gt3);                        /* 2.0*GRADIENT_DELAY */
      delay(gstab);
      delay(t1a - 2.0*pwC);

   decrgpulse(2.0*pwC, zero, 0.0, 0.0);

      delay(t1b);

   rgpulse(2.0*pw, zero, 0.0, 0.0);

      zgradpulse(gzlvl3, gt3);                        /* 2.0*GRADIENT_DELAY */
      txphase(one);
      delay(gstab);
      delay(t1c);
                                                                 /* point b */
   rgpulse(pw, one, 0.0, 0.0);

      obsoffset(tof);
      zgradpulse(gzlvl4, gt4);
      decphase(t3);
      delay(gstab);
     
   decrgpulse(pwC, t3, 0.0, 0.0);
      decphase(zero);
      delay(t2a);
                                  /* WFG_START_DELAY+POWER_DELAY+PWRF_DELAY */
      c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 0.0);    /* pwCO180 */

      zgradpulse(gzlvl5, gt5);                        /* 2.0*GRADIENT_DELAY */
      delay(gstab);
      delay(epsilon_1 - 2.0*pw);

   rgpulse(2.0*pw, zero, 0.0, 0.0);
      delay(t2b);

   c13pulse("gly", "co", "square", 180.0, zero, 2.0e-6, 0.0);
      zgradpulse(gzlvl5, gt5);                        /* 2.0*GRADIENT_DELAY */
      delay(gstab);
      delay(t2c);

      c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 0.0);

      delay(2.0*pwC/PI);                            /* Compensation for pwC */
      delay(WFG_START_DELAY+PWRF_DELAY + POWER_DELAY);

   decrgpulse(0.5e-3, zero, 2.0e-6, 0.0);           /* 0.5 ms trim(X) pulse */
   
   c13decouple("gly", "DIPSI3", 120.0, ncyc);             /* PRG_STOP_DELAY */


   /* ========= Begin Ca(i)x --> Ca(i)zN(i)z ================*/

   if (cbdec[A] == 'y')
   {  decpower(cbpwr);
      decphase(zero);
      decprgon(cbdecseq,1/cbdmf,cbres);
      decon();

      delay(tauCaCO*2.0 - pwCO90*0.6366 - 2.0e-6 - PRG_START_DELAY
            - 2*PRG_STOP_DELAY - WFG_START_DELAY - POWER_DELAY - PWRF_DELAY  );

      decoff();
      decprgoff();
   }
   else
   {  delay(tauCaCO*2.0 - pwCO90*0.6366 - 2.0e-6
            - PRG_STOP_DELAY - WFG_START_DELAY - POWER_DELAY - PWRF_DELAY  );
   }

   c13pulse("co", "ca", "sinc", 90.0, zero, 2.0e-6, 2.0e-6);

   if (cbdec[A] == 'y')
   {  decpower(cbpwr);
      decphase(zero);
      decprgon(cbdecseq,1/cbdmf,cbres);
      decon();

      delay(tauNCO - pwCO90*0.6366 - pwCO180 - pwZ/2.0 - 8.0e-6
            - PRG_START_DELAY - PRG_STOP_DELAY
            - WFG_START_DELAY - WFG3_START_DELAY - 4.0*POWER_DELAY - 4.0*PWRF_DELAY);

      decoff();
      decprgoff();
   }
   else
   {
      zgradpulse(gzlvl6, gt6);
      delay(gstab);

      delay(tauNCO - pwCO90*0.6366 - pwCO180 - pwZ/2.0 - 8.0e-6
            - gt6 - gstab - 2.0*GRADIENT_DELAY - WFG_START_DELAY
            - WFG3_START_DELAY - 4.0*POWER_DELAY - 4.0*PWRF_DELAY);
   }

   c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 2.0e-6);

   sim3_c13pulse("", "ca", "co", "square", "", 0.0, 180.0, 2.0*pwN,
                    zero, zero, zero, 2.0e-6, 2.0e-6);

   if (cbdec[A] == 'y')
   {  decpower(cbpwr);
      decphase(zero);
      decprgon(cbdecseq,1/cbdmf,cbres);
      decon();

      delay(tauNCO - pwZ/2.0 - pwCO180 - pwCO90*0.6366 - 8.0e-6
            - PRG_START_DELAY - PRG_STOP_DELAY 
            - 2*WFG_START_DELAY - 4.0*POWER_DELAY - 4.0*PWRF_DELAY - SAPS_DELAY);

      decoff();
      decprgoff();
   }
   else
   {
      zgradpulse(gzlvl6, gt6);
      delay(gstab);

      delay(tauNCO - pwZ/2.0 - pwCO180 - pwCO90*0.6366 - 8.0e-6
            - gt6 - gstab - 2.0*GRADIENT_DELAY - 2*WFG_START_DELAY
            - 4.0*POWER_DELAY - 4.0*PWRF_DELAY - SAPS_DELAY);   
   }

   c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 2.0e-6);        /* BSP */

      initval(phi7cal, v7);   decstepsize(1.0);
      decphase(one);    dcplrphase(v7);                     /* SAPS_DELAY */
 
   c13pulse("co", "ca", "sinc", 90.0, one, 2.0e-6, 2.0e-6);
      dcplrphase(zero);

   if (cbdec[A] == 'y')
   {  decpower(cbpwr);
      decphase(zero);
      decprgon(cbdecseq,1/cbdmf,cbres);
      decon();

      delay(tauCaCO*2.0 -pwCO90*0.6366 - pwCa90*0.6366 - 4.0e-6
            - PRG_START_DELAY - PRG_STOP_DELAY
            - WFG_START_DELAY - 2.0*POWER_DELAY - 2.0*PWRF_DELAY);

      decoff();
      decprgoff();
   }
   else
   {  delay(tauCaCO*2.0 -pwCO90*0.6366 - pwCa90*0.6366 - 4.0e-6
            - WFG_START_DELAY - 2.0*POWER_DELAY - 2.0*PWRF_DELAY);
   }

      decphase(one);
   c13pulse("ca", "co", "square", 90.0, one, 2.0e-6, 2.0e-6);

   /* ========= End Ca(i)x --> Ca(i)zN(i)z ================*/  
      zgradpulse(gzlvl7, gt7);
      delay(gstab);

      h1decon("DIPSI2", widthHd, 0.0);
                                   /*POWER_DELAY+PWRF_DELAY+PRG_START_DELAY */

/*  xxxxxxxxxxxx   TRIPLE RESONANCE NH EVOLUTION & SE TRAIN   xxxxxxxxxxxx  */

   dec2rgpulse(pwN, t8, 2.0e-6, 2.0e-6);
      decphase(zero);   dec2phase(t9);
      delay(timeTN - WFG3_START_DELAY - tau3);
                                                          /* WFG3_START_DELAY  */
   sim3_c13pulse("", "ca", "co", "square", "", 0.0, 180.0, 2.0*pwN, 
                  zero, zero, t9, 2.0e-6, 2.0e-6);
   dec2phase(t10);

   if (tau3 > kappa + PRG_STOP_DELAY)
   {
      delay(timeTN - pwCO180 - WFG_START_DELAY - 2.0*POWER_DELAY 
                  - 2.0*PWRF_DELAY - 2.0e-6);

   c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 0.0);          /* pwCO180 */

      delay(tau3 - kappa - PRG_STOP_DELAY - POWER_DELAY - PWRF_DELAY);
      h1decoff();                     /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
      txphase(zero);
      delay(kappa - gt1 - 2.0*GRADIENT_DELAY - gstab);

      zgradpulse(gzlvl1, gt1);                    /* 2.0*GRADIENT_DELAY */
      delay(gstab);
   }
   else if (tau3 > (kappa - pwCO180 - WFG_START_DELAY - 2.0*POWER_DELAY - 2.0e-6))
   {
      delay(timeTN + tau3 - kappa -PRG_STOP_DELAY -POWER_DELAY -PWRF_DELAY);
      h1decoff();                     /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
      txphase(zero);                     /* WFG_START_DELAY  + 2.0*POWER_DELAY */

   c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 0.0);          /* pwCO180 */

      delay(kappa - pwCO180 - WFG_START_DELAY - 2.0*POWER_DELAY - 1.0e-6 - gt1 
                       - 2.0*GRADIENT_DELAY - gstab);

      zgradpulse(gzlvl1, gt1);                    /* 2.0*GRADIENT_DELAY */
      delay(gstab);
   }
   else if (tau3 > gt1 + 2.0*GRADIENT_DELAY + 1.0e-4)
   {
      delay(timeTN + tau3 - kappa -PRG_STOP_DELAY -POWER_DELAY -PWRF_DELAY);
      h1decoff();                     /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
      txphase(zero);
      delay(kappa - tau3 - pwCO180 - WFG_START_DELAY - 2.0*POWER_DELAY
                             - 2.0e-6);
   c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 0.0);          /* pwCO180 */

      delay(tau3 - gt1 - 2.0*GRADIENT_DELAY - gstab);

      zgradpulse(gzlvl1, gt1);                    /* 2.0*GRADIENT_DELAY */
      delay(gstab);
   }
   else
   {
      delay(timeTN + tau3 - kappa -PRG_STOP_DELAY -POWER_DELAY -PWRF_DELAY);
      h1decoff();                     /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
      txphase(zero);
      delay(kappa - tau3 - pwCO180 - WFG_START_DELAY - 2.0*POWER_DELAY
                  - 2.0e-6 - gt1 - 2.0*GRADIENT_DELAY - gstab);

      zgradpulse(gzlvl1, gt1);                    /* 2.0*GRADIENT_DELAY */
      delay(gstab);

   c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 0.0);          /* pwCO180 */
      delay(tau3);
   }

   sim3pulse(pw, 0.0, pwN, zero, zero, t10, 0.0, 0.0);

      txphase(zero);   dec2phase(zero);
      zgradpulse(gzlvl8, gt8);
      delay(lambda - 1.3*pwN - gt8);

   sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

      zgradpulse(gzlvl8, gt8);
      txphase(one);  dec2phase(t11);
      delay(lambda - 1.3*pwN - gt8);

   sim3pulse(pw, 0.0, pwN, one, zero, t11, 0.0, 0.0);

      txphase(zero);
      dec2phase(zero);
      zgradpulse(gzlvl9, gt9);
      delay(lambda - 1.3*pwN - gt9);

   sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

      dec2phase(t10);
      zgradpulse(gzlvl9, gt9);
      delay(lambda - 0.65*(pw + pwN) - gt9);

   rgpulse(pw, zero, 0.0, 0.0); 
      delay((gt1/10.0) + 1.0e-4 - 0.3*pw + 2.0*GRADIENT_DELAY + POWER_DELAY);  
      delay(1.0e-4);
   rgpulse(2.0*pw, zero, 0.0, 0.0);
      dec2power(dpwr2);                                         /* POWER_DELAY */
      zgradpulse(icosel*gzlvl2, gt1/10.0);                      /* 2.0*GRADIENT_DELAY */
      delay(gstab);

statusdelay(C, gstab);
   setreceiver(t12);
}
