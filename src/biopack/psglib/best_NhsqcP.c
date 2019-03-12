/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
   BEST- 1H-15N-HSQC experiment                

for SE='y' use f1coef='1 0 -1 0 0 1 0 1' for
 Vnmr/VnmrJ processing
for SE='n' use f1coef=''

Note the delay following the gzlvl1/gt1
coherence transfer gradient. It needs to be
zero or greater. This means that for SE='y'
gt1 typically needs to be shorter than for other
gradient experiments, typ. 1msec. This may
necessitate recalibrating gzlvl2 for best s/n
in a first increment spectrum.

shlvl1/shpw1/shname1 is enterable but the
broadband inversion "BIP_720_50_20_360" has
been recommended by Shanda and Brutscher
(see JACS, 128,9042-9043(2006)).       

The default values for the BIP pulse are
tpwr and 8*pw for the power and pulse width. 

dof should be set at carbonyl frequency

 BEST experiments are based on the longitudinal relaxation
 enhancement provided by minimal perturbation of aliphatic protons. 
 All HN pulses are band-selective EBURP2/time-reversed EBURP2/PC9/time
 -reversed PC9. The recycle delay can be adjusted for optimal pulsing
 regime (d1~0.4-0.5s) or for fast pulsing regime (d1~0.1-0.4s).
 For fast pulsing regime, care should be taken to the demands on the
 probe.Low power 15N decoupling and short acquisition times should be
 used (GARP/WURST). 

 The coherence pathway is conserved with standard hard pulse-based
 experiments. NC and NC' couplings are refocused during 15N evolution
 by the application of two ISNOB5 pulses centered on CA and CO
 regions. Carbon carrier frequency dof should be set to the center of
 carbonyl frequency. A 180° BIP pulse (shname1="BIP_720_50_20_360",
 shpw1=8*pw at tpwr) is used to refocuse HN coupling during 15N t1.
 
 Gradient sensitivity-enhanced (SE_flg='y', f1coef='1 0 -1 0 0 1 0
 1' ) and non sensitivity-enhanced (SE_flg='n', f1coef='1 0 0 0 0 0
 -1 0' ) versions are available. For SE_flg='y', gt1 typically needs
 to be shorter than for other gradient experiments, typ. 1msec.

 The flag f1180 should be set to 'y' if t1 is to be started at
 halfdwell time. This will give 90, -180 phasing in f1. If it is set
 to 'n' the phasing should be 0,0 and will still give a flat
 baseline.

 phase = 1,2 for States-TPPI acquisition or EchoAntiecho in
 t1 [15N].

		* Schanda, Paul
		* Lescop, Ewen
		* Van Melckebeke, Hélène
		* Brutscher, Bernhard

 Institut de Biologie Structurale, Laboratoire de RMN,
 41, rue Jules Horowitz, 38027 Grenoble Cedex 1 FRANCE

 see: - Schanda, P., Van Melckebeke, H. and Brutscher, B.,
        JACS, 128,9042-9043(2006)
     - Lescop, E., Schanda, P. and Brutscher, B.,
        submitted (2007)

Added to BioPack, GG, Varian Feb 2007
 */

#include <standard.h>
#include "bionmr.h"

static int 

   phi3[2] = {0,2},
   phi10[1] ={0},
   phi12[2] = {1,3},
   phi13[2] = {2,0};


void pulsesequence()
{
   char   
          shname1[MAXSTR],
	  f1180[MAXSTR],
          SE_flg[MAXSTR];

   int    icosel = 0,
          t1_counter,
          phase;


   double d2_init=0.0,
          pwS4,pwS5,pwS6,pwS7,
          lambda = getval("lambda"),
          gzlvl1 = getval("gzlvl1"),
          gzlvl2 = getval("gzlvl2"), 
          gzlvl3 = getval("gzlvl3"), 
          gzlvl5 = getval("gzlvl5"), 
          gt1 = getval("gt1"),
          gt3 = getval("gt3"),
          gt5 = getval("gt5"),
          gstab = getval("gstab"),
          shpw1,shlvl1=getval("shlvl1"),
          pwClvl = getval("pwClvl"),
          pwNlvl = getval("pwNlvl"),
          pwN = getval("pwN"),
          ni = getval("ni"),
          d2 = getval("d2"),
          tau1 = getval("tau1");

   getstr("shname1", shname1);
   getstr("SE_flg",SE_flg);
   getstr("f1180",f1180);



  phase = (int) (getval("phase") + 0.5);
   
   settable(t3,2,phi3);
   settable(t10,1,phi10);
   settable(t12,2,phi12);
   settable(t13,2,phi13);

/*   INITIALIZE VARIABLES   */

   shpw1 = getval("shpw1");
   pwS4 = h_shapedpw("eburp2",4.0,3.5,zero, 0.0, 0.0);  
   pwS5 = h_shapedpw("pc9f",4.0,3.5,zero, 2.0e-6, 0.0);
   pwS6 = h_shapedpw("reburp",4.0,3.5,zero, 0.0, 0.0);
   pwS7 = c_shapedpw2("isnob5",40.0,-125.0,"isnob5",40.0,0.0 , two, 0.0, 0.0);

if (SE_flg[0] =='y')
{
  if (phase == 2)  {tsadd(t10,2,4); icosel = +1;}
            else                               icosel = -1;
}
else
{
  if (phase == 2) {tsadd(t3,1,4); icosel=1;}
}
 

    tau1 = d2;
    if((f1180[A] == 'y') )
        { tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1;

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2)
        { tsadd(t3,2,4);tsadd(t12,2,4); tsadd(t13,2,4); }



   status(A);
   decpower(pwClvl);
   dec2power(pwNlvl);
   set_c13offset("co");
   zgradpulse(gzlvl3, gt3);
   delay(d1-gt3);
   lk_hold();
   rcvroff();
        h_shapedpulse("pc9f",4.0,3.5,zero, 2.0e-6, 0.0);  

	delay(lambda-pwS5*0.5-pwS6*0.4); 

   	h_sim3shapedpulse("reburp",4.0,3.5,0.0,2.0*pwN, one, zero, zero, 0.0, 0.0);

	delay(lambda-pwS5*0.5-pwS6*0.4);

        h_shapedpulse("pc9f_",4.0,3.5,one, 0.0, 0.0);  


   obspower(shlvl1);
/**************************************************************************/ 
/** Sensitivity enhanced version                     **********************/
/**************************************************************************/ 
      if (SE_flg[0] == 'y')
      {
        shaped_pulse(shname1,shpw1,zero,0.0,0.0);
     dec2rgpulse(pwN,t3,0.0,0.0);
           delay(tau1*0.5);
        shaped_pulse(shname1,shpw1,two,0.0,0.0);
 c_shapedpulse2("isnob5",40.0,-125.0,"isnob5",40.0,0.0 , two, 0.0, 0.0);
           delay(tau1*0.5);
        zgradpulse(-gzlvl1, gt1);
           delay(pwS4-shpw1-pwS7-gt1);
     dec2rgpulse(pwN*2.0,zero,0.0,0.0);

        h_shapedpulse("eburp2",4.0,3.5,zero, 2.0e-6, 0.0); 
	dec2rgpulse(pwN, t10, 0.0, 0.0);
      }

/**************************************************************************/ 
/** standard INEPT-based version                     **********************/
/**************************************************************************/ 
      else
      {
      if (ni < 1.0)
      {
       dec2rgpulse(pwN,t3,0.0,0.0);
       dec2rgpulse(pwN*2.0, zero, 0.0, 0.0);
       dec2rgpulse(pwN, zero, 0.0, 0.0);
      }
      else
      {
           if (tau1 < shpw1)
           {
           shaped_pulse(shname1,shpw1,two,0.0,0.0);
           shaped_pulse(shname1,shpw1,zero,0.0,0.0);
           dec2rgpulse(pwN,t3,0.0,0.0);
           delay(tau1);
           dec2rgpulse(pwN*3.0, zero, 0.0, 0.0);
           }
           else
           {
             if  (tau1*0.5 < (pwS7+shpw1*0.5))
             {
             shaped_pulse(shname1,shpw1,two,0.0,0.0);
             dec2rgpulse(pwN,t3,0.0,0.0);
             delay(tau1*0.5-shpw1*0.5);
             shaped_pulse(shname1,shpw1,zero,0.0,0.0);
             delay(tau1*0.5-shpw1*0.5);
             dec2rgpulse(pwN*3.0, zero, 0.0, 0.0);
             }
             else
             {
             shaped_pulse(shname1,shpw1,two,0.0,0.0);
             dec2rgpulse(pwN,t3,0.0,0.0);
             delay(tau1*0.5-shpw1*0.5);
             shaped_pulse(shname1,shpw1,zero,0.0,0.0);
             c_shapedpulse2("isnob5",40.0,-125.0,"isnob5",40.0,0.0 , two, 0.0, 0.0);
             delay(tau1*0.5-shpw1*0.5-pwS7);
             dec2rgpulse(pwN*3.0, zero, 0.0, 0.0);
             }
           } 
      }
      }
/**************************************************************************/ 

if (SE_flg[0] == 'y')
{
	zgradpulse(gzlvl5, gt5);
	delay(lambda-pwS6*0.4  - gt5);

   	h_sim3shapedpulse("reburp",4.0,3.5,0.0,2.0*pwN, zero, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl5, gt5);
	delay(lambda-pwS6*0.4  - gt5);

	dec2rgpulse(pwN, one, 0.0, 0.0);
  
        h_shapedpulse("eburp2_",4.0,3.5,one, 0.0, 0.0); 
 

	txphase(zero);
	dec2phase(zero);
	delay(lambda-pwS4*0.5-pwS6*0.4);

   	h_sim3shapedpulse("reburp",4.0,3.5,0.0,2.0*pwN, zero, zero, zero, 0.0, 0.0);

	dec2phase(t10);
	delay(lambda-pwS4*0.5-pwS6*0.4);

 
        h_shapedpulse("eburp2",4.0,3.5,zero, 0.0, 0.0); 


	delay((gt1/10.0) + 1.0e-4 +gstab  + 2.0*GRADIENT_DELAY + POWER_DELAY); 

        h_shapedpulse("reburp",4.0,3.5,zero, 0.0, 0.0); 
        zgradpulse(icosel*gzlvl2, gt1/10.0);            /* 2.0*GRADIENT_DELAY */
        delay(gstab);
}
else
{
        h_shapedpulse("eburp2",4.0,3.5,zero, 2.0e-6, 0.0);
        zgradpulse(gzlvl5, gt5);
        delay(lambda-pwS6*0.4  - gt5);

        h_sim3shapedpulse("reburp",4.0,3.5,0.0,2.0*pwN, zero, zero, zero, 0.0, 0.0);

        zgradpulse(gzlvl5, gt5);
        delay(lambda-pwS6*0.4  - gt5-POWER_DELAY-1.0e-4); 
}

	dec2power(dpwr2);				       /* POWER_DELAY */
lk_sample();
if (SE_flg[0] == 'y')
	setreceiver(t13);
else
	setreceiver(t12);
statusdelay(C,1.0e-4 );

}		 


