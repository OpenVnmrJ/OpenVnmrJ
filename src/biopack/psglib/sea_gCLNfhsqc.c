/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* sea-gCLNfhsqc.c

        2D 1H-15N fhsqc experiments only on water accessable residues
        for N15- or N15/C13-labeled proteins, using CLEANEX-PM mixing scheme.
      
        For N15/C13-labeled samples, both C13 double filter and spin-echo filter
        can be used to eliminate CH-NH NOE and TOCSY contributions. 
            use C13 double filter by setting 
               C13filter='y' (the parameter "fact" has no efftect.
         or use spin-echo filter by setting
               C13filter='n', fact=3 (odd number).     
        
        For N15-labeled samples, only spin-echo filter can be used to eliminate
        CH-NH NOE and TOCSY contributions by setting
               C13filter='n', fact=3 (odd number).
               
        Set f1180='y' to indentify the folded peaks, 
            then (-90, 180) phase correction in t1 should be made.  
   
        H1 frequency on H2O (4.7ppm), 
        except for PM spin-lock during which tof is shifted to tofPM (~8ppm)
            
        N15 frequency on the amide region (~120ppm).  
             
        During the C13 double filter, C13 frequency is on the aliphatic region (35ppm),
        then is shifted to 116ppm for decoupling amide N from Calfa/C=O during t1.
	  
	Written by D. Lin on March 22, 2002
        Based on "Nfhsqc.c" and "sea-fhsqc.c" written by D. Lin
        Modified by D. Lin on May 16, 2002
        to include 1H  carrier shift prior PM trains 
	
        Refs:
	
        M. Pellecchia, et al., JACS 123, 4633(2001). (sea-trosy)
        S. Mori et al., JBNMR 7, 77(1996).    (spin-echo filter)
        S. Mori, et al., JMR B108, 94(1995).             (fhsqc)
	Hwang T-L., et al., JACS, 119:6203-6204(1997)  (CLEAN-PM	

        see Lin et.al.,JBioNMR, 23, 317-322(2002)
*/

#include <standard.h>

static double d2_init = 0.0;

static int phi1[4] = {0,0,2,2},
           phi2[2] = {0,2},
           phi3[4] = {1,1,3,3},
           phi4[8] = {0,0,0,0,2,2,2,2},
           phi5[2] = {0,2},
           phi6[4] = {0,0,2,2},
           rec[8] =  {0,2,0,2,2,0,2,0};

pulsesequence()
{
/* DECLARE VARIABLES */

 char        f1180[MAXSTR],C13refoc[MAXSTR], C13filter[MAXSTR];
             
 int	     t1_counter;

 double 
	tau1,	      	    /* t1/2 */
	JNH = getval("JNH"),
  	tauNH  = 1/(4*JNH),                       /* delay for 1H-15N INEPT  */
  	tauNH1 = getval("tauNH1"),                                /* 1/2JNH  */
  	tauNH2 = getval("tauNH2"),                                /* 1/2JNH  */
  	tauCH1 = getval("tauCH1"),                                /* 1/2JCH  */
  	tauCH2 = getval("tauCH2"),        /* 1/2JCH   tauCH2=2tauNH1-2tauCH1 */
  	fact = getval("fact"),          /* scale factor for spin-echo filter */
  	pwN = getval("pwN"),
  	pwNlvl = getval("pwNlvl"), 	      	  	              
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   compC = getval("compC"),
   rf0,            	          /* maximum fine power when using pwC pulses */
   rfst,	                           /* fine power for the stCall pulse */
   tofPM = getval("tofPM"),
   compH = getval("compH"),
   mix = getval("mix"),           /* mixing time for H2O - NH  */
   cycles,                               /* cycle number for CLEANEX_PM pulses */
   waltzB1 = getval("waltzB1"),           /* rf strength for PM spin-lock module      */
   pwlvlPM ,                      /* power level for CLEANEX_PM pulses */
   pwPM180,                       /* 1H 180 degree pulse width @pwlvlPM   */ 
   pwPM135, pwPM120, pwPM110,    
        dofCHn = getval("dofCHn"),           /* C13 carrier during C13 filter */
        gt1 = getval("gt1"),
        gt2 = getval("gt2"),
        gt3 = getval("gt3"),
        gt4 = getval("gt4"),
        gt5 = getval("gt5"),
        gstab =getval("gstab"),
        gzlvl1 = getval("gzlvl1"),
        gzlvl2 = getval("gzlvl2"),
        gzlvl3 = getval("gzlvl3"),
        gzlvl4 = getval("gzlvl4"),
        gzlvl5 = getval("gzlvl5"),
 gzlvld1 = getval("gzlvld1"), /* remove radiation damping in spin-echo filter */ 
 gzlvld2 = getval("gzlvld2"); /* remove radiation damping in mixing time */
 
    pwPM180 = 1/(2.0*waltzB1);
    pwPM135 = pwPM180 / 180.0 * 135.0;
    pwPM120 = pwPM180 / 180.0 * 120.0;
    pwPM110 = pwPM180 / 180.0 * 110.0;
    
    cycles = mix / (730.0/180.0 * pwPM180) - 8.0;

    initval(cycles/2, v10);        /* mixing time cycles */  
 

/* LOAD VARIABLES */

   getstr("f1180",f1180); 
   getstr("C13refoc",C13refoc);
   getstr("C13filter",C13filter);
  
   initval(3.0,v2);
   initval(1.0,v3);
   
/* maximum fine power for pwC pulses (and initialize rfst) */

	rf0 = 4095.0;    rfst=0.0;

/* 180 degree adiabatic C13 pulse from 0 to 200 ppm */

     if (C13refoc[A]=='y')
       {rfst = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35));   
	rfst = (int) (rfst + 0.5);
	if ( 1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n", 
	    (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) );    psg_abort(1); }}

