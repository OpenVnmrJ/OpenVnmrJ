/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
   BEST-HNCOCA 1H-15N-13COCA experiment   

   Correlates Ca(i) with N(i+1), NH(i+1).		

optimize tauC (CO->CA transfer) within range of 400usec to 4.5msec

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
 tpwr) are used to refocus NyHz coherence to Nx if href_flg is
 set to "y".

 Gradient sensitivity-enhanced (SE_flg='y', f2coef='1 0 -1 0 0
 1 0 1' ) and non sensitivity-enhanced (SE_flg='n', f2coef='1 0 0
 0 0 0 -1 0' ) versions are available.

 The flags f1180/f2180 should be set to 'y' if t1/t2 is to be
 started at half dwell time. This will give 90, -180 phasing in
 f1/f2. If they are set to 'n' the phasing should be 0,0 and will
 still give a flat baseline.

 phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition
 in t1 [C13]  and States-TPPI acquisition/ EchoAntiecho in t2
 [N15].

		* Schanda, Paul
		* Lescop, Ewen
		* Van Melckebeke, Hélène
		* Brutscher, Bernhard

Institut de Biologie Structurale, Laboratoire de RMN, 41, 
rue Jules Horowitz, 38027 Grenoble Cedex 1 FRANCE

see: - Schanda, P., Van Melckebeke, H. and Brutscher, B.,
       JACS,128,9042-9043(2006)
     - Lescop, E., Schanda, P. and Brutscher, B.,
       submitted (2007)
*/

#include <standard.h>
#include "bionmr.h"

static int 

   phi1[2] = {0,2},
   phi3[4] = {0,0,2,2},
   phi10[1] ={0},
   phi12[4] = {1,3,3,1},
   phi13[4] = {2,0,0,2};


pulsesequence()
{
   char   
          shname1[MAXSTR],
	  f1180[MAXSTR],
	  f2180[MAXSTR],
          SE_flg[MAXSTR];

   int    icosel = 0,
          t1_counter,
          t2_counter,
          ni2 = getval("ni2"),
          phase;


   double d2_init=0.0,
          d3_init=0.0,
          pwS1,pwS2,pwS3,pwS4,pwS5,pwS6,
          lambda = getval("lambda"),
          gzlvl1 = getval("gzlvl1"),
          gzlvl2 = getval("gzlvl2"), 
          gzlvl3 = getval("gzlvl3"), 
          gzlvl4 = getval("gzlvl4"), 
          gzlvl5 = getval("gzlvl5"), 
          gzlvl6 = getval("gzlvl6"), 
          gt1 = getval("gt1"),
          gt3 = getval("gt3"),
          gt4 = getval("gt4"),
          gt5 = getval("gt5"),
          gt6 = getval("gt6"),
          gstab = getval("gstab"),
          shlvl1 = getval("shlvl1"),
          shpw1 = getval("shpw1"),
          pwClvl = getval("pwClvl"),
          pwC = getval("pwC"),
          pwNlvl = getval("pwNlvl"),
          pwN = getval("pwN"),
          d2 = getval("d2"),
          timeTN = getval("timeTN"),
          tauC = getval("tauC"),
          tau1 = getval("tau1"),
          tau2 = getval("tau2"),
          taunh = getval("taunh");

   getstr("shname1", shname1);
   getstr("SE_flg",SE_flg);
   getstr("f1180",f1180);
   getstr("f2180",f2180);

  phase = (int) (getval("phase") + 0.5);
   
   settable(t1,2,phi1);
   settable(t3,4,phi3);
   settable(t10,1,phi10);
   settable(t12,4,phi12);
   settable(t13,4,phi13);

/*   INITIALIZE VARIABLES   */

   pwS1 = c13pulsepw("co", "ca", "sinc", 90.0);
   pwS2 = c13pulsepw("co", "ca", "sinc", 180.0);
   pwS3 = c13pulsepw("ca", "co", "square", 180.0);
   pwS4 = h_shapedpw("eburp2",4.0,3.5,zero, 0.0, 0.0);  
   pwS6 = h_shapedpw("reburp",4.0,3.5,zero, 0.0, 0.0);
   pwS5 = h_shapedpw("pc9f",4.0,3.5,zero, 2.0e-6, 0.0);


if (SE_flg[0] == 'y')
{
   if ( ni2*1/(sw2)/2.0 > (timeTN-pwS2*0.5-pwS4))
       { printf(" ni2 is too big. Make ni2 equal to %d or less.\n",
         ((int)((timeTN-pwS2*0.5-pwS4)*2.0*sw2)));    psg_abort(1);}
}
else
{
   if ( ni2*1/(sw2)/2.0 > (timeTN-pwS2*0.5))
       { printf(" ni2 is too big. Make ni2 equal to %d or less.\n",
         ((int)((timeTN-pwS2*0.5)*2.0*sw2)));    psg_abort(1);}
}



  if (phase == 1) ;
  if (phase == 2) tsadd(t1,1,4);

if (SE_flg[0] =='y')
{
  if (phase2 == 2)  {tsadd(t10,2,4); icosel = +1;}
            else                               icosel = -1;
}
else
{
  if (phase2 == 2) {tsadd(t3,1,4); icosel =1;}
}

 

    tau1 = d2;
    if((f1180[A] == 'y') )
        { tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1;

   tau2 = d3;
    if((f2180[A] == 'y') )
        { tau2 += ( 1.0 / (2.0*sw2) ); if(tau2 < 0.2e-6) tau2 = 0.0; }
    tau2 = tau2;

  
    


   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2)
        { tsadd(t1,2,4);  tsadd(t12,2,4); tsadd(t13,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2)
        { tsadd(t3,2,4); tsadd(t12,2,4); tsadd(t13,2,4); }



   status(A);
   decpower(pwClvl);
   dec2power(pwNlvl);
   set_c13offset("co");
   zgradpulse(gzlvl6, gt6);
   delay(1.0e-4);
   delay(d1-gt6);
lk_hold();
   rcvroff();

        h_shapedpulse("pc9f",4.0,3.5,zero, 2.0e-6, 0.0);  

	delay(lambda-pwS5*0.5-pwS6*0.4); 

   	h_sim3shapedpulse("reburp",4.0,3.5,0.0,2.0*pwN, one, zero, zero, 0.0, 0.0);

	delay(lambda-pwS5*0.5-pwS6*0.4);

        h_shapedpulse("pc9f_",4.0,3.5,one, 0.0, 0.0);  


   obspower(shlvl1);
/**************************************************************************/
      dec2rgpulse(pwN,zero,0.0,0.0);

           zgradpulse(gzlvl3, gt3);
           delay(timeTN-pwS2*0.5-gt3);

      sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
                                             zero, zero, zero, 2.0e-6, 2.0e-6);

           delay(timeTN-pwS2*0.5-taunh*0.5-shpw1);
           shaped_pulse(shname1,shpw1,two,0.0,0.0);
           zgradpulse(gzlvl3, gt3);
           delay(taunh*0.5-gt3);

     dec2rgpulse(pwN,zero,0.0,0.0);				     
           shaped_pulse(shname1,shpw1,zero,0.0,0.0);
/**************************************************************************/
/***        CO -> CA transfer             *********************************/
/**************************************************************************/
        c13pulse("co", "ca", "sinc", 90.0, zero, 2.0e-6, 0.0);

        zgradpulse(-gzlvl4, gt4);
        decphase(zero);
        delay(tauC - gt4 - 0.5*10.933*pwC);

        decrgpulse(pwC*158.0/90.0, zero, 0.0, 0.0);
        decrgpulse(pwC*171.2/90.0, two, 0.0, 0.0);
        decrgpulse(pwC*342.8/90.0, zero, 0.0, 0.0);      /* Shaka 6 composite */
        decrgpulse(pwC*145.5/90.0, two, 0.0, 0.0);
        decrgpulse(pwC*81.2/90.0, zero, 0.0, 0.0);
        decrgpulse(pwC*85.3/90.0, two, 0.0, 0.0);

        zgradpulse(-gzlvl4, gt4);
        decphase(one);
        delay(tauC - gt4 - 0.5*10.933*pwC - WFG_START_DELAY - 2.0e-6);
        c13pulse("co", "ca", "sinc", 90.0, one, 2.0e-6, 0.0);
/**************************************************************************/
/*  xxxxxxxxxxxxxxxxxxxx       13Ca EVOLUTION       xxxxxxxxxxxxxxxxxx    */
/**************************************************************************/
           shaped_pulse(shname1,shpw1,two,0.0,0.0);
        set_c13offset("ca");

        c13pulse("ca", "co", "square", 90.0, t1, 2.0e-6, 2.0e-6);   
        delay(tau1*0.5);
        sim3_c13pulse(shname1, "co", "ca", "sinc", "", shpw1, 180.0, 2.0*pwN,
                                             zero, zero, zero, 2.0e-6, 2.0e-6);
        delay(tau1*0.5);

        c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 2.0e-6);
        c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 2.0e-6);   
        if (pwN*2.0 > pwS2)  delay(pwN*2.0 - pwS2);

        c13pulse("ca", "co", "square", 90.0, zero, 2.0e-6, 2.0e-6);   

        set_c13offset("co");
