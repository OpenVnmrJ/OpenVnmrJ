/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* sea_gNtrosy.c

    2D 1H-15N trosy experiments only on water accessable residues
    for N15- or N15/C13-labeled proteins
       
    For N15/C13-labeled samples, both C13 double filter and spin-echo filter
    can be used to eliminate CH-NH NOE and TOCSY contributions. 
        use C13 double filter by setting 
        C13filter='y' (fact has no effect)
        or use spin-echo filter by setting
        C13filter='n', fact=3 (odd number).     
        
    For N15-labeled samples, only spin-echo filter can be used to eliminate
        CH-NH NOE and TOCSY contributions by setting
        C13filter='n', fact=3 (odd number).
     
         
    H1 frequency on H2O (4.7ppm), 
    N15 frequency on the amide region (~120ppm).  
             
      During the C13 double filter, C13 frequency is on aliphatic region (35ppm),
      then is shifted to (116ppm) for decoupling C13 from  N15
      in N15 t1 evolution.
	
    Refs:
	      
      M. Pellecchia, et al., JACS 123, 4633(2001). (sea-trosy)
      S. Mori et al., JBNMR 7, 77(1996).    (spin-echo filter)

    written by D. Lin on Feb 26, 2002 
    Based on "trosy.c" written by Y. Xia and "sea-gehsqc.c" written by D.Lin
    (see Lin et.al, JBioNMR, 23, 317-322(2002))
*/

#include <standard.h>

static double d2_init = 0.0;
 
static int phi1[4] = {1,0,3,2},
           phi2[1] = {1},
           phi3[8] = {0,0,0,0,2,2,2,2},
           phi4[8] = {0,0,0,0,2,2,2,2},                                  
           phi5[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
           phi6[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                      2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
           phi10[4] = {0,2,0,2},
           
           rec[4] = {0,3,2,1};
            
           
pulsesequence()
{
/* DECLARE VARIABLES */

 char        f1180[MAXSTR],C13refoc[MAXSTR],C13filter[MAXSTR];
             
 int	     t1_counter;

 double    	
   	dofCHn = getval("dofCHn"),
   	tauNH  = 1.0/(4.0*getval("JNH")),  /* delay for 1H-15N INEPT   1/4JNH  */
  	tauNH1 = getval("tauNH1"), /* 1/2JNH  */
  	tauNH2 = getval("tauNH2"), /* 1/2JNH  */
  	tauCH1 = getval("tauCH1"), /* 1/2JCH  */
  	tauCH2 = getval("tauCH2"), /* 1/2JCH   tauCH2=2tauNH1-2tauCH1 */
  	fact = getval("fact"),     /* scale factor for spin-echo filter */ 
  	mix = getval("mix"),             /* mixing time for H2O - NH  */ 	     
   	     tau1,
   	     pwHs = getval("pwHs"),  	      
   	     tpwrs,tpwrsf=getval("tpwrsf"),
   	     compH=getval("compH"),	  	              
  pwN = getval("pwN"),
  pwNlvl = getval("pwNlvl"), 
  pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
  pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
  compC = getval("compC"),
  rf0,            	          /* maximum fine power when using pwC pulses */
  rfst,	                           /* fine power for the stCall pulse */
  gt1 = getval("gt1"),
  gt2 = getval("gt2"),
  gt3 = getval("gt3"),
  gt4 = getval("gt4"),
  gt5 = getval("gt5"),
  gstab = getval("gstab"),
  gzlvl1 = getval("gzlvl1"),
  gzlvl2 = getval("gzlvl2"),
  gzlvl3 = getval("gzlvl3"),
  gzlvl4 = getval("gzlvl4"),
  gzlvl5 = getval("gzlvl5"),
  gzlvlf = getval("gzlvlf"), /* remove radiation damping in spin-echo filter */
  gzlvlm = getval("gzlvlm"), /* remove radiation damping in mixing time */
  gzlvld = getval("gzlvld"); /* remove radiation damping in t1 evolution */

/* LOAD VARIABLES */

  getstr("f1180",f1180); 
  getstr("C13refoc",C13refoc);
  getstr("C13filter",C13filter);
   
/* maximum fine power for pwC pulses (and initialize rfst) */

	rf0 = 4095.0;    rfst=0.0;

/* 180 degree adiabatic C13 pulse from 0 to 200 ppm */

     if (C13refoc[A]=='y')
       {rfst = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35));   
	rfst = (int) (rfst + 0.5);
	if ( 1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n", 
	    (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) );    psg_abort(1); }}
   
