/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gNtrosyS3.c

s pulse sequence records 1J(NH) couplings in a 1H-15N correlation spectrum. 
Couplings can be measured either from 15N or from 1H dimension.

Set 13C carrier (dof) at 100 ppm. Set dm2='nnnn'.

15N-steady-state magnetization is added to the TROSY as well as to the 
semi-TROSY cross-peaks. 

phase=1,2 gives the TROSY peak, slow 15N, slow 1H.
wft2d(1,0,-1,0,0,-1,0,-1) should give the TROSY
spectrum with flat baseline (i.e. rp1=0, lp1=0).

if semitrosy='n' then

phase=3,4 gives the semi-TROSY peak, fast 15N, slow 1H. 
wft2d(1,0,-1,0,0,-1,0,-1) should give the semi-TROSY 
spectrum with flat baseline (i.e. rp1=0, lp1=0).

if semitrosy='h' then

phase=3,4 gives the semi-TROSY peak, slow 15N, fast 1H.
wft2d(1,0,-1,0,0,-1,0,-1) should give the semi-TROSY 
spectrum with flat baseline (i.e. rp1=0, lp1=0).


if semitrosy='n' then

phase=1,2,3,4 gives the TROSY and semi-TROSY peaks in the same data set 
suitable for measuring 1JNH from 15N -dimension.
wft2d(1,0,-1,0,0,0,0,0,0,-1,0,-1,0,0,0,0) gives TROSY peak.
wft2d(0,0,0,0,1,0,-1,0,0,0,0,0,0,-1,0,-1) gives semi-TROSY peak.
wft2d(1,0,-1,0,1,0,-1,0,0,-1,0,-1,0,-1,0,-1) should give these peaks in the
same spectrum with opposite phases.

if semitrosy='h' then

phase=1,2,3,4 gives the TROSY and semi-TROSY -peaks in the same data set 
suitable for measuring 1JNH from 1H -dimension.
wft2d(1,0,-1,0,0,0,0,0,0,-1,0,-1,0,0,0,0) gives TROSY peak.
wft2d(0,0,0,0,1,0,-1,0,0,0,0,0,0,-1,0,-1) gives semi-TROSY peak.
wft2d(1,0,-1,0,1,0,-1,0,0,-1,0,-1,0,-1,0,-1) should give these peaks in the
same spectrum with opposite phases.

Written by Perttu Permi, Univ. Helsinki 2000,
    (gabtrosy_ns_pp.c)
modification of the pulse sequence: 
   J. Weigelt, J. Am. Chem. Soc., 120, 10778-10779 (1998).
Modified for BioPack, G.Gray, Sept 2004.

*/

#include <standard.h>

static double d2_init = 0.0;


static int phi1[2] = {0,2}, /* N-90 before t1 */
           phi2[1] = {0},   /* H-90 after t1 */
           phi3[1] = {1},   /* N-90 before acq. */
           phi4[1] = {0}, /* N-90 purge */
           phi5[1] = {3}, /* H-90 in first INEPT */
           phi6[1] = {3}, /* water flip-back */
           phi7[2] = {0,2},  /* receiver */
           phi8[1] = {1}; /* water flip-back */