/* power level for CLEANEX-PM pulse train */

	pwlvlPM = tpwr - 20.0*log10(pwPM180/(2*compH*pw));        
        pwlvlPM = (int) (pwlvlPM + 0.5);  

/* check validity of parameter range */

    if((dm[A] == 'y' || dm[B] == 'y' ))
	{
	printf("incorrect Dec1 decoupler flags!  ");
	psg_abort(1);
    } 

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'n'))
	{
	printf("incorrect Dec2 decoupler flags!  dm2 Should be 'nny' ");
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
 
    if((gt1 > 5.0e-3) || (gt2 > 5.0e-3) || (gt3 > 5.0e-3))
    {
        printf("gti must be less than 5 ms \n");
        psg_abort(1);
    }
   
    if((gt4 > 5.0e-3) || (gt5 > 5.0e-3))
    {
        printf("gti must be less than 5 ms \n");
        psg_abort(1);
    }

    if(gzlvld1>200 || gzlvld2>200  )
    {
        printf("gzlvldi should not be larger than 200 DAC \n");
        psg_abort(1);
    }

/* LOAD VARIABLES */

  settable(t1, 4,  phi1);
  settable(t2, 2,  phi2);
  settable(t3, 4,  phi3);
  settable(t4, 8,  phi4);
  settable(t5, 2,  phi5);
  settable(t6, 4,  phi6);
  settable(t14, 8, rec);

/* Phase incrementation for hypercomplex data */

   if ( phase1 == 2 )     /* Hypercomplex in t1 */
    {    
    tsadd(t2,1,4);
    }    
    
/* calculate modification to phases based on current t1 values
   to achieve States-TPPI acquisition */
 
   if(ix == 1)
    {
      d2_init = d2;
    }

      t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);
      if(t1_counter %2) 
       {
        tsadd(t2,2,4);
        tsadd(t14,2,4);
       }

/* set up so that get (-90,180) phase corrects in F1 if f1180 flag is y */

    tau1=d2;
    if( (f1180[A] == 'y') && (ni >1.0) )  tau1 += (1.0/(2.0*sw1));
    tau1 = tau1/2.0 -pw -2*pwN/PI;
    if (tau1 < 0.0) tau1=0.0;
    

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

/* spin-echo filter and 15N/13C double filters */

  rgpulse(pw,t1,rof1,0.0);  

  if(C13filter[A]=='y')   

{   
  decphase(t5);
  zgradpulse(gzlvl5,gt5); 
  delay(tauCH1-gt5); 
  decrgpulse(pwC,t5,0.0,0.0);
 
  txphase(t3); dec2phase(t5);
  delay(tauNH1-tauCH1-pwN*0.5);
  sim3pulse(2.0*pw,2*pwC,pwN,t3,zero,t5,0.0,0.0);
 
  decphase(t6);
  delay(tauCH2+tauCH1-tauNH1-pwN*0.5);
  decrgpulse(pwC,t6,0.0,0.0);
  
  delay(tauNH1+tauNH2-tauCH1-tauCH2-gt5-gstab);
  zgradpulse(gzlvl5,gt5);
  delay(gstab);     
}
 
 else
  