/* selective H20 one-lobe sinc pulse */
   
        tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   
	tpwrs = (int) (tpwrs);                   
        if (tpwrsf<4095.0) tpwrs=tpwrs+6; /* permit fine power control */
  

/* check validity of parameter range */

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y'))
	{
	printf("incorrect Dec decoupler flags! dm2 should be 'nnn'  ");
	psg_abort(1);
    } 

    if((dm[A] == 'y' || dm[B] == 'y'))
	{
	printf("incorrect Dec1 decoupler flags!  ");
	psg_abort(1);
    } 

    if (dmm[A] == 'y')
	{
	printf("incorrect Dec2 decoupler flag!  ");
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
   
    if((gt1>5.0e-3) || (gt2>5.0e-3) || (gt3>5.0e-3) || (gt4>5.0e-3) || (gt5>5.0e-3))
    {
        printf("gti must be less than 5 ms \n");
        psg_abort(1);
    }
  
  if(gzlvlf >200 || gzlvlf >200 || gzlvld >200)
    {
        printf("gzlvlf/m/d should not be larger than 200 DAC \n");
        psg_abort(1);
    }


/* LOAD VARIABLES */

  settable(t1, 4, phi1);
  settable(t2, 1, phi2);
  settable(t3, 8, phi3);
  settable(t4, 8, phi4);
  settable(t5, 16,  phi5);
  settable(t6, 32,  phi6); 
  settable(t10, 4, phi10);
  settable(t14, 4, rec);
  
/* Phase incrementation for hypercomplex data */

   if ( phase1 == 2 )     /* Hypercomplex in t1 */
   {     tsadd(t2,2,4);
	 ttadd(t14,t10,4);  
    }

       
/* calculate modification to phases based on current t1 values
   to achieve States-TPPI acquisition */
 
 
   if(ix == 1)
      d2_init = d2;
      t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);

      if(t1_counter %2) {
        tsadd(t1,2,4);
        tsadd(t14,2,4);
      }

    tau1=d2;
    if( (f1180[A] == 'y') && (ni >1.0) )  tau1 += (1.0/(2.0*sw1));
    tau1=0.5*tau1;      
 
/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tpwr);               /* Set power for pulses  */
   dec2power(pwNlvl);            /* Set decoupler2 power to pwNlvl */
   decpower(pwClvl);
   decpwrf(rf0);
   obsoffset(tof); 
   decoffset(dofCHn);  
 delay(d1);
status(B);
  rcvroff();
  
  txphase(t3);
  delay(9.0e-5);

/* spin-echo filter and 15N/13C double filters */

  rgpulse(pw,t3,0.0,0.0); 
 
  if(C13filter[A]=='y')   

{ 
  decphase(t5);
  zgradpulse(gzlvl1,gt1);
  delay(tauCH1-gt1); 
  decrgpulse(pwC,t5,0.0,0.0);
 
  txphase(one); dec2phase(t5);
  delay(tauNH1-tauCH1-pwN*0.5);
  sim3pulse(2.0*pw,2*pwC,pwN,one,zero,t5,0.0,0.0);
 
  decphase(t6);
  delay(tauCH2+tauCH1-tauNH1-pwN*0.5);
  decrgpulse(pwC,t6,0.0,0.0);
  
  delay(tauNH1+tauNH2-tauCH1-tauCH2-gt1-gstab); 
  zgradpulse(gzlvl1,gt1);
  delay(gstab);   
}
 
 else
  
{ 
  txphase(one); dec2phase(t5);
 if (gzlvlf>0.0) 
 {
  delay(2.0e-6);
  zgradpulse(gzlvlf,0.5*fact*tauNH1-pwN*0.25-11.0e-6);
  delay(2.0e-5); 
  zgradpulse(-gzlvlf,0.5*fact*tauNH1-pwN*0.25-11.0e-6);
  delay(2.0e-6);
  
  sim3pulse(2.0*pw,0.0e-6,pwN,one,zero,t5,0.0,0.0);
 
  delay(2.0e-6);
  zgradpulse(gzlvlf,0.5*fact*tauNH2-pwN*0.25-11.0e-6);
  delay(2.0e-5);
  zgradpulse(-gzlvlf,0.5*fact*tauNH2-pwN*0.25-11.0e-6);
  delay(2.0e-6);
 }
 else
 {
  delay(fact*tauNH1-pwN*0.5);
  sim3pulse(2.0*pw,0.0e-6,pwN,one,zero,t5,0.0,0.0);
  delay(fact*tauNH1-pwN*0.5);
 }
}
  
  txphase(two); dec2phase(t6);

  sim3pulse(pw,0.0e-6,pwN,two,zero,t6,0.0,0.0);
  txphase(t4);
  decoffset(dof);
  decpwrf(rfst);

