/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*************************************************/
/*   BEST-TROSY HBONDS (best_trosy_hbonds.c)     

See "Recovering magnetization: polarization enhancement in
biomolecular NMR", J. Biomol. NMR, 49,9-15(2011);*/

/*************************************************/

#include <standard.h>
#include "bionmr.h"
#include <Pbox_psg.h>


static int 

   phi1[4] = {0,2,0,2},
   phi2[4] = {2,2,0,0},
   phi3[1] = {3},
   phi5[1] ={2},
   phi14[4] = {0,2,2,0},
   phi15[4] = {0,2,2,0},
   phi24[4] = {2,0,0,2},
   phi25[4] = {2,0,0,2};






pulsesequence()
{
   char   
	  f1180[MAXSTR],
	  f2180[MAXSTR],
          n15_flg[MAXSTR];


   int    icosel,
          t1_counter,
          t2_counter,
          ni2 = getval("ni2"),
          phase;


   double d2_init=0.0,
          d3_init=0.0,
          pwS1,pwS2,pwS3,pwS4,pwS5,pwS6,
          kappa,
          lambda = getval("lambda"),
          gzlvl1 = getval("gzlvl1"),
          gzlvl2 = getval("gzlvl2"), 
          gzlvl3 = getval("gzlvl3"), 
          gzlvl4 = getval("gzlvl4"), 
          gzlvl5 = getval("gzlvl5"), 
          gzlvl6 = getval("gzlvl6"), 
          gt1 = getval("gt1"),
          gt2 = getval("gt2"),
          gt3 = getval("gt3"),
          gt4 = getval("gt4"),
          gt5 = getval("gt5"),
          gt6 = getval("gt6"),
          gstab = getval("gstab"),
          scale = getval("scale"),
          sw1 = getval("sw1"),
          tpwrsf = getval("tpwrsf"),
          shlvl1,
          pwClvl = getval("pwClvl"),
          pwNlvl = getval("pwNlvl"),
          pwN = getval("pwN"),
          dpwr2 = getval("dpwr2"),
          d2 = getval("d2"),
          t2a,t2b,halfT2,
          shbw = getval("shbw"),
          shofs = getval("shofs")-4.77,
          timeTN = getval("timeTN"),
          tau1 = getval("tau1"),
          tau2 = getval("tau2"),
          taunh = getval("taunh");



   getstr("n15_flg",n15_flg);



  phase = (int) (getval("phase") + 0.5);
   
   settable(t1,4,phi1);
   settable(t2,4,phi2);
   settable(t3,1,phi3);
   settable(t5,1,phi5);
   settable(t14,4,phi14);
   settable(t15,4,phi15);
   settable(t24,4,phi24);
   settable(t25,4,phi25);


/*   INITIALIZE VARIABLES   */
   kappa = 5.4e-3;
   shlvl1 = tpwr;
   f1180[0] ='n'; 
   f2180[0] ='n'; 

   pwS1 = c13pulsepw("co", "ca", "sinc", 90.0);
   pwS2 = c13pulsepw("co", "ca", "sinc", 180.0);
   pwS3 = c13pulsepw("ca", "co", "square", 180.0);
   pwS4 = h_shapedpw("eburp2",shbw,shofs,zero, 0.0, 0.0);
   pwS6 = h_shapedpw("reburp",shbw,shofs,zero, 0.0, 0.0);
   pwS5 = h_shapedpw("pc9f",shbw,shofs,zero, 2.0e-6, 0.0);




  if (phase == 1) ;
  if (phase == 2) tsadd(t1,1,4);

if   ( phase2 == 2 )
        {
        tsadd ( t3,2,4  );
        tsadd ( t5,2,4  );
        icosel = +1;
        }
else icosel = -1;


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

/************************************************************/
/* modification for phase-cycling in consecutive experiments*/
/*  for kinetic measurements                                */
/************************************************************/

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2)
        { tsadd(t1,2,4); tsadd(t14,2,4); tsadd(t15,2,4);tsadd(t24,2,4); tsadd(t25,2,4);  }

  if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2)
        { tsadd(t2,2,4); tsadd(t14,2,4); tsadd(t15,2,4);tsadd(t24,2,4); tsadd(t25,2,4);  }


/* Set up CONSTANT/SEMI-CONSTANT time evolution in N15 */

   if (ni2 > 1)
   {
   halfT2 = 0.5*(ni2-1)/sw2;
   t2b = (double) t2_counter*((halfT2 - timeTN)/((double)(ni2-1)));
    if(t2b < 0.0) t2b = 0.0;
    t2a = timeTN - tau2*0.5 + t2b;
    if(t2a < 0.2e-6)  t2a = 0.0;
    }
    else
    {
    t2b = 0.0;
    t2a = timeTN - tau2*0.5;
    }



   status(A);
      rcvroff();  

   decpower(pwClvl);
   decoffset(dof);
   dec2power(pwNlvl);
   dec2offset(dof2);
   obspwrf(tpwrsf);
   decpwrf(4095.0);
   obsoffset(tof);
   set_c13offset("co");


      dec2rgpulse(pwN*2.0,zero,0.0,0.0);
     zgradpulse(gzlvl4, gt4);
       delay(1.0e-4);

