/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
   ihadamacP.c 

This sequence should be able to separate gly,ser,asn/asp,cys/aromatic,ala and 
thr. Thr appears for free in a separate subspectrum due to its high Cb-Cg 
coupling. When setting tauC3 (Cb->Ca & Cb->Cg) to 16ms, they appears mostly 
negative.
tauC1 (Cb->Ca transfer) can be optimized to the JCBCA value without being 
bothered by the Ca remaining from the first block. 
Using the Hadamard steps defined as below, the bands have the following signs:


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
- inverse Cb and CO at the same time in the first block.
  (double CB refocusing/ CO inversion and 1H refocusing) 
  The net effect of two consecutive double band pulses (see sign inversion
  1/JCbCO for fil_flg1='y') is a ~0 overall phase shift (compensated BS). 

set f1coef='1 0 -1 0 0 1 0 1' for SE_flg='y'

Optimized phi7cal on a first increment for best s/n
See Sophie Feuerstein, Michael J. Plevin, Dieter Willbold and Bernhard Brutscher,
 J. Magn. Reson., 214, 329-33(2012)
*/
 
#include <standard.h>
#include "bionmr.h"

 /*wft2d(1,0,0,0,-1,0,0,0,0,-1,0,0,0,-1,0,0) 
  wft2d(0,0,-1,0,0,0,1,0,0,0,0,1,0,0,0,1)
  wft2d(1,0,-1,0,-1,0,1,0,0,-1,0,1,0,-1,0,1)*/
 
static int   /*  T is for TROSY='y', phx etc also enable TROSY phase changes */
             phx[1]={0},   phy[1]={1},

             phi1[4]   = {1,1,3,3},
             phi3[4]   = {0,0,0,0},
             phi5[2]  = {0,2},
             phi6[2]  = {2,0},  
             phi9[8]  = {0,0,0,0,1,1,1,1},
             phi12[8]   = {0,2,2,0,2,0,0,2},		     
             phi13[8]   = {2,0,0,2,0,2,2,0},
	     rec2[8] = {3,1,1,3,1,3,3,1};

pulsesequence()
{

/* DECLARE AND LOAD VARIABLES; parameters used in the last half of the */
/* sequence are declared and initialized as 0.0 in bionmr.h, and       */
/* reinitialized below  */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
            fil_flg1[MAXSTR],
            had_flg[MAXSTR],
            shname1[MAXSTR],
	    shname2[MAXSTR],
	    ser_flg[MAXSTR],
	    SE_flg[MAXSTR],			    /* SE_flg */
  	    TROSY[MAXSTR];			    /* do TROSY on N15 and H1 */

int         t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  		        /* used for states tppi in t1 */
	    ni = getval("ni"),
	    ni2 = getval("ni2");

double      d2_init=0.0,  		        /* used for states tppi in t1 */
            d3_init=0.0,  		        /* used for states tppi in t1 */
            stCwidth = 80.0,
	    tau1,         				         /*  t1 delay */
	    tauCH = getval("tauCH"), 		         /* 1/4J delay for CH */
	    tauC1 = getval("tauC1"),
	    tauC3 = getval("tauC3"),
            had2,had3,
            timeTN = getval("timeTN"),     /* constant time for 15N evolution */
            timeCN = getval("timeCN"),     /* constant time for 15N evolution */
            
	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */

   pwS1, pwS2,	pwS3,	pwS4, pwS5,pwS6,pwS7,pwS8,
   phi7cal = getval("phi7cal"),  /* phase in degrees of the last C13 90 pulse */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),
        t2a,t2b,halfT2,

	gt0 = getval("gt0"),				   /* other gradients */
	gt1 = getval("gt1"),
	gt3 = getval("gt3"),
	gt5 = getval("gt5"),
	gt6 = getval("gt6"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl6 = getval("gzlvl6"),
	flip_angle,had1,
	epsilon = getval("epsilon");
	had1 = getval("had1");


    getstr("f1180",f1180);
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

 	kappa = 5.4e-3;
	lambda = 2.4e-3;
        had2=0.5/135.0;
        had3=0.5/135.0;

        ser_flg[0]='n'; fil_flg1[0]='n';
        if (had_flg[0] == '1')
          { fil_flg1[0]='n';ser_flg[0]='n';flip_angle=120.0;had1=0.0;} 
        if (had_flg[0] == '2')
          { fil_flg1[0]='y';ser_flg[0]='n';flip_angle=120.0;had1=0.0;} 
        if (had_flg[0] == '3')
          { fil_flg1[0]='n';ser_flg[0]='y';flip_angle=120.0;had1=0.5/140;} 
        if (had_flg[0] == '4')
          { fil_flg1[0]='y';ser_flg[0]='y';flip_angle=120.0;had1=0.5/140;} 
        if (had_flg[0] == '5')
          { fil_flg1[0]='n';ser_flg[0]='n';flip_angle=60.0;had1=0.0;} 
        if (had_flg[0] == '6')
          { fil_flg1[0]='y';ser_flg[0]='n';flip_angle=60.0;had1=0.0;} 
        if (had_flg[0] == '7')
          { fil_flg1[0]='n';ser_flg[0]='y';flip_angle=60.0;had1=0.5/140;} 
        if (had_flg[0] == '8')
          { fil_flg1[0]='y';ser_flg[0]='y';flip_angle=60.0;had1=0.5/140;} 
	


    if( pwC > 20.0*600.0/sfrq )
	{ printf("increase pwClvl so that pwC < 20*600/sfrq");
	  psg_abort(1); }

    /* get calculated pulse lengths of shaped C13 pulses */
	pwS1 = c13pulsepw("cab", "co", "square", 90.0); 
	pwS2 = c13pulsepw("ca", "co", "square", 180.0); 
	pwS3 = c13pulsepw("co", "ca", "sinc", 180.0); 
        pwS4 = c_shapedpw("isnob5",80.0,0.0,zero, 2.0e-6, 2.0e-6);
	pwS6 = c13pulsepw("cab", "co", "square", 180.0); 

        pwS7 = c_shapedpw(shname2,80.0,150.0,zero, 2.0e-6, 2.0e-6);
        pwS5 = c_shapedpw("isnob5",30.0,0.0,zero, 2.0e-6, 2.0e-6);

/* CHECK VALIDITY OF PARAMETER RANGES */


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
	 {  if (phase1 == 2)   				      icosel = +1;
            else 	    {tsadd(t4,2,4);  tsadd(t10,2,4);  icosel = -1;}
	 }
    else {
	if (SE_flg[0]=='y') 
                  {
		  if (phase1 == 2)  {tsadd(t10,2,4); icosel = +1;}
	          else 			       icosel = -1;    
		  }
	else {  if (phase1 == 2)  {tsadd(t8,1,4); }
              }
	 }



/*  Set up f1180  */

    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;



/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2)
        { tsadd(t8,2,4); tsadd(t12,2,4);  tsadd(t13,2,4);  }

