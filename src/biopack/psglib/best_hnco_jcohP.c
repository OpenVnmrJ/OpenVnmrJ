/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*   BEST-HNCO 1H-15N-13CO experiment       

 Measurement of coupling constants J(CO-HN)
 in peptide planes                         
 using DIPSAP: ab_flg='a','b','c'        
  combine the spectra as 0.73(c) + 0.27(a) + 1.00(b)
 using IPAP:   ab_flg='a','b'          

 Measure CO-HN couplings along F1 (C13 axis)

 See R.M.Rasia, E. Lescop, J.F. Palatnik,
 J Boisbouvier and B. Brutscher, J. Biomol. NMR,
 51, 369-378(2011)
 
 Coupling patterns are clearly resolved by 
 running a 3D experiment and using the N15
 dimension to separate the C13 planes.


This experiment Correlates C'(i) with  N(i+1), NH(i+1).

set f2coef=''  for Vnmr/VnmrJ processing

 BEST experiments are based on the longitudinal relaxation
 enhancement provided by minimal perturbation of aliphatic proton
 polarization. All HN pulses are band-selective EBURP2/time
 -reversed EBURP2/PC9/time-reversed PC9/REBURP. The recycle delay
 can be adjusted for optimal pulsing regime (d1~0.4-0.5s) or for
 fast pulsing regime (d1~0.1-0.4s). For fast pulsing regime,
 care should be taken relative to the probe used: low power 15N
 decoupling (GARP/WURST) and short acquisition times should be
 used. 

 The coherence pathway is conserved with standard hard pulse-based
 experiments. Standard features include maintaining the 13C
 carrier in the CO region throughout using off-res SLP pulses;
 square pulses on Ca/Cb with first null at 13CO; one lobe sinc
 pulses on 13CO with first null at Ca. Carbon carrier frequency
 dof should be set to the center of carbonyl frequency. Uses
 constant time evolution for the 15N shifts and real time
 evolution for 13C.

 No 1H decoupling sequence is applied during N->CO/CA transfer.
 180° BIP pulses (shname1="BIP_720_50_20_360", shpw1=8*pw at
 tpwr) are used to refocus NyHz coherence to Nx.

 The flags f1180/f2180 should be set to 'y' if t1/t2 is to be
 started at half dwell time. This will give 90, -180 phasing in
 f1/f2. If they are set to 'n' the phasing should be 0,0 and will
 still give a flat baseline.

 phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition
 in t1 [C13]  and States-TPPI acquisition/ EchoAntiecho in t2
 [N15].

 Original best_hncoP.c sequence contributed by 
		* Schanda, Paul
		* Lescop, Ewen
		* Van Melckebeke, Hélène
		* Brutscher, Bernhard

Institut de Biologie Structurale, Laboratoire de RMN, 41, 
rue Jules Horowitz, 38027 Grenoble Cedex 1 FRANCE

see: - Schanda, P., Van Melckebeke, H. and Brutscher, B.,
       JACS,128,9042-9043(2006)
*/

#include <standard.h>
#include "bionmr.h"
  
static int  
             phx[1]={0},   phy[1]={1},

	     phi3[2]  = {0,2},
	     phi5[1]  = {0},
             phi9[1]  = {0},
             rec[2]   = {0,2};



