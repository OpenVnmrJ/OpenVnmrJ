/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* hadamac.c

This sequence should be able to separate gly,ser,asn/asp,cys/aromatic,ala and thr.
Thr appears for free in a separate subspectrum due to its high Cb-Cg coupling. 
When setting tauC3 (Cb->Ca & Cb->Cg) to 16ms, they appears mostly negative.
tauC1 (Cb->Ca transfer) can be optimized to the JCBCA value without being bothered
by the Ca remaining from the first block. 

Using the Hadamard steps defined as belove, the bands have the following signs:


had_flg		Thr	Gly	Ser	Ala	Asn/Asp		Rest(Cys/aro)
1		+	+	+	+	+		+
2		+	+	-	-	-		+
3		-	+	+	+	-		+
4		-	+	-	-	+		+
5		-	-	+	-	+		+
6		-	-	-	+	-		+
7		+	-	+	-	-		+	
8		+	-	-	+	+		+

epsilon value should be between 4 and 6ms.

 wft2d(1,0,0,0,-1,0,0,0,0,-1,0,0,0,-1,0,0) 
 wft2d(0,0,-1,0,0,0,1,0,0,0,0,1,0,0,0,1)
 wft2d(1,0,-1,0,-1,0,1,0,0,-1,0,1,0,-1,0,1)

NOTE: The parameter "had_flg" is a string parameter. Therefore, arraying this 
parameter should be done as had_flg='1','2','3',...., NOT had_flg=1,2,3,...

Contributed to BioPack by Bernhard Brutscher, IBS, Grenoble
See "Hadamard Amino-Acid-Type Edited NMR Experiment for Fast
     Protein Resonance Assignment", JACS, 130,5014-5015(2008)
  E. Lescop, R.Rasia and B. Brutscher

Modified for BioPack, GG, Varian, April 2008
  Based on IBS_hadamard.c  (changed tau1/tau2 sw1/sw2 phase1 calls to use
  sw2/ni2/d3/f2180 for N15 evolution. Sequence is coded for 2D only)
*/




 
#include <standard.h>
#include "bionmr.h"
 
static int   /*  T is for TROSY='y', phx etc also enable TROSY phase changes */
             phx[1]={0},   phy[1]={1},

             phi1[4]   = {1,1,3,3},
             phi3[4]   = {0,0,0,0},
             phi5[2]  = {0,2},
             phi6[2]  = {2,0},  
             phi9[8]  = {0,0,0,0,1,1,1,1},
             phi12[8]   = {0,2,2,0,2,0,0,2},		     
	     rec2[8] = {3,1,1,3,1,3,3,1};



