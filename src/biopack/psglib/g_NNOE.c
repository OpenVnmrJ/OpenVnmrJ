/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* g_NNOE.c

	written by Youlin Xia, on Jan. 20, 1999.
	re-written by Marco Tonelli in 2005
	
	The pulse sequence is used to measure NOE of 1H to 15N.
	
Note:   
        
1       relaxT = saturation time
        
        


2       The power of 1H 120 degree pulse in the periord of saturation is 
        reduced by 6 dB to decrease the heating effect.
        
         
Reference:
	   
	1.   Neil A.F., ...., Lewis E.K., Biochemistry 33, 5984-6003(1994)
	   
	2.   K. Pervushin,R.Riek,G.Wider,K.Wuthrich,
	     Proc.Natl.Acad.Sci.USA,94,12366-71,1997.
	
	3.   G. Zhu, Y. Xia, L.K. Nicholson, and K.H. Sze,
	     J. Magn. Reson. 143, 423-426(2000).
        
        set f1coef='1 0 -1 0 0 -1 0 -1'	

Please quote Reference 3 if you use this pulse sequence
Modified for BioPack by GG, Palo Alto May 2002
Added modified Tonelli version to BioPack, Sept. 2006
	   
	
		
*/

#include <standard.h>

static double d2_init = 0.0;
 
static int phi2[4] = {1,0,3,2},
           phi3[1] = {1}, 
           phi10[4] = {0,2,0,2},

            rec[4] = {0,3,2,1};
            
           
