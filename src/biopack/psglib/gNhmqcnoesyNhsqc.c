/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gNhmqcnoesyNhsqc.c

DESCRIPTION and INSTRUCTION:

    This pulse sequence will allow one to perform the following experiment:
    4D NN HMQC-NOESY-HSQC
    ni(t1) --> 1H (Amide protons)
    ni2(t2)--> 15N
    ni3(t3)--> 15N

    OFFSET POSITION:
        tof =   ~4.75 ppm (1H on water).
        tofhn =  ~8 ppm  (center of amide protons).
        dof =   56 ppm (13Ca).
        dof2 =  120 ppm (15N region).

    CHOICE OF FLAGS:
    mag_flg = y -> Using magic angle gradients (Triax PFG probe is required).
    flipback = y -> Water flipback is used.

    C13 Decoupling during evolution:  use Pbox to make waveform for alpha and C=O decoupling 
                                      set dmm='cpcc' or 'cppc' or 'ccpc' or 'cccc'
                                      set dseq to name of Pbox-generated shape
                                      set dpwr to Pbox-generated dpwr 
                                      set dmf to Pbox-generated dmf 
                                      set dres to Pbox-generated dres 
                                      use dm='nynn' or 'nyyn' for decoupling during t2
                                      use dm='nnyn' or 'nyyn' for decoupling during t3


    Writen by Weixing Zhang, December 11, 2000
    St. Jude Children's Research Hospital
    Memphis, TN 38134
    USA
    (901)495-3169
    Modified on April 26, 2002 for submission to BioPack.
    Modified by GG for BioPack. Used status control for
     13C decoupling during t2/t3.

*/

#include <standard.h>
extern int dps_flag;

static double d2_init = 0.0, d3_init = 0.0, d4_init = 0.0;

static int phi1[2] = {0,2},
           phi2[4] = {0,0,2,2},
           phi3[1] = {0},
           phi4[8] = {0,0,0,0,1,1,1,1}, 
           phi5[1] = {0},
           rec[8] = {0,2,2,0,2,0,0,2};