void pulsesequence()
{

/* DECLARE AND LOAD VARIABLES; parameters used in the last half of the */
/* sequence are declared and initialized as 0.0 in bionmr.h, and       */
/* reinitialized below  */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
            shname1[MAXSTR],
            ab_flg[MAXSTR];             /* Flag for sinus and cosinus mod in t1 */
 
int         t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  	 	        /* used for states tppi in t2 */
	    ni2 = getval("ni2");

double      d2_init=0.0,  		        /* used for states tppi in t1 */
	    d3_init=0.0,  	 	        /* used for states tppi in t2 */
	    tau1,         				         /*  t1 delay */
            shpw1,
            shlvl1 = getval("shlvl1"),
            timeTN = getval("timeTN"),     /* constant time for 15N evolution */
            taunh = getval("taunh"),     /* constant time for 15N evolution */
            
	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */

   pwS1,pwS2,pwS3,pwS4,pwS5,pwS6,					   /* length of sinc 90 on CO */
   phshift,        /*  phase shift induced on CO by 180 on Ca in middle of t1 */
   pwS = getval("pwS"), /* used to change 180 on Ca in t1 for 1D calibrations */
   pwZ,					   /* the largest of pwS2 and 2.0*pwN */
   pwZ1,	        /* the largest of pwS2 and 2.0*pwN for 1D experiments */


	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),

	gt0 = getval("gt0"),				   /* other gradients */
	gt2 = getval("gt2"),				   /* other gradients */
	gt5 = getval("gt5"),				   /* other gradients */
	gt3 = getval("gt3"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl2 = getval("gzlvl2"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl3 = getval("gzlvl3"),
          shbw = getval("shbw"),
          shofs = getval("shofs")-4.77;


    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("shname1",shname1);
    getstr("ab_flg", ab_flg);


/*   LOAD PHASE TABLE    */

	settable(t3,2,phi3);
	settable(t4,1,phx);
	settable(t5,1,phi5);
        settable(t8,1,phx);
	settable(t9,1,phi9);
	settable(t10,1,phx);
	settable(t11,1,phy);
	settable(t12,2,rec);


/*   INITIALIZE VARIABLES   */
	
        shpw1= pw*8.0;
        kappa = 5.4e-3;
	lambda = 2.4e-3;


    /* get calculated pulse lengths of shaped C13 pulses */
	pwS1 = c13pulsepw("co", "ca", "sinc", 90.0); 
	pwS2 = c13pulsepw("ca", "co", "square", 180.0); 
	pwS3 = c13pulsepw("co", "ca", "sinc", 180.0); 

   pwS4 = h_shapedpw("eburp2",shbw,shofs,zero, 0.0, 0.0);
   pwS6 = h_shapedpw("reburp",shbw,shofs,zero, 0.0, 0.0);
   pwS5 = h_shapedpw("pc9f",shbw,shofs,zero, 2.0e-6, 0.0);


    /* the 180 pulse on Ca at the middle of t1 */
	if ((ni2 > 0.0) && (ni == 1.0)) ni = 0.0;
        if (pwS2 > 2.0*pwN) pwZ = pwS2; else pwZ = 2.0*pwN;
        if ((pwS==0.0) && (pwS2>2.0*pwN)) pwZ1=pwS2-2.0*pwN; else pwZ1=0.0;
	if ( ni > 1 )     pwS = 180.0;
	if ( pwS > 0 )   phshift = 48.0;
	else             phshift = 0.0;



/* CHECK VALIDITY OF PARAMETER RANGES */

    if ( 0.5*ni2*1/(sw2) > timeTN -gt2-1.0e-4)
       { printf(" ni2 is too big. Make ni2 equal to %d or less.\n", 
  	 ((int)((timeTN - gt2-1.0e-4)*2.0*sw2))); 	     psg_abort(1);}



    if ( dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' )
       { printf("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1);}

    if ( dm2[A] == 'y' || dm2[B] == 'y' )
       { printf("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1);}

    if ( dpwr2 > 46 )
       { printf("dpwr2 too large! recheck value  ");		     psg_abort(1);}

    if ( pw > 20.0e-6 )
       { printf(" pw too long ! recheck value ");	             psg_abort(1);} 
  
    if ( (pwN > 100.0e-6) && ((ni>1) || (ni2>1)) )
       { printf(" pwN too long! recheck value ");	             psg_abort(1);} 
 

/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2)   tsadd(t3,1,4);  
    if (phase2 == 2)   tsadd(t9,1,4);  


/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1;


/*  Set up f2180  */

    tau2 = d3;
    if((f2180[A] == 'y') && (ni2 > 1.0)) 
	{ tau2 += ( 1.0 / (2.0*sw2) ); if(tau2 < 0.2e-6) tau2 = 0.0; }
    tau2 = tau2;


/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t3,2,4); tsadd(t12,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) 
	{ tsadd(t9,2,4); tsadd(t12,2,4); }



/* BEGIN PULSE SEQUENCE */

status(A);
   	delay(d1);

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


/*****************************************************************************************/
        h_shapedpulse("pc9f",shbw,shofs,zero, 2.0e-6, 0.0);

        delay(lambda-pwS5*0.5-pwS6*0.4);

        h_sim3shapedpulse("reburp",shbw,shofs,0.0,2.0*pwN, one, zero, zero, 0.0, 0.0);

        delay(lambda-pwS5*0.5-pwS6*0.4);

        h_shapedpulse("pc9f_",shbw,shofs,one, 0.0, 0.0);


        obspower(shlvl1);

/**************************************************************************/

if (ab_flg[0] == 'a')
{
      dec2rgpulse(pwN,zero,0.0,0.0);

    delay(timeTN-taunh*0.5-gt0-2.0e-4);
        zgradpulse(gzlvl0, gt0);
        delay(2.0e-4);
    shaped_pulse(shname1,shpw1,zero,0.0,0.0);
    delay(taunh*0.5);
        sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
                                             zero, zero, zero, 2.0e-6, 2.0e-6);
    delay(taunh*0.5);
    shaped_pulse(shname1,shpw1,two,0.0,0.0);
        zgradpulse(gzlvl0, gt0);
        delay(timeTN-taunh*0.5-gt0);

        dec2rgpulse(pwN, one, 0.0, 0.0);

}

if (ab_flg[0] == 'b')
{
      dec2rgpulse(pwN,zero,0.0,0.0);

    delay(timeTN-taunh*0.25-gt0-2.0e-4);
        zgradpulse(gzlvl0, gt0);
        delay(2.0e-4);
    shaped_pulse(shname1,shpw1,zero,0.0,0.0);
    delay(taunh*0.25);
        sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
                                             zero, zero, zero, 2.0e-6, 2.0e-6);
    delay(taunh*0.75);
    shaped_pulse(shname1,shpw1,two,0.0,0.0);
        zgradpulse(gzlvl0, gt0);
        delay(timeTN-taunh*0.75-gt0);

        dec2rgpulse(pwN, zero, 0.0, 0.0);

}

if (ab_flg[0] == 'c')
{
      dec2rgpulse(pwN,zero,0.0,0.0);

    delay(timeTN-gt0-2.0e-4);
        zgradpulse(gzlvl0, gt0);
        delay(2.0e-4);
    shaped_pulse(shname1,shpw1,zero,0.0,0.0);
        sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
                                             zero, zero, zero, 2.0e-6, 2.0e-6);
    delay(taunh);
    shaped_pulse(shname1,shpw1,two,0.0,0.0);
        zgradpulse(gzlvl0, gt0);
        delay(timeTN-taunh-gt0);

        dec2rgpulse(pwN, one, 0.0, 0.0);
}
        zgradpulse(gzlvl2, gt2*2.0);
        delay(2.0e-4);


	zgradpulse(-gzlvl3, gt3);
 	delay(2.0e-4);

/*   xxxxxxxxxxxxxxxxxxxxxx       13CO EVOLUTION     xxxxxxxxxxxxxxxxx    */
        c13pulse("co", "ca", "sinc", 90.0, t3, 2.0e-6, 0.0);
        delay(tau1*0.5);
        sim3_c13pulse("", "ca", "co", "square", "", 0.0, 180.0, pwN*2.0,
                                                  one, zero, zero, 2.0e-6, 0.0);
        delay(tau1*0.5);
        c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);
        sim3_c13pulse("", "ca", "co", "square", "", 0.0, 180.0, 0.0,
                                                  one, zero, zero, 2.0e-6, 0.0);
        if (pwN*2.0 > pwS3) delay(pwN*2.0-pwS3);
        c13pulse("co", "ca", "sinc", 90.0, t5, 0.0, 0.0);


/*  xxxxxxxxxxxxxxxxxxxx  N15 EVOLUTION    xxxxxxxxxxxxxxxxxxxxxxx  */	
        zgradpulse(gzlvl2, gt2);
        delay(2.0e-4);
        obspower(shlvl1);

     dec2rgpulse(pwN,t9,0.0,0.0);
           delay(tau2*0.5);
        sim_c13pulse(shname1,"ca", "co", "square",shpw1, 180.0, two,zero, 0.0, 0.0);
	zgradpulse(gzlvl2, gt2);
        delay(timeTN-gt2);

      sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
                                             zero, zero, zero, 2.0e-6, 2.0e-6);
	zgradpulse(gzlvl2, gt2);
        delay(1.0e-4);
       delay(timeTN-gt2-1.0e-4-tau2*0.5);
       shaped_pulse(shname1,shpw1,zero,0.0,0.0);
       dec2rgpulse(pwN, one, 0.0, 0.0);
	zgradpulse(gzlvl3, gt3*1.4);
 	delay(2.0e-4);

/*****************************************************************************************/
        h_shapedpulse("eburp2",shbw,shofs,zero, 2.0e-6, 0.0);
        zgradpulse(gzlvl5, gt5);
        delay(lambda-pwS6*0.4  - gt5);

        h_sim3shapedpulse("reburp",shbw,shofs,0.0,2.0*pwN, zero, zero, zero, 0.0, 0.0);

        zgradpulse(gzlvl5, gt5);
        delay(lambda-pwS6*0.4  - gt5-POWER_DELAY-1.0e-4);
        dec2power(dpwr2);         

        setreceiver(t12);
        rcvron();
statusdelay(C,1.0e-4 );

}
