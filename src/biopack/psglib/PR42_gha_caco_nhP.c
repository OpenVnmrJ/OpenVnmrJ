/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* PR42_gha_caco_nhP.c

Ref: (4,2)D Projection-Reconstruction Experiemnts for Protein Backbone
Assignment:  Application to Human Carbonic Anhydrase II and Calbindin
D28K.  Venters, R.A., Coggins, B.E., Kojetin, D., Cavanagh, J. and
Zhou, P. JACS 127(24), 8785-8795 (2005)

    PR42_HACACONH.c 4D version (no CO evolution) R. Venters 10/07/04 
    Semi-constant time Ha evolution corrected by R. Venters 12/11/04

array phi7cal to find maximum signal

To obtain reconstruction software package, please visit
http://zhoulab.biochem.duke.edu/software/pr-calc

*/


#include <standard.h>
#include "bionmr.h"

static int  
             phx[1]={0},   phy[1]={1},

             phi3[1]  = {0},
             phi4[1]  = {0},
             phi5[2]  = {0,2},
             phi6[2]  = {2,0},  
             phi9[8]  = {0,0,1,1},
             rec[4]   = {0,2,2,0};	



pulsesequence()
{

/* DECLARE AND LOAD VARIABLES; parameters used in the last half of the */
/* sequence are declared and initialized as 0.0 in bionmr.h, and       */
/* reinitialized below  */


char        cbdecseq[MAXSTR];                     /*  selective CB decoupling */

int         t1_counter,  		        /* used for states tppi in t1 */
	    ni = getval("ni");

double      d2_init=0.0,  		        /* used for states tppi in t1 */
	    tau1,         		  /* Ha, J active for 1/4 of the time */
            t1a,                       /* time increments for first dimension */
            t1b,
            t1c,
            tau2,                               /* Ca */
            tau3,                               /* CO */
	    tauCH = getval("tauCH"), 		         /* 1/4J delay for CH */
            timeTN = getval("timeTN"),     /* constant time for 15N evolution */
	    zeta = 4.7e-3,                            /* 1/4J delay for C-CO' */
	    theta = 14.0e-3,                          /* 1/4J delay for N-CO' */
            cbpwr,                 /* power level for selective CB decoupling */
            cbdmf,                  /* pulse width for selective CB decoupling */
            cbres,                  /* decoupling resolution of CB decoupling */
           sheila,  /* to transfer J evolution time hyperbolically into tau1 */
	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */

   pwCa180,
   pwCO180,

   phi7cal = getval("phi7cal"),  /* phase in degrees of the last C13 90 pulse */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),

        swHa = getval("swHa"),
        swCa = getval("swCa"),
        swN  = getval("swN"),
        swTilt,                     /* This is the sweep width of the tilt vector */

        cos_N, cos_Ca, cos_Ha, 
        angle_N, angle_Ca, angle_Ha,   /* angle_N is calculated automatically */

        gstab = getval("gstab"),

        gt1 = getval("gt1"),
        gt5 = getval("gt5"),
        gt3 = getval("gt3"),
        gt4= getval("gt4"),
        gt8= getval("gt8"),
        gt6=getval("gt6"),
        gt7=getval("gt7"),

	gzlvl0 = getval("gzlvl0"),
        gzlvl1 = getval("gzlvl1"),
        gzlvl2 = getval("gzlvl2"),
        gzlvl5 = getval("gzlvl5"),
        gzlvl6 = getval("gzlvl6"),
        gzlvl3 = getval("gzlvl3"),
        gzlvl4= getval("gzlvl4"),
        gzlvl8= getval("gzlvl8"),
        gzlvl7= getval("gzlvl7");

/* Load variable */
        cbpwr = getval("cbpwr");
        cbdmf = getval("cbdmf");
        cbres = getval("cbres");
        tau1 = 0;
        tau2 = 0;
        tau3 = 0;
        cos_N = 0;
        cos_Ca = 0;
        cos_Ha = 0;

    getstr("cbdecseq", cbdecseq);


/*   LOAD PHASE TABLE    */

	settable(t3,1,phi3);
	settable(t4,1,phi4);
	settable(t5,2,phi5);
	settable(t6,2,phi6);

        settable(t8,1,phx);
	settable(t9,4,phi9);
	settable(t10,1,phx);
	settable(t11,1,phy);
	settable(t12,4,rec);

        

