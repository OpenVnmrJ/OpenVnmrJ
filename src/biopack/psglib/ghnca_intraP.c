/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghnca_intraP.c

    3D iHNCA gradient sensitivity enhanced version.

This pulse sequence records an intraresidue 1H(i)-15N(i)-13Ca(i)
correlation spectrum (non-TROSY or TROSY).

Set 13C carrier (dof) at 174ppm.
Set dm2='nnn' dm='nnn'

Set jnca-20-24, jnco=15, jhn=95-105 and jcoca=53-65.
Note that by setting jnca=20, the sequential connectivities
 can be completely suppressed.

Shapes are automatically created by Pbox. All pulse powers and
pulse widths on 13C are automatically calculated.

Sequence by Perttu Permi, University of Helsinki
   (submitted as i2hnca_trosy_ns_pp.c)
Modified for BioPack by G.Gray, Varian October 2004

Sequence revised by Perttu Permi Sept 2006:
   (submitted as gi2hn_caP_pp.c)

 Bloch-Siegert compensation is accomplished in a manner similar to
 other standard Biopack sequences i.e. using a single 180(13CO) pulse
 and adding a small angle phase shift to 90 degree 13Ca pulse.

 There are two major differences between ihn_evol and hn_evol in bionmr.h:

 i)  the 15N shift is decremented --> f2coef='1 0 -1 0 0 1 0 1'
 ii) 90 degree 13CO and 13Ca pulses are applied in addition to 13C
     180 pulses during back-transfer, whereas in hn_evol only 180 pulses
     are applied.

Ref: P. Permi, J.Biomol.NMR 23, 201-209 (2002). (non-TROSY version)
     H. Tossavainen and P. Permi, J. Magn. Reson. 170, 244-251 (2004).
  
The revised version of the sequence has not been published.
*/


#include <standard.h>
#include "bionmr.h"
  
static int   /*  T is for TROSY='y', phx etc also enable TROSY phase changes */
             phx[1]={0},   phy[1]={1},

	     phi3[2]  = {0,2},
	     phi5[4]  = {0,0,2,2},
             phi9[8]  = {0,0,0,0,2,2,2,2},
             rec[4]   = {0,2,2,0},		     recT[4]  = {3,1,1,3};



pulsesequence()
{

/* DECLARE AND LOAD VARIABLES; parameters used in the last half of the */
/* sequence are declared and initialized as 0.0 in bionmr.h, and       */
/* reinitialized below  */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
            COrefoc[MAXSTR],
 	    TROSY[MAXSTR];			    /* do TROSY on N15 and H1 */
 
int         t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  	 	        /* used for states tppi in t2 */
	    ni2 = getval("ni2");

double      d2_init=0.0,  		        /* used for states tppi in t1 */
	    d3_init=0.0,  	 	        /* used for states tppi in t2 */
	    tau1,         				         /*  t1 delay */
            timeTN = getval("timeTN"),     /* constant time for 15N evolution */
            timeNCA = getval("timeNCA"),
            timeC = getval("timeC"),
            lambda = 1.0/(4.0*getval("JNH")),
            
	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */

   pwS1,					 /* length of square 90 on Ca */
   phshift,        /*  phase shift induced on Ca by 180 on CO in middle of t1 */
   pwS2,					       /* length of 180 on CO */
   pwS = getval("pwS"), /* used to change 180 on CO in t1 for 1D calibrations */
   pwZ,					   /* the largest of pwS2 and 2.0*pwN */
   pwZ1,	        /* the largest of pwS2 and 2.0*pwN for 1D experiments */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),

	gt0 = getval("gt0"),				   /* other gradients */
	gt3 = getval("gt3"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3");

    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("COrefoc",COrefoc);
    getstr("TROSY",TROSY);


/*   LOAD PHASE TABLE    */

	settable(t3,2,phi3);
	settable(t4,1,phx);
	settable(t5,4,phi5);
   if (TROSY[A]=='y')
       {settable(t8,1,phy);
	settable(t9,1,phx);
 	settable(t10,1,phy);
	settable(t11,1,phx);
	settable(t12,4,recT);}
    else
       {settable(t8,1,phx);
	settable(t9,8,phi9);
	settable(t10,1,phx);
	settable(t11,1,phy);
	settable(t12,4,rec);}




/*   INITIALIZE VARIABLES   */

 	kappa = 5.4e-3;

   pwHs = 1.7e-3*500.0/sfrq;       /* length of H2O flipback, 1.7ms at 500 MHz*/
   widthHd = 34.0;  /* bandwidth of H1 WALTZ16 decoupling, 7.3 kHz at 600 MHz */
   pwHd = h1dec90pw("WALTZ16", widthHd, 0.0);     /* H1 90 length for WALTZ16 */
 
    /* get calculated pulse lengths of shaped C13 pulses */
        pwS1 = c13pulsepw("co", "ca", "sinc", 90.0); 
        pwS2 = c13pulsepw("ca", "co", "square", 180.0);

    /* get calculated pulse lengths of shaped C13 pulses
	pwS1 = c13pulsepw("ca", "co", "square", 90.0); 
	pwS2 = c13pulsepw("co", "ca", "sinc", 180.0); */

    /* the 180 pulse on CO at the middle of t1 */
	if ((ni2 > 0.0) && (ni == 1.0)) ni = 0.0;
        if (pwS2 > 2.0*pwN) pwZ = pwS2; else pwZ = 2.0*pwN;
        if ((pwS==0.0) && (pwS2>2.0*pwN)) pwZ1=pwS2-2.0*pwN; else pwZ1=0.0;
	if ( ni > 1 )     pwS = 180.0;
	if ( pwS > 0 )   phshift = 130.0;
	else             phshift = 130.0;



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
 
    if ( TROSY[A]=='y' && dm2[C] == 'y')
       { text_error("Choose either TROSY='n' or dm2='n' ! ");        psg_abort(1);}



/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2)   tsadd(t3,1,4);  
    if (TROSY[A]=='y')
	 {  if (phase2 == 2)   				      icosel = +1;
            else 	    {tsadd(t4,2,4);  tsadd(t10,2,4);  icosel = -1;}
	 }
    else {  if (phase2 == 2)  {tsadd(t10,2,4); icosel = +1;}
            else 			       icosel = -1;    
	 }


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