lk_sample();
       delay(d1-gt4);
lk_hold();
        h_shapedpulse("pc9f",shbw,shofs,zero, 2.0e-6, 0.0);

        delay(lambda-pwS5*0.5-pwS6*0.5);

        h_sim3shapedpulse("reburp",shbw,shofs,0.0,2.0*pwN, one, zero, zero, 0.0, 0.0);

        delay(lambda-pwS5*0.5-pwS6*0.5);

   if(n15_flg[0]=='y') h_shapedpulse("pc9f_",shbw,shofs,three, 0.0, 0.0);
     else h_shapedpulse("pc9f_",shbw,shofs,one, 0.0, 0.0);


   obspower(shlvl1);
/**************************************************************************/
      dec2rgpulse(pwN,zero,0.0,0.0);

           zgradpulse(gzlvl4, 0.8*gt4);
           delay(timeTN-pwS2*0.5-0.8*gt4);

      sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
                                             zero, zero, zero, 2.0e-6, 2.0e-6);

           zgradpulse(gzlvl4, 0.8*gt4);
           delay(timeTN-pwS2*0.5-0.8*gt4);
     dec2rgpulse(pwN,one,0.0,0.0);
/**************************************************************************/
/*   xxxxxxxxxxxxxxxxxxxxxx       13CO EVOLUTION        xxxxxxxxxxxxxxxxxx    */
   
        obspower(tpwr);
	c13pulse("co", "ca", "sinc", 90.0, t1, 2.0e-6, 0.0);       
        delay(tau1*0.5);
        sim3_c13pulse("", "ca", "co", "square", "", 0.0, 180.0, 2.0*pwN,
                                                  zero, zero, zero, 2.0e-6, 0.0);
        delay(tau1*0.5);
	c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 0.0);      
        sim3_c13pulse("", "ca", "co", "square", "", 0.0, 180.0, 0.0,
                                                  one, zero, zero, 2.0e-6, 0.0);
        if  (pwN*2.0 > pwS3) delay(pwN*2.0-pwS3);
	c13pulse("co", "ca", "sinc", 90.0, zero, 2.0e-6, 0.0);       

/**************************************************************************/
     dec2rgpulse(pwN,t2,0.0,0.0);

           delay(tau2*0.5);
         c13pulse("ca", "co", "square", 180.0, zero, 0.0, 0.0);
       zgradpulse(gzlvl3, gt3); 
       delay(1.0e-4);
         //delay(timeTN-pwS3-pwS2-gt1-2.0e-4-gt3);
        delay(timeTN-pwS3-pwS2-gt1-1.0e-4-2.0*GRADIENT_DELAY-4*POWER_DELAY-4*PWRF_DELAY-(4/PI)*pwN);
       zgradpulse(-gzlvl1, gt1); 
       delay(1.0e-4);
        c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);
        delay (t2b);
        dec2rgpulse (2.0*pwN, zero, 0.0, 0.0);
       zgradpulse(gzlvl3, gt3); 
       delay(1.0e-4);
        delay (t2a-gt3-1.0e-4);

/**************************************************************************/
        delay(gt1/10.0+1.0e-4);
        h_shapedpulse("eburp2_",shbw,shofs,t3, 2.0e-6, 0.0);

        zgradpulse(gzlvl5, gt5);
        delay(lambda-pwS6*0.5-pwS4*scale- gt5);

        h_sim3shapedpulse("reburp",shbw,shofs,0.0,2.0*pwN, zero, zero, zero, 0.0, 0.0);

        zgradpulse(gzlvl5, gt5);
        delay(lambda-pwS6*0.5-pwS4*scale- gt5);

        h_shapedpulse("eburp2",shbw,shofs,zero, 0.0, 0.0);
        delay(gt1/10.0+1.0e-4);

     dec2rgpulse(pwN,one,0.0,0.0);
        zgradpulse(gzlvl6, gt6);

        txphase(zero);
        delay(lambda-pwS6*0.5-gt6);

        h_sim3shapedpulse("reburp",shbw,shofs,0.0,2.0*pwN, zero, zero, zero, 0.0, 0.0);
        zgradpulse(gzlvl6, gt6);

        delay(lambda-pwS6*0.5-gt6);
     dec2rgpulse(pwN,t5,0.0,0.0);
/**************************************************************************/

        zgradpulse(-icosel*gzlvl2, gt1/10.0);
        dec2power(dpwr2);                                      /* POWER_DELAY */
        decpower(dpwr);                                      /* POWER_DELAY */
lk_sample();
 if (n15_flg[0] =='y')
{
   if (phase2==1) setreceiver(t14);
   else setreceiver(t15);
}
else
{
   if (phase2==1) setreceiver(t24);
   else setreceiver(t25);
}

      rcvron();
statusdelay(C,1.0e-4 );

}		 



       





