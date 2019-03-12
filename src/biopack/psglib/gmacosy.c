/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gmacosy.c      

   gradient based magic-angle dqcosy
	uses PSelements:
	vagradpulse(gzlvl(G/cm),gtime,theta,phi) as arguments
   (from  gemqfcosy_magradpulse_dlm.c, D.Mattiello, Varian )   

   First Use: destroy any current gcoil parameter in your parameter set.
              then enter "updtgcoil". this assumes you have a sysgcoil
              parameter and an entry in /vnmr/imaging/gradtables

   Phase Cycling:
	basic scheme is t1 t2 t3 t4 --> 0 0 1 0
	then add 180 to first pulse and receiver for x,-x cycle for improved
		water suppression and because the receiver is cycled
		x,-x it also corrects for dc imbalance --> 2 0 1 2
	then add 90 to every pulse in the nt=2 cycle for cyclops for 
		correction of quadrature imbalance 
	the States (0,90 on 1st pulse and receiver) and
	TPPI (FAD - f1 axial displacement) done within pulse sequence  

  number of dwell times by which signal is delayed is calculated and displayed
  in the text window for the first increment. This permits the accurate setting
  of backward linear prediction (lsfid is set to -n, where n is the number of
  delayed dwell times. This puts the points in the proper time registry and
  then LP can predict the proper number of points backward).

  modified for BioPack, GG Varian Palo Alto, jan 1999
*/

#include <standard.h>
#define MAGRADPULSE_DELAY 6.0*(GRADIENT_DELAY) 

static double   d2_init = 0.0;
static int   phi1[4]  = {0,2,1,3},
             phi2[4]  = {0,0,1,1},
             phi3[4]  = {1,1,2,2},
             phi4[4]  = {0,2,1,3};

void pulsesequence()
{
        double nsw,gzlvl1,gzlvl2,gzlvl2a,
	 gt1,gt2,qlvl,gstab,theta,phi,
	 deltatime;
        int t1_counter;
        char sspul[MAXSTR];
        getstr("sspul",sspul);

/*   LOAD PHASE TABLE    */

        settable(t1,4,phi1);
        settable(t2,4,phi2);
        settable(t3,4,phi3);
        settable(t4,4,phi4);
	getelem(t1,ct,v1);		/* 1st pulse phase */
	getelem(t4,ct,v4);		/* receiver phase */
	if (phase1 > 1.5) incr(v1);	/* hypercomplex phase shift */
	assign(v4,oph);

	if (ix == 1) d2_init = d2;      /* for States-TPPI */
   	t1_counter = (int) ( (d2 - d2_init)*sw1 + 0.5 );
        if (t1_counter % 2)
   	   {
        	add(v1, two, v1);      /* first pulse phase cycle */
        	add(v4, two, oph);      /* receiver phase cycle */
           }
        theta =getval("theta");
          phi =getval("phi");
        gzlvl1=getval("gzlvl1");
        gzlvl2=getval("gzlvl2");
	gt1=getval("gt1");
	gt2=getval("gt2");
        gstab = getval("gstab");
        qlvl=getval("qlvl");

	initval(7.0,v7);	/* 45 degree shift for first pulse */
	obsstepsize(45.0);

     status(A);
        if (sspul[A] == 'y')    /* saturates all spins for same steady-state*/
        {
         vagradpulse(gzlvl2,0.35*gt2,theta,phi);
         rgpulse(pw,zero,rof1,rof1);
         vagradpulse(gzlvl2,0.77*gt2,theta,phi);
        }
	xmtrphase(v7);
	if (satmode[A] == 'y')
        {
          obspower(satpwr);
          rgpulse(satdly,v5,rof1,rof1);
          obspower(tpwr);
        }
	xmtrphase(v7);
	if (satmode[A] == 'n') delay(d1);
	rgpulse(pw,v1,rof1,0.0);         
	xmtrphase(zero);
	txphase(t3);	
        vagradpulse(gzlvl1,gt1,theta,phi);
      status(B);
        delay(gstab);
        delay(d2);
        rgpulse(2*pw,t3,2.0e-6,0.0);          
      status(A);
        vagradpulse(gzlvl1,gt1,theta,phi);
        delay(gstab);
	rgpulse(pw,t2,rof1,rof1);         
        delay(gt2+MAGRADPULSE_DELAY + gstab);
        rgpulse(2*pw,t3,rof1,rof1);          
        vagradpulse(gzlvl2,gt2,theta,phi);
        delay(gstab);
	rgpulse(pw,t2,rof1,rof1);       
	gzlvl2a=gzlvl2*qlvl;
        vagradpulse(gzlvl2a,gt2,theta,phi);
	deltatime = gt2 + MAGRADPULSE_DELAY;
	nsw = (int)(deltatime*sw) + 1;
        if (ix == 1) printf("\n No of dwell times delayed = %6.0f",nsw);
	deltatime=((double)(nsw)/sw) - deltatime;
        delay(deltatime);
	status(C);

}