{  
  txphase(t3); dec2phase(t5); 
  delay(2.0e-6);
  zgradpulse(gzlvld1,0.5*fact*tauNH1-pwN*0.25-11.0e-6);
  delay(2.0e-5);
  zgradpulse(-gzlvld1,0.5*fact*tauNH1-pwN*0.25-11.0e-6);
  delay(2.0e-6);

  sim3pulse(2.0*pw,0.0e-6,pwN,t3,zero,t5,0.0,0.0);

  delay(2.0e-6);
  zgradpulse(gzlvld1,0.5*fact*tauNH2-pwN*0.25-11.0e-6);
  delay(2.0e-5);
  zgradpulse(-gzlvld1,0.5*fact*tauNH2-pwN*0.25-11.0e-6);
  delay(2.0e-6);
}
  
  txphase(zero); dec2phase(t6);

  sim3pulse(pw,0.0e-6,pwN,zero,zero,t6,0.0,0.0);
  zgradpulse(gzlvl4,gt4);
  delay(gstab);
  
  decoffset(dof);
  decpwrf(rfst);
  
  rgpulse(pw, two, 0.0, 0.0); 
  
 
 /* start CLEANEX-PM mixing period */
 
     if (cycles > 1.5000)
      {

       obsoffset(tofPM);
       obspower(pwlvlPM);
       txphase(zero);

            rgpulse(pwPM135, zero, 0.0, 0.0);
            rgpulse(pwPM120, two,  0.0, 0.0);
            rgpulse(pwPM110, zero, 0.0, 0.0);
            rgpulse(pwPM110, two,  0.0, 0.0);
            rgpulse(pwPM120, zero, 0.0, 0.0);
            rgpulse(pwPM135, two,  0.0, 0.0);

       rgradient('z',gzlvld2/4.0);

            rgpulse(pwPM135, zero, 0.0, 0.0);
            rgpulse(pwPM120, two,  0.0, 0.0);
            rgpulse(pwPM110, zero, 0.0, 0.0);
            rgpulse(pwPM110, two,  0.0, 0.0);
            rgpulse(pwPM120, zero, 0.0, 0.0);
            rgpulse(pwPM135, two,  0.0, 0.0);

       rgradient('z',gzlvld2/2.0);

            rgpulse(pwPM135, zero, 0.0, 0.0);
            rgpulse(pwPM120, two,  0.0, 0.0);
            rgpulse(pwPM110, zero, 0.0, 0.0);
            rgpulse(pwPM110, two,  0.0, 0.0);
            rgpulse(pwPM120, zero, 0.0, 0.0);
            rgpulse(pwPM135, two,  0.0, 0.0);

       rgradient('z',gzlvld2/4.0*3.0);

            rgpulse(pwPM135, zero, 0.0, 0.0);
            rgpulse(pwPM120, two,  0.0, 0.0);
            rgpulse(pwPM110, zero, 0.0, 0.0);
            rgpulse(pwPM110, two,  0.0, 0.0);
            rgpulse(pwPM120, zero, 0.0, 0.0);
            rgpulse(pwPM135, two,  0.0, 0.0);

       rgradient('z',gzlvld2);

         starthardloop(v10);
            rgpulse(pwPM135, zero, 0.0, 0.0);
            rgpulse(pwPM120, two,  0.0, 0.0);
            rgpulse(pwPM110, zero, 0.0, 0.0);
            rgpulse(pwPM110, two,  0.0, 0.0);
            rgpulse(pwPM120, zero, 0.0, 0.0);
            rgpulse(pwPM135, two,  0.0, 0.0);
         endhardloop();
         
        rgradient('z',-gzlvld2);

         starthardloop(v10);
            rgpulse(pwPM135, zero, 0.0, 0.0);
            rgpulse(pwPM120, two,  0.0, 0.0);
            rgpulse(pwPM110, zero, 0.0, 0.0);
            rgpulse(pwPM110, two,  0.0, 0.0);
            rgpulse(pwPM120, zero, 0.0, 0.0);
            rgpulse(pwPM135, two,  0.0, 0.0);
         endhardloop();
         
       rgradient('z',-gzlvld2/4.0*3.0);

            rgpulse(pwPM135, zero, 0.0, 0.0);
            rgpulse(pwPM120, two,  0.0, 0.0);
            rgpulse(pwPM110, zero, 0.0, 0.0);
            rgpulse(pwPM110, two,  0.0, 0.0);
            rgpulse(pwPM120, zero, 0.0, 0.0);
            rgpulse(pwPM135, two,  0.0, 0.0);

       rgradient('z',-gzlvld2/2.0);

            rgpulse(pwPM135, zero, 0.0, 0.0);
            rgpulse(pwPM120, two,  0.0, 0.0);
            rgpulse(pwPM110, zero, 0.0, 0.0);
            rgpulse(pwPM110, two,  0.0, 0.0);
            rgpulse(pwPM120, zero, 0.0, 0.0);
            rgpulse(pwPM135, two,  0.0, 0.0);

       rgradient('z',-gzlvld2/4.0);

            rgpulse(pwPM135, zero, 0.0, 0.0);
            rgpulse(pwPM120, two,  0.0, 0.0);
            rgpulse(pwPM110, zero, 0.0, 0.0);
            rgpulse(pwPM110, two,  0.0, 0.0);
            rgpulse(pwPM120, zero, 0.0, 0.0);
            rgpulse(pwPM135, two,  0.0, 0.0);

       rgradient('z', 0.0);
            
            rgpulse(pwPM135, zero, 0.0, 0.0);
            rgpulse(pwPM120, two,  0.0, 0.0);
            rgpulse(pwPM110, zero, 0.0, 0.0);
            rgpulse(pwPM110, two,  0.0, 0.0);
            rgpulse(pwPM120, zero, 0.0, 0.0);
            rgpulse(pwPM135, two,  0.0, 0.0);

       obsoffset(tof);
       obspower(tpwr);   

      }


