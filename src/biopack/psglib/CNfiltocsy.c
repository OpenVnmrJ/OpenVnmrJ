/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* CNfiltocsy.c

       This pulse sequence will allow one to perform the following experiment:
       2D tocsy of bound peptide with suppression of signals from 15N,13C
       labeled protein. The experiment is performed in water.

       To perform the D2O version set samplet='d' (vs samplet = 'w' for water)
                  and set dofaro = dof = 43 ppm. 

        Do not decouple during acquisition

        Written by L. E. Kay Oct 12, 1996 based on noesyf1cnf2cn_h2o_pfg_600.c

        Modified by L. E. Kay on Oct. 28, 1996 to change the shape of the flip-back
        pulse after the mixing time 
 
        Modified by GG (Varian) from noesyf1cnf2cn_h2o_pfg_v2.c (LEK, U.Toronto) Sept 2003

	Modified by Ian Robertson and Brian Sykes (Univ. Alberta) to
        concatenate CHfilnoesy with zdipsitocsy to form CNfiltocsy

*/

#include <standard.h>

void dipsi(phse1,phse2)
codeint phse1,phse2;
{
        double slpw5;
	        slpw5 =(1.0/(4.0*getval("strength")*18.0));

	        rgpulse(64*slpw5,phse1,0.0,0.0);
	        rgpulse(82*slpw5,phse2,0.0,0.0);
	        rgpulse(58*slpw5,phse1,0.0,0.0);
	        rgpulse(57*slpw5,phse2,0.0,0.0);
	        rgpulse(6*slpw5,phse1,0.0,0.0);
	        rgpulse(49*slpw5,phse2,0.0,0.0);
	        rgpulse(75*slpw5,phse1,0.0,0.0);
	        rgpulse(53*slpw5,phse2,0.0,0.0);
	        rgpulse(74*slpw5,phse1,0.0,0.0);
}

static double d2_init = 0.0;

static int phi1[2] = {1,3},
           phi2[2] = {0,2},
           phi3[8] = {0,0,1,1,2,2,3,3}, 
           phi4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
           phi6[8]  = {2,2,3,3,0,0,1,1},
           rec[8] = {0,2,1,3,2,0,3,1};

void pulsesequence()
{
/* DECLARE VARIABLES */

 char        f1180[MAXSTR],satmode[MAXSTR],
             samplet[MAXSTR];

 int	     phase,
             t1_counter;

 double     
             pwN,                   /* PW90 for N-nuc at pwNlvl     */
             pwC,                   /* PW90 for C-nuc at pwClvl      */
             satpwr,               /* low power level for presat*/
             pwNlvl,                /* power level for N hard pulses */
             pwClvl,                 /* power level for C hard pulses */
             jhc1,                  /* coupling #1 for HC (F1)       */
             jhc2,                  /* coupling #2 for HC (F1)       */
             taua,                  /* defined below  */ 
             taub,                   
             tauc,                   
	     tau1,	      	    /* t1 */
	     sw1,                  /* spectral width in 1H dimension */
	     cycles,
	     strength,		/* gamma B2 for tocsy	*/
	     mix,		/* length of tocsy period	*/
	     satdly,
	     slpw5,
	     slpwr,
	     compH,
             gzlvl1,
             gzlvl2,
             gstab,
             gt1,
             gt2;

/* LOAD VARIABLES */

  pwC = getval("pwC");
  pwN = getval("pwN");
  satpwr = getval("satpwr");
  pwNlvl = getval("pwNlvl"); 
  pwClvl = getval("pwClvl");
  jhc1 = getval("jhc1");
  jhc2 = getval("jhc2");
  phase = (int) (getval("phase") + 0.5);
  sw1 = getval("sw1");
  mix  = getval("mix");
  satdly = getval("satdly");
  strength = getval("strength");
  compH = getval("compH");
  slpw5 = 1.0/(4.0*strength*18);
  slpwr = 4095*(compH*pw*4.0*strength);
  slpwr = (int) (slpwr+0.5);
  cycles = (mix/(2072*slpw5));
  cycles = 2.0*(double)(int)(cycles/2.0);
  initval(cycles, v9);			/* v9 is the mix loop count	*/

  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gstab = getval("gstab");
  gt1 = getval("gt1");
  gt2 = getval("gt2");

  getstr("satmode",satmode); 
  getstr("f1180",f1180); 
  getstr("samplet",samplet);


/* check validity of parameter range */

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
	{
	printf("incorrect Dec1 decoupler flags! No decoupling in the seq. ");
	psg_abort(1);
    } 

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y'))
	{
	printf("incorrect Dec2 decoupler flags! No decoupling in the seq. ");
	psg_abort(1);
    } 


   if(samplet[A] != 'w' && samplet[A] != 'd')
       {
         printf("samplet must be set to either w (H2O) or d (D2O)");
         psg_abort(1);
       }

    if( satpwr > 8 )
    {
	printf("satpwr too large !!!  ");
	psg_abort(1);
    }

    if( dpwr > 50 )
    {
	printf("don't fry the probe, dpwr too large!  ");
	psg_abort(1);
    }

    if( dpwr2 > 50 )
    {
	printf("don't fry the probe, dpwr2 too large!  ");
	psg_abort(1);
    }

    if( gt1 > 15e-3 || gt2 > 15e-3 ) 
    {
        printf("gradients are on for too long !!! ");
        psg_abort(1);
    } 
 
   if(jhc1 > jhc2)
   {
       printf(" jhc1 must be less than jhc2\n");
       psg_abort(1);
   }


/* LOAD VARIABLES */

  settable(t1, 2,  phi1);
  settable(t2, 2,  phi2);
  settable(t3, 8,  phi3);
  settable(t4,16,  phi4);
  settable(t6, 8,  phi6);
  settable(t5, 8,  rec);

/* INITIALIZE VARIABLES */

  taua = 1.0/(2.0*jhc1);
  taub = taua - 1.0/(2.0*jhc2);
  tauc = 5.5e-3 - taua - taub;

/* Phase incrementation for hypercomplex data */

   if ( phase == 2 )     /* Hypercomplex in t1 */
        tssub(t1, 1, 4);

/* calculate modifications to phases based on current t1 values
   to achieve States-TPPI acquisition */

   if(ix==1)
     d2_init = d2;
     t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);
     if(t1_counter %2) {
       tssub(t1,2,4);
       tssub(t5,2,4);
     }

