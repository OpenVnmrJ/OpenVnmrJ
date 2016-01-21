/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 11noesy

   1-1 echo NOESY (coded by Marco Tonelli @ NMRFAM)
   Sklenar and Bax (1987) JMR 74 pg. 469.

   maxHz : maximum of excitation profile in Hz (from tof) 
	   it is used to calculate echo delay: echo = 1.0/(maxHz*4.0)-(pw*0.667)

   gzlvl1 : gradients to control water magnetization during t1 evolution (use low value)
   gstab  : recovery delay after gradients

   tof    : frequency for the H1 direct dimension (MUST be set to H2O).
   tof1   : frequency for the H1 indirect dimension (if different from H2O). 
	    For RNA samples it may be convenient to set the offset for the 1H indirect 
	    dimension in between the imino and aromatic peaks and then reduce the indirect 
	    spectral window to ~13ppm (vs. ~24ppm for the direct dimension).
	    If tof1 parameter does not exists create it in vnmr with the command:
		create('tof1','frequency')
	    then set it to the desired value. 
	    If no frequency shift is needed, do not create this parameter (if it doesn't
	    exist) or if it does exist set it to the same value as tof.

            *****************************************************************************
	    PLEASE NOTE: if tof1 exists and is different from tof, the indirect dimension 
			 frequency will be shifted to the tof1 value
			 if you do NOT want this to happen, either destroy the parameter 
			 tof1 in vnmr using the command:
					destroy("tof1")
                         or set tof1=tof (and remember to do this if you change tof).
            *****************************************************************************
	    Added October 2005. Marco@NMRFAM

*/
#include <standard.h>


static int   phi1[32]  = {0,2,0,2, 1,3,1,3, 2,0,2,0, 3,1,3,1, 
			  2,0,2,0, 3,1,3,1, 0,2,0,2, 1,3,1,3},
             phi2[32]  = {0,0,2,2, 1,1,3,3, 2,2,0,0, 3,3,1,1,
	                  2,2,0,0, 3,3,1,1, 0,0,2,2, 1,1,3,3},
             phi3[16]  = {0,0,0,0, 1,1,1,1, 2,2,2,2, 3,3,3,3},
	     phi6[16]  = {2,2,2,2, 3,3,3,3, 0,0,0,0, 1,1,1,1}, 
             phi7[16]  = {0,0,0,0, 1,1,1,1, 2,2,2,2, 3,3,3,3},
	     phi8[16]  = {2,2,2,2, 3,3,3,3, 0,0,0,0, 1,1,1,1}, 
             rec[16]   = {0,2,2,0, 1,3,3,1, 2,0,0,2, 3,1,1,3};

static double   d2_init=0.0;

pulsesequence()
{
   char            f1180[MAXSTR];			/* Flag to start t1 @ halfdwell */

   double          tof1,			/* frequency offset of H1 indirect dimension */
   		   tau1,
		   corr,corr1,
                   gzlvl0 = getval("gzlvl0"),
                   gzlvl1 = getval("gzlvl1"),
                   gzlvl2 = getval("gzlvl2"),
                   gzlvl3 = getval("gzlvl3"),
                   gt0 = getval("gt0"),
                   gt2 = getval("gt2"),
                   gt3 = getval("gt3"),
                   gstab = getval("gstab"),
                   mix = getval("mix"),		/* mixing time */
                   echo,			/* delay for excitation maximum, calculated based on maxHz */
  		   maxHz = getval("maxHz");	/* maxHz = offset of first max in Hz */


   int             t1_counter;			/* used for states tppi in t1 */


   getstr("f1180",f1180);


/*   LOAD PHASE TABLE    */

   settable(t1,32,phi1);
   settable(t2,32,phi2);
   settable(t3,16,phi3);
   settable(t6,16,phi6);
   settable(t7,16,phi7);
   settable(t8,16,phi8);
   settable(t11,16,rec);


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2) tsadd(t1,1,4);  


/*  Set up f1180  */
   
   tau1 = d2;
   if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }


/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t1,2,4); tsadd(t11,2,4); }

/* calculate echo delay in 1-1 echo sequence from first maximum excitation at maxHz */

   echo = 1.0/(maxHz*4.0)-(pw*0.667);


/* if exists read tof1 */
   if (find("tof1") > 0.0) tof1=getval("tof1");
     else tof1=tof;


/* BEGIN THE ACTUAL PULSE SEQUENCE
      obsstepsize(45.0);
      initval(1.0,v1);
      xmtrphase(v1);
*/

 status(A);
      zgradpulse(gzlvl0,gt0);
      if (tof1 != tof) obsoffset(tof1);
      delay(d1);

 status(B);
      rgpulse(pw, t1, rof1, rof1);

      corr = 2.0*rof1 +2.0*GRADIENT_DELAY +4.0*pw/PI;
      corr1 = 2.0*rof1 +4.0*pw/PI;

      if (tau1>0.0007)
      {
       if (gzlvl1>0.0)			/* set gzlvl1=0.0 for no PFG during evolution */
       {
         if (0.2*(tau1-corr) > gstab)	/* make sure enough time is allowed for recovery after gradient */
          {				/* this works !! */
           zgradpulse(gzlvl1,0.4*(tau1-corr) -0.5*gstab);
           delay(0.2*(tau1-corr));
           zgradpulse(-1.0*gzlvl1,0.4*(tau1-corr) -0.5*gstab);
           delay(gstab);
          }
         else delay(tau1-corr1);
       }
       else if ((tau1-corr1)>0.0) delay(tau1-corr1);
      }
      else if ((tau1-corr1)>0.0) delay(tau1-corr1);

/*
      xmtrphase(zero);
*/
      rgpulse(pw, t2, rof1, 0.0);

 status(C);
/* mixing time */
      if (tof1 != tof) obsoffset(tof);
      delay(mix -gt2 -gstab);
      zgradpulse(gzlvl2,gt2);
      delay(gstab);

/* now start the 1-1 echo sequence */
      rgpulse(pw, t3,0.0,0.0);
      delay(echo);
      rgpulse(pw, t6,0.0,0.0);

      delay(2.0e-6);
      zgradpulse(gzlvl3,gt3);
      delay(gstab);

      rgpulse(pw, t7,0.0,0.0);
      delay(2.0*echo);
      rgpulse(pw, t8,0.0,0.0);

      delay(2.0e-6);
      zgradpulse(gzlvl3,gt3);
      delay(gstab);
status(D);
	setreceiver(t11);

}

