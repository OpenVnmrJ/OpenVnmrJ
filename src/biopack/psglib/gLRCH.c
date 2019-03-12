/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gLRCH.c

DESCRIPTION and INSTRUCTION:

    This pulse sequence will allow one to perform the following experiment:
    3D LRCH for the measurement of long range H-C coupling constants.
    These couplings are useful for sterospecific assignment of CD in Leu.
    ni(t1) --> 1H
    ni2(t2)--> 13C 
  
    REFERENCE:
    REF: Bax et al., J. Biomol. NMR, 4, 787-797 (1994)
    Methods in Enzymolody, Vol. 239, pqge 96
 

    OFFSET POSITION:
        tof =   ~4.75 ppm (1H on water).
        tofh = ~2 ppm (Methyl protons)
        dof =  ~30 ppm (13CB of Leu) 
        dof2 =  120 ppm (15N region, not used in this experiment).

    CHOICE OF FLAGS:
    mag_flg = y -> Uses magic angle gradients (Triax PFG probe is required).
    ref_flg = y -> Runs ni2 2D reference experiment (See reference).
    flg_3919 = y -> Uses 3-9-19 water suppression.
    flipback = y -> Activates water flipback. 

    Writen by Weixing Zhang, June 22, 1999
    Added extra delay in t2 to correct phasing problem.
    February 1, 2002
    St. Jude Children's Research Hospital
    Memphis, TN 38134
    USA
    (901)495-3169
    Modified on April 26, 2002 for submission to BioPack.

*/

#include <standard.h>

static int  phi1[1] = {0},
	    phi2[16] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3},
	    phi3[2] = {0,2},
            phi4[4] = {0,0,2,2},
	    phi5[32]= {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		       3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3},

            phi5ref[4] = {1,3,3,1},
            ref[8] =  {0,2,2,0,2,0,0,2},

            rec[32]  = {0,2,2,0,2,0,0,2,0,2,2,0,2,0,0,2,
			2,0,0,2,0,2,2,0,2,0,0,2,0,2,2,0};


extern int dps_flag;
static double d2_init=0.0, d3_init=0.0;
            
