/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gLRCC.c 

DESCRIPTION and INSTRUCTION:

    This pulse sequence will allow one to perform the following experiment:
    3D LRCC with choice of sensitivity enhancement.

    ni(t1) --> 13C
    ni2(t2)--> 13C

    REFERENCE:
        Ad Bax et al., JBNMR, 4, 787-797 (1994)

    OFFSET POSITION:
        tof =   ~4.75 ppm (1H on water).
        dof =   35 ppm (middle of CA and methyl carbon).
        dof2 =  120 ppm (15N region, not used in this experiment).

    CHOICE OF FLAGS:
    mag_flg = y -> Using magic angle gradients (Triax PFG probe is required).
    SE_flg = y  --> Sensitivity enhanced in t2 (f2coef = '1 0 -1 0 0 -1 0 -1').


    Written by Weixing Zhang, June 21, 1999
    St. Jude Children's Research Hospital
    Memphis, TN 38134
    USA
    (901)495-3169

    Revised by Weixing Zhang
    September 19, 2001
    Modified on April 26, 2002 for submission to BioPack.


*/

#include <standard.h>


static int   phi1[2]  = {0,2},
             phi2[4] =  {1,1,3,3},
	     phi3[8]  = {1,1,1,1,3,3,3,3},
             phi4[1]  = {0},
             phi5[8]  = {0,0,1,1,2,2,3,3},
             phi6[1]  = {2},
             phi7[1]  = {2},
             phi8[1]  = {2},
             phi9[8]  = {2,2,3,3,0,0,1,1},
             rec[4]   = {0,2,2,0};

static double   d2_init=0.0, d3_init=0.0;