/* mixing time */ 
  
   zgradpulse(gzlvl2,gt2);  
   delay(gstab);    
  if(mix -gstab - 4.0*GRADIENT_DELAY - gt2> 0.0)
  {
   if (gzlvlm>0.0)
   {
     zgradpulse( gzlvlm, mix-gt2-gstab);
   }
   else
    delay(mix-gt2-gstab);
  }
  
/* HN INEPT */

  rgpulse(pw,t4,0.0,0.0);  
  txphase(one);    
  zgradpulse(gzlvl3,gt3);
  delay(tauNH - pwN- gt3);               /* delay=1/4J(NH)   */

  sim3pulse(2.0*pw,0.0e-6,2*pwN,one,zero,zero,0.0,0.0);

  dec2phase(t1);
  zgradpulse(gzlvl3,gt3);
  delay(tauNH - pwN -gt3);               /* delay=1/4J(NH)   */
  
  sim3pulse(pw,0.0e-6,pwN,one,zero,t1,0.0,0.0);

  txphase(t2);  
  
/*  evolution of t1  */

 if ( (C13refoc[A]=='y') && (tau1 > 0.5e-3 + WFG2_START_DELAY) )
    {
     delay(2.0e-6);
     zgradpulse(gzlvld, tau1 - 4.0e-6 - 2.0*GRADIENT_DELAY- 0.5e-3 - WFG2_START_DELAY);
     delay(2.0e-6);
     decshaped_pulse("stC200", 1.0e-3, zero, 0.0, 0.0);
     delay(2.0e-6);
     zgradpulse(-gzlvld, tau1 - 4.0e-6 - 2.0*GRADIENT_DELAY- 0.5e-3 );
     delay(2.0e-6);
    }
 else if (tau1 - 3.0e-6 - 2.0*GRADIENT_DELAY>0 )
    {  
     delay(2.0e-6);
     zgradpulse(gzlvld, tau1 - 11.0e-6 - 2.0*GRADIENT_DELAY);
     delay(2.0e-5);
     zgradpulse(-gzlvld, tau1 - 11.0e-6 - 2.0*GRADIENT_DELAY);
     delay(2.0e-6);
    }

/* ST2PT   */

  rgpulse(pw,t2,0.0,0.0);

  txphase(zero);
  zgradpulse(gzlvl4,gt4);
  delay(tauNH - pwN - gt4);               /* delay=1/4J(NH)   */

  sim3pulse(2.0*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);

  dec2phase(t2);
  zgradpulse(gzlvl4,gt4);
  delay(tauNH - pwN  - gt4);               /* delay=1/4J(NH)   */

  sim3pulse(pw,0.0e-6,pwN,zero,zero,t2,0.0,0.0);

/*  watergate   */

  zgradpulse(gzlvl5,gt5);
 
  delay(tauNH-pwN-gt5-pwHs-4.0e-6-PRG_START_DELAY);
  dec2phase(zero);     

   	obspower(tpwrs); obspwrf(tpwrsf); txphase(two);
   	shaped_pulse("H2Osinc", pwHs, two, 2.0e-6, 2.0e-6);
  	obspower(tpwr); obspwrf(4095.0);
  	txphase(zero); 
 
  sim3pulse(2.0*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);

   	obspower(tpwrs); txphase(two); obspwrf(tpwrsf);
   	shaped_pulse("H2Osinc", pwHs, two, 2.0e-6 ,2.0e-6);
  	obspower(tpwr); obspwrf(4095.0);
  	txphase(zero); 
 
  zgradpulse(gzlvl5,gt5);
  delay(tauNH-pwN-gt5-pwHs-4.0e-6-PRG_START_DELAY);     

  dec2rgpulse(pwN,zero,0.0,0.0);
  dec2power(dpwr2);


/* acquire data */

status(C);
     setreceiver(t14);
}
