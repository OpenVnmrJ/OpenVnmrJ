/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* PR43_ghc_co_nhP.c   (non-TROSY version)

PR(4,3)D
coevolving side-chain protons and aliphatic carbons together
with proton decoupling and optional Cb decoupling during Ca evolution

Ling Jiang and Pei Zhou, Duke University 
  (PR43_HCCCONH.c)

References:
Yingxi Lin & Gerhard Wagner, JBNMR, 15, 227-239, 1999
Ling Jiang, Brian Coggins and Pei Zhou, JMR, 175, 170-176(2005)

To obtain reconstruction software package, please visit
http://zhoulab.biochem.duke.edu/software/pr-calc
*/

#include <standard.h>
#include "bionmr.h"

static int   
        phx[1]={0},   
        phy[1]={1},
        phi5[2]  = {0,2},
        phi6[2]  = {2,0},
        phi9[8]  = {0,0,1,1,2,2,3,3},
        rec[4]   = {0,2,2,0};

pulsesequence()
{

/* DECLARE AND LOAD VARIABLES; parameters used in the last half of the */
/* sequence are declared and initialized as 0.0 in bionmr.h, and       */
/* reinitialized below  */

char   f2180[MAXSTR],
       cbdec[MAXSTR], cbdecseq[MAXSTR];

int    t1_counter,  t2_counter,
       ni = getval("ni"), ni2 = getval("ni2");

double
   d2_init=0.0, d3_init=0.0,
   tau1, tau2, tau3,                                              /*  t1 delay */
   t1a, t1b, t1c, sheila_1,
   t2a, t2b, t2c, sheila_2,
   tauCH = getval("tauCH"),                               /* 1/4J delay for CH */
   timeTN = getval("timeTN"),               /* constant time for 15N evolution */
   epsilon = 1.05e-3,                                          /* other delays */
   tauCH_1, epsilon_1,
   zeta = 3.0e-3,
   eta = 4.6e-3,
   theta = 14.0e-3,
   Hali_offset=getval("Hali_offset"),         /* aliphatic proton offset in Hz */

   pwClvl = getval("pwClvl"),                    /* coarse power for C13 pulse */
   pwC = getval("pwC"),                /* C13 90 degree pulse length at pwClvl */

   pwN = getval("pwN"),                /* N15 90 degree pulse length at pwNlvl */
   pwNlvl = getval("pwNlvl"),                          /* power for N15 pulses */
   dpwr2 = getval("dpwr2"),                        /* power for N15 decoupling */

   cbpwr, cbdmf, cbres,

   swH = getval("swH"), swC = getval("swC"), swTilt,
   angle_H = getval("angle_H"), angle_C, cos_H, cos_C,

   pwS1,                                         /* length of square 90 on Cab */
   pwS2,                                         /* length of square 180 on Ca */
   pwS3,                                           /* length of sinc 180 on CO */
   pwS9,                                           /* length of last C13 pulse */
   phi7cal = getval("phi7cal"),   /* phase in degrees of the last C13 90 pulse */
   ncyc = getval("ncyc"),        /* no. of cycles of DIPSI-3 decoupling on Cab */
   waltzB1 = getval("waltzB1"),  /* H1 decoupling strength in Hz for DIPSI-2  */

   sw1 = getval("sw1"),   sw2 = getval("sw2"),
   gstab=getval("gstab"),
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
   widthHd = 2.069*(waltzB1/sfrq);  /* produces same B1 as gc_co_nh.c */

   cbpwr = getval("cbpwr");  cbdmf = getval("cbdmf");  cbres = getval("cbres");
   getstr("cbdecseq", cbdecseq);   getstr("cbdec", cbdec);

/*   LOAD PHASE TABLE    */

   settable(t2,1,phx);    settable(t3,1,phx);    settable(t4,1,phx);
   settable(t5,2,phi5);   settable(t6,2,phi6);
   settable(t8,1,phx);    settable(t9,8,phi9);
   settable(t10,1,phx);   settable(t11,1,phy);   settable(t12,4,rec);

/*   INITIALIZE VARIABLES   */

   kappa = 5.4e-3;   lambda = 2.4e-3;

   if( pwC > 20.0*600.0/sfrq )
   { printf("increase pwClvl so that pwC < 20*600/sfrq");
     psg_abort(1); }

   /* get calculated pulse lengths of shaped C13 pulses */
   pwS1 = c13pulsepw("ca", "co", "square", 90.0); 
   pwS2 = c13pulsepw("ca", "co", "square", 180.0); 
   pwS3 = c13pulsepw("co", "ca", "sinc", 180.0); 
   pwS9 = c13pulsepw("ca", "co", "square", 180.0); 

/* CHECK VALIDITY OF PARAMETER RANGES */

    if ( 0.5*ni2*1/(sw2) > timeTN - WFG3_START_DELAY)
       { printf(" ni2 is too big. Make ni2 equal to %d or less.\n", 
      ((int)((timeTN - WFG3_START_DELAY)*2.0*sw2)));                 psg_abort(1);}

    if ( dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' )
       { printf("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1);}

    if ( dm2[A] == 'y' || dm2[B] == 'y' )
       { printf("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1);}

    if ( dm3[A] == 'y' || dm3[C] == 'y' )
       { printf("incorrect dec3 decoupler flags! Should be 'nyn' or 'nnn' ");
                                  psg_abort(1);}
    if ( dpwr2 > 46 )
       { printf("dpwr2 too large! recheck value  ");                 psg_abort(1);}

    if ( pw > 20.0e-6 )
       { printf(" pw too long ! recheck value ");                    psg_abort(1);} 
  
    if ( pwN > 100.0e-6 )
       { printf(" pwN too long! recheck value ");                    psg_abort(1);} 
 
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
      printf ("PR(4,3)D cbd_hccconh\n");
      printf ("Set phase=1,2,3,4 and phase2=1,2 \n");
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
    
/* Calculate modifications to phases for States-TPPI acquisition          */
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

/*  Hyperbolic sheila_2 seems superior to original zeta approach  */ 
    tauCH_1 = tauCH - gt3 - 2.0*GRADIENT_DELAY - 5.0e-5;

 if ((ni-1)/(2.0*swTilt/cos_H) > 2.0*tauCH_1)
    {
      if (tau1 > 2.0*tauCH_1) sheila_1 = tauCH_1;
      else if (tau1 > 0) sheila_1 = 1.0/(1.0/tau1+1.0/tauCH_1-1.0/(2.0*tauCH_1));
      else          sheila_1 = 0.0;
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
   epsilon_1 = epsilon - pwS3 - WFG_START_DELAY - 1.0e-6 - POWER_DELAY 
         - PWRF_DELAY - gt5 - 2.0*GRADIENT_DELAY - 5.0e-5;

 if ((ni-1)/(2.0*swTilt/cos_C) > 2.0*epsilon_1)
    { 
      if (tau2 > 2.0*epsilon_1) sheila_2 = epsilon_1;
      else if (tau2 > 0) sheila_2 = 1.0/(1.0/tau2+1.0/epsilon_1-1.0/(2.0*epsilon_1));
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
   delay(d1);   if (dm3[B]=='y') lk_hold();
   rcvroff();

   obsoffset(tof-Hali_offset);       obspower(tpwr);        obspwrf(4095.0);
   set_c13offset("gly");         decpower(pwClvl);      decpwrf(4095.0);
   dec2offset(dof2);                 dec2power(pwNlvl);     dec2pwrf(4095.0);

   txphase(one);
   delay(1.0e-5);

   dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 and C13 magnetization*/
   decrgpulse(pwC, zero, 0.0, 0.0);
      zgradpulse(gzlvl0, gt0);
      delay(gstab);

   rgpulse(pw, three, 0.0, 0.0);                  /* 1H pulse excitation */
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
      txphase(t2);
      delay(gstab);
      delay(t1c);
                                                                /* point b */
   rgpulse(pw, t2, 0.0, 0.0);
      obsoffset(tof);

      zgradpulse(gzlvl4, gt4);
      delay(gstab);

      decphase(t3);

      if (dm3[B] == 'y')           /*optional 2H decoupling on */
      {
         dec3unblank(); dec3rgpulse(1/dmf3, one, 0.0, 0.0); 
         dec3unblank(); setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
      } 
   decrgpulse(pwC, t3, 0.0, 0.0);
      decphase(zero);
      delay(t2a);
                               /* WFG_START_DELAY+POWER_DELAY+PWRF_DELAY */
      c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 0.0);       /* pwS3 */

      zgradpulse(gzlvl5, gt5);              /* 2.0*GRADIENT_DELAY */
      delay(gstab);
      delay(epsilon_1 - 2.0*pw);

   rgpulse(2.0*pw, zero, 0.0, 0.0);
      delay(t2b);

   c13pulse("gly", "co", "square", 180.0, zero, 2.0e-6, 0.0);
      zgradpulse(gzlvl5, gt5);              /* 2.0*GRADIENT_DELAY */
      delay(gstab);
      delay(t2c);

      c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 0.0);

      delay(2.0*pwC/PI);
      delay(WFG_START_DELAY+PWRF_DELAY + POWER_DELAY);
   decrgpulse(0.5e-3, zero, 2.0e-6, 0.0);
   
   c13decouple("gly", "DIPSI3", 120.0, ncyc);       /* PRG_STOP_DELAY */
                                                              /* point e */   

   h1decon("DIPSI2", widthHd, 0.0);/*POWER_DELAY+PWRF_DELAY+PRG_START_DELAY */
   if (cbdec[A] == 'y')
   {  decpower(cbpwr);
      decphase(zero);
      decprgon(cbdecseq,1/cbdmf,cbres);
      decon();

      decphase(t5);
      decpower(pwClvl);
      delay(zeta - 2*PRG_STOP_DELAY - 2*PRG_START_DELAY - POWER_DELAY 
                                      - PWRF_DELAY - 0.5*10.933*pwC);

      decoff();
      decprgoff();
   }
   else
   {
      decphase(t5);
      decpower(pwClvl);
      delay(zeta - PRG_STOP_DELAY - PRG_START_DELAY - POWER_DELAY 
                                      - PWRF_DELAY - 0.5*10.933*pwC);
   }

      decrgpulse(pwC*158.0/90.0, t5, 0.0, 0.0);
      decrgpulse(pwC*171.2/90.0, t6, 0.0, 0.0);
      decrgpulse(pwC*342.8/90.0, t5, 0.0, 0.0);       /* Shaka composite   */
      decrgpulse(pwC*145.5/90.0, t6, 0.0, 0.0);
      decrgpulse(pwC*81.2/90.0, t5, 0.0, 0.0);
      decrgpulse(pwC*85.3/90.0, t6, 0.0, 0.0);

   if (cbdec[A] == 'y')
   {  decpower(cbpwr);
      decphase(zero);
      decprgon(cbdecseq,1/cbdmf,cbres);
      decon();

      decphase(zero);
      delay(zeta - 0.5*10.933*pwC - 0.6*pwS1 - WFG_START_DELAY - 2.0e-6 
             - PRG_START_DELAY - PRG_STOP_DELAY);

      decoff();
      decprgoff();
   }
   else
   {  decphase(zero);
      delay(zeta - 0.5*10.933*pwC - 0.6*pwS1 - WFG_START_DELAY - 2.0e-6);
   }
                                                     /* WFG_START_DELAY  */
   c13pulse("ca", "co", "square", 90.0, zero, 2.0e-6, 0.0);  /* point f */
      h1decoff();           
      decphase(t5);
      if (dm3[B] == 'y')               /*optional 2H decoupling off */
      {  
         dec3rgpulse(1/dmf3, three, 0.0, 0.0); dec3blank();
         setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3); dec3blank();
      }

      zgradpulse(gzlvl6, gt6);
      delay(gstab);

   h1decon("DIPSI2", widthHd, 0.0);/*POWER_DELAY+PWRF_DELAY+PRG_START_DELAY */
   c13pulse("co", "ca", "sinc", 90.0, t5, 2.0e-6, 0.0);
      decphase(zero);
      delay(eta - 2.0*POWER_DELAY - 2.0*PWRF_DELAY);

   c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0);     /* pwS2 */
      dec2phase(zero);
      delay(theta - eta - pwS2 - WFG3_START_DELAY);

   sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
                    zero, zero, zero, 2.0e-6, 2.0e-6);

      initval(phi7cal, v7);
      decstepsize(1.0);
      dcplrphase(v7);                       /* SAPS_DELAY */
      dec2phase(t8);
      delay(theta - SAPS_DELAY);
                                             /* point h */

   c13pulse("co", "ca", "sinc", 90.0, zero, 2.0e-6, 0.0);
      dcplrphase(zero);
      h1decoff();           /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */

      zgradpulse(gzlvl7, gt7);
      delay(gstab);

   h1decon("DIPSI2", widthHd, 0.0);/*POWER_DELAY+PWRF_DELAY+PRG_START_DELAY */
/*  xxxxxxxxxxxxx   TRIPLE RESONANCE NH EVOLUTION & SE TRAIN   xxxxxxxxxxxxx  */
   dec2rgpulse(pwN, t8, 0.0, 0.0);
      decphase(zero);
      dec2phase(t9);
      delay(timeTN - WFG3_START_DELAY - tau3);
                      /* WFG3_START_DELAY  */
   sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN, 
                  zero, zero, t9, 2.0e-6, 2.0e-6);

   dec2phase(t10);

    if (tau3 > kappa + PRG_STOP_DELAY)
   {
          delay(timeTN - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY 
                  - 2.0*PWRF_DELAY - 2.0e-6);
   c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
      delay(tau3 - kappa - PRG_STOP_DELAY - POWER_DELAY - PWRF_DELAY);
      h1decoff();           /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
      txphase(t4);
      delay(kappa - gt1 - 2.0*GRADIENT_DELAY - gstab);
      zgradpulse(gzlvl1, gt1);      /* 2.0*GRADIENT_DELAY */
      delay(gstab);
   }
   else if (tau3 > (kappa - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY - 2.0e-6))
   {
      delay(timeTN + tau3 - kappa -PRG_STOP_DELAY -POWER_DELAY -PWRF_DELAY);
      h1decoff();           /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
      txphase(t4);          /* WFG_START_DELAY  + 2.0*POWER_DELAY */
   c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
      delay(kappa - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY - 1.0e-6 - gt1 
                        - 2.0*GRADIENT_DELAY - gstab);
      zgradpulse(gzlvl1, gt1);      /* 2.0*GRADIENT_DELAY */
      delay(gstab);
   }

   else if (tau3 > gt1 + 2.0*GRADIENT_DELAY + 1.0e-4)
   {
      delay(timeTN + tau3 - kappa -PRG_STOP_DELAY -POWER_DELAY -PWRF_DELAY);
      h1decoff();           /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
      txphase(t4);
      delay(kappa - tau3 - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY
                                 - 2.0e-6);
   c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
      delay(tau3 - gt1 - 2.0*GRADIENT_DELAY - gstab);
      zgradpulse(gzlvl1, gt1);      /* 2.0*GRADIENT_DELAY */
      delay(gstab);
   }

   else
   {
      delay(timeTN + tau3 - kappa -PRG_STOP_DELAY -POWER_DELAY -PWRF_DELAY);
      h1decoff();           /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
      txphase(t4);
      delay(kappa - tau3 - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY
                  - 2.0e-6 - gt1 - 2.0*GRADIENT_DELAY - gstab);
      zgradpulse(gzlvl1, gt1);      /* 2.0*GRADIENT_DELAY */
      delay(gstab);

   c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
          delay(tau3);
   }

   sim3pulse(pw, 0.0, pwN, t4, zero, t10, 0.0, 0.0);
      txphase(zero);
      dec2phase(zero);
      zgradpulse(gzlvl8, gt8);
      delay(lambda - 1.3*pwN - gt8);

   sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
      zgradpulse(gzlvl8, gt8);
      txphase(one);
      dec2phase(t11);
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
      delay((gt1/10.0) + gstab - 0.3*pw + 2.0*GRADIENT_DELAY
                     + POWER_DELAY);  
      delay(1.0e-4);
   rgpulse(2.0*pw, zero, 0.0, 0.0);
      dec2power(dpwr2);                               /* POWER_DELAY */
      zgradpulse(icosel*gzlvl2, gt1/10.0);            /* 2.0*GRADIENT_DELAY */
      delay(gstab);
      rcvron();

statusdelay(C, 1.0e-4);
   setreceiver(t12);
   if (dm3[B]=='y') lk_sample();
}       