/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t3,2,4); tsadd(t12,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) 
	{ tsadd(t8,2,4); tsadd(t12,2,4); }



/* BEGIN PULSE SEQUENCE */

status(A);
   	delay(d1);
 	if (dm3[B]=='y') lk_hold();

	rcvroff();
        set_c13offset("co");
	obsoffset(tof);
	obspower(tpwr);
 	obspwrf(4095.0);
	decpower(pwClvl);
	decpwrf(4095.0);
 	dec2power(pwNlvl);
	txphase(zero);
   	delay(1.0e-5);

	dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 and C13 magnetization*/
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(gzlvl0, 0.5e-3);
	delay(1.0e-4);
	dec2rgpulse(pwN, one, 0.0, 0.0);
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(0.7*gzlvl0, 0.5e-3);
	delay(5.0e-4);

   	rgpulse(pw,zero,0.0,0.0);                      /* 1H pulse excitation */

   	dec2phase(zero);
	zgradpulse(gzlvl0, gt0);
	delay(lambda - gt0);

   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

   	txphase(one);
	zgradpulse(gzlvl0, gt0);
	delay(lambda - gt0);

 	rgpulse(pw, one, 0.0, 0.0);

if (TROSY[A]=='y')
   {txphase(two);
    shiftedpulse("sinc", pwHs, 90.0, 0.0, two, 2.0e-6, 0.0);
    zgradpulse(gzlvl3, gt3);
    delay(2.0e-4);
    dec2rgpulse(pwN, zero, 0.0, 0.0);

    delay(0.5*kappa - 2.0*pw);

    rgpulse(2.0*pw, two, 0.0, 0.0);

    decphase(zero);
    dec2phase(zero);
    delay(timeTN - 0.5*kappa - WFG3_START_DELAY);
   }
