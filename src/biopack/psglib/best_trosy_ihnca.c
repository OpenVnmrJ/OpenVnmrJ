/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*************************************************/
/*   BEST TROSY -intraHNCA (best_trosy_ihnca.c)  */
/*
See "Recovering magnetization: polarization enhancement in
biomolecular NMR", J. Biomol. NMR, 49,9-15(2011);
*/
/*************************************************/

#include <standard.h>
#include "bionmr.h"
#include <Pbox_psg.h>



static int 

   phi1[2] = {0,2},
   phi3[1] = {3},
  phi4[4] = {2,2,0,0}, 
   phi5[1] ={2},
   phi14[4] = {0,2,2,0},
   phi24[4] = {2,0,0,2};


pulsesequence()
{
   char   
          shname1[MAXSTR],
	  f1180[MAXSTR],
	  f2180[MAXSTR],
          CT_flg[MAXSTR],
          n15_flg[MAXSTR];

   int    icosel,
          t1_counter,
          t2_counter,
          ni2 = getval("ni2"),
          phase;


   double d2_init=0.0,
          d3_init=0.0,
          pwS1,pwS2,pwS3,pwS4,pwS5,pwS6,pwS7,max,
          kappa,
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
          tpwrsf = getval("tpwrsf"),
          shlvl1,
          shpw1 = getval("shpw1"),
          pwClvl = getval("pwClvl"),
          pwNlvl = getval("pwNlvl"),
          pwN = getval("pwN"),
          dpwr2 = getval("dpwr2"),
          d2 = getval("d2"),
          shbw = getval("shbw"),
          shofs = getval("shofs")-4.77,
          scale = getval("scale"),
          sw1 = getval("sw1"),
          timeTN = getval("timeTN"),
          timeTN1,
          Delta,
          t2a,t2b,halfT2,ctdelay,
          tauNCO = getval("tauNCO"),
          CTdelay = getval("CTdelay"),
          tauC = getval("tauC"),
          tau1, tau2,
          taunh = getval("taunh");



   getstr("shname1", shname1);
   getstr("CT_flg",CT_flg);
   getstr("n15_flg",n15_flg);
   getstr("f1180",f1180);
   getstr("f2180",f2180);



  phase = (int) (getval("phase") + 0.5);
   
   settable(t1,2,phi1);
   settable(t3,1,phi3);
   settable(t4,4,phi4);
   settable(t5,1,phi5);
   settable(t14,4,phi14);
   settable(t24,4,phi24);


/*   INITIALIZE VARIABLES   */

  timeTN1= timeTN-tauC;
  Delta = timeTN-tauC-tauNCO;

  //shpw1 = pw*8.0;
  shlvl1=tpwr;

   pwS1 = c13pulsepw("ca", "co", "square", 90.0);
   pwS2 = c13pulsepw("ca", "co", "square", 180.0);
   pwS3 = c13pulsepw("co", "ca", "sinc", 180.0);
   pwS7 = c13pulsepw("co", "ca", "sinc", 90.0);
   pwS4 = h_shapedpw("eburp2",shbw,shofs,zero, 0.0, 0.0);  
   pwS6 = h_shapedpw("reburp",shbw,shofs,zero, 0.0, 0.0);
   pwS5 = h_shapedpw("pc9f",shbw,shofs,zero, 2.0e-6, 0.0);



if (CT_flg[0] == 'y')
{
   if ( ni*1/(sw1)/2.0 > (CTdelay*0.5-gt3-1.0e-4))
       { printf(" ni is too big. Make ni equal to %d or less.\n",
         ((int)((CTdelay*0.5-gt3-1.0e-4)*2.0*sw1)));    psg_abort(1);}
}

  if (phase == 1) ;
  if (phase == 2) {tsadd(t1,1,4);}

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
   
   
/*  Set up f2180  */
   
    tau2 = d3;
    if((f2180[A] == 'y') && (ni2 > 1.0))
        { tau2 += ( 1.0 / (2.0*sw2) ); if(tau2 < 0.2e-6) tau2 = 0.0; }

/************************************************************/

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2)
        { tsadd(t1,2,4); tsadd(t14,2,4); tsadd(t24,2,4); }
   
  if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2)
        { tsadd(t4,2,4); tsadd(t14,2,4); tsadd(t24,2,4);  }
   
/************************************************************/
/* Set up CONSTANT/SEMI-CONSTANT time evolution in N15 */
/************************************************************/

   ctdelay = timeTN1-gt1-1.0e-4;
   // ctdelay = timeTN1-gt1-1.0e-4-2.0*GRADIENT_DELAY-4*POWER_DELAY-4*PWRF_DELAY-(4/PI)*pwN;
   if (ni2 > 1)
   {
   halfT2 = 0.5*(ni2-1)/sw2;
   t2b = (double) t2_counter*((halfT2 - ctdelay)/((double)(ni2-1)));
   if( ix==1 && halfT2 - timeTN > 0 ) printf("SCT mode on, max ni2=%g\n",timeTN*sw2*2+1);
    if(t2b < 0.0) t2b = 0.0;
   
    t2a = ctdelay - tau2*0.5 + t2b;
    if(t2a < 0.2e-6)  t2a = 0.0;
    }
    else
    {
    t2b = 0.0;
    t2a = ctdelay - tau2*0.5;
    }