/* H1-N15 INEPT */

  zgradpulse(gzlvl1,gt1);

  dec2phase(zero); decphase(zero);

  delay(tauNH-gt1);               /* delay=1/4J(XH)   */

  sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);

  zgradpulse(gzlvl1,gt1);

  txphase(one);
  dec2phase(t2);
  delay(tauNH-gt1 );               /* delay=1/4J(XH)   */
 
  rgpulse(pw, one, 0.0,0.0);

  zgradpulse(gzlvl2,gt2);
  delay(gstab);

  dec2rgpulse(pwN, t2, 0.0, 0.0);

  txphase(zero); dec2phase(t4);

/* t1 evolution period  */

  	if ( (C13refoc[A]=='y') && (tau1 > 0.5e-3 + WFG2_START_DELAY) )
           {delay(tau1 - 0.5e-3 - WFG2_START_DELAY);     /* WFG2_START_DELAY */
            simshaped_pulse("", "stC200", 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);
            delay(tau1 - 0.5e-3);}
	else
           {delay(tau1);
            rgpulse(2.0*pw, zero, 0.0, 0.0);
            delay(tau1);} 

  dec2rgpulse(pwN, t4, 0.0, 0.0);

  txphase(t1);
  zgradpulse(gzlvl2,gt2);
  delay(gstab);

  rgpulse(pw, t1,0.0,0.0);
  
/*   WATERGATE sequence to suppress water */  

   dec2phase(zero);
   
   delay(tauNH-gt3-gstab-pw*2.385-d3*2.5); 

   zgradpulse(gzlvl3,gt3);
   delay(gstab);

   pulse(pw*0.231,v2);
   delay(d3);
   pulse(pw*0.692,v2);
   delay(d3);
   pulse(pw*1.462,v2);

   delay(d3/2-pwN);
   dec2rgpulse(2*pwN, zero, 0.0,0.0);
   delay(d3/2-pwN);

   pulse(pw*1.462,v3);
   delay(d3);
   pulse(pw*0.692,v3);
   delay(d3);
   pulse(pw*0.231,v3);
   
   zgradpulse(gzlvl3,gt3);
   delay(gstab);
   
   delay(tauNH-gt3-gstab-pw*2.385-d3*2.5);
      
 /*  end of WATERGATE  */ 
 
   dec2power(dpwr2);
   
/* acquire data */  

status(C);

   setreceiver(t14);

}
