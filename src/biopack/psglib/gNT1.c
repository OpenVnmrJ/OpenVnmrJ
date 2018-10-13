/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gNT1.c     Heteronuclear T1 with TROSY option

	written by Youlin Xia, HKUST on Jan. 20, 1999.
	
	The pulse sequence is used to measure T1 of 15N.
	
Note:   
        
1       Tdelay should be even times of 2.0*delta, here delta=2.5ms.
         
2       Shapedpulse shape_ss is used to 
        invert NH resonance,not disturbing H2O. Its shape is produced by
        the proteincal macro at installation of BioPack as cosine-
        modulated pulse with 180's +-4ppm from H2O. The pw_shpss is
        calculated by the setup macro "gNT1" to match the stored shape.
        Its power ,shss_pwr, is calculated in the 
        pulse sequence. 
        
3       Set ncyc > 0 (even number) to vary 15N T1 relaxation period (approximately
        ncyc*5ms).

        Note: ncyc should be even

4.      Flipback pulses are calculated and if tpwrsf<4095 the log attenuator is
        increased by 6db and the fine attenuator is set around 2048. This fine
        power can then be varied to correct for the effects of radiation damping.

5.      Watergate is used for solvent suppression during the last spin-echo.

Reference:
	   
	1.   Neil A.F., ...., Lewis E.K., Biochemistry 33, 5984-6003(1994)
	   
	2.   K. Pervushin,R.Riek,G.Wider,K.Wuthrich,
	     Proc.Natl.Acad.Sci.USA,94,12366-71,1997.
	
	3.   G. Zhu, Y. Xia, L.K. Nicholson, and K.H. Sze,
	     J. Magn. Reson. 143, 423-426(2000).
	
Please quote Reference 3 if you use this pulse sequence
Modified for BioPack by GG, Palo Alto, May 2002
		
*/

#include <standard.h>

static double d2_init = 0.0;
 
static int phi1[8] = {0,0,0,0,2,2,2,2},
           phi2[4] = {1,0,3,2},
           phi3[1] = {1}, 
           phi10[8]= {0,2,0,2,0,2,0,2},

            rec[8] = {0,3,2,1,2,1,0,3};
            
           