pulsesequence()
{
/* DECLARE VARIABLES */

 char        f1180[MAXSTR],f2180[MAXSTR],f3180[MAXSTR],
             mag_flg[MAXSTR], flipback[MAXSTR];

 int	     ni2, ni3, phase, phase2, phase3, icosel,t1_counter, t2_counter, t3_counter;

 double      
             tofhn,              /* adjust carrier to the center of amide protons   */
             tauxh,              /* 1 / 4J(NH)                */
             pwN,                /* PW90 for N-nuc            */
             pwNlvl,             /* power level for N hard pulses */
             jnh,                /* coupling for NH           */
             gzcal = getval("gzcal"),
             compH = getval("compH"),
	     tau1,	     
             tau2,
             tau3,
	     sw1,
             sw2,
             sw3,
             flippw,             /* pw for selective pulse at flippwr  */
             flippwr,
             fliphase,
             mix,

             gzlvl0,             
             gt0,             
             gzlvl1,        
             gt1,        
             gzlvl2,
             gt7,
             gzlvl3,
             gt3,
             gzlvl4,
             gt4,
             gzlvl5,
             gt5,
             gzlvl6,
             gt6,
             gzlvl7,
             gstab;


/* LOAD VARIABLES */

  tofhn = getval("tofhn");
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  sw3 = getval("sw3");
  ni = getval("ni");
  ni2 = getval("ni2");
  ni3 = getval("ni3");
  phase = (int)(getval("phase") + 0.5);
  phase2 = (int)(getval("phase2") + 0.5);
  phase3 = (int)(getval("phase3") + 0.5);
  jnh = getval("jnh");
  pwN = getval("pwN");
  pwNlvl = getval("pwNlvl"); 
  gstab = getval("gstab");
  flippw = getval("flippw");
  fliphase = getval("fliphase");
  mix = getval("mix");

  gt0 = getval("gt0");
  gt1 = getval("gt1");
  gt7 = getval("gt7");
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");
  gt6 = getval("gt6");
  gzlvl0 = getval("gzlvl0");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl6 = getval("gzlvl6");
  gzlvl7 = getval("gzlvl7");

  getstr("mag_flg", mag_flg);
  getstr("f1180",f1180); 
  getstr("f2180", f2180);
  getstr("f3180", f3180);
  getstr("flipback",flipback);

/* check validity of parameter range */


    if( dpwr > 50 )
    {
	printf("don't fry the probe, dpwr too large!  ");
	psg_abort(1);
    }

    if ( dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' )
       { printf("incorrect dec2 decoupler flags! Should be 'nnnn' or 'nnny' "); psg_abort(1);}

    if( dpwr2 > 50 )
    {
	printf("don't fry the probe, dpwr2 too large!  ");
	psg_abort(1);
    }

    if(gt0 > 15.0e-3 || gt1 > 15.0e-3 || gt7 > 15.0e-3 || gt3 > 15.0e-3 || gt4 > 15.0e-3 || gt5 > 15.0e-3)
    {
        printf("gti must be less than 15 ms \n");
        psg_abort(1);
    }


/* LOAD VARIABLES */

  settable(t1, 2, phi1);
  settable(t2, 4, phi2);
  settable(t3, 1, phi3);
  settable(t4, 8, phi4);
  settable(t5, 1, phi5);

  settable(t6, 8, rec);

/* INITIALIZE VARIABLES */

  tauxh = ((jnh != 0.0) ? 1/(4*(jnh)) : 2.35e-3);

/* Phase incrementation for hypercomplex data */

   if (phase == 2)
   {
       tsadd(t1, 1, 4);
   }

   if (phase2 == 2)
   {
       tsadd(t2, 1, 4);
   }

   if ( phase3 == 1 )     
   {
        tsadd(t5, 2, 4); 
        icosel = 1; 
   } 
   else icosel = -1;


/* calculate modification to phases based on current t1 values
   to achieve States-TPPI acquisition */
 
 
   if(ix == 1) d2_init = d2;
   t1_counter = (int)((d2-d2_init)*sw1 + 0.5);

   if(t1_counter %2) 
   {
      tsadd(t1,2,4);
      tsadd(t6,2,4);
   }

   if(ix == 1) d3_init = d3;
   t2_counter = (int)((d3-d3_init)*sw2 + 0.5);

   if(t2_counter %2) 
   {
      tsadd(t2,2,4);
      tsadd(t6,2,4);
   }

   if(ix == 1) d4_init = d4;
   t3_counter = (int)((d4-d4_init)*sw3 + 0.5);

   if(t3_counter %2) 
   {
      tsadd(t3,2,4);
      tsadd(t6,2,4);
   }


/* set up so that get (90, -180) phase corrects in F1 if f1180 flag is y */

   tau1 = d2;
   if(f1180[A] == 'y') 
   {
       tau1 += (1.0/(2.0*sw1));
   }
   if (tau1 < 0.2e-6) tau1 = 0.0;
   tau1 = tau1/2.0;

   tau2 = d3 - 2.0*pw - 4.0*pwN/PI;
   if(f2180[A] == 'y') 
   {
       tau2 += (1.0/(2.0*sw2));
   }
   if (tau2 < 0.2e-6) tau2 = 0.0;
   tau2 = tau2/2.0;
   
   tau3 = d4;
   if(f3180[A] == 'y') 
   {
       tau3 += (1.0/(2.0*sw3));
   }
   tau3 = tau3/2.0;
   
   flippwr = tpwr - 20.0*log10(flippw/(compH*pw*1.69));
   flippwr = (int)(flippwr + 0.4);

   if (fliphase < 0.2e-6) fliphase = fliphase + 360.0;
   initval(fliphase, v10);
   initval(7.0, v9);
   obsstepsize(45.0);

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tpwr);            
   decpower(dpwr);        
   dec2power(pwNlvl);     
   obsoffset(tofhn);
   xmtrphase(v9);
   delay(d1);

status(B);   /* for 13C decoupling during t2, use dm='nynn' or 'nyyn' */

   dec2rgpulse(pwN,zero,0.0,2.0e-6);
   zgradpulse(gzlvl0,gt0);

   rgpulse(pw,t1,1.0e-6,2.0e-6);    
   zgradpulse(gzlvl6,gt6);
   delay(1.9*tauxh - gt1 - 2.0e-6);

   dec2rgpulse(pwN, t2, 0.0, 0.0);
 
   delay(tau2);
   rgpulse(2.0*pw, t1, 0.0, 0.0);
   dec2phase(zero);
   delay(tau2);
  
   dec2rgpulse(pwN, zero, 0.0, 0.0);
   xmtrphase(zero);

   delay(1.9*tauxh - gt6 - 600.0e-6 - SAPS_DELAY);
   txphase(zero);

   zgradpulse(gzlvl6,gt6);
   txphase(zero);
   delay(0.6e-3);

   if (tau1 > pwN)
   {
      delay(tau1 - pwN);
      dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
      delay(tau1 - pwN);
   }
   else
   {
      delay(2.0*tau1);
   }
   rgpulse(pw, zero, 0.0, 1.0e-6);

status(A);             /* no decoupling during mix period */

   delay(mix - pwN - 1.5*gt7 - 2.0e-3);
   zgradpulse(gzlvl7,gt7);
   delay(1.0e-3);
   dec2rgpulse(pwN, zero, 0.0, 2.0e-6);
   zgradpulse(gzlvl7,gt7/2.0);
   delay(1.0e-3 - 2.0e-6);

   rgpulse(pw,zero,1.0e-6,2.0e-6);    

   zgradpulse(gzlvl3,gt3);

   delay(tauxh - gt3 - 2.0e-6);               /* delay=1/4J(NH)   */

   sim3pulse(2.0*pw,(double)0.0,2*pwN,zero,zero,zero,0.0,0.0);

   txphase(one);
   delay(tauxh - gt3 - 500.0e-6);               /* delay=1/4J(NH)   */
   zgradpulse(gzlvl3,gt3);
   delay(500.0e-6);

   rgpulse(pw, one,0.0,2.0e-6);
   obsoffset(tof);

   if (flipback[A]=='y')
   {
      xmtrphase(v10);
      obspower(flippwr);
      shaped_pulse("H2Osinc",flippw,two,2.0e-6,2.0e-6);
      obspower(tpwr);
      xmtrphase(zero);
   }

   zgradpulse(gzlvl4,gt4);
   dec2phase(t3);
   txphase(zero); 
status(C);        /* for 13C decoupling during t3 set dm='nnyn' or 'nyyn'  */
   delay(250.0e-6);

   dec2rgpulse(pwN,t3,0.0,0.0);
   dec2phase(t4);

   delay(tau3);
   rgpulse(2.0*pw, zero,0.0,0.0);
   delay(tau3);
status(A);          /* no decoupling  */
   if (mag_flg[A] == 'y')
   {
      delay(4.0*GRADIENT_DELAY);
   }
   delay(gstab + gt6 + 2.0*GRADIENT_DELAY - 2.0*pw - PRG_STOP_DELAY);

   dec2rgpulse(2.0*pwN,t4,0.0,0.0);
   dec2phase(t5);

   if (mag_flg[A] == 'y')
   {
      magradpulse(gzcal*gzlvl1, gt1);
   }
   else
   {
      zgradpulse(gzlvl1,gt1);
   }
 
   delay(gstab);  

   sim3pulse(pw,0.0e-6,pwN,zero,zero,t5,0.0,0.0);   

   txphase(zero);
   dec2phase(zero);

   if (gt5 > 0.2e-6)
   {
      zgradpulse(gzlvl5, 1.3*gt5);
   }

   delay(tauxh - 1.3*gt5);       

   sim3pulse(2.0*pw,(double)0.0,2*pwN,zero,zero,zero,0.0,0.0);  

   txphase(one); dec2phase(one);

   delay(tauxh - 1.3*gt5 - 500.0e-6);     

   if (gt5 > 0.2e-6)
   {
      zgradpulse(gzlvl5, 1.3*gt5);
   }
   delay(500.0e-6);

   sim3pulse(pw,(double)0.0,pwN,one,zero,one,0.0,0.0);

   dec2phase(zero); txphase(zero);

   if (gt5 > 0.2e-6)
   {
      zgradpulse(gzlvl5,gt5);
   }

   delay(tauxh - gt5);          

   sim3pulse(2.0*pw,(double)0.0,2*pwN,zero,zero,zero,0.0,0.0);

   dec2power(dpwr2);  

   delay(tauxh - 3.0*POWER_DELAY - gt5 - 500.0e-6); 

   if (gt5 > 0.2e-6)
   {
      zgradpulse(gzlvl5,gt5);
   }
   delay(500.0e-6);

   rgpulse(pw,zero,0.0,0.0);

   if(mag_flg[A] == 'y')
   {
      delay(4.0*GRADIENT_DELAY);
   }

   delay(gstab + gt1/10.0 + 2.0*GRADIENT_DELAY);

   rgpulse(2.0*pw,zero,0.0,0.0);

   if(mag_flg[A] == 'y')
   {
      magradpulse(icosel*icosel*gzcal*gzlvl2, gt1/10.0);
   }
   else
   {
      zgradpulse(icosel*gzlvl2,gt1/10.0);
   }

   delay(gstab);

status(D);
   setreceiver(t6);
}