/*   INITIALIZE VARIABLES   */

 	kappa = 5.4e-3;
	lambda = 2.4e-3;


    /* get calculated pulse lengths of shaped C13 pulses */
      pwCa180=c13pulsepw("ca", "co", "square", 180.0);
      pwCO180=c13pulsepw("co", "ca", "sinc", 180.0);


/* PHASES AND INCREMENTED TIMES */

   /* Set up angles and phases */

   angle_Ha=getval("angle_Ha");  cos_Ha=cos(PI*angle_Ha/180.0);
   angle_Ca=getval("angle_Ca");  cos_Ca=cos(PI*angle_Ca/180.0);

   if ( (angle_Ha < 0) || (angle_Ha > 90) )
   {  printf ("angle_Ha must be between 0 and 90 degree.\n"); psg_abort(1); }

   if ( (angle_Ca < 0) || (angle_Ca > 90) )
   {  printf ("angle_Ca must be between 0 and 90 degree.\n"); psg_abort(1); }

   if ( 1.0 < (cos_Ha*cos_Ha + cos_Ca*cos_Ca) )
   {
       angle_N = 0.0;
       printf ("Impossible angles.\n"); psg_abort(1);
   }
   else
   {
           cos_N=sqrt(1.0- (cos_Ha*cos_Ha + cos_Ca*cos_Ca));
           angle_N = 180.0*acos(cos_N)/PI;
   }

   swTilt=swHa*cos_Ha + swCa*cos_Ca + swN*cos_N;

   if (ix ==1)
   {
      printf("\n\nn\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
      printf ("Maximum Sweep Width: \t\t %f Hz\n", swTilt);
      printf ("Angle_Ha:\t%6.2f\n", angle_Ha);
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
   else if (phase1 == 2)  { tsadd(t3,1,4);}                                /* SC */
   else if (phase1 == 3)  { tsadd(t4,1,4); }                               /* CS */
   else if (phase1 == 4)  { tsadd(t3,1,4); tsadd(t4,1,4);}                 /* SS */
   else { printf ("phase1 can only be 1,2,3,4. \n"); psg_abort(1); }

   if (phase2 == 2)  { tsadd(t10,2,4); icosel = +1; }                      /* N  */
            else                       icosel = -1;

   tau1 = 1.0*t1_counter*cos_Ha/swTilt;
   tau2 = 1.0*t1_counter*cos_Ca/swTilt;
   tau3 = 1.0*t1_counter*cos_N/swTilt;

   tau1 = tau1/2.0;  tau2 = tau2/2.0;  tau3 = tau3/2.0;


/* CHECK VALIDITY OF PARAMETER RANGES */

    if (0.5*ni*(cos_N/swTilt) > timeTN - WFG3_START_DELAY)
       { printf(" ni is too big. Make ni equal to %d or less.\n",
         ((int)((timeTN - WFG3_START_DELAY)*2.0*swTilt/cos_N)));       psg_abort(1);}

    if ( 0.5*0.25*ni*(cos_Ha/swTilt) > tauCH - 2*pwC - 2.0e-6 - gt3)
       {
         printf(" ni is too big for Ha. Make ni equal to %d or less.\n",
            (int) ((tauCH - 2*pwC - 2.0e-6 - gt3)/(0.5*0.25*cos_Ha/swTilt))  );
         psg_abort(1);
       }

    if (0.5*ni*(cos_Ca/swTilt) > zeta - gt8 - pwCa180/2
                         -pwCO180 - WFG2_START_DELAY
                 - 3.0*POWER_DELAY - 3.0*PWRF_DELAY - 4.0e-6)
       {
         printf(" ni is too big for Ca. Make ni equal to %d or less.\n",
            (int) ((zeta - gt8 - pwCa180/2 - pwCO180 - WFG2_START_DELAY
               - 3.0*POWER_DELAY - 3.0*PWRF_DELAY - 4.0e-6)/(0.5*cos_Ca/swTilt)));
         psg_abort(1);
       }


    if ( dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' )
       { printf("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1);}

    if ( dm2[A] == 'y' || dm2[B] == 'y' )
       { printf("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1);}

    if ( dm3[A] == 'y' || dm3[C] == 'y' )
       { printf("incorrect dec3 decoupler flags! Should be 'nyn' or 'nnn' ");
							             psg_abort(1);}
    if ( dpwr2 > 46 )
       { printf("dpwr2 too large! recheck value  ");		     psg_abort(1);}

/*  Hyperbolic sheila seems superior to original zeta approach  */

                                  /* subtract unavoidable delays from tauCH */
    tauCH = tauCH - gt3 - 2.0*GRADIENT_DELAY - 5.0e-5;

 if (angle_Ca == 90.0)
  {
   sheila = 0.0;
  }
 else
 {

 if ((ni-1)/(2.0*sw1) > 2.0*tauCH)
    {
      if (tau1 > 2.0*tauCH) sheila = tauCH;
      else if (tau1 > 0) sheila = 1.0/(1.0/tau1+1.0/tauCH-1.0/(2.0*tauCH));
      else          sheila = 0.0;
    }
 else
    {
      if (tau1 > 0) sheila = 1.0/(1.0/tau1 + 1.0/tauCH - 2.0*sw1/((double)(ni-1)));
      else          sheila = 0.0;
    }
  }
    t1a = tau1 + tauCH;
    t1b = tau1 - sheila;
    t1c = tauCH - sheila;


/*   BEGIN PULSE SEQUENCE   */

status(A);
   	delay(d1);
        if (dm3[B]=='y') lk_hold();

	rcvroff();
        set_c13offset("ca");


	obspower(tpwr);
 	obspwrf(4095.0);
        obsoffset(tof);       /* tof set to water */
	decpower(pwClvl);
	decpwrf(4095.0);
 	dec2power(pwNlvl);
	txphase(one);
	delay(1.0e-5);

	dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 and C13 magnetization*/
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(gzlvl0, 0.5e-3);
	delay(gstab);

status(B);

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
        txphase(t3);
        delay(gstab);
        delay(t1c);
                                                                /* point b */
        rgpulse(pw, t3, 0.0, 0.0);

      txphase(zero);        decphase(t4);

                                               /* -----------HzCz---------- */


      zgradpulse(gzlvl4, gt4);              /* Crush graidient G12*/
      delay(gstab);
                                              /* end of HzCz */
   c13pulse("ca", "co", "square", 90.0, t4, 2.0e-6, 0.0);

      decpower(cbpwr);
      decphase(zero);
      decprgon(cbdecseq,1.0/cbdmf,cbres);
      decon(); 

      delay(tau2);
      dec2rgpulse(2*pwN, zero, 2.0e-6, 2.0e-6);

      decoff();  
      decprgoff();

      zgradpulse(gzlvl8, gt8);

      decpower(cbpwr);
      decphase(zero);
      decprgon(cbdecseq,1.0/cbdmf,cbres);
      decon();

      delay(tauCH- gt8 - pw - 2*pwN - 6.0e-6);

      rgpulse(2.0*pw, zero, 2.0e-6, 0.0);

      delay(zeta - tauCH -pw - pwCO180 - pwCa180/2 - 2.0*WFG_START_DELAY
                 - 3.0*POWER_DELAY - 3.0*PWRF_DELAY - 4.0e-6);

      decoff();
      decprgoff();

      c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 0.0);
   c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0);

      zgradpulse(gzlvl8, gt8);              /* 2.0*GRADIENT_DELAY */

      decpower(cbpwr);
      decphase(zero);
      decprgon(cbdecseq,1.0/cbdmf,cbres);
      decon();

      delay(zeta - gt8 - pwCa180/2 - pwCO180 - 2.0*WFG_START_DELAY
                 - 3.0*POWER_DELAY - 3.0*PWRF_DELAY - 4.0e-6 - tau2);   /* const-time */

      decoff();
      decprgoff();

      c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 0.0);

   c13pulse("ca", "co", "square", 90.0, zero, 2.0e-6, 0.0);
                                             /* ---------CazCOz----------- */

      set_c13offset("co");   

      zgradpulse(gzlvl6, gt6);            /* Crush gradient G14 */
      delay(gstab);


      h1decon("DIPSI2", 27.0, 0.0);
      decphase(t5);
                                             /* ------- CazCOz ------------*/

   c13pulse("co", "ca", "sinc", 90.0, t5, 2.0e-6, 0.0);


      decphase(zero);
      delay(zeta - WFG_START_DELAY - 2.0*POWER_DELAY - 2.0*PWRF_DELAY - pwCa180/2 - 2.0e-6);

    c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0);     

      dec2phase(zero);
      delay(theta - zeta - pwCa180/2 - WFG_START_DELAY - pwCO180/2
                                  - 2.0*POWER_DELAY - 2.0*PWRF_DELAY - 2.0e-6);

   sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
                    zero, zero, zero, 2.0e-6, 2.0e-6);

      initval(phi7cal, v7);
      decstepsize(1.0);
      dcplrphase(v7);                      
      dec2phase(t8);
      delay(theta - SAPS_DELAY - WFG_START_DELAY - pwCO180/2 - 4.0e-6  
                             - 2.0*POWER_DELAY - 2.0*PWRF_DELAY);

   c13pulse("co", "ca", "sinc", 90.0, zero, 2.0e-6, 0.0);
                                          /* -----------CzNz----------- */   
      dcplrphase(zero);
      h1decoff();

      zgradpulse(gzlvl7, gt7);      
      delay(gstab);

      h1decon("DIPSI2", 27.0, 0.0);
                                                  
                                          /*  -------------CzNz---------- */
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
          delay(timeTN - pwCa180 - WFG_START_DELAY - 2.0*POWER_DELAY 
						- 2.0*PWRF_DELAY - 2.0e-6);
	c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0); /*pwCa180*/
          delay(tau3 - kappa - PRG_STOP_DELAY - POWER_DELAY - PWRF_DELAY);
          h1decoff();		     /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
	  txphase(zero);
          delay(kappa - gt1 - 2.0*GRADIENT_DELAY - gstab);


    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  delay(gstab);
	}
    else if (tau3 > (kappa - pwCa180 - WFG_START_DELAY - 2.0*POWER_DELAY - 2.0e-6))
	{
          delay(timeTN + tau3 - kappa -PRG_STOP_DELAY -POWER_DELAY -PWRF_DELAY);
          h1decoff();		     /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
	  txphase(zero); 			/* WFG_START_DELAY  + 2.0*POWER_DELAY */
	c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0); /*pwCa180*/
          delay(kappa - pwCa180 - WFG_START_DELAY - 2.0*POWER_DELAY - 1.0e-6 - gt1 
					        - 2.0*GRADIENT_DELAY - gstab);


  zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  delay(gstab);
	}
    else if (tau3 > gt1 + 2.0*GRADIENT_DELAY + 1.0e-4)
	{
          delay(timeTN + tau3 - kappa -PRG_STOP_DELAY -POWER_DELAY -PWRF_DELAY);
          h1decoff();		     /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
	  txphase(zero);
          delay(kappa - tau3 - pwCa180 - WFG_START_DELAY - 2.0*POWER_DELAY
 								    - 2.0e-6);
	c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0); /*pwCa180*/
          delay(tau3 - gt1 - 2.0*GRADIENT_DELAY - gstab);


   zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  delay(gstab);
	}
    else
	{
          delay(timeTN + tau3 - kappa -PRG_STOP_DELAY -POWER_DELAY -PWRF_DELAY);
          h1decoff();		     /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
	  txphase(zero);
    	  delay(kappa - tau3 - pwCa180 - WFG_START_DELAY - 2.0*POWER_DELAY
			         - 2.0e-6 - gt1 - 2.0*GRADIENT_DELAY - gstab);

    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  delay(gstab);
	c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0); /*pwCa180*/
          delay(tau3);
	}


        sim3pulse(pw, 0.0, pwN, zero, zero, t10, 0.0, 0.0);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(gzlvl5, gt5);
	delay(lambda - 1.3*pwN - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl5, gt5);
	txphase(one);
	dec2phase(t11);
	delay(lambda - 1.3*pwN - gt5);

	sim3pulse(pw, 0.0, pwN, one, zero, t11, 0.0, 0.0);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(gzlvl6, gt5);
	delay(lambda - 1.3*pwN - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	dec2phase(t10);
	zgradpulse(gzlvl6, gt5);

             {delay(lambda - 0.65*(pw + pwN) - gt5);
	      rgpulse(pw, zero, 0.0, 0.0); 
	      delay((gt1/10.0) + gstab - 0.3*pw + 2.0*GRADIENT_DELAY
							+ POWER_DELAY);  }
	rgpulse(2.0*pw, zero, 0.0, 0.0);

	dec2power(dpwr2);				       /* POWER_DELAY */
               zgradpulse(icosel*gzlvl2, gt1/10.0);            /* 2.0*GRADIENT_DELAY */

	setreceiver(t12);
	statusdelay(C, gstab);
}	

