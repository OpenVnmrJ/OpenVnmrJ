/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* sea_gNfhsqc.c

        2D 1H-15N fhsqc experiment for on water accessable residues only
        for N15- or N15/C13-labeled proteins
      
        For N15/C13-labeled samples: Both C13 double filter and spin-echo filter
        can be used to eliminate CH-NH NOE and TOCSY contributions. 
        Use C13 double filter by setting 
               C13filter='y' (fact has no effect). 
        or use spin-echo filter by setting
               C13filter='n', fact=3 (odd number).     
        
        For N15-labeled samples, only spin-echo filter can be used to eliminate
        CH-NH NOE and TOCSY contributions by setting
               C13filter='n', fact=3 (odd number).
        
        Set f1180='y' to indentify the folded peaks, 
        then (-90, 180) phase correction in t1 should be made.
       
        Center  H1 frequency on H2O (4.7ppm), 
        N15 frequency on the amide region (~120ppm).  
             
        During the C13 double filter, C13 frequency is in aliphatic region (35ppm),
        then is shifted to (~116ppm) for decoupling Calfa and C=O from amide N
        in N15 t1 evolution.	
	
	written by D. Lin, Hong Kong University of Science and Technology, Jan 16, 2002
        Based on "Nfhsqc.c" and "sea-gehsqc.c" written by D. Lin
          (see Donghai Lin, Kong Jung Sze, Yangfang Cui and Guang Zhu, JBioNMR,
           23,317-322 (2002))
        
        	
        Refs:
	
        M. Pellecchia, et al., JACS 123, 4633(2001). (sea-trosy)
        S. Mori et al., JBNMR 7, 77(1996).    (spin-echo filter)
        S. Mori, et al., JMR B108, 94(1995).             (fhsqc)
		
        Modified for BioPack, GG, Varian, Feb 2008
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


void pulsesequence()
{
/* DECLARE VARIABLES */

 char        f1180[MAXSTR],C13refoc[MAXSTR], C13filter[MAXSTR];
             
 int	     t1_counter;

 double 
	tau1,	      	    /* t1/2 */
	JNH = getval("JNH"),
  	tauNH  = 1/(4*JNH),         /* delay for 1H-15N INEPT   1/4JNH  */
  	tauNH1 = getval("tauNH1"),   /* 1/2JNH  */
  	tauNH2 = getval("tauNH2"),   /* 1/2JNH  */
  	tauCH1 = getval("tauCH1"),   /* 1/2JCH  */
  	tauCH2 = getval("tauCH2"),   /* 1/2JCH   tauCH2=2tauNH1-2tauCH1 */
  	fact = getval("fact"),       /* scale factor for spin-echo  */
  	pwN = getval("pwN"),
  	pwNlvl = getval("pwNlvl"), 	      	  	              
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   compC = getval("compC"),
   rf0,            	          /* maximum fine power when using pwC pulses */
   rfst,	                           /* fine power for the stCall pulse */
   mix = getval("mix"),           /* mixing time for H2O - NH  */
        dofCHn = getval("dofCHn"),
        gt1 = getval("gt1"),
        gt2 = getval("gt2"),
        gt3 = getval("gt3"),
        gt4 = getval("gt4"),
        gt5 = getval("gt5"),
        gstab=getval("gstab"),
        gzlvl1 = getval("gzlvl1"),
        gzlvl2 = getval("gzlvl2"),
        gzlvl3 = getval("gzlvl3"),
        gzlvl4 = getval("gzlvl4"),
        gzlvl5 = getval("gzlvl5"),
 gzlvld1 = getval("gzlvld1"), /* remove radiation damping in spin-echo filter */ 
 gzlvld2 = getval("gzlvld2"); /* remove radiation damping in mixing time */

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

   initval(3.0,v2);
   initval(1.0,v3);          
             
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

  if(gzlvld1>200 || gzlvld2>200 )
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
  settable(t14,8, rec);

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
  
   txphase(t1);
   delay(9.0e-5);

/* spin-echo filter and 15N/13C double filters */

  rgpulse(pw,t1,0.0,0.0);  

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
 if (gzlvld1>0.0)
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
 else
 {
  txphase(t3); dec2phase(t5); 
  delay(fact*tauNH1-pwN*0.5);

  sim3pulse(2.0*pw,0.0e-6,pwN,t3,zero,t5,0.0,0.0);

  delay(2.0e-6);
  delay(0.5*fact*tauNH2-pwN*0.25-3.0e-6);
  delay(2.0e-6);
  delay(0.5*fact*tauNH2-pwN*0.25-3.0e-6);
  delay(2.0e-6);
 }
}

  txphase(zero); dec2phase(t6);

  sim3pulse(pw,0.0e-6,pwN,zero,zero,t6,0.0,0.0);
  
  decoffset(dof);
  decpwrf(rfst);
 
/* mixing time */ 
  
   zgradpulse(gzlvl4,gt4);    
   delay(gstab); 
   if(mix - 4.0e-6 - 4.0*GRADIENT_DELAY - gt4> 0.0)
    {
     if (gzlvld2>0.0)
      {
        zgradpulse(gzlvld2,mix-gt4-gstab);
      }
     else
      delay(mix-gt4-gstab);
    }

/* H1-N15 INEPT */

  rgpulse(pw, zero, 0.0, 0.0);

  zgradpulse(gzlvl1,gt1);

  dec2phase(zero); decphase(zero);

  delay(tauNH-gt1);               /* delay=1/4J(XH)   */

  sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,rof1,rof1);

  zgradpulse(gzlvl1,gt1);

  txphase(one);
  dec2phase(t2);
  delay(tauNH-gt1 );               /* delay=1/4J(XH)   */
 
  rgpulse(pw, one, rof1, rof1);

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

  rgpulse(pw, t1, rof1, rof1);

  dec2phase(zero);

   zgradpulse(gzlvl3,gt3);
   txphase(v2);
   delay(gstab);

   delay(tauNH-gt3-gstab-pw*2.385-6.0*rof1 -d3*2.5);
 
   rgpulse(pw*0.231,v2,rof1,rof1);
   delay(d3);
   rgpulse(pw*0.692,v2,rof1,rof1);
   delay(d3);
   rgpulse(pw*1.462,v2,rof1,rof1);

   delay(d3/2-pwN);
   dec2rgpulse(2*pwN, zero, rof1, rof1);
   delay(d3/2-pwN);

   rgpulse(pw*1.462,v3,rof1,rof1);
   delay(d3);
   rgpulse(pw*0.692,v3,rof1,rof1);
   delay(d3);
   rgpulse(pw*0.231,v3,rof1,rof1);
   delay(tauNH-gt3-gstab-pw*2.385-6.0*rof1 -d3*2.5);
   dec2power(dpwr2);
   zgradpulse(gzlvl3,gt3);
   
   delay(gstab); 

/* acquire data */  

status(C);
   setreceiver(t14);

}
