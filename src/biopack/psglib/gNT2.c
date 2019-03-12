/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gNT2.c

	written by Youlin Xia, on Jan. 20, 1999.
	
	The pulse sequence is used to measure T2 of 15N.
	
Note:   
        
1       Set ncyc > 0 to vary 15N T2 relaxation period (approximately
        ncyc*14.4ms).


2       The power of 15N PI pulse in the periord of CPMG is reduced
        by 3 dB to decrease the heating effect.


Reference:
	   
	1.   Neil A.F., ...., Lewis E.K., Biochemistry 33, 5984-6003(1994)
	   
	2.   K. Pervushin,R.Riek,G.Wider,K.Wuthrich,
	     Proc.Natl.Acad.Sci.USA,94,12366-71,1997.
	
	3.   G. Zhu, Y. Xia, L.K. Nicholson, and K.H. Sze,
	     J. Magn. Reson. 143, 423-426(2000).
	
Please quote Reference 3 if you use this pulse sequence
Modified for BioPack by GG, Palo Alto, May 2002
Modified to remove bipolar gradient in t1. GG, Palo Alto, Aug 2008
	
		
*/

#include <standard.h>

static double d2_init = 0.0;
 
static int phi1[2] = {1,0},
           phi2[4] = {1,2,3,0},
           phi3[1] = {1}, 
           phi10[4]= {0,2,0,2},

            rec[4] = {0,3,2,1};
            
           