/* Set up CONSTANT/SEMI-CONSTANT time evolution in N15 */

    halfT2 = 0.0;

   if (ni > 1)
   {
   halfT2 = 0.5*(ni-1)/sw1;
   t2b = (double) t1_counter*((halfT2 - timeTN)/((double)(ni-1)));
    if(t2b < 0.0) t2b = 0.0;
    t2a = timeTN- tau1 + t2b;
    if(t2a < 0.2e-6)  t2a = 0.0;
    }
    else
    {
    t2b = 0.0;
    t2a = timeTN - tau1;
    }





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
	delay(1.0e-4);
        if (TROSY[A] == 'n')
	dec2rgpulse(pwN, one, 0.0, 0.0);
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(0.7*gzlvl0, 0.5e-3);
	delay(5.0e-4);

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
	delay(2.0e-4);

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
	delay(had3*0.5-pw*8.0*0.5-1.1*gt3);		
	shaped_pulse(shname1,pw*8.0,two,0.0,0.0);
	delay((tauC3-(had2+pw*120/90*2))*0.5-pwS4*0.5-had3*0.5-pw*8.0*0.5-pwS7);
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

if (had3*0.5-pw*8.0*0.5-epsilon/4.0-pwS7*0.5>0.0)		
{
zgradpulse(gzlvl3, 1.1*gt3);		
	delay(epsilon/4.0-pwS7*0.5-1.1*gt3);
	c_simshapedpulse(shname2,80.0,150.0,0.0,0.0,zero,zero,zero, 2.0e-6, 2.0e-6);			
	delay(had3*0.5-pw*8.0*0.5-epsilon/4.0-pwS7*0.5);		
	shaped_pulse(shname1,pw*8.0,two,0.0,0.0);
	delay((tauC3-(had2+pw*120/90*2))*0.5-pwS4*0.5-had3*0.5-pw*8.0*0.5);
}
else 
{
zgradpulse(gzlvl3, 1.1*gt3);		
	delay(had3*0.5-pw*8.0*0.5-1.1*gt3);		
	shaped_pulse(shname1,pw*8.0,two,0.0,0.0);
	delay(epsilon/4.0-pwS7*0.5-had3*0.5-pw*8.0*0.5);
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
	delay(had3*0.5-pw*8.0*0.5-1.1*gt3);		
	shaped_pulse(shname1,pw*8.0,two,0.0,0.0);
	delay((tauC3-(had2+pw*120.0/90.0*2.0))*0.5-pwS4*0.5-had3*0.5-pw*8.0*0.5);
	c_shapedpulse("isnob5",80.0,0.0,two, 2.0e-6, 2.0e-6);  
zgradpulse(gzlvl3, 1.1*gt3);
	delay((tauC3-(had2+pw*120.0/90.0*2.0))*0.5-pwS4*0.5-1.1*gt3);
}