pulsesequence()
{
/* DECLARE VARIABLES */

 char        shape_ss[MAXSTR];
             
 int	     t1_counter;

 double   
             tau1,                  /* t1/2  */
  	     taua = getval("taua"),     /* 2.25ms  */
  	     taub = getval("taub"),     /* 2.75ms  */
             time_T1,
             pwN,                   /* PW90 for N-nuc            */
             pwNlvl,                /* power level for N hard pulses */
        ncyc = getval("ncyc"),
   	compH= getval("compH"),
   	pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
   	tpwrs ,                    /* power for the pwHs ("H2Osinc") pulse */
   	tpwrsf ,                   /* fine power for the pwHs ("H2Osinc") pulse */
        shss_pwr,             /* power for cos modulated NH pulses */
        pw_shpss=getval("pw_shpss"),
        waterdly,              /* pw for water pulse  */
        waterpwrf,             /* fine power for water pulse  */
        waterpwr,              /* power for water pulse  */
  gt0,
  gt1 = getval("gt1"),
  gt2 = getval("gt2"),
  gt3 = getval("gt3"),
  gt4 = getval("gt4"),
  gt5 = getval("gt5"),
  gt6 = getval("gt6"),
  gzlvl0 = getval("gzlvl0"),
  gzlvl1 = getval("gzlvl1"),
  gzlvl2 = getval("gzlvl2"),
  gzlvl3 = getval("gzlvl3"),
  gzlvl4 = getval("gzlvl4"),
  gzlvl5 = getval("gzlvl5"),
  gzlvl6 = getval("gzlvl6");

/* LOAD VARIABLES */

  pwN = getval("pwN");
  pwNlvl = getval("pwNlvl"); 
  tpwrsf = getval("tpwrsf");
  waterpwrf = getval("waterpwrf");
  waterdly = getval("waterdly");


  getstr("shape_ss",shape_ss);
  
  time_T1=ncyc*(2.0*2.5e-3+pw_shpss);
  
  if (ix==1) printf(" ncyc= %f,   time_T1= %f \n", ncyc,time_T1);    


    /* selective H20 one-lobe sinc pulse */
        tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /* needs 1.69 times more */
	tpwrs = (int) (tpwrs);                   /* power than a square pulse */

    /* selective H20 watergate pulse */
        waterpwr = tpwr - 20.0*log10(waterdly/(compH*pw));  
	waterpwr = (int) (waterpwr);        

    /* selective cos modulated NH 180 degree pulse */
        shss_pwr = tpwr - 20.0*log10(pw_shpss/((compH*2*pw)*2));   /* needs 2 times more */
	shss_pwr = (int) (shss_pwr);                   /* power than a square pulse */




/* check validity of parameter range */

    if((dm[A] == 'y' || dm[B] == 'y' ))
	{
	printf("incorrect Dec1 decoupler flags!  ");
	psg_abort(1);
    } 

    if (dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y')
	{
	printf("incorrect Dec2 decoupler flag! dm2 should be 'nnn' ");
	psg_abort(1);
    } 

    if (dmm2[A] == 'g' || dmm2[B] == 'g' || dmm2[C] == 'g')
	{
	printf("incorrect Dec2 decoupler flag! dmm2 should be 'ccc' ");
	psg_abort(1);
    } 

    if( dpwr > 50 )
    {
	printf("don't fry the probe, dpwr too large!  ");
	psg_abort(1);
    }

    if( dpwr2 > 50 )
    {
	printf("don't fry the probe, dpwr2 too large!  ");
	psg_abort(1);
    }

    if(gt1 > 15.0e-3 || gt2 > 15.0e-3 || gt3 > 15.0e-3 || gt4 > 15.0e-3)
    {
        printf("gti must be less than 15 ms \n");
        psg_abort(1);
    }


/* LOAD VARIABLES */

  settable(t1, 8, phi1);
  settable(t2, 4, phi2);
  settable(t3, 1, phi3);
  settable(t10, 8, phi10);

  settable(t14, 8, rec);
  
  

/* Phase incrementation for hypercomplex data */

   if ( phase1 == 2 )     /* Hypercomplex in t1 */
   {     ttadd(t14,t10,4);
         tsadd(t3,2,4); 
    }

       
/* calculate modification to phases based on current t1 values
   to achieve States-TPPI acquisition */
 
 
   if(ix == 1)
      d2_init = d2;
      t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);

      tau1=0.5*d2;	
	
      if(t1_counter %2) {
        tsadd(t2,2,4);
        tsadd(t14,2,4);
      }
      
 
/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tpwr);               /* Set power for pulses  */
   dec2power(pwNlvl);            /* Set decoupler2 power to pwNlvl */

   initval(ncyc+0.1,v10);  /* for DIPSI-2 */

 delay(d1);

status(B);
  rcvroff();

/*destroy N15  magnetization*/	

   	dec2rgpulse(pwN, zero, 0.0, 0.0);

	zgradpulse(gzlvl1, gt1);
	delay(9.0e-5);

/*  1H-15N INEPT  */

  rgpulse(pw,zero,1.0e-6,0.0);    

  txphase(zero);  dec2phase(zero);
  zgradpulse(gzlvl2,gt2);
  delay(taua -pwN-0.5*pw  -gt2 );               /* delay=1/4J(NH)   */

  sim3pulse(2.0*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);

  txphase(one);  dec2phase(t1);
  zgradpulse(gzlvl2,gt2);
  delay(taua -1.5*pwN  -gt2);               /* delay=1/4J(NH)   */

  sim3pulse(pw,0.0e-6,pwN,one,zero,t1,0.0,0.0);


  if (tpwrsf < 4095.0)
     {obspwrf(tpwrsf); tpwrs=tpwrs+6.0;}

	obspower(tpwrs); 
   	shaped_pulse("H2Osinc", pwHs, two, 2.0e-6, 0.0);   
	obspower(tpwr); obspwrf(4095.0); tpwrs=tpwrs-6.0; 
 
  txphase(zero);  dec2phase(zero);
  zgradpulse(gzlvl3,gt3);
  delay(taub -1.5*pwN  -gt3 -pwHs-2.0e-6-2.0*POWER_DELAY-WFG_START_DELAY);               /* delay=1/4J(NH)   */

  sim3pulse(2.0*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);

  dec2phase(one);
  zgradpulse(gzlvl3,gt3);
  delay(taub -1.5*pwN  -gt3 );               /* delay=1/4J(NH)   */
  dec2phase(one);

  dec2rgpulse(pwN,one,0.0,0.0);
  