void pulsesequence()
{
/* DECLARE VARIABLES */

             
 int	     t1_counter;

 double     
             tau1,                  /* t1/2  */
  	     taua = getval("taua"),     /* 2.25ms  */
  	     taub = getval("taub"),     /* 2.75ms  */
 	     delta= 0.00055,
             time_T2,
             pwN = getval("pwN"),
             pwNlvl,                /* power level for N hard pulses */
        ncyc = getval("ncyc"),
    	pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
   	tpwrsf = getval("tpwrsf"),     /* fine power for the pwHs ("H2Osinc") pulse */
   	waterpwrf = getval("waterpwrf"),     /* fine power for the watergate pulse */
   	tpwrs,	  	              /* power for the pwHs ("H2Osinc") pulse */
        compH = getval("compH"),
        compN = getval("compN"),
        waterdly,              /* pw for water pulse  */
        waterpwr,              /* power for water pulse  */
  pwNlvl_cpmg,
  pwN_cpmg,
/*gt0, */
  gt1 = getval("gt1"),
  gt2 = getval("gt2"),
  gt3 = getval("gt3"),
  gt4 = getval("gt4"),
  gt5 = getval("gt5"),
/*gzlvl0 = getval("gzlvl0"), */
  gzlvl1 = getval("gzlvl1"),
  gzlvl2 = getval("gzlvl2"),
  gzlvl3 = getval("gzlvl3"),
  gzlvl4 = getval("gzlvl4"),
  gzlvl5 = getval("gzlvl5");

/* LOAD VARIABLES */

  pwN = getval("pwN");
  pwNlvl = getval("pwNlvl"); 
  waterdly = getval("waterdly");



/*  reduced by 3dB to decreasing heating effect */  
   pwNlvl_cpmg=pwNlvl-3.0; 
   pwN_cpmg=exp((3.0/20.0)*log(10))*compN*pwN;




    time_T2 = ncyc*(32.0*pwN_cpmg + 32.0*delta);
  
    if (ix==1) printf(" ncyc= %f,   time_T2= %f \n", ncyc,time_T2);    

    /* selective H20 one-lobe sinc pulse */
        tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /* needs 1.69 times more */
	tpwrs = (int) (tpwrs);                   /* power than a square pulse */

        waterpwr = tpwr - 20.0*log10(waterdly/(compH*pw));  
	waterpwr = (int) (waterpwr);                 

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

    if(time_T2 > 0.30)
    {
       printf("net time_T2 recovery time exceeds 0.30 sec. May be too long \n");
       psg_abort(1);
    }  

   if(ncyc > 14)
   {
      printf("value of ncyc must be less than or equal to 14\n");
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

  settable(t1, 2, phi1);
  settable(t2, 4, phi2);
  settable(t3, 1, phi3);
  settable(t10, 4, phi10);

  settable(t14, 4, rec);
  
  

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

   initval(ncyc+0.1,v10);  

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

  txphase(three);  dec2phase(t2);
  zgradpulse(gzlvl2,gt2);
  delay(taua -1.5*pwN  -gt2 );               /* delay=1/4J(NH)   */

  sim3pulse(pw,0.0e-6,pwN,one,zero,t2,0.0,0.0);
    if (tpwrsf < 4095.0)
     {obspwrf(tpwrsf); tpwrs=tpwrs+6.0;}
    obspower(tpwrs);
    shaped_pulse("H2Osinc", pwHs, two, 5.0e-4, 0.0);
    if (tpwrsf < 4095.0)
     {obspwrf(4095.0); tpwrs=tpwrs-6.0;}
    obspower(tpwr);
 
  txphase(zero);  dec2phase(zero);
  zgradpulse(gzlvl3,gt3);
  delay(taub -pwN  -gt3-pwHs-2.0e-6-2.0*POWER_DELAY-WFG_START_DELAY);               /* delay=1/4J(NH)   */

  sim3pulse(2.0*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);

  zgradpulse(gzlvl3,gt3); dec2phase(t1);
  dec2power(pwNlvl_cpmg);            /* Set decoupler2 power to pwNlvl_cpmg */
  delay(taub -pwN  -gt3 +(2.0/PI)*pwN);               /* delay=1/4J(NH)   */

/* CPMG  */

      if (ncyc>0.6)
      {
         loop(v10,v11); 
    
            initval(3.0,v1);
            loop(v1,v2);
   	       delay(delta);
	       dec2rgpulse(2.0*pwN_cpmg,t1,0.0,0.0);
	       delay(delta);
	    endloop(v2);
   	       delay(delta);
	       dec2rgpulse(2.0*pwN_cpmg,t1,0.0,0.0);
	       delay(delta-pw);

	       rgpulse(2.0*pw,zero,0.0,0.0);

	       delay(delta-pw);
	       dec2rgpulse(2.0*pwN_cpmg,t1,0.0,0.0);
   	       delay(delta);

            initval(3.0,v1);
            loop(v1,v2);
   	       delay(delta);
	       dec2rgpulse(2.0*pwN_cpmg,t1,0.0,0.0);
	       delay(delta);
   	       delay(delta);
	       dec2rgpulse(2.0*pwN_cpmg,t1,0.0,0.0);
	       delay(delta);
	    endloop(v2);
   	       delay(delta);
	       dec2rgpulse(2.0*pwN_cpmg,t1,0.0,0.0);
	       delay(delta-pw);

	       rgpulse(2.0*pw,two,0.0,0.0);

	       delay(delta-pw);
	       dec2rgpulse(2.0*pwN_cpmg,t1,0.0,0.0);
   	       delay(delta);

            initval(3.0,v1);
            loop(v1,v2);
   	       delay(delta);
	       dec2rgpulse(2.0*pwN_cpmg,t1,0.0,0.0);
	       delay(delta);
	    endloop(v2);

        endloop(v11);   

      } 

       dec2power(pwNlvl); 
       delay((2.0/PI)*pwN_cpmg);                   

/*  inept of 15N and 1H and evolution of t1  */

  txphase(t3); dec2phase(zero);
  
/*  evolution of t1  */

delay(d2);

  
/* ST2   */

  rgpulse(pw,t3,0.0,0.0);


    if (tpwrsf < 4095.0)
     {obspwrf(tpwrsf); tpwrs=tpwrs+6.0;}
    obspower(tpwrs);
    shaped_pulse("H2Osinc", pwHs, t3, 2.0e-6, 0.0);   
    if (tpwrsf < 4095.0)
     {obspwrf(4095.0); tpwrs=tpwrs-6.0;}
	obspower(tpwr);  

  txphase(zero);
  zgradpulse(gzlvl4,gt4);
  delay(taua -pwN -0.5*pw -gt4-pwHs-2.0e-6-2.0*POWER_DELAY-WFG_START_DELAY);               /* delay=1/4J(NH)   */

  sim3pulse(2.0*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);

  dec2phase(t3);
  zgradpulse(gzlvl4,gt4);
  delay(taua -1.5*pwN  -gt4-pwHs-2.0e-6-2.0*POWER_DELAY-WFG_START_DELAY);               /* delay=1/4J(NH)   */

    	obspower(tpwrs); 
   	shaped_pulse("H2Osinc", pwHs, two, 2.0e-6, 0.0);   
	obspower(tpwr);  
  sim3pulse(pw,0.0e-6,pwN,zero,zero,t3,0.0,0.0);

/*  watergate   */

  zgradpulse(gzlvl5,gt5);
 
  delay(taua-1.5*pwN-waterdly-gt5);
  dec2phase(zero);     

  if (waterpwrf < 4095.0)
     {obspwrf(waterpwrf); waterpwr=waterpwr+6.0;}
  obspower(waterpwr);
  rgpulse(waterdly,two,rof1,rof1);
  if (waterpwrf < 4095.0)
    {obspwrf(4095.0); waterpwr=waterpwr-6.0;}
  obspower(tpwr);
 
  sim3pulse(2.0*pw,0.0e-6,2*pwN,zero,zero,zero,rof1,rof1);

  if (waterpwrf < 4095.0)
    {obspwrf(waterpwrf); waterpwr=waterpwr+6.0;}
  obspower(waterpwr);
  rgpulse(waterdly,two,rof1,0.0);  
  if (waterpwrf < 4095.0)
    {obspwrf(4095.0); waterpwr=waterpwr-6.0;}

  zgradpulse(gzlvl5,gt5);
  obspower(tpwr);
  delay(taua-1.5*pwN-waterdly-gt5);       /* delay=1/4J(NH)   */

  dec2rgpulse(pwN,zero,0.0,0.0);

/* acquire data */

status(C);
     setreceiver(t14);
}