/*********************************** 2nd transfer  CB->CA +DEPT CAH ***********/
/*********************************** intra-residue CA-N transfer       ******/

        decrgpulse(pwC, zero, 0.0, 0.0);
	h1decon("DIPSI2", 27.0, 0.0);/*POWER_DELAY+PWRF_DELAY+PRG_START_DELAY */
        delay(tauC1+pwS6);
	c13pulse("co", "ca", "sinc", 90.0, t5, 0.0, 0.0);  
        delay(timeCN-pwS6);
        c13pulse( "cab", "co", "square", 180.0, zero, 0.0, 0.0); 
	sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
					     zero, zero, zero, 2.0e-6, 2.0e-6);
        delay(timeCN);
	initval(phi7cal, v7);
	decstepsize(1.0);
	dcplrphase(v7);					        /* SAPS_DELAY */
	c13pulse("co", "ca", "sinc", 90.0, one, 0.0, 0.0);  
        dcplrphase(zero);
        delay(tauC1-had1*2.0-pwN*2.0);
          h1decoff();                /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */

        delay(had1*2.0);
        decrgpulse(pwC, one, 0.0, 0.0);

/***************************************************************************/
/****     15N-13CA back transfer             ******************************/
/***************************************************************************/

        zgradpulse(gzlvl3, gt3);
        delay(2.0e-4);

	h1decon("DIPSI2", 27.0, 0.0);/*POWER_DELAY+PWRF_DELAY+PRG_START_DELAY */

        dec2rgpulse(pwN, t8, 0.0, 0.0);

        decphase(zero);
        dec2phase(t9);
      delay (t2a+pwS2);
        dec2rgpulse (2.0*pwN, t9, 0.0, 0.0);
        delay (t2b);
        c13pulse( "ca", "co", "square", 180.0, zero, 0.0, 0.0); /*pwS2*/

        dec2phase(t10);
/***************************************************************************/
    if (tau1 > kappa + PRG_STOP_DELAY)
        {
          delay(timeTN - pwS3 -WFG_START_DELAY - 2.0*POWER_DELAY  
                                                - 2.0*PWRF_DELAY - 2.0e-6);
        c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);
          delay(tau1 - kappa - PRG_STOP_DELAY - POWER_DELAY - PWRF_DELAY);
          h1decoff();                /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
          txphase(t4);
          delay(kappa - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          zgradpulse(icosel*gzlvl1, gt1);       /* 2.0*GRADIENT_DELAY */
          delay(1.0e-4);
        }
    else if (tau1 > (kappa - pwS3 - WFG_START_DELAY - 2.0*POWER_DELAY - 2.0e-6))
        {
          delay(timeTN + tau1 - kappa -PRG_STOP_DELAY -POWER_DELAY -PWRF_DELAY);
          h1decoff();                /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
          txphase(t4);                  /* WFG_START_DELAY  + 2.0*POWER_DELAY */
        c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);
          delay(kappa - pwS3 - WFG_START_DELAY - 2.0*POWER_DELAY - 1.0e-6 - gt1
                                                - 2.0*GRADIENT_DELAY - 1.0e-4);
          zgradpulse(icosel*gzlvl1, gt1);       /* 2.0*GRADIENT_DELAY */
          delay(1.0e-4);
        }
    else if (tau1 > gt1 + 2.0*GRADIENT_DELAY + 1.0e-4)
        {
          delay(timeTN + tau1 - kappa -PRG_STOP_DELAY -POWER_DELAY -PWRF_DELAY);
          h1decoff();                /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
          txphase(t4);
          delay(kappa - tau1 - pwS3 - WFG_START_DELAY - 2.0*POWER_DELAY
                                                                    - 2.0e-6);
        c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);
          delay(tau1 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          zgradpulse(icosel*gzlvl1, gt1);       /* 2.0*GRADIENT_DELAY */
          delay(1.0e-4);
        }
    else
        {
          delay(timeTN + tau1 - kappa -PRG_STOP_DELAY -POWER_DELAY -PWRF_DELAY);
          h1decoff();                /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
          txphase(t4);
          delay(kappa - tau1 - pwS3 - WFG_START_DELAY - 2.0*POWER_DELAY
                                 - 2.0e-6 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          zgradpulse(icosel*gzlvl1, gt1);       /* 2.0*GRADIENT_DELAY */
          delay(1.0e-4);
        c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);
          delay(tau1);
        }
/* xxxxxxxxx  SE TRAIN  xxxxxxxxx */
        sim3pulse(pw, 0.0, pwN, t4, zero, t10, 0.0, 0.0);

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

        delay(lambda - 0.65*(pw + pwN) - gt5);
              rgpulse(pw, zero, 0.0, 0.0);
              delay((gt1/10.0) + 1.0e-4 - 0.3*pw + 2.0*GRADIENT_DELAY
                                                        + POWER_DELAY*2.0); 
        rgpulse(2.0*pw, zero, 0.0, 0.0);

        dec2power(dpwr2);                                      /* POWER_DELAY */
        decpower(dpwr);                                /* POWER_DELAY */
        zgradpulse(gzlvl2, gt1/10.0);            /* 2.0*GRADIENT_DELAY */

        rcvron();
        statusdelay(C, 1.0e-4);
        setreceiver(t12);




}		 