/* set up so that get (-90,180) phase corrects in F1 if f1180 flag is y */

   tau1 = d2;
   if(f1180[A] == 'y')  tau1 += ( 1.0/(2.0*sw1) );
   if( tau1 < 0.2e-6) tau1 = 0.2e-6; 

   assign(one,v2);		/*	phases for dipsi homonuclear spin lock	*/
   add(two, v2, v4);

/* BEGIN ACTUAL PULSE SEQUENCE */


status(A);
   obspower(satpwr);            /* Set power for presaturation  */
   decpower(pwClvl);              /* Set decoupler1 power to dpwr */
   dec2power(pwNlvl);            /* Set decoupler2 power to pwNlvl */


/* Presaturation Period */


 if(satmode[0] == 'y')
{
  txphase(zero);
  rgpulse(satdly,zero,0.0,0.0);           /* presat */
  obspower(tpwr);                /* Set power for hard pulses  */
}

else  {
 obspower(tpwr);                /* Set power for hard pulses  */
 delay(d1);
}


   rcvroff();
   delay(20.0e-6);
status(B);
  if(samplet[A] == 'w') {

     initval(1.0,v2);
     obsstepsize(135.0);
     xmtrphase(v2);
 
  }


  rgpulse(pw,t1,1.0e-6,0.0);        /* Proton excitation pulse */

  if(samplet[A] == 'w') 
     xmtrphase(zero);

  txphase(zero);
  dec2phase(t2);

  if(samplet[A] == 'w') 
     delay(taua - pwC - SAPS_DELAY);
  else
     delay(taua - pwC);

  decrgpulse(pwC,zero,0.0,0.0);
  delay(2.0e-6);
  zgradpulse(gzlvl1,gt1);
  delay(taub - gt1 - 2.0e-6);
  sim3pulse(2*pw,0.0,2*pwN,zero,zero,t2,0.0,0.0);
  dec2phase(zero);
  delay(tauc - pwN);
  dec2rgpulse(pwN,zero,0.0,0.0);
  delay(2.0e-6);
  zgradpulse(gzlvl1,gt1);
  delay(taua - tauc - gt1 - 2.0e-6 - pwC);
  decrgpulse(pwC,zero,0.0,0.0);
  delay(taub);
  delay(tau1);
  rgpulse(pw,zero,0.0,0.0);
  obspwrf(slpwr);
  zgradpulse(gzlvl2,gt2);
  delay(gstab);
  if (cycles > 1.0)
  {
  	rcvroff();
	starthardloop(v9);
	    dipsi(v2,v4);dipsi(v4,v2);dipsi(v4,v2);dipsi(v2,v4);
	endhardloop();
  }
  obspwrf(4095.0);
  zgradpulse(-gzlvl2,gt2);
  delay(gstab);
  rcvron();
  dec2power(dpwr2);  
  decpower(dpwr);  
  rgpulse(pw,t3,rof1,rof2);
status(C);			/* acquire data */
  setreceiver(t5);
}