void pulsesequence()
{
 char      
            f1180[MAXSTR],    
            f2180[MAXSTR],    
            mag_flg[MAXSTR],
            flg_3919[MAXSTR],
            flipback[MAXSTR],
            flipshap[MAXSTR],
	    ref_flg[MAXSTR];

 int        phase, 
            ni2,		
            t1_counter, 
            t2_counter;

 double     tau1,     
            tau4,         /* tau1/2.0    */
            tau2,     
            taua,         /*  ~ 3.0 ms */
            taub,         /*  ~ 4.0 ms  */
            tauc,         /*  ~ 1.7 ms  */
            taunoe = getval("taunoe"),  /*  5.0 - 10.0 ms   */
            flippwr = getval("flippwr"),
            flippw = getval("flippw"),
            BigTC,        /* carbon constant time period ~ 29.6 ms */
	    cycles,
            factor = 0.08,
            tau_3919 = getval("tau_3919"),
            gzcal = getval("gzcal"),
            gstab= getval("gstab"),
            tofh = getval("tofh"),   /*  center position in t1    */

            pwClvl,        /* power level for high power 13C pulses on dec1 */
            pwC,          /* 90 c pulse at dhpwr            */

	    pwNlvl,
	    pwN,

            gt1,
            gt2,
            gt4,
            gt5,

            gzlvl1,
            gzlvl2,
            gzlvl4,
            gzlvl5;

/* LOAD VARIABLES */

   getstr("f1180",f1180);
   getstr("f2180",f2180);
   getstr("mag_flg", mag_flg);
   getstr("flg_3919", flg_3919);
   getstr("flipback", flipback);
   getstr("flipshap", flipshap);
   getstr("ref_flg", ref_flg);

   taua = getval("taua"); 
   taub = getval("taub");
   tauc = getval("tauc");
   BigTC = getval("BigTC");

   tpwr = getval("tpwr");

   pwClvl = getval("pwClvl");
   pwC = getval("pwC");
   pwNlvl = getval("pwNlvl");
   pwN = getval("pwN");

   phase = (int) ( getval("phase") + 0.5);
   phase2 = (int) ( getval("phase2") + 0.5);

   sw1 = getval("sw1");
   sw2 = getval("sw2");

   ni = getval("ni");
   ni2 = getval("ni2");

   gt1 = getval("gt1");
   gt2 = getval("gt2");
   gt4 = getval("gt4");
   gt5 = getval("gt5");

   gzlvl1 = getval("gzlvl1");
   gzlvl2 = getval("gzlvl2");
   gzlvl4 = getval("gzlvl4");
   gzlvl5 = getval("gzlvl5");

/* LOAD PHASE TABLE */

   settable(t1,1,phi1);
   settable(t2,16,phi2);
   settable(t3,2,phi3);
   settable(t4,4,phi4);

   if (ref_flg[A] == 'y')
   {
      settable(t10, 8, ref);
      settable(t5, 4, phi5ref);
   }
   else
   {
      settable(t5,32,phi5);
      settable(t10, 32, rec);
   }

/* CHECK VALIDITY OF PARAMETER RANGES */

   if(ni2/sw2 > 2.0*(BigTC - 2.0*taua - gt2 - 400.0e-6))
   {
      printf(" ni2 is too big,  should be < %f\n", 
      2.0*sw2*(BigTC - 2.0*taua - gt2 - 400.0e-6));
      psg_abort(1);
   }
   if( taua - ni/(4.0*sw1) < 0.0 )
   {
      printf(" ni is too big,  should be < %f\n", sw1*(4.0*taua));
      psg_abort(1);
   }

   if (ref_flg[A] == 'y' && ni > 1)
   {
      printf("Incorrect combination of ni and ref_flg\n");
      psg_abort(1);
   }

   if((dm[A] == 'y' || dm[B] == 'y' ))
   {
      printf("incorrect dec1 decoupler flags!  ");
      psg_abort(1);
   }

   if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y'))
   {
      printf("incorrect dec2 decoupler flags!  ");
      psg_abort(1);
   }

   if( dpwr > 50 )
   {
      printf("don't fry the probe, DPWR too large!  ");
      psg_abort(1);
   }

   if( dpwr2 > 50 )
   {
      printf("don't fry the probe, DPWR2 too large!  ");
      psg_abort(1);
   }

/*  Phase incrementation for hypercomplex 2D data */

   if (phase == 2) 
   {
      tsadd(t3,1,4);
   }

   if (phase2 == 2)
   {
      tsadd(t1, 1, 4);
   }

/*  Set up f1180  tau1 = t1               */
   
   tau1 = d2 - (4.0*pw/PI + 2.0*pwC);
   if(f1180[A] == 'y') 
   {
      tau1 += ( 1.0/(2.0*sw1));
   }
   if(tau1 < 0.4e-6) tau1 = 0.0;
   tau1 = tau1/2.0;

   tau4 = tau1/2.0;

/*  Set up f2180  tau2 = t2               */
   
   tau2 = d3;
   if(f2180[A] == 'y') 
   {
      tau2 += (1.0/(2.0*sw2));
   }
   if(tau2 < 0.2e-6) tau2 = 0.0;
   tau2 = tau2/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
   {
      tsadd(t3,2,4);     
      tsadd(t10,2,4);    
   }

   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) 
   {
      tsadd(t1,2,4);     
      tsadd(t10,2,4);    
   }
   cycles = (double)(int)((d1 - 10.0e-3)/taunoe + 0.5);
   initval(cycles, v1);
   if (dps_flag)
   {
      printf("Total number of cycles is : %f\n", cycles);
   }


/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);

   delay(20.0e-6);
   obsoffset(tofh);
   obspower(tpwr);          
   decpower(pwClvl);
   dec2power(pwNlvl);
   rcvroff();

   txphase(zero);
   decphase(t1);
   dec2phase(zero);

   delay(10.0e-3);


   if (cycles >= 1.0)
   {
      loop(v1, v2);
	 delay(taunoe/2.0 - 1.5*pw/2.0);
	 rgpulse(1.5*pw, zero, 0.0, 0.0);
	 delay(taunoe/2.0 - 1.5*pw/2.0);
      endloop(v2);
   }
  else
   delay(d1);

   if(gt1 > 0.2e-6)
   {
      zgradpulse(gzlvl1, gt1);
      delay(0.5e-3);
   }