/************************************************************/

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

       delay(d1-gt4);
lk_hold();

        h_shapedpulse("pc9f",shbw,shofs,zero, 2.0e-6, 0.0);  

	delay(lambda-pwS5*0.5-pwS6*0.4); 

   	h_sim3shapedpulse("reburp",shbw,shofs,0.0,2.0*pwN, one, zero, zero, 0.0, 0.0);

	delay(lambda-pwS5*0.5-pwS6*0.4);

     if(n15_flg[0]=='y') h_shapedpulse("pc9f_",shbw,shofs,three, 0.0, 0.0);
     else h_shapedpulse("pc9f_",shbw,shofs,one, 0.0, 0.0);

           zgradpulse(gzlvl4, gt4*4.0);
           delay(1.0e-4);

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
           delay(tauC);
        c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 2.0e-6);
	c13pulse("co", "ca", "sinc", 90.0, one, 0.0, 0.0);      
        

     dec2rgpulse(pwN,one,0.0,0.0);				     
/**************************************************************************/
/*   xxxxxxxxxxxxxxxxxxxxxx       13CA EVOLUTION        xxxxxxxxxxxxxxxxxx    */
/**************************************************************************/
        set_c13offset("ca");

        c13pulse("ca", "co", "square", 90.0, t1, 2.0e-6, 0.0);
   if(CT_flg[0]=='y')
   {
        delay(tau1*0.5);
         sim3_c13pulse(shname1, "co", "ca", "sinc", "", shpw1, 180.0, 2.0*pwN,
                                             zero, zero, zero, 2.0e-6, 2.0e-6);
        zgradpulse(gzlvl3, gt3);
        delay(1.0e-4);
        delay(CTdelay*0.5-gt3-1.0e-4);
        c13pulse("cab", "co", "square", 180.0, zero, 2.0e-6, 2.0e-6);
        delay(CTdelay*0.5-gt3-1.0e-4-tau1*0.5);
         sim3_c13pulse(shname1, "co", "ca", "sinc", "", shpw1, 180.0, 2.0*pwN,
                                             zero, zero, zero, 2.0e-6, 2.0e-6);
        zgradpulse(gzlvl3, gt3);
        delay(1.0e-4);
   }
   else
   {
        delay(tau1*0.5);
        sim3_c13pulse(shname1, "co", "ca", "sinc", "", shpw1, 180.0, 2.0*pwN,
                                             zero, zero, zero, 2.0e-6, 2.0e-6);
        zgradpulse(gzlvl3, gt3);
        delay(1.0e-4);
        delay(tau1*0.5);

        c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 2.0e-6);
        sim3_c13pulse(shname1, "co", "ca", "sinc", "", shpw1, 180.0, 2.0*pwN,
                                             zero, zero, zero, 2.0e-6, 2.0e-6);
        zgradpulse(gzlvl3, gt3);
        delay(1.0e-4);
   }
        c13pulse("ca", "co", "square", 90.0, zero, 0.0, 0.0);

        set_c13offset("co");

/**************************************************************************/
/*   xxxxxxxxxxxxxxxxxxxxxx   N-> CA back transfer     xxxxxxxxxxxxxxx    */
/**************************************************************************/
   obspower(shlvl1);

     dec2rgpulse(pwN,t4,0.0,0.0);

	c13pulse("co", "ca", "sinc", 90.0, one, 0.0, 0.0);      
        c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 2.0e-6);
           delay(tauC);
        sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
                                             zero, zero, zero, 2.0e-6, 2.0e-6);
        c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 2.0e-6);
           delay(tauC);
	c13pulse("co", "ca", "sinc", 90.0, zero, 0.0, 0.0);      

 	 // delay(timeTN1-Delta+tau2*0.5-pwS2-pwS3);
 	 delay(timeTN1-Delta+tau2*0.5-pwS2-pwS3-2.0*GRADIENT_DELAY+4*POWER_DELAY+4*PWRF_DELAY);
	c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);      
           delay(Delta);
	c13pulse("ca", "co", "square", 180.0, zero, 0.0, 0.0);      
           delay (t2b);
        dec2rgpulse (2.0*pwN, zero, 0.0, 0.0);
       zgradpulse(gzlvl1, gt1);
           delay(1.0e-4);
           delay (t2a);

/**************************************************************************/
/**  gradient-selected TROSY sequence                             *********/
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

	dec2power(dpwr2);
lk_sample();
 if (n15_flg[0] =='y')
{
   setreceiver(t14);
}
else
{
   setreceiver(t24);
}
      rcvron();  
statusdelay(C,1.0e-4 );

}		 