pulsesequence()
{
/* DECLARE VARIABLES */

 char	f1180[MAXSTR],
	C13refoc[MAXSTR],
	wtg3919[MAXSTR];
             
 int	t1_counter;

 double 
        tofHN = 0.0,
    
        tau1,				/* t1/2  */
  	taua = getval("taua"),		/* 2.25ms  */

        pwN = getval("pwN"),		/* PW90 for N-nuc            */
        pwNlvl = getval("pwNlvl"),	/* power level for N hard pulses */

        pwC = getval("pwC"),		/* PW90 for N-nuc            */
        pwClvl = getval("pwClvl"),	/* power level for N hard pulses */

        relaxT = getval("relaxT"),      /* total time for NOE measuring     */
	ncyc = 0,			/* number of pulsed cycles in relaxT */

        compH = getval("compH"),
   	pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
   	tpwrs,				/* power for the pwHs ("H2Osinc") pulse */
        tpwrsf_a = getval("tpwrsf_a"),  /* fine power for the pwHs ("H2Osinc") pulse */
        tpwrsf_u = getval("tpwrsf_u"),  /* fine power for the pwHs ("H2Osinc") pulse */
        tpwrsf_d = getval("tpwrsf_d"),  /* fine power for the pwHs ("H2Osinc") pulse */
        phincr_a = getval("phincr_a"),  /* fine power for the pwHs ("H2Osinc") pulse */
        phincr_u = getval("phincr_u"),  /* fine power for the pwHs ("H2Osinc") pulse */
        phincr_d = getval("phincr_d"),  /* fine power for the pwHs ("H2Osinc") pulse */

        d3919 = getval("d3919"),
        pwHs_dly,
        wtg_dly,

	gstab = getval("gstab"),
	gt0 = getval("gt0"),
	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5");


/* LOAD VARIABLES */

    getstr("f1180",f1180);
    getstr("wtg3919",wtg3919);
    getstr("C13refoc",C13refoc);

    if (find("tofHN")>1) tofHN=getval("tofHN");
    if (tofHN == 0.0) tofHN=tof;

/* selective H20 one-lobe sinc pulse */
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /* needs 1.69 times more */
    tpwrs = (int) (tpwrs);                   /* power than a square pulse */

    pwHs_dly = pwHs +WFG_START_DELAY +2.0e-6 +2.0*POWER_DELAY +2.0*SAPS_DELAY;
    if (tpwrsf_a < 4095.0) pwHs_dly = pwHs_dly +2.0*PWRF_DELAY; /* if one flipback is calibrated */
    if (phincr_a != 0.0) pwHs_dly = pwHs_dly +2.0*SAPS_DELAY;   /* so will probably be the others */

    if (wtg3919[0] != 'y')
	wtg_dly = pwHs_dly;
    else
	wtg_dly = pw*2.385 +7.0*rof1 +d3919*2.5 +SAPS_DELAY;



/* LOAD VARIABLES */

  assign(one,v11);
  assign(three,v12);

  settable(t2, 4, phi2);
  settable(t3, 1, phi3);
  settable(t10, 4, phi10);

  settable(t14, 4, rec);
  

/* Phase incrementation for hypercomplex data */

   if ( phase1 == 2 )     /* Hypercomplex in t1 */
     { ttadd(t14,t10,4); tsadd(t3,2,4); }
       
/* calculate modification to phases based on current t1 values
   to achieve States-TPPI acquisition */
 
   if(ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);

   if (t1_counter %2) 
     { tsadd(t2,2,4); tsadd(t14,2,4); }


/* Set up f1180  */

   tau1 = d2;
   if((f1180[A] == 'y') && (ni > 1.0))
       { tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
   tau1 = tau1/2.0;


/* Set up phincr corrections  */
   if (phincr_a < 0.0)  phincr_a=phincr_a+360.0;
   if (phincr_u < 0.0)  phincr_u=phincr_u+360.0;
   if (phincr_d < 0.0)  phincr_d=phincr_d+360.0;

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tpwr); 
   decpower(pwClvl);
   dec2power(pwNlvl);

   txphase(zero);
   dec2phase(zero);

   obsstepsize(1.0);

   rcvroff();

   initval(ncyc+0.1,v10);  
   delay(1.0e-5);


/* destroy magnetization of proton and nitrogen   */


   rgpulse(pw,zero,0.0,0.0);

   initval(phincr_u,v14);
   txphase(two); if (phincr_u != 0) xmtrphase(v14);
   if (tpwrsf_u < 4095.0) { obspwrf(tpwrsf_u); obspower(tpwrs+6.0); }
     else obspower(tpwrs);
   shaped_pulse("H2Osinc", pwHs, two, 2.0e-6, 0.0);
   obspower(tpwr);
   if (tpwrsf_u < 4095.0) obspwrf(4095.0);
   txphase(t3); if (phincr_u != 0) xmtrphase(zero);

   dec2rgpulse(pwN, zero, 0.0, 0.0);
   zgradpulse(gzlvl0,gt0);
   dec2phase(one);
   delay(gstab);
   dec2rgpulse(pwN, one, 0.0, 0.0);
   zgradpulse(0.7*gzlvl0,gt0);
   dec2phase(zero);
   delay(gstab);

/*  Saturation of 1H to produce NOE  */

status(B);

    if (tofHN != tof) obsoffset(tofHN);
    delay(0.5e-5);

    if (relaxT < 1.0e-4 )
     {
      ncyc = (int)(200.0*d1 + 0.5);	/* no H1 saturation */
      initval(ncyc,v1);
      obspower(-15.0); obspwrf(0.0);	/* powers set to minimum, but amplifier is unblancked identically */
           loop(v1,v2);
                   delay(2.5e-3 - 4.0*compH*1.34*pw);
                   rgpulse(4.0*compH*1.34*pw, zero, 5.0e-6, 0.0e-6);
                   rgpulse(4.0*compH*1.34*pw, zero, 0.0e-6, 0.0e-6);
                   rgpulse(4.0*compH*1.34*pw, zero, 0.0e-6, 0.0e-6);
                   rgpulse(4.0*compH*1.34*pw, zero, 0.0e-6, 5.0e-6);
                   delay(2.5e-3 - 4.0*compH*1.34*pw);
           endloop(v2);
     }
    else
     {
      ncyc = (int)(200.0*relaxT + 0.5);	/* H1 saturation */
      initval(ncyc,v1);
      if (ncyc > 0)
          {
           obspower(tpwr-12);
           loop(v1,v2);
           delay(2.5e-3 - 4.0*compH*1.34*pw);
           rgpulse(4.0*compH*1.34*pw, zero, 5.0e-6, 0.0e-6);
           rgpulse(4.0*compH*1.34*pw, zero, 0.0e-6, 0.0e-6);
           rgpulse(4.0*compH*1.34*pw, zero, 0.0e-6, 0.0e-6);
           rgpulse(4.0*compH*1.34*pw, zero, 0.0e-6, 5.0e-6);
           delay(2.5e-3 - 4.0*compH*1.34*pw);
           endloop(v2);
          }
      }
   obspower(tpwr); obspwrf(4095.0);

   if (tofHN != tof) obsoffset(tof);

/* two spin state mixing */

  zgradpulse(0.7*gzlvl3,gt3);
  delay(gstab);   

  rgpulse(pw,zero,0.0,0.0);

  initval(phincr_u,v14);
  txphase(two); if (phincr_u != 0) xmtrphase(v14);
  if (tpwrsf_u < 4095.0) { obspwrf(tpwrsf_u); obspower(tpwrs+6.0); }
    else obspower(tpwrs);
  shaped_pulse("H2Osinc", pwHs, two, 2.0e-6, 0.0);
  obspower(tpwr);
  if (tpwrsf_u < 4095.0) obspwrf(4095.0);
  txphase(t3); if (phincr_u != 0) xmtrphase(zero);
              
  zgradpulse(gzlvl3,gt3);
  delay(gstab);   

/*********************************************/
/*  inept of 15N and 1H and evolution of t1  */
/*********************************************/

  dec2rgpulse(pwN,t2,0.0,0.0);
/* H1 EVOLUTION BEGINS */
 
  txphase(t3); dec2phase(zero);
  if (C13refoc[A]=='y' && (tau1 -2.0*pwC -3.0*SAPS_DELAY) > 0.2e-6)
    {
     delay(tau1 -2.0*pwC -3.0*SAPS_DELAY);

     decrgpulse(pwC, zero, 0.0, 0.0);
     decphase(one);
     decrgpulse(2.0*pwC, one, 0.0, 0.0);
     decphase(zero);
     decrgpulse(pwC, zero, 0.0, 0.0);

     delay(tau1 -2.0*pwC -SAPS_DELAY);
    }
  else if (2.0*tau1 -2.0*SAPS_DELAY > 0.2e-6)
     delay(2.0*tau1 -2.0*SAPS_DELAY);
  
/* H1 EVOLUTION ENDS */
  rgpulse(pw, t3, 0.0, 0.0);

  initval(phincr_a,v14);
  if (phincr_a != 0) xmtrphase(v14);
  if (tpwrsf_a < 4095.0) { obspwrf(tpwrsf_a); obspower(tpwrs+6.0); }
    else obspower(tpwrs);
  shaped_pulse("H2Osinc", pwHs, t3, 2.0e-6, 0.0);
  obspower(tpwr);
  if (tpwrsf_a < 4095.0) obspwrf(4095.0);
  txphase(zero); if (phincr_a != 0) xmtrphase(zero);

  zgradpulse(gzlvl4,gt4);
  delay(taua -pwN -0.5*pw -gt4 -2.0*GRADIENT_DELAY -pwHs_dly);		/* delay=1/4J(NH)   */

  sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

  dec2phase(t3);
  zgradpulse(gzlvl4,gt4);
  delay(taua -1.5*pwN -gt4 -2.0*GRADIENT_DELAY -pwHs_dly);		/* delay=1/4J(NH)   */
 
  initval(phincr_d,v14);
  txphase(two); if (phincr_d != 0) xmtrphase(v14);
  if (tpwrsf_d < 4095.0) { obspwrf(tpwrsf_d); obspower(tpwrs+6.0); }
    else obspower(tpwrs);
  shaped_pulse("H2Osinc", pwHs, two, 2.0e-6, 0.0);
  obspower(tpwr);
  if (tpwrsf_d < 4095.0) obspwrf(4095.0);
  txphase(zero); if (phincr_d != 0) xmtrphase(zero);

  sim3pulse(pw, 0.0, pwN, zero, zero, t3, rof1, rof1);

  dec2phase(zero);     
  zgradpulse(gzlvl5,gt5);
  delay(taua -1.5*pwN -wtg_dly -gt5 -2.0*GRADIENT_DELAY);

     if(wtg3919[0] == 'y')		/*  3919 watergate   */
     {
       txphase(v11);
       rgpulse(pw*0.231,v11,rof1,rof1);
       delay(d3919);
       rgpulse(pw*0.692,v11,rof1,rof1);
       delay(d3919);
       rgpulse(pw*1.462,v11,rof1,rof1);

       delay(d3919/2.0-pwN);
       dec2rgpulse(2.0*pwN, zero, rof1, rof1);
       txphase(v12);
       delay(d3919/2.0-pwN);

       rgpulse(pw*1.462,v12,rof1,rof1);
       delay(d3919);
       rgpulse(pw*0.692,v12,rof1,rof1);
       delay(d3919);
       rgpulse(pw*0.231,v12,rof1,rof1);
     }
     else				/*  soft pulse watergate   */
     {
       initval(phincr_d,v14);
       txphase(two); if (phincr_d != 0) xmtrphase(v14);
       if (tpwrsf_d < 4095.0) { obspwrf(tpwrsf_d); obspower(tpwrs+6.0); }
       obspower(tpwrs);
       shaped_pulse("H2Osinc", pwHs, two, 2.0e-6, 0.0);
       obspower(tpwr);
       if (tpwrsf_d < 4095.0) obspwrf(4095.0);
       txphase(zero); if (phincr_d != 0) xmtrphase(zero);

       sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

       initval(phincr_u,v14);
       txphase(two); if (phincr_u != 0) xmtrphase(v14);
       if (tpwrsf_u < 4095.0) { obspwrf(tpwrsf_u); obspower(tpwrs+6.0); }
       obspower(tpwrs);
       shaped_pulse("H2Osinc", pwHs, two, 2.0e-6, 0.0);
       obspower(tpwr);
       if (tpwrsf_u < 4095.0) obspwrf(4095.0);
       txphase(zero); if (phincr_u != 0) xmtrphase(zero);
     }

  zgradpulse(gzlvl5,gt5);
  delay(taua -1.5*pwN -wtg_dly -gt5 -2.0*GRADIENT_DELAY);

  dec2rgpulse(pwN,zero,0.0,0.0);

/* acquire data */

  rcvron();
status(C);
  setreceiver(t14);
}