void pulsesequence()
{

/* DECLARE AND LOAD VARIABLES; parameters used in the last half of the */
/* sequence are declared and initialized as 0.0 in bionmr.h, and       */
/* reinitialized below  */

char        f2180[MAXSTR],   		      /* Flag to start t2 @ halfdwell */
            fil_flg1[MAXSTR],
            had_flg[MAXSTR],
            shname1[MAXSTR],
	    shname2[MAXSTR],
	    ala_flg[MAXSTR],	    
            ser_flg[MAXSTR],
	    SE_flg[MAXSTR],			    /* SE_flg */
  	    TROSY[MAXSTR];			    /* do TROSY on N15 and H1 */

int         t2_counter,  		        /* used for states tppi in t2 */
	    ni2 = getval("ni2");

double      d3_init=0.0,  		        /* used for states tppi in t2 */
            stCwidth = 80.0,
	    shpw1,shpw2,         				         /*  t1 delay */
	    tauCH = getval("tauCH"), 		         /* 1/4J delay for CH */
	    tauC1 = getval("tauC1"),
	    tauC2 = getval("tauC2"),
	    tauC3 = getval("tauC3"),
            had2,had3,
            timeTN = getval("timeTN"),     /* constant time for 15N evolution */
	    eta = 4.6e-3,
	    theta = 14.0e-3,
            
	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */

   pwS1, pwS2,	pwS3,	pwS4, pwS5,pwS6,pwS7,
   phi7cal = getval("phi7cal"),  /* phase in degrees of the last C13 90 pulse */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw2 = getval("sw2"),

	gt3 = getval("gt3"),
	gt5 = getval("gt5"),
	gstab = getval("gstab"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl5 = getval("gzlvl5"),
	flip_angle=120.0,had1=0.0,
	epsilon = getval("epsilon");
    fil_flg1[0]='n'; 
    ser_flg[0]='n';   /*initialize*/

    getstr("f2180",f2180);
    getstr("had_flg",had_flg);
    getstr("shname1",shname1);
    getstr("shname2",shname2);    
    getstr("TROSY",TROSY);
    getstr("SE_flg",SE_flg);


/*   LOAD PHASE TABLE    */

	settable(t1,4,phi1);
	settable(t3,4,phi3);
	settable(t4,1,phx);
	settable(t5,2,phi5);
	settable(t6,2,phi6);
        settable(t8,1,phx);
	settable(t9,8,phi9);
	settable(t10,1,phx);
	settable(t11,1,phy);

	settable(t12,8,phi12);
	settable(t13,8,rec2);




/*   INITIALIZE VARIABLES   */

        shpw1 = pw*8.0;
        shpw2 = pwC*8.0;
 	kappa = 5.4e-3;
	lambda = 2.4e-3;
        had2=0.5/135.0;
        had3=0.5/135.0;

        ala_flg[0]='n';

        if (had_flg[0] == '1')
          { fil_flg1[0]='n';ser_flg[0]='n';flip_angle=120.0;had1=0.0;} 
        if (had_flg[0] == '2')
          { fil_flg1[0]='y';ser_flg[0]='n';flip_angle=120.0;had1=0.0;} 
        if (had_flg[0] == '3')
          { fil_flg1[0]='n';ser_flg[0]='y';flip_angle=120.0;had1=0.0;} 
        if (had_flg[0] == '4')
          { fil_flg1[0]='y';ser_flg[0]='y';flip_angle=120.0;had1=0.0;} 
        if (had_flg[0] == '5')
          { fil_flg1[0]='n';ser_flg[0]='n';flip_angle=60.0;had1=0.5/140.0;} 
        if (had_flg[0] == '6')
          { fil_flg1[0]='y';ser_flg[0]='n';flip_angle=60.0;had1=0.5/140.0;} 
        if (had_flg[0] == '7')
          { fil_flg1[0]='n';ser_flg[0]='y';flip_angle=60.0;had1=0.5/140.0;} 
        if (had_flg[0] == '8')
          { fil_flg1[0]='y';ser_flg[0]='y';flip_angle=60.0;had1=0.5/140.0;} 
	


    if( pwC > 20.0*600.0/sfrq )
	{ printf("increase pwClvl so that pwC < 20*600/sfrq");
	  psg_abort(1); }

    /* get calculated pulse lengths of shaped C13 pulses */
	pwS1 = c13pulsepw("cab", "co", "square", 90.0); 
	pwS2 = c13pulsepw("ca", "co", "square", 180.0); 
	pwS3 = c13pulsepw("co", "ca", "sinc", 180.0); 
        pwS4 = c_shapedpw("isnob5",80.0,0.0,zero, 2.0e-6, 2.0e-6);

        pwS6 = c_shapedpw("reburp",80.0,0.0,zero, 2.0e-6, 2.0e-6); /* attention, y a aussi des 180 CaCb après les filtres*/

        pwS7 = c_shapedpw(shname2,80.0,150.0,zero, 2.0e-6, 2.0e-6);
        pwS5 = c_shapedpw("isnob5",30.0,0.0,zero, 2.0e-6, 2.0e-6);

/* CHECK VALIDITY OF PARAMETER RANGES */


    if ( 0.5*ni2*1/(sw2) > timeTN - WFG3_START_DELAY)
       { printf(" ni2 is too big. Make ni2 equal to %d or less.\n", 
  	 ((int)((timeTN - WFG3_START_DELAY)*2.0*sw2))); 	     psg_abort(1);}

    if ( dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' )
       { printf("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1);}

    if ( dm2[A] == 'y' || dm2[B] == 'y' )
       { printf("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1);}

    if ( dm3[A] == 'y' || dm3[C] == 'y' )
       { printf("incorrect dec3 decoupler flags! Should be 'nyn' or 'nnn' ");
							             psg_abort(1);}
    if ( dpwr2 > 46 )
       { printf("dpwr2 too large! recheck value  ");		     psg_abort(1);}

    if ( pw > 20.0e-6 )
       { printf(" pw too long ! recheck value ");	             psg_abort(1);} 
  
    if ( pwN > 100.0e-6 )
       { printf(" pwN too long! recheck value ");	             psg_abort(1);} 
 
    if ( TROSY[A]=='y' && dm2[C] == 'y' )
       { text_error("Choose either TROSY='n' or dm2='n' ! ");        psg_abort(1);}



/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (TROSY[A]=='y')
	 {  if (phase2 == 2)   				      icosel = +1;
            else 	    {tsadd(t4,2,4);  tsadd(t10,2,4);  icosel = -1;}
	 }
    else {
	if (SE_flg[0]=='y') 
                  {
		  if (phase2 == 2)  {tsadd(t10,2,4); icosel = +1;}
	          else 			       icosel = -1;    
		  }
	else {  if (phase2 == 2)  {tsadd(t8,1,4); }
              }
	 }



/*  Set up f2180  */

    tau2 = d3;    /* run 2D exp for NH correlation, but must use tau2 instead of tau1
                     because bionmr.h is written for nh_evol* to do tau2 evolution*/

    if((f2180[A] == 'y') && (ni2 > 1.0))  /* use f2180 to control tau2 */
	{ tau2 += ( 1.0 / (2.0*sw2) ); if(tau2 < 0.2e-6) tau2 = 0.0; }
    tau2 = tau2/2.0;



/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw1 + 0.5 );
   if(t2_counter % 2)
        { tsadd(t8,2,4); tsadd(t12,2,4);  tsadd(t13,2,4);  }




/* BEGIN PULSE SEQUENCE */

status(A);
   	delay(d1);
      if (dm3[B]=='y') lk_hold();

	rcvroff();
        set_c13offset("cab");
	obsoffset(tof);
	obspower(tpwr);
 	obspwrf(4095.0);
	decpower(pwClvl);
	decpwrf(4095.0);
 	dec2power(pwNlvl);
	txphase(one);
	delay(1.0e-5);
        if (TROSY[A] == 'n')
	dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 and C13 magnetization*/
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(gzlvl0, 0.5e-3);
	delay(gstab);
      if (TROSY[A] == 'n')
	dec2rgpulse(pwN, one, 0.0, 0.0);
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(0.7*gzlvl0, 0.5e-3);
	delay(gstab);

      if(dm3[B] == 'y')			  /*optional 2H decoupling on */
         {dec3unblank(); dec3rgpulse(1/dmf3, one, 0.0, 0.0); 
          dec3unblank(); setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);} 
        rgpulse(pw, zero, 2.0e-6, 0.0);
        zgradpulse(gzlvl5, gt5);
        delay(tauCH - gt5 - WFG2_START_DELAY - 0.5e-3 + 68.0e-6 );

        sim_c13adiab_inv_pulse("", "aliph", stCwidth, "sech1", 2.0*pw, 1.0e-3,
                                                  zero, zero, 2.0e-6, 2.0e-6);

        zgradpulse(gzlvl5, gt5);
        delay(tauCH - gt5 - 0.5e-3 + 68.0e-6);
        rgpulse(pw, one, 0.0, 0.0);

      if (ser_flg[0] == 'n' )
         delay(pwS5);
      if (ser_flg[0] == 'y' )
        c_shapedpulse("isnob5",30.0,24.0,zero, 2.0e-6, 2.0e-6);  

/*********************************** transfer  CB->CA + DEPT CBH **************/
	zgradpulse(gzlvl3, gt3*1.2);
	delay(gstab);

        decrgpulse(pwC, t3, 0.0, 0.0);

        rgpulse(pw, three, 0.0, 0.0);
      if (flip_angle > 90.0) delay(pw*(flip_angle/90.0-1));

      if (fil_flg1[0] == 'y') 
        {
         /* JCOCA & JCOCB is turned on*/
          zgradpulse(gzlvl3, gt3);
	  delay(had2*0.5-pwS4*0.5-pwS7-gt3);
          c_simshapedpulse(shname2,80.0,150.0,0.0,0.0,zero,zero,zero, 2.0e-6, 2.0e-6);	
          c_simshapedpulse("isnob5",80.0,0.0,pw*2.0,0.0,zero,zero,zero, 2.0e-6, 2.0e-6);
          zgradpulse(gzlvl3, gt3);        
          delay(had2*0.5-pwS4*0.5-gt3);
          rgpulse(pw*flip_angle/90.0, t1, 0.0, 0.0);
	if (flip_angle < 90.0) delay(pw*(1-flip_angle/90.0));
          zgradpulse(gzlvl3, 1.1*gt3);		
          delay(had3*0.5-shpw1*0.5-1.1*gt3);		
          shaped_pulse(shname1,shpw1,two,0.0,0.0);
          delay((tauC3-(had2+pw*120/90*2))*0.5-pwS4*0.5-had3*0.5-shpw1*0.5-pwS7);
          c_simshapedpulse(shname2,80.0,150.0,0.0,0.0,zero,zero,zero, 2.0e-6, 2.0e-6);			
          c_shapedpulse("isnob5",80.0,0.0,two, 2.0e-6, 2.0e-6);  
          zgradpulse(gzlvl3, 1.1*gt3);	
          delay((tauC3-(had2+pw*120/90*2))*0.5-pwS4*0.5-1.1*gt3);
       }

     if (fil_flg1[0] == 'n') 
       {
         /* JCOCA & JCOCB is turned off*/
          zgradpulse(gzlvl3, gt3);
          delay(epsilon/4.0-pwS7*0.5-gt3);
	  c_simshapedpulse(shname2,80.0,150.0,0.0,0.0,zero,zero,zero, 2.0e-6, 2.0e-6);	
          delay(had2*0.5-pwS4*0.5-epsilon/4.0-pwS7*0.5);
          c_simshapedpulse("isnob5",80.0,0.0,pw*2.0,0.0,zero,zero,zero, 2.0e-6, 2.0e-6);
          zgradpulse(gzlvl3, gt3);
          delay(had2*0.5-pwS4*0.5-gt3);
          rgpulse(pw*flip_angle/90.0, t1, 0.0, 0.0);
	if (flip_angle < 90.0) delay(pw*(1-flip_angle/90.0));
        if (had3*0.5-shpw1*0.5-epsilon/4.0-pwS7*0.5>0.0)		
          {
            zgradpulse(gzlvl3, 1.1*gt3);		
	    delay(epsilon/4.0-pwS7*0.5-1.1*gt3);
	    c_simshapedpulse(shname2,80.0,150.0,0.0,0.0,zero,zero,zero, 2.0e-6, 2.0e-6);		
	    delay(had3*0.5-shpw1*0.5-epsilon/4.0-pwS7*0.5);		
	    shaped_pulse(shname1,shpw1,two,0.0,0.0);
	    delay((tauC3-(had2+pw*120/90*2))*0.5-pwS4*0.5-had3*0.5-shpw1*0.5);
          }
        else 
          {
            zgradpulse(gzlvl3, 1.1*gt3);		
	    delay(had3*0.5-shpw1*0.5-1.1*gt3);		
	    shaped_pulse(shname1,shpw1,two,0.0,0.0);
	    delay(epsilon/4.0-pwS7*0.5-had3*0.5-shpw1*0.5);
	    c_simshapedpulse(shname2,80.0,150.0,0.0,0.0,zero,zero,zero, 2.0e-6, 2.0e-6);		
	    delay((tauC3-(had2+pw*120/90*2))*0.5-pwS4*0.5-epsilon/4.0-pwS7*0.5);
          }
	
	  c_shapedpulse("isnob5",80.0,0.0,two, 2.0e-6, 2.0e-6);  
          zgradpulse(gzlvl3, 1.1*gt3);		
	  delay((tauC3-(had2+pw*120/90*2))*0.5-pwS4*0.5-1.1*gt3);
       }

     if (fil_flg1[0] == 'c') 
       {
        /* JCOCA & JCOCB is turned off*/
          zgradpulse(gzlvl3, gt3);
	  delay(had2*0.5-pwS4*0.5-gt3);
	  c_simshapedpulse("isnob5",80.0,0.0,pw*2.0,0.0,zero,zero,zero, 2.0e-6, 2.0e-6);
          zgradpulse(gzlvl3, gt3);
	  delay(had2*0.5-pwS4*0.5-gt3);
          rgpulse(pw*flip_angle/90.0, t1, 0.0, 0.0);
	if (flip_angle < 90.0) delay(pw*(1-flip_angle/90.0));
          zgradpulse(gzlvl3, 1.1*gt3);
	  delay(had3*0.5-shpw1*0.5-1.1*gt3);		
	  shaped_pulse(shname1,shpw1,two,0.0,0.0);
	  delay((tauC3-(had2+pw*120.0/90.0*2.0))*0.5-pwS4*0.5-had3*0.5-shpw1*0.5);
	  c_shapedpulse("isnob5",80.0,0.0,two, 2.0e-6, 2.0e-6);  
          zgradpulse(gzlvl3, 1.1*gt3);
       	  delay((tauC3-(had2+pw*120.0/90.0*2.0))*0.5-pwS4*0.5-1.1*gt3);
       }

/*********************************** 2nd transfer  CB->CA +DEPT CAH ***********/
          decrgpulse(pwC, zero, 0.0, 0.0);
	  c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);  
          delay(tauC1-pwS3-pwS4*0.5);
          c_shapedpulse("reburp",80.0,0.0,zero, 2.0e-6, 2.0e-6);  
          delay(tauC1-tauC2-pwS3-pwS4*0.5);
	  c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);  
          delay(tauC2-pw*8.0-had1);
          shaped_pulse(shname1,shpw1,two,0.0,0.0);
          delay(had1);
	  c13pulse("cab", "co", "square", 90.0, zero, 0.0, 0.0);  
/******************************************************************************/
        if(dm3[B] == 'y')		         /*optional 2H decoupling off */
           {dec3rgpulse(1/dmf3, three, 0.0, 0.0); dec3blank();
            setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3); dec3blank();}
  	  zgradpulse(gzlvl3, gt3);
	  delay(2.0e-4);
	  h1decon("DIPSI2", 27.0, 0.0);/*POWER_DELAY+PWRF_DELAY+PRG_START_DELAY */
	  c13pulse("co", "ca", "sinc", 90.0, t5, 2.0e-6, 0.0);          /* point e */
 	  decphase(zero);
	  delay(eta - 2.0*POWER_DELAY - 2.0*PWRF_DELAY);
					        /* 2*POWER_DELAY+2*PWRF_DELAY */
	  c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0);     /* pwS2 */
	  dec2phase(zero);
	  delay(theta - eta - pwS2 - WFG3_START_DELAY);
							  /* WFG3_START_DELAY */
	  sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
					     zero, zero, zero, 2.0e-6, 2.0e-6);
  	  initval(phi7cal, v7);
	  decstepsize(1.0);
	  dcplrphase(v7);					        /* SAPS_DELAY */
	  dec2phase(t8);
	  delay(theta - SAPS_DELAY);
      if (SE_flg[0]=='y')                                               /* point f */
	{
 	  nh_evol_se_train("co", "ca"); /* common part of sequence in bionmr.h  */
          if (dm3[B]=='y') lk_sample();
	}
	else
	{
	  nh_evol_train("co", "ca"); /* common part of sequence in bionmr.h  */
          if (dm3[B]=='y') lk_sample();
	}
}		 


