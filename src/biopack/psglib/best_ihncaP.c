/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 BEST-intraHNCA 1H-15N-13CA experiment       
set f2coef='1 0 -1 0 0 0 1 0 1' for SE_flg='y'
and f2coef='' for SE_flg='n' for Vnmr/VnmrJ
processing
*/
#include <standard.h>
#include "bionmr.h"

static int 

   phi1[2] = {0,2},
   phi3[4] = {0,0,2,2},
   phi10[1] ={0},
   phi12[4] = {1,3,3,1},
   phi13[4] = {2,0,0,2};

void pulsesequence()
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
          pwS1,pwS2,pwS3,pwS4,pwS5,pwS6,pwS7,
          lambda = getval("lambda"),
          gzlvl1 = getval("gzlvl1"),
          gzlvl2 = getval("gzlvl2"), 
          gzlvl6 = getval("gzlvl6"), 
          gzlvl5 = getval("gzlvl5"), 
          gt1 = getval("gt1"),
          gt6 = getval("gt6"),
          gt5 = getval("gt5"),
          gstab = getval("gstab"),
          shlvl1 = getval("shlvl1"),
          shpw1 = getval("shpw1"),
          pwClvl = getval("pwClvl"),
          pwNlvl = getval("pwNlvl"),
          pwN = getval("pwN"),
          d2 = getval("d2"),
          timeTN = getval("timeTN"),
          timeTN1,
          Delta,
          tauNCO = getval("tauNCO"),
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

  timeTN1= timeTN-tauC;
  Delta = timeTN-tauC-tauNCO;

   pwS1 = c13pulsepw("ca", "co", "square", 90.0);
   pwS2 = c13pulsepw("ca", "co", "square", 180.0);
   pwS3 = c13pulsepw("co", "ca", "sinc", 180.0);
   pwS7 = c13pulsepw("co", "ca", "sinc", 90.0);
   pwS4 = h_shapedpw("eburp2",4.0,3.5,zero, 0.0, 0.0);  
   pwS5 = h_shapedpw("reburp",4.0,3.5,zero, 0.0, 0.0);
   pwS6 = h_shapedpw("pc9f",4.0,3.5,zero, 2.0e-6, 0.0);



if (SE_flg[0] == 'y')
{
   if ( ni2*1/(sw2)/2.0 > (timeTN1-Delta-pwS3-pwS4))
       { printf(" ni2 is too big. Make ni2 equal to %d or less.\n",
         ((int)((timeTN1-Delta-pwS3-pwS4)*2.0*sw2)));    psg_abort(1);}
}
else
{

   if ( ni2*1/(sw2)/2.0 > (timeTN1-Delta-pwS3))
       { printf(" ni2 is too big. Make ni2 equal to %d or less.\n",
         ((int)((timeTN1-Delta-pwS3)*2.0*sw2)));    psg_abort(1);}
}



  if (phase == 1) ;
  if (phase == 2) tsadd(t1,1,4);