/**************************************************************************/
/***        CO -> CA transfer             *********************************/
/**************************************************************************/
       c13pulse("co", "ca", "sinc", 90.0, one, 2.0e-6, 0.0);

        zgradpulse(gzlvl4, gt4*0.7);
        delay(tauC - gt4*0.7 - 0.5*10.933*pwC);

        decrgpulse(pwC*158.0/90.0, zero, 0.0, 0.0);
        decrgpulse(pwC*171.2/90.0, two, 0.0, 0.0);
        decrgpulse(pwC*342.8/90.0, zero, 0.0, 0.0);     /* Shaka 6 composite */
        decrgpulse(pwC*145.5/90.0, two, 0.0, 0.0);
        decrgpulse(pwC*81.2/90.0, zero, 0.0, 0.0);
        decrgpulse(pwC*85.3/90.0, two, 0.0, 0.0);

        zgradpulse(gzlvl4, gt4*0.7);
        delay(tauC - gt4*0.7 - 0.5*10.933*pwC - WFG_START_DELAY - 2.0e-6);
                                                           /* WFG_START_DELAY */
        c13pulse("co", "ca", "sinc", 90.0, zero, 2.0e-6, 0.0);



/**************************************************************************/

   obspower(shlvl1);
        shaped_pulse(shname1,shpw1,zero,0.0,0.0);

     dec2rgpulse(pwN,t3,0.0,0.0);

      if (SE_flg[0] == 'y')
      {
           delay(tau2*0.5);
        c13pulse("ca", "co", "square", 180.0, zero, 0.0, 0.0);
          delay(taunh*0.5-pwS3);
        shaped_pulse(shname1,shpw1,two,0.0,0.0);
          delay(timeTN-pwS2*0.5-shpw1-taunh*0.5-gt1-1.0e-4);

        zgradpulse(-gzlvl1, gt1);
        delay(1.0e-4);
      sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
                                             zero, zero, zero, 2.0e-6, 2.0e-6);
           delay(timeTN-pwS2*0.5-tau2*0.5-pwS4);
        h_shapedpulse("eburp2",4.0,3.5,zero, 2.0e-6, 0.0); 
	dec2rgpulse(pwN, t10, 0.0, 0.0);
      }
      else
      {
           delay(tau2*0.5);
        c13pulse("ca", "co", "square", 180.0, zero, 0.0, 0.0);
          delay(taunh*0.5-pwS3);
        shaped_pulse(shname1,shpw1,two,0.0,0.0);
          delay(timeTN-pwS2*0.5-taunh*0.5-shpw1);

      sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
                                             zero, zero, zero, 2.0e-6, 2.0e-6);
       delay(timeTN-pwS2*0.5-tau2*0.5);
       dec2rgpulse(pwN, zero, 0.0, 0.0);
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
      rcvron();  
statusdelay(C,1.0e-4 );

}		 

 
