// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/* ghmqcps - a phase-sensitive PFG HMQC

        phase - use phase=1,2 to select N,P-type selection to be sorted later
                (use phase=1 to generate an absolute value dataset)


  PROCESSING:

        process phase=1 (N-type) data with wft2d(1,0,0,1)
        process phase=2 (P-type) data with wft2d(1,0,0,-1)
                   the ('t2dc') argument to wft2d may be useful

        process phase sensitive data (phase = 1,2) with:
                wft2d(1,0,0,1,0,1,1,0) (wft2dnp)

*/

#include <standard.h>

static double d2_init = 0.0;

pulsesequence()
{
double	gzlvl1,   gzlvl2,   gzlvl3;
double	gt1,      gt2,      gt3;    

double	grise, gstab, j, phase, pwx, pwxlvl, sw1, taumb;
double	tau;
int	iphase, icosel, t1_counter;
char	mbond[MAXSTR], sspul[MAXSTR];

/* GATHER AND INITIALIZE VARIABLES */
   gzlvl1 = getval("gzlvl1");
   gt1    = getval("gt1");
   gzlvl2 = getval("gzlvl2");
   gt2    = getval("gt2");
   gzlvl3 = getval("gzlvl3");
   gt3    = getval("gt3");

   grise  = getval("grise");
   gstab  = getval("gstab");
   j      = getval("j");
   phase  = getval("phase");
   pwx    = getval("pwx");
   pwxlvl = getval("pwxlvl");
   sw1    = getval("sw1");
   taumb  = getval("taumb");

   getstr("sspul",  sspul);
   getstr("mbond",  mbond);


   iphase = (int) (phase + 0.5); 


/* Check Conditions and Parameters */

   tau = 1.0 / (2.0 * j);
   if (tau < (gt3+grise)) 
   {  text_error("tau must be greater than gt3+grise\n");
      psg_abort(1);
   }

      
   if (fabs(gt1-gt2) >= 1.0e-8)
   {  text_error("gt1 and gt2 must be equal for correct d2 timming !\n");
      psg_abort(1);
   }
    
   if ( (gt1 > 0.01) || (gt2 > 0.01) || (gt3 > 0.01) )
   {  text_error("gt1, gt2  or gt3  durations longer than 10 ms !\n");
      psg_abort(1);
   }

/* Check for gradient level settings */

   if (ix == 1.0)
   {  if ( ((fabs(gzlvl1)) > 2047) || ((fabs(gzlvl2)) > 2047) || ((fabs(gzlvl3)) > 2047) )
      {  printf("Gradient level values are too large !\n");
      }
   }


/* Gradient incrementation for hypercomplex data */

   if (iphase == 2)     /* Hypercomplex in t1 */
   {  icosel = -1;     /* change sign of gradient */
   }
   else 
   {  icosel = 1;
   }


/* calculate and modify the phases based on current t1 values	*/
/* to achieve States-TPPI acquisition				*/
/*			   ct = 0 1 2 3 4 5 6 7			*/
   hlv(ct,v1);		/* v1 = 0 0 1 1 2 2 3 3	*/
   mod2(v1,v1);		/* v1 = 0 0 1 1 0 0 1 1	*/
   dbl(v1,v1);		/* v1 = 0 0 2 2 0 0 2 2	*/
   mod2(ct,v2);		/* v2 = 0 1 0 1 0 1 0 1 */
   dbl(v2,v2);		/* v2 = 0 2 0 2 0 2 0 2 */
   add(ct,one,v3);	/* v3 = 1 2 3 4 5 6 7 8 */
   hlv(v3,v3);		/* v3 = 0 1 1 2 2 3 3 4 */
   mod2(v3,v3);		/* v3 = 0 1 1 0 0 1 1 0 */
   dbl(v3,v3);		/* v3 = 0 2 2 0 0 2 2 0 */
   
   if (ix == 1)
      d2_init = d2;
 
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);
       
   if (t1_counter % 2)  
   {  add(v2,two,v2);
      add(v3,two,v3);
   }
 


/* ACTUAL PULSE SEQUENCE */

   status(A);
      rlpower(pwxlvl,DODEV);

/* purge initial magnetization */
   
      if (sspul[A] == 'y')
      {  rgradient('z', gzlvl1*0.75);
         delay(0.0015);
         rgradient('z', 0.0);
         delay(gstab);
         rgpulse(pw, zero, rof1, rof1);
         rgradient('z', gzlvl1*0.6);
         delay(0.001);
         rgradient('z', 0.0);
         delay(gstab+0.005);
      }
     
      delay(d1);
 
   status(B);
      rgpulse(pw,zero,rof1,rof1);
      delay(tau);
      if (mbond[0] == 'y')                      /* one-bond J(CH)-filter */
      {  decrgpulse(pwx, zero, rof1, 0.0);
         delay(taumb - tau - rof1 - pwx);
      }

      decrgpulse(pwx,v1,rof1,rof1);
      delay(gt1+ grise + gstab + 2.0*GRADIENT_DELAY + pw);
      decrgpulse(pwx*2.0,v1,rof1,rof1);
      rgradient('z', gzlvl1);
      delay(gt1+grise);
      rgradient('z',0.0);
      delay(gstab);

      txphase(zero);
      if ((d2/2 )  > 0.0)   
         delay(d2/2 );
      rgpulse(pw*2.0,zero,rof1,rof1);
      decphase(v2);
      if ((d2/2)  > 0.0)
         delay(d2/2 );

      rgradient('z', gzlvl2);
      delay(gt2+grise);
      rgradient('z',0.0);
      delay(gstab);
      decrgpulse(pwx*2.0,v2, rof1, rof1);
      delay(gt2+ grise + gstab + 2.0*GRADIENT_DELAY + pw);
      decrgpulse(pwx,v2,rof1,rof1);
      rgradient('z',gzlvl3*icosel);
      delay(gt3+grise);
      rgradient('z',0.0);
      rlpower(dhp,DODEV);
      delay(tau-(gt3+grise));
 
   status(C);
      assign(v3,oph); 
} 