/* relaxation recovery */

      if (ncyc>0.6)
      {
         obspower(shss_pwr); 
         
         starthardloop(v10);
	    delay(2.5e-3);
	    shaped_pulse(shape_ss,pw_shpss,zero,0.0,0.0);
	    delay(2.5e-3);
         endhardloop();  
      
         obspower(tpwr);
      } 

  zgradpulse(gzlvl6,gt6);
  delay(200.0e-6);


  dec2rgpulse(pwN,t2,0.0,0.0);
 
  txphase(t3); dec2phase(zero);
  
/*  evolution of t1  */

  if(d2>0.001)
  {  
     zgradpulse( gzlvl0,(d2/2.0-0.0003-2.0*GRADIENT_DELAY));
     delay(300.0e-6);
     zgradpulse(-gzlvl0,(d2/2.0-0.0003-2.0*GRADIENT_DELAY));
     delay(300.0e-6);
  }
  else
     delay(d2);

  
/* ST2   */

  rgpulse(pw,t3,0.0,0.0);

  txphase(t3);
  if (waterpwrf < 4095.0)
     {obspwrf(waterpwrf); waterpwr=waterpwr+6.0;}
  obspower(waterpwr);

  rgpulse(waterdly,t3,0.0,rof1);
  if (waterpwrf < 4095.0)
     {obspwrf(4095.0); waterpwr=waterpwr-6.0;}

  obspower(tpwr); 
  txphase(zero); 

  zgradpulse(gzlvl4,gt4);
  delay(taua -pwN -0.5*pw -gt4-waterdly-rof1);           

  sim3pulse(2.0*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);

  dec2phase(t3);
  zgradpulse(gzlvl4,gt4);
  delay(taua -1.5*pwN  -gt4  -waterdly-rof1);               /* delay=1/4J(NH)   */

  if (waterpwrf < 4095.0)
     {obspwrf(waterpwrf); waterpwr=waterpwr+6.0;}
  obspower(waterpwr);
  txphase(two);

  rgpulse(waterdly,two,rof1,0.0);  
  if (waterpwrf < 4095.0)
     {obspwrf(4095.0); waterpwr=waterpwr-6.0;}

  obspower(tpwr);
  sim3pulse(pw,0.0e-6,pwN,zero,zero,t3,0.0,0.0);

/*  watergate   */

  zgradpulse(gzlvl5,gt5);
 
  delay(taua-1.5*pwN-waterdly-gt5);
  txphase(two);
  if (waterpwrf < 4095.0)
     {obspwrf(waterpwrf); waterpwr=waterpwr+6.0;}
  obspower(waterpwr);
  dec2phase(zero);     

  rgpulse(waterdly,two,0.0,rof1);
  if (waterpwrf < 4095.0)
     {obspwrf(4095.0); waterpwr=waterpwr-6.0;}
  
  obspower(tpwr);
  txphase(zero); 
 
  sim3pulse(2.0*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);

  if (waterpwrf < 4095.0)
     {obspwrf(waterpwrf); waterpwr=waterpwr+6.0;}
  obspower(waterpwr);
  txphase(two);

  rgpulse(waterdly,two,rof1,0.0);  
  if (waterpwrf < 4095.0)
     {obspwrf(4095.0); waterpwr=waterpwr-6.0;}

  zgradpulse(gzlvl5,gt5);
  obspwrf(4095.0); obspower(tpwr);
  delay(taua-1.5*pwN-waterdly-gt5);       

  dec2rgpulse(pwN,zero,0.0,0.0);

  
/* acquire data */

status(C);
     setreceiver(t14);
}