else
   {txphase(zero);
    shiftedpulse("sinc", pwHs, 90.0, 0.0, zero, 2.0e-6, 0.0);
    zgradpulse(gzlvl3, gt3);
    delay(2.0e-4);
    dec2rgpulse(pwN, zero, 0.0, 0.0);

    delay(kappa - POWER_DELAY - PWRF_DELAY - pwHd - 4.0e-6 - PRG_START_DELAY);
					   /* delays for h1waltzon subtracted */
    h1waltzon("WALTZ16", widthHd, 0.0);
    decphase(zero);
    dec2phase(zero);
    delay(timeTN - kappa - WFG3_START_DELAY);
   }

        c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);      /*  pwS2  */

        delay(timeNCA - timeTN - timeC);

        dec2rgpulse(2.0*pwN,zero,0.0,0.0);

	c13pulse("ca", "co", "sinc", 180.0, zero, 0.0, 0.0);
	decphase(zero);
	delay(timeNCA - timeC + 1.3*pwN);

        c13pulse("co", "ca", "sinc", 90.0, zero, 0.0, 0.0);      /*  pwS1  */
        delay(timeC);

        c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 2.0e-6);
        sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0); /*  pwS2  */
       
        delay(timeC);

        c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 2.0e-6);
        c13pulse("co", "ca", "sinc", 90.0, one, 0.0, 0.0);      /*  pwS1  */        
	dec2rgpulse(pwN, zero, 0.0, 0.0);

	if (TROSY[A]=='n')   h1waltzoff("WALTZ16", widthHd, 0.0);
	zgradpulse(gzlvl3, gt3);
 	delay(2.0e-4);
        if(dm3[B] == 'y')			  /*optional 2H decoupling on */
         {dec3unblank(); dec3rgpulse(1/dmf3, one, 0.0, 0.0); 
          dec3unblank(); setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);} 
    	h1waltzon("WALTZ16", widthHd, 0.0);

/*   xxxxxxxxxxxxxxxxxxxxxx       13Ca EVOLUTION        xxxxxxxxxxxxxxxxxx    */

        set_c13offset("ca");
	c13pulse("ca", "co", "square", 90.0, t3, 2.0e-6, 0.0);      /*  pwS1  */
	decphase(zero);

if ((ni>1.0) && (tau1>0.0))          /* total 13C evolution equals d2 exactly */
   {           

   /*  2.0*pwS1/PI compensates for evolution at 64% rate during 90 */
     if (tau1 - 2.0*pwS1/PI - WFG3_START_DELAY - 0.5*pwZ - 2.0e-6
			 	- 2.0*PWRF_DELAY - 2.0*POWER_DELAY > 0.0)
	{
	delay(tau1 - 2.0*pwS1/PI - WFG3_START_DELAY - 0.5*pwZ - 2.0e-6 - 2.0*PWRF_DELAY - 2.0*POWER_DELAY);
							 
	sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,zero, zero, zero, 2.0e-6, 0.0);
	initval(phshift, v3);
	decstepsize(1.0);
	dcplrphase(v3);  				         
	delay(tau1 - 2.0*pwS1/PI  - SAPS_DELAY - 0.5*pwZ - WFG_START_DELAY - 2.0e-6 - 2.0*PWRF_DELAY - 2.0*POWER_DELAY);
         }
      else
	 {
	initval(180.0, v3);
	decstepsize(1.0);
	dcplrphase(v3);  				        
	delay(2.0*tau1 - 4.0*pwS1/PI - SAPS_DELAY - WFG_START_DELAY - 2.0e-6 - PWRF_DELAY - POWER_DELAY);
	  } 

     /*  delay(tau1);
       sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,zero, zero, zero, 2.0e-6, 0.0);
       delay(tau1);
       c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0);
       sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,zero, zero, zero, 2.0e-6, 0.0);*/ 
   }

else if (ni==1.0) 
   {
        delay(10.0e-6 + SAPS_DELAY + 0.5*pwZ1 + WFG_START_DELAY);
	sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, pwS, 2.0*pwN, zero, zero, zero, 2.0e-6, 0.0);
	initval(phshift, v3);
	decstepsize(1.0);
	dcplrphase(v3); 
	delay(10.0e-6 + WFG3_START_DELAY + 0.5*pwZ1);
   }

else	 	  
   {
        delay(10.0e-6);					  	
        c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0);
	delay(10.0e-6); 

     /*  delay(tau1);
       sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,zero, zero, zero, 2.0e-6, 0.0);
       delay(tau1);
       c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0);
       sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,zero, zero, zero, 2.0e-6, 0.0); */

   }
        decphase(t5);
	c13pulse("ca", "co", "square", 90.0, t5, 2.0e-6, 0.0);      /*  pwS1  */

	h1waltzoff("WALTZ16", widthHd, 0.0);
        if(dm3[B] == 'y')		         /*optional 2H decoupling off */
         {dec3rgpulse(1/dmf3, three, 0.0, 0.0); dec3blank();
          setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3); dec3blank();}

        set_c13offset("co");
/*  xxxxxxxxxxxxxxxxxxxx  N15 EVOLUTION & SE TRAIN   xxxxxxxxxxxxxxxxxxxxxxx  */	
	ihn_evol_se_train("co", "ca"); /* common part of sequence in bionmr.h  */

if (dm3[B] == 'y')  lk_sample();

}