status(B);

 if(ref_flg[A] == 'y')
 {
   decrgpulse(pwC, t1, 2.0e-6, 2.0e-6);
   decphase(t2);
   delay(tau2);
   rgpulse(2.0*pw, zero, 0.0, 0.0);
   delay(BigTC - 2.0*pw - gt2 - 2.0*GRADIENT_DELAY - 1.0e-3);
   if (gt2 > 0.2e-6)
   {
      zgradpulse(gzlvl2, gt2);
   }
   delay(1.0e-3);

   decrgpulse(2.0*pwC, t2, 0.0, 0.0);

   delay(BigTC - gt2 - 2.0*GRADIENT_DELAY - tau2 - 2.0*pw - 2.0e-3);
   if (gt2 > 0.2e-6)
   {
      zgradpulse(gzlvl2, gt2);
   }
   decphase(t5);
   delay(1.0e-3);
   rgpulse(2.0*pw, zero, 0.0, 0.0);
   delay(1.0e-3);

   decrgpulse(pwC, t5, 2.0e-6,2.0e-6);
 }
 else
 {
   decrgpulse(pwC, t1, 0.0, 0.0);
   decphase(t2);
   delay(tau2);
   decrgpulse(2.0*pwC, t2, 0.0, 0.0);
   delay(taua - tau4);
   rgpulse(2.0*pw, zero, 0.0, 0.0);

   delay(1.1*pwC);     /* phase correction in t2, February 1, 2002  */
   delay(BigTC - taua - gt2 - tau2 + tau4 - tau1 - 400.0e-6);

   if (gt2 > 0.2e-6)
   {
      zgradpulse(gzlvl2, gt2);
   }

   txphase(t3);
   decphase(t4);
   delay(400.0e-6);   
   rgpulse(pw, t3, 0.0, 0.0);
   txphase(t4);
   delay(tau1);
   decrgpulse(2.0*pwC, t4, 0.0, 0.0);
   delay(tau1);
   rgpulse(pw, t4, 0.0, 0.0);

   delay(BigTC - taub - gt2 + tau4 - tau1 - 1.0e-3);
   if (gt2 > 0.2e-6)
   {
      zgradpulse(gzlvl2, gt2);
   }
   txphase(zero);
   decphase(t5);
   delay(1.0e-3);
   rgpulse(2.0*pw, zero, 0.0, 0.0);
   delay(taub - tau4);

   decrgpulse(pwC, t5, 0.0,2.0e-6);
 }
   zgradpulse(gzlvl4, gt4);
   obsoffset(tof);

   delay(200.0e-6);

   if (flipback[A] == 'y') 
   {
      txphase(two);
      obspower(flippwr);
      shaped_pulse(flipshap, flippw, two, 1.0e-6, 0.0);
      txphase(zero);
      obspower(tpwr);
   }
   rgpulse(pw,zero,2.0e-6,2.0e-6);
   if (flg_3919[A] == 'y')
   {
      if (mag_flg[A] == 'y')
      {
         magradpulse(gzcal*gzlvl5, gt5);
      }
      else
      {
         zgradpulse(gzlvl5, gt5);
      }
      txphase(two);
      decphase(zero);
      delay(tauc - gt5 - 31.0*factor*pw - 2.5*tau_3919 - 2.0e-6);
        rgpulse(pw*factor*3.0, two, 0.0, 0.0);
        delay(tau_3919);
        rgpulse(pw*factor*9.0, two, 0.0, 0.0);
        delay(tau_3919);
        rgpulse(pw*factor*19.0, two, 0.0, 0.0);
        delay(tau_3919/2.0 - pwC);
        decrgpulse(2.0*pwC, zero, 0.0, 0.0);
        txphase(zero);
        delay(tau_3919/2.0 - pwC);
        rgpulse(pw*factor*19.0, zero, 0.0, 0.0);
        delay(tau_3919);
        rgpulse(pw*factor*9.0, zero, 0.0, 0.0);
        delay(tau_3919);
        rgpulse(pw*factor*3.0, zero, 0.0, 0.0);
        delay(100.0e-6);
        if (mag_flg[A] == 'y')
        {
           magradpulse(gzcal*gzlvl5, gt5);
        }
        else
        {
           zgradpulse(gzlvl5,gt5);
        }
        decpower(dpwr);
        delay(tauc - 31.0*factor*pw-2.5*tau_3919 - gt5 - POWER_DELAY - 100.0e-6);
   }
   else
   {
      if (mag_flg[A] == 'y')
      {
         magradpulse(gzcal*gzlvl5, gt5);
      }
      else
      {
         zgradpulse(gzlvl5, gt5);
      }
      decphase(zero);
      delay(tauc - gt5 - 2.0e-6);
      simpulse(2.0*pw,2.0*pwC, zero,zero,0.0,0.0);
      delay(tauc - gt5 - gstab);
      if (mag_flg[A] == 'y')
      {
         magradpulse(gzcal*gzlvl5, gt5);
      }
      else
      {
         zgradpulse(gzlvl5, gt5);
      }
      decpower(dpwr); 
      txphase(two);
      delay(gstab - POWER_DELAY);
      rgpulse(pw, zero,0.0,0.0);
   }

status(C);
   setreceiver(t10);
}