if (SE_flg[0] =='y')
{
  if (phase2 == 2)  {tsadd(t10,2,4); icosel = +1;}
            else 			       icosel = -1;   
}
else
{
  if (phase2 == 2) {tsadd(t3,1,4); icosel = 1;}
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
        { tsadd(t1,2,4); tsadd(t12,2,4); tsadd(t13,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2)
        { tsadd(t3,2,4); tsadd(t12,2,4); tsadd(t13,2,4); }



   status(A);
   decpower(pwClvl);
   dec2power(pwNlvl);
   set_c13offset("co");

   zgradpulse(gzlvl6, gt6);
   delay(d1-gt6 +1.0e-4);
lk_hold();
     rcvroff();
        h_shapedpulse("pc9f",4.0,3.5,zero, 2.0e-6, 0.0);  

	delay(lambda-pwS5*0.5-pwS6*0.4); 

   	h_sim3shapedpulse("reburp",4.0,3.5,0.0,2.0*pwN, one, zero, zero, 0.0, 0.0);

	delay(lambda-pwS5*0.5-pwS6*0.4);

        h_shapedpulse("pc9f_",4.0,3.5,one, 2.0e-6, 0.0);  


   obspower(shlvl1);
/**************************************************************************/
/*   xxxxxxxxxxxxxxxxxxxxxx   N-> CA transfer           xxxxxxxxxxxxxxxxxx    */
/**************************************************************************/
      dec2rgpulse(pwN,zero,0.0,0.0);

           delay(timeTN1);

      sim3_c13pulse("", "ca", "co", "square", "", 0.0, 180.0, 2.0*pwN,
                                             zero, zero, zero, 2.0e-6, 2.0e-6);

           delay(Delta);
	c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);      
           delay(timeTN1-Delta-pwS3+pwN*4.0/3.0);

	c13pulse("co", "ca", "sinc", 90.0, zero, 0.0, 0.0);      
           delay(tauC);
        c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 2.0e-6);
        sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
                                             zero, zero, zero, 2.0e-6, 2.0e-6);
           delay(tauC-taunh*0.5-shpw1+pwS2+pwS7);
        shaped_pulse(shname1,shpw1,zero,0.0,0.0);
           delay(taunh*0.5-pwS2-pwS7);
        c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 2.0e-6);
	c13pulse("co", "ca", "sinc", 90.0, one, 0.0, 0.0);      
        

     dec2rgpulse(pwN,zero,0.0,0.0);				     
/**************************************************************************/
/*   xxxxxxxxxxxxxxxxxxxxxx       13CA EVOLUTION        xxxxxxxxxxxxxxxxxx    */
/**************************************************************************/
   set_c13offset("ca");
   
	c13pulse("ca", "co", "square", 90.0, t1, 2.0e-6, 0.0);       
        delay(tau1*0.5);
        sim3_c13pulse(shname1, "co", "ca", "sinc", "", shpw1, 180.0, 2.0*pwN,
                                                  zero, zero, zero, 0.0, 0.0);
        delay(tau1*0.5);
	c13pulse("ca", "co", "square", 180.0, zero, 0.0, 0.0);      

        sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 0.0,
                                                   zero, zero, zero, 0.0, 0.0);
        if (pwN*2.0 > pwS3) delay(pwN*2.0-pwS3);
	c13pulse("ca", "co", "square", 90.0, zero, 0.0, 0.0);       

   set_c13offset("co");
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
   obspower(shlvl1);

     dec2rgpulse(pwN,t3,0.0,0.0);

	c13pulse("co", "ca", "sinc", 90.0, one, 0.0, 0.0);      
        c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 2.0e-6);
           delay(tauC);
        sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
                                             zero, zero, zero, 2.0e-6, 2.0e-6);
        c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 2.0e-6);
           delay(tauC-taunh*0.5-shpw1);
        shaped_pulse(shname1,shpw1,zero,0.0,0.0);
           delay(taunh*0.5);
	c13pulse("co", "ca", "sinc", 90.0, zero, 0.0, 0.0);      
/**************************************************************************/

      if (SE_flg[0] == 'y')
      {
 	   delay(tau2*0.5);
        shaped_pulse(shname1,shpw1,two,0.0,0.0);
           delay(timeTN1+pwN*4.0/3.0-shpw1-gt1-1.0e-4);
        zgradpulse(-gzlvl1, gt1);
        delay(1.0e-4);
      sim3_c13pulse("", "ca", "co", "square", "", 0.0, 180.0, 2.0*pwN,
                                             zero, zero, zero, 2.0e-6, 2.0e-6);
       delay(Delta);
	c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);      
           delay(timeTN1-tau2*0.5-pwS4-Delta-pwS3);
        h_shapedpulse("eburp2",4.0,3.5,zero, 2.0e-6, 0.0); 
	dec2rgpulse(pwN, t10, 0.0, 0.0);
      }
      else
      {
 	   delay(tau2*0.5);
        shaped_pulse(shname1,shpw1,two,0.0,0.0);
           delay(timeTN1+pwN*4.0/3.0-shpw1);
      sim3_c13pulse("", "ca", "co", "square", "", 0.0, 180.0, 2.0*pwN,
                                             zero, zero, zero, 2.0e-6, 2.0e-6);
       delay(Delta);
	c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);      
       delay(timeTN1-tau2*0.5-Delta-pwS3);
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