void pulsesequence()
{
  char      f1180[MAXSTR],   
            f2180[MAXSTR],    		     
            SE_flg[MAXSTR],
            mag_flg[MAXSTR];    

 
  int       icosel,     
            t1_counter,  		
            t2_counter,
            ni2 = getval("ni2");  	

  double    tau1,       
            tau2,      
            gstab = getval("gstab"),
	    taua = getval("taua"),	  /* 1/4JCH   */
            taub = getval("taub"),        /* 1/8JCH   */
            BigTC = getval("BigTC"),      /* 29 ms,  n*1/2JCC */
        
   pwClvl = getval("pwClvl"), 	      /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */

   pwNlvl = getval("pwNlvl"),	                      /* power for N15 pulses */
   pwN = getval("pwN"),               /* N15 90 degree pulse length at pwNlvl */

   sw1 = getval("sw1"),
   sw2 = getval("sw2"),

   gzcal=getval("gzcal"),
   gt1 = getval("gt1"),  	               /* coherence pathway gradients */
   gzlvl1 = getval("gzlvl1"),
   gzlvl2 = getval("gzlvl2"),

   gt0 = getval("gt0"),		                           /* other gradients */
   gt3 = getval("gt3"),
   gt4 = getval("gt4"),
   gt5 = getval("gt5"),
   gt6 = getval("gt6"),
   gt7 = getval("gt7"),
   gt8 = getval("gt8"),
   gzlvl0 = getval("gzlvl0"),
   gzlvl3 = getval("gzlvl3"),
   gzlvl4 = getval("gzlvl4"),
   gzlvl5 = getval("gzlvl5"),
   gzlvl6 = getval("gzlvl6"),
   gzlvl7 = getval("gzlvl7"),
   gzlvl8 = getval("gzlvl8");

   getstr("f1180",f1180);
   getstr("f2180",f2180);
   getstr("SE_flg", SE_flg);
   getstr("mag_flg",mag_flg);

   /*   LOAD PHASE TABLE    */

   settable(t1,2,phi1);
   settable(t2,4,phi2);
   settable(t3,8,phi3);
   settable(t4,1,phi4);
   settable(t5,8,phi5);
   settable(t6,1,phi6);
   settable(t7,1,phi7);
   settable(t8,1,phi8);
   settable(t9,8,phi9);

   settable(t11,4,rec);

   /* CHECK VALIDITY OF PARAMETER RANGES */

   if (ni2/sw2 > 2.0*(BigTC - gt6 - 500.0e-6) && (SE_flg[A] != 'y'))
   {
      printf("ni2 is too big, should be < %f\n",2.0*sw2*(BigTC - gt6 - 500.0e-6));
      psg_abort(1);
   }
   if (ni2/sw2 > 2.0*(BigTC - gt1 - 500.0e-6) && (SE_flg[A] == 'y'))
   {
      printf("ni2 is too big, should be < %f\n",2.0*sw2*(BigTC - gt1 - 500.0e-6));
      psg_abort(1);
   }

   if(dm[A] == 'y' || dm[B] == 'y' )
   {
      text_error("incorrect dec1 decoupler flags! Should be 'nny' ");
      psg_abort(1);
   }

   if(dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y')
   {
      text_error("incorrect dec2 decoupler flags! Should be 'nnn' ");
      psg_abort(1);
   }

   if( dpwr > 50 )
   {
      text_error("don't fry the probe, DPWR is too high !!  ");   
      psg_abort(1); 
   }

   /*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

   if (phase1 == 2) 
   {
      tsadd(t1,1,4);  
      tsadd(t2,1,4);
      tsadd(t4,1,4);
      tsadd(t8,1,4);
   }
   
   
   icosel = 1; 
   if (phase2 == 2) 
   {
      if(SE_flg[A] == 'y')
      {
          tsadd(t6,2,4); icosel = -1; 
      }
      else
      {
         tsadd(t7,1,4);
      }
   }

   tau1 = d2 - 4.0*pwC/PI - 2.0*pw;
   if(f1180[A] == 'y') 
   { 
      tau1 += (1.0/(2.0*sw1)); 
   }
   if(tau1 < 0.2e-6) tau1 = 0.0; 
   tau1 = tau1/2.0;

   tau2 = d3;
   if(f2180[A] == 'y') 
   {
      tau2 += (1.0/(2.0*sw2));
   }
   if(tau2 < 0.2e-6) tau2 = 0.0; 
   tau2 = tau2/2.0;

   /* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int)((d2-d2_init)*sw1 + 0.5);
   if(t1_counter % 2) 
   {  
      tsadd(t1,2,4); 
      tsadd(t2,2,4);
      tsadd(t4,2,4);
      tsadd(t11,2,4); 
   }

   if( ix == 1) d3_init = d3;
   t2_counter = (int)((d3-d3_init)*sw2 + 0.5);
   if(t2_counter % 2) 
   {
      tsadd(t1,2,4); 
      tsadd(t2,2,4);
      tsadd(t3,2,4); 
      tsadd(t4,2,4);
      tsadd(t11,2,4); 
   }

   /* BEGIN PULSE SEQUENCE */

   status(A);
   obspower(tpwr);
   decpower(pwClvl);
   dec2power(pwNlvl);
   txphase(zero);
   decphase(zero);
   dec2phase(zero);

   if (gt0 > 0.2e-6)
   {
      rgpulse(pw,zero, 0.0, 0.0); 
      zgradpulse(gzlvl0, gt0);
      txphase(one);
      dec2rgpulse(pwN, zero, 50.0e-6, 0.0);
      decrgpulse(pwC, zero, 0.0, 0.0);
      rgpulse(pw, one, 0.0, 0.0);
      zgradpulse(0.7*gzlvl0, gt0);
      delay(1.0e-3);
   }
   delay(d1);

   if (satmode[A] == 'y')
   {
       obsoffset(satfrq);
       obspower(satpwr);
       rgpulse(satdly, zero, 1.0e-6, 0.0);
       obspower(tpwr);
       obsoffset(tof);
   }
   txphase(zero);

   rgpulse(pw, zero, 10.0e-6, 1.0e-6);                  
   zgradpulse(gzlvl3, gt3);
   delay(taua - gt3);
   simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);
   txphase(one);
   delay(taua - gt3 - 500.0e-6);
   zgradpulse(gzlvl3, gt3);
   delay(500.0e-6);
   rgpulse(pw, one, 1.0e-6, 1.0e-6);

   if (mag_flg[A] == 'y')
   {
      magradpulse(gzcal*gzlvl4, gt4);
   }
   else
   {
      zgradpulse(gzlvl4, gt4);
   }
   decphase(t1);
   txphase(zero);
   delay(500.0e-6);

   decrgpulse(pwC, t1, 0.0, 0.0);
   zgradpulse(gzlvl5, gt5);
   decphase(t4);

   {
      delay(BigTC - gt5 - 2.0*GRADIENT_DELAY - pwC);
      decrgpulse(2.0*pwC, t4, 0.0, 0.0);
      delay(BigTC - gt5 - pwC - 500.0e-6 - 2.0*GRADIENT_DELAY);
   }
   zgradpulse(gzlvl5, gt5);
   decphase(t2);
   delay(500.0e-6);

   txphase(two);
   decrgpulse(pwC, t2, 0.0, 0.0);
   decphase(t3);
   if (tau1 > (pwN - pw))
   {
      delay(tau1 - pwN + pw);
      sim3pulse(2.0*pw, 0.0, 2.0*pwN, two, zero, zero, 0.0, 0.0);
      delay(tau1 - pwN + pw);
   }
   else
   {
      delay(tau1);
      rgpulse(2.0*pw, two, 0.0, 0.0);
      delay(tau1);
   }
   decrgpulse(pwC, t3, 0.0, 0.0);
 
   if(SE_flg[A] != 'y')
   {
      delay(tau2);
      rgpulse(2.0*pw, zero, 0.0, 0.0);
      decphase(t5);
      zgradpulse(gzlvl6, gt6);

      {
         delay(BigTC - gt6 - 2.0*GRADIENT_DELAY - 2.0*pw - pwC);
         decrgpulse(2.0*pwC, t5, 0.0, 0.0);
         delay(BigTC - tau2 - gt6 - 2.0*GRADIENT_DELAY - pwC - 500.0e-6);
      }


      zgradpulse(gzlvl6, gt6);
      decphase(t7);
      delay(500.0e-6);
      decrgpulse(pwC, t7, 0.0, 2.0e-6);
      if(mag_flg[A] == 'y')
      {
         magradpulse(gzcal*gzlvl8, gt8);
      }
      else
      {
         zgradpulse(gzlvl8, gt8);
      }
      txphase(zero);
      decphase(zero);
      delay(200.0e-6);
   
      rgpulse(pw, zero, 2.0e-6, 2.0e-6);
   
      txphase(two);
      decphase(zero);
      if(mag_flg[A] == 'y')
      {
         magradpulse(gzcal*gzlvl7, gt7);
      }
      else
      {
         zgradpulse(gzlvl7, gt7);
      }
      delay(taua - gt7);
   
      simpulse(2.0*pw, 2.0*pwC, two, zero, 0.0, 0.0);
   
      delay(taua - gt7 - POWER_DELAY - 500.0e-6);
      if(mag_flg[A] == 'y')
      {
         magradpulse(gzcal*gzlvl7, gt7);
      }
      else
      {
         zgradpulse(gzlvl7, gt7);
      }
      decpower(dpwr);
      delay(500.0e-6);
   
      rgpulse(pw, zero, 2.0e-6, 0.0);
   }
   else
   {
      delay(tau2);
      rgpulse(2.0*pw, zero, 0.0, 0.0);
      decphase(t5);
   
      {
         delay(BigTC - 2.0*pw - pwC);
         decrgpulse(2.0*pwC, t5, 0.0, 0.0);
         delay(BigTC - tau2 - pwC - gt1 - 6.0*GRADIENT_DELAY - 500.0e-6);
      }
   
      if (mag_flg[A] == 'y')
      {
         magradpulse(gzcal*gzlvl1, gt1);
      }
      else
      {
         zgradpulse(gzlvl1, gt1);
         delay(4.0*GRADIENT_DELAY);
      }
      txphase(zero);
      decphase(t6);
      delay(500.0e-6 );
   
      simpulse(pw, pwC, zero, t6, 0.0, 2.0e-6);
   
      decphase(zero);
      zgradpulse(gzlvl6, gt6);
      delay(taub - gt6);
   
      simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);
   
      zgradpulse(gzlvl6, gt6);
      txphase(one);
      decphase(one);
      delay(taub - gt6);
   
      simpulse(pw, pwC, one, one, 2.0e-6, 2.0e-6);
   
      txphase(zero);
      decphase(zero);
      zgradpulse(gzlvl7, gt7);
      delay(taua - gt7);
   
      simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);
   
      delay(taua - gt7 - POWER_DELAY - 500.0e-6);
      zgradpulse(gzlvl7, gt7);
      decpower(dpwr);
      delay(500.0e-6);
   
      rgpulse(pw, zero, 2.0e-6, 0.0);
   
      delay((gt1/4.0) + gstab -2.0*GRADIENT_DELAY);
   
      rgpulse(2.0*pw, zero, 0.0, 0.0);
   
      if (mag_flg[A] == 'y')
      {
          magradpulse(icosel*gzcal*gzlvl2, gt1/4.0);
          delay(gstab - 4.0*GRADIENT_DELAY);		
      }
      else
      {
          zgradpulse(icosel*gzlvl2, gt1/4.0);
          delay(gstab);		
      }
   }

   status(C);
   setreceiver(t11);
}		 