void pulsesequence()
{
/* DECLARE VARIABLES */

 char      
       f1180[MAXSTR],
       semitrosy[MAXSTR]; /* flag for selecting either fast relaxing peak in 1H or 15N-dimension
       */

 int	     phase,t1_counter,icosel;

 double      /* DELAYS */
             JNH = getval("JNH"),
             tau1,tauhn,


             /* PULSES */
             pwN = getval("pwN"),               /* PW90 for N-nuc */
             pwC = getval("pwC"),               /* PW90 for C-nuc */
   
             /* POWER LEVELS */
             pwClvl = getval("pwClvl"),         /* power level for C hard pulses */ 
             pwNlvl = getval("pwNlvl"),         /* power level for N hard pulses */

             /* CONSTANTS */
             sw1 = getval("sw1"),
             dof = getval("dof"),

  tpwrsf_d = getval("tpwrsf_d"), /* fine power adustment for first soft pulse(down)*/
  pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
  compH =getval("compH"),
  tpwrs,	  	              /* power for the pwHs ("H2Osinc") pulse */

             /* GRADIENT DELAYS AND LEVELS */
             gt0 = getval("gt0"),             /* gradient time */
             gt1 = getval("gt1"),             /* gradient time */
             gt3 = getval("gt3"),
             gt5 = getval("gt5"),
             gstab = getval("gstab"),         /* stabilization delay */
             gzlvl0 = getval("gzlvl0"),       /* level of gradient */
             gzlvl1 = getval("gzlvl1"),       /* level of gradient */
             gzlvl2 = getval("gzlvl2"),
             gzlvl3 = getval("gzlvl3"),       /* level of gradient */
             gzlvl5 = getval("gzlvl5");

/* LOAD VARIABLES */

  phase = (int) (getval("phase") + 0.5);
  ni = getval("ni");
  getstr("f1180",f1180); 
  getstr("semitrosy",semitrosy);
  tauhn=1/(4.0*JNH);
/* check validity of parameter range */

    if ((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
	{
	printf("incorrect Dec1 decoupler flags!  ");
	psg_abort(1);
    } 

    if ((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' ))
	{
	printf("incorrect Dec2 decoupler flags!  ");
	psg_abort(1);
    } 

    if ( satpwr > 8 )
    {
	printf("satpwr too large !!!  ");
	psg_abort(1);
    }

    if ( dpwr > 50 )
    {
	printf("don't fry the probe, dpwr too large!  ");
	psg_abort(1);
    }

    if ( dpwr2 > 50 )
    {
	printf("don't fry the probe, dpwr2 too large!  ");
	psg_abort(1);
    }

/* LOAD VARIABLES */

  settable(t1, 2, phi1);
  settable(t2, 1, phi2);
  settable(t3, 1, phi3);
  settable(t4, 1, phi4);
  settable(t5, 1, phi5);
  settable(t6, 1, phi6);
  settable(t7, 2, phi7);
  settable(t8, 1, phi8);

/* INITIALIZE VARIABLES */

  tauhn = ((JNH != 0.0) ? 1/(4*(JNH)) : 2.25e-3);
  
/* selective H20 one-lobe sinc pulse */
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
    tpwrs = (int) (tpwrs);                   	  /*power than a square pulse */

        if (tpwrsf_d<4095.0)
        {
         tpwrs=tpwrs+6.0;  /* add 6dB to let tpwrsf_d control fine power ~2048*/
        }
 
/* Phase incrementation for hypercomplex data */
icosel = 1;  /* initialize */
 if ( semitrosy[A] == 'n' ) /* to choose for the semitrosy multiplet in 15N dimension, 'n' */
{

  if (phase == 1)      /* Hypercomplex in t1 */
     {
        icosel = -1;
     }  
   
   if (phase == 2)      /* Hypercomplex in t1 */
     {
	tsadd(t2, 2, 4);
	tsadd(t3, 2, 4);
        icosel = +1;
     }
   
   if (phase == 3)      /* Hypercomplex in t1 */
     {
        tsadd(t5, 2, 4);
	tsadd(t2, 2, 4);
	tsadd(t8, 2, 4);
	icosel = -1;
     }
   if (phase == 4)      /* Hypercomplex in t1 */
     {
        tsadd(t5, 2, 4);
	tsadd(t3, 2, 4);
        tsadd(t8, 2, 4);
	icosel = +1;
     }
}

   if ( semitrosy[A] == 'h' ) /* to choose for the semitrosy multiplet in 1H dimension, 'h' */
{
   if (phase == 1)      /* Hypercomplex in t1 */
     {
        icosel = -1;
     }  
   
   if (phase == 2)      /* Hypercomplex in t1 */
     {
        tsadd(t2, 2, 4);
	tsadd(t3, 2, 4);
        icosel = +1;
     }
   
   if (phase == 3)      /* Hypercomplex in t1 */
     {
	tsadd(t3, 2, 4);
	icosel = -1;
     }
   if (phase == 4)        /* Hypercomplex in t1 */
     {
	tsadd(t2, 2, 4);
	icosel = +1;
     }
}
   
/* calculate modification to phases based on current t1 values
   to achieve States-TPPI acquisition */
  
   if(ix == 1)
      d2_init = d2;
      t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);

      if(t1_counter %2) 
      {
        tsadd(t1, 2, 4);
        tsadd(t7, 2, 4);
      }

/* set up so that get (-90,180) phase corrects in F1 if f1180 flag is y */

   tau1 = d2;
   if (tau1>0.0)
     tau1 = tau1 -rof1 -2.0*pwN/PI -4.0*pwC;
   if (f1180[A] == 'y')  tau1 += ( 1.0/(2.0*sw1));
   tau1 = tau1/2.0;
   

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(satpwr);            /* Set power for presaturation    */
   decpower(pwClvl);             /* Set decoupler1 power to pwClvl  */
   dec2power(pwNlvl);            /* Set decoupler2 power to pwNlvl */
   obsoffset(tof);
   decoffset(dof);
   dec2offset(dof2);

/* Presaturation Period */

 if (satmode[0] == 'y')
   {
     rgpulse(d1,zero,rof1,rof1);
     obspower(tpwr);               /* Set power for hard pulses */
   }
 else  
   {
     obspower(tpwr);                	/* Set power for hard pulses */
     delay(d1);
   }

status(B);

  obspower(tpwrs); obspwrf(tpwrsf_d);
  shaped_pulse("H2Osinc",pwHs,t8,rof1,0.0);
  obspower(tpwr); obspwrf(4095.0);

  rgpulse(pw,zero,rof1,rof1);

  zgradpulse(gzlvl0, gt0);
 
  delay(tauhn - gt0);   /* delay for 1/4JHN coupling */
   
  sim3pulse(2.0*pw,0.0,2.0*pwN,zero,zero,zero,rof1,rof1);

  zgradpulse(gzlvl0, gt0);

  delay(tauhn - gt0);  /* delay for 1/4JHN coupling */
   
  rgpulse(pw,t5,rof1,0.0);
            
   zgradpulse(gzlvl3, gt3);
   delay(gstab);
   
   dec2rgpulse(pwN,t1,rof1,0.0);

   delay(tau1);
   if (tau1>0.0)
   {
    decrgpulse(pwC,zero,rof1,0.0);
    decrgpulse(2.0*pwC,one,0.0,0.0);
    decrgpulse(pwC,zero,0.0,0.0);
   }
   delay(tau1);

   zgradpulse(gzlvl1, gt1);  
   delay(gstab);
   
   dec2rgpulse(2.0*pwN,t4,rof1,rof1);

   delay(gt1 + gstab -pwHs +2.0*GRADIENT_DELAY -4.0*POWER_DELAY -2.0*rof1 );

   obspower(tpwrs); obspwrf(tpwrsf_d);
   shaped_pulse("H2Osinc",pwHs,t6,rof1,0.0);
   obspower(tpwr); obspwrf(4095.0);
 
   rgpulse(pw,t2,rof1,rof1);  /* Pulse for 1H  */ 
 
   zgradpulse(gzlvl5, gt5);
   delay(tauhn  - gt5 -2.0*rof1 -pwN);

   sim3pulse(2.0*pw,0.0,2.0*pwN,zero,zero,zero,rof1,rof1);  

   zgradpulse(gzlvl5, gt5);

  delay(tauhn - gt5 - 1.5*pwN -2.0*rof1); /* 1/4J (XH)  */

  sim3pulse(pw,0.0,pwN,one,zero,zero,rof1,rof1); /* 90 for 1H and 15N */

  zgradpulse(1.5*gzlvl5, gt5);

  delay(tauhn - gt5 -1.5*pwN -2.0*rof1);   /* delay=1/4J (XH) */

  sim3pulse(2.0*pw,0.0,2.0*pwN,zero,zero,zero,rof1,rof1);

  zgradpulse(1.5*gzlvl5, gt5);

  delay(tauhn - gt5 -2.0*rof1 -pwN/2.0);   /* 1/4J (XH)  */

  dec2rgpulse(pwN,t3,rof1,0.0);
   
  delay(gt1/10.0 -rof1 -pwN/2.0 +gstab + 2.0*GRADIENT_DELAY + POWER_DELAY);
         
  rgpulse(2.0*pw, zero, rof1, rof1);

  dec2power(dpwr2);

  zgradpulse(icosel*gzlvl2, 0.1*gt1);
  delay(gstab-rof1);

status(C);
     setreceiver(t7);
}




