/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* sea_gCLNtrosy.c

    2D 1H-15N trosy experiments for water accessable residues
    for N15- or N15/C13-labeled proteins, using CLEANEX-PM mixing scheme.
    
    For N15/C13-labeled samples, set C13refoc='y', C13filter='y', fact=1;
    For N15-labeled samples, set C13refoc='n', C13filter='n', fact=1 or 3.
         
    H1 frequency on H2O (4.7ppm),
    except for PM spin-lock during when tof is shifted to tofPM (~8ppm) 
    N15 frequency on the amide region (~120ppm).  
             
    During the C13 double filter, C13 frequency is on the aliphatic region (35ppm),
    then is shifted to the middle netween whole CO and Ca (~116ppm) 
    for decoupling Ca-N and CO-N in t1 evolution.
     	
    Refs:
	      
      M. Pellecchia, et al., JACS 123, 4633(2001). (sea-trosy)
      S. Mori et al., JBNMR 7, 77(1996).    (spin-echo filter)
      Hwang T-L., et al., JACS, 119:6203-6204(1997)  (CLEANEX-PM)

    written by D. Lin on April 19, 2002 
    Based on "sea-trosy.c" written by D.Lin
     (see Lin et.al., JBioNMR, 23, 317-322(2002))
    
*/

#include <standard.h>

static double d2_init = 0.0;
 
static int phi1[4] = {1,0,3,2},
           phi2[1] = {1},           
           phi4[32] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                       3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3},
           phi5[8] =  {0,0,0,0,2,2,2,2},
           phi6[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},   
         
           phi7[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                       2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
           
           phi10[4] = {0,2,0,2},

           rec[4] = {0,3,2,1};
            
           
pulsesequence()
{
/* DECLARE VARIABLES */

 char        f1180[MAXSTR],C13filter[MAXSTR],C13refoc[MAXSTR];
             
 int	     t1_counter;

 double      tauNH,                 /* 1 / 4J(NH)                */
   	     tau1,JNH,
   	     pwHs = getval("pwHs"), /* pulse width of 1H 90 degree shaped pulse */	      
   	     tpwrsf_u=getval("tpwrsf_u"),
   	     tpwrs, tpwrsf_d=getval("tpwrsf_d"),
   	     compH=getval("compH"),   
   	     tofPM = getval("tofPM"),
  pwN = getval("pwN"),
  pwNlvl = getval("pwNlvl"), 
  pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
  pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
  compC = getval("compC"),
  rf0,            	          /* maximum fine power when using pwC pulses */
  rfst,
   
  dofCHn = getval("dofCHn"),
  tauNH1 = getval("tauNH1"), /* 1/2JNH  */
  tauNH2 = getval("tauNH2"), /* 1/2JNH  */
  tauCH1 = getval("tauCH1"), /* 1/2JCH  */
  tauCH2 = getval("tauCH2"), /* 1/2JCH   tauCH2=2tauNH1-2tauCH1 */
  fact = getval("fact"),     /* scale factor for spin-echo filter */   /* fine power for the stCall pulse */
  mix = getval("mix"),
  gt0 = getval("gt0"), 
  gt1 = getval("gt1"),
  gt2 = getval("gt2"),
  gt3 = getval("gt3"),
  gt4 = getval("gt4"),
  gt5 = getval("gt5"),
  gstab = getval("gstab"),
  gzlvlf = getval("gzlvlf"),     /* gradient strength in spin-echo filter  */
  gzlvld = getval("gzlvld"),     /* gradient strength in mixing time  */
  gzlvlm = getval("gzlvlm"),     /* gradient strength in t1 evolution period  */
  gzlvl0 = getval("gzlvl0"),
  gzlvl1 = getval("gzlvl1"),
  gzlvl2 = getval("gzlvl2"),
  gzlvl3 = getval("gzlvl3"),
  gzlvl4 = getval("gzlvl4"),
  gzlvl5 = getval("gzlvl5"),
  waltzB1 = getval("waltzB1"), /* rf strength for CLEANEX_PM pulses */
  cycles,                         /* for CLEANEX_PM  */
  pwPM180,       /* 1H 180 degree pulse width @pwlvlPM   */ 
  pwlvlPM,pwPM135, pwPM120, pwPM110;      
 
  pwPM180 = 1/(2.0*waltzB1);
  pwPM135 = pwPM180 / 180.0 * 135.0;
  pwPM120 = pwPM180 / 180.0 * 120.0;
  pwPM110 = pwPM180 / 180.0 * 110.0;
    
  pwlvlPM = tpwr - 20.0*log10(pwPM180/(2.0*compH*pw));
  pwlvlPM = (int) (pwlvlPM + 0.5);  /* power level for CLEANEX_PM pulses */
  cycles = mix / (730.0/180.0 * pwPM180) - 8.0;
  initval(cycles/2, v10);        /* mixing time cycles */ 
    
/* LOAD VARIABLES */

  getstr("f1180",f1180); 
  getstr("C13refoc",C13refoc);
  getstr("C13filter",C13filter);

  initval(2.0,v4);
  initval(0.0,v5); 

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
   
        pwHs=getval("pwHs");
        tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   
	tpwrs = (int) (tpwrs);                   
        if (tpwrsf_d<4095.0) tpwrs=tpwrs+6;
        
  

/* check validity of parameter range */

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y'))
	{
	printf("incorrect Dec decoupler flags! dm2 should be 'nnn'  ");
	psg_abort(1);
    } 

    if((dm[A] == 'y' || dm[B] == 'y' ))
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

    if((gt1 >5.0e-3) || (gt2 >5.0e-3) || (gt3 >5.0e-3) || (gt0 >5.0e-3))
    {
        printf("gti must be less than 5 ms \n");
        psg_abort(1);
    }  
        
   if(gzlvld>200 || gzlvlm>200 || gzlvlf>200)
    {
        printf("gzlvld/gzlvlm/gzlvlf must be less than 200 DAC value \n");
        psg_abort(1);
    }  


/* LOAD VARIABLES */

  settable(t1, 4, phi1);
  settable(t2, 1, phi2);
  settable(t4, 32, phi4); 
  settable(t5, 8, phi5); 
  settable(t6, 16, phi6);
  settable(t7, 32, phi7);
  
  settable(t10, 4, phi10);

  settable(t14, 4, rec);
  
  
/* INITIALIZE VARIABLES */
  JNH=getval("JNH");

  tauNH = ((JNH != 0.0) ? 1/(4*(JNH)) : 2.25e-3);

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
   obsoffset(tof);   decoffset(dofCHn); 
  
   delay(d1);
 
status(B);

  rcvroff();
  
   txphase(t7);
   delay(9.0e-5);

/* spin-echo filter and double 15N/13C filters */

   rgpulse(pw,t7,0.0,0.0); 
 
  if(C13filter[A]=='y')   

{ 
  decphase(t5);
  zgradpulse(gzlvl4,gt4);
  delay(tauCH1-gt4); 
  decrgpulse(pwC,t5,0.0,0.0);
 
  txphase(one); dec2phase(t5);
  delay(tauNH1-tauCH1-pwN*0.5);
  sim3pulse(2.0*pw,2*pwC,pwN,one,zero,t5,0.0,0.0);
 
  decphase(t6);
  delay(tauCH2+tauCH1-tauNH1-pwN*0.5);
  decrgpulse(pwC,t6,0.0,0.0);
  
  delay(tauNH1+tauNH2-tauCH1-tauCH2-gt4-gstab); 
  zgradpulse(gzlvl4,gt4);
  delay(gstab);   
}
 
 else
  
{ 
  txphase(one); dec2phase(t5);
  
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
  
  txphase(two); dec2phase(t6);

  sim3pulse(pw,0.0e-6,pwN,two,zero,t6,0.0,0.0);
  zgradpulse(gzlvl5,gt5);
  delay(gstab);
  txphase(two);
  decoffset(dof);
  decpwrf(rfst); 
 
   rgpulse(pw,zero,0.0,0.0);

/* start CLEANEX-PM mixing period */
 
      if (cycles > 1.5000)
      {

       obsoffset(tofPM);
       obspower(pwlvlPM);
       txphase(v4);

            rgpulse(pwPM135, v4, 0.0, 0.0);
            rgpulse(pwPM120, v5, 0.0, 0.0);
            rgpulse(pwPM110, v4, 0.0, 0.0);
            rgpulse(pwPM110, v5, 0.0, 0.0);
            rgpulse(pwPM120, v4, 0.0, 0.0);
            rgpulse(pwPM135, v5, 0.0, 0.0);

       rgradient('z',gzlvlm/4.0);
            
            rgpulse(pwPM135, v4, 0.0, 0.0);
            rgpulse(pwPM120, v5, 0.0, 0.0);
            rgpulse(pwPM110, v4, 0.0, 0.0);
            rgpulse(pwPM110, v5, 0.0, 0.0);
            rgpulse(pwPM120, v4, 0.0, 0.0);
            rgpulse(pwPM135, v5, 0.0, 0.0);

       rgradient('z',gzlvlm/2.0);
            
            rgpulse(pwPM135, v4, 0.0, 0.0);
            rgpulse(pwPM120, v5, 0.0, 0.0);
            rgpulse(pwPM110, v4, 0.0, 0.0);
            rgpulse(pwPM110, v5, 0.0, 0.0);
            rgpulse(pwPM120, v4, 0.0, 0.0);
            rgpulse(pwPM135, v5, 0.0, 0.0);


       rgradient('z',gzlvlm/4.0*3.0);

            rgpulse(pwPM135, v4, 0.0, 0.0);
            rgpulse(pwPM120, v5, 0.0, 0.0);
            rgpulse(pwPM110, v4, 0.0, 0.0);
            rgpulse(pwPM110, v5, 0.0, 0.0);
            rgpulse(pwPM120, v4, 0.0, 0.0);
            rgpulse(pwPM135, v5, 0.0, 0.0);

       rgradient('z',gzlvlm);

         starthardloop(v10);
            
            rgpulse(pwPM135, v4, 0.0, 0.0);
            rgpulse(pwPM120, v5, 0.0, 0.0);
            rgpulse(pwPM110, v4, 0.0, 0.0);
            rgpulse(pwPM110, v5, 0.0, 0.0);
            rgpulse(pwPM120, v4, 0.0, 0.0);
            rgpulse(pwPM135, v5, 0.0, 0.0);
         
         endhardloop();
         
        rgradient('z',-gzlvlm);

         starthardloop(v10);
           
            rgpulse(pwPM135, v4, 0.0, 0.0);
            rgpulse(pwPM120, v5, 0.0, 0.0);
            rgpulse(pwPM110, v4, 0.0, 0.0);
            rgpulse(pwPM110, v5, 0.0, 0.0);
            rgpulse(pwPM120, v4, 0.0, 0.0);
            rgpulse(pwPM135, v5, 0.0, 0.0);

         endhardloop();
             
       rgradient('z',-gzlvlm/4.0*3.0);
           
            rgpulse(pwPM135, v4, 0.0, 0.0);
            rgpulse(pwPM120, v5, 0.0, 0.0);
            rgpulse(pwPM110, v4, 0.0, 0.0);
            rgpulse(pwPM110, v5, 0.0, 0.0);
            rgpulse(pwPM120, v4, 0.0, 0.0);
            rgpulse(pwPM135, v5, 0.0, 0.0);
 
       rgradient('z',-gzlvlm/2.0);
            
            rgpulse(pwPM135, v4, 0.0, 0.0);
            rgpulse(pwPM120, v5, 0.0, 0.0);
            rgpulse(pwPM110, v4, 0.0, 0.0);
            rgpulse(pwPM110, v5, 0.0, 0.0);
            rgpulse(pwPM120, v4, 0.0, 0.0);
            rgpulse(pwPM135, v5, 0.0, 0.0);

       rgradient('z',-gzlvlm/4.0);

            rgpulse(pwPM135, v4, 0.0, 0.0);
            rgpulse(pwPM120, v5, 0.0, 0.0);
            rgpulse(pwPM110, v4, 0.0, 0.0);
            rgpulse(pwPM110, v5, 0.0, 0.0);
            rgpulse(pwPM120, v4, 0.0, 0.0);
            rgpulse(pwPM135, v5, 0.0, 0.0);

       rgradient('z', 0.0);
                    
            rgpulse(pwPM135, v4, 0.0, 0.0);
            rgpulse(pwPM120, v5, 0.0, 0.0);
            rgpulse(pwPM110, v4, 0.0, 0.0);
            rgpulse(pwPM110, v5, 0.0, 0.0);
            rgpulse(pwPM120, v4, 0.0, 0.0);
            rgpulse(pwPM135, v5, 0.0, 0.0); 

       obsoffset(tof);
       obspower(tpwr);   

      }


/* HN INEPT  */


  txphase(one);  dec2phase(zero);
   
  zgradpulse(gzlvl1,gt1);
  delay(tauNH-pwN-gt1);

  sim3pulse(2.0*pw,0.0e-6,2*pwN,one,zero,zero,0.0,0.0);

  txphase(t4);dec2phase(t1);
  zgradpulse(gzlvl1,gt1);
  delay(tauNH-pwN-gt1);      

/*  sim3pulse(pw,0.0e-6,pwN,t4,zero,t1,0.0,0.0);   */

   rgpulse(pw,t4,0.0,0.0);
   zgradpulse(gzlvl0,gt0);
   delay(gstab);
   dec2rgpulse(pwN,t1,0.0,0.0);
  

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

/* ST2   */

  rgpulse(pw,t2,0.0,0.0);

  txphase(zero);
  zgradpulse(gzlvl2,gt2);
  delay(tauNH -pwN -gt2);               /* delay=1/4J(NH)   */

  sim3pulse(2.0*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);

  dec2phase(t2);
  zgradpulse(gzlvl2,gt2);
  delay(tauNH -pwN  -gt2);               /* delay=1/4J(NH)   */

  sim3pulse(pw,0.0e-6,pwN,zero,zero,t2,0.0,0.0);   
  
/* watergate  sequence to suppress water  */

  zgradpulse(gzlvl3,gt3);
 
  delay(tauNH-pwN-gt3-pwHs-4.0e-6-PRG_START_DELAY);
  dec2phase(zero);     

   	obspower(tpwrs); txphase(two); obspwrf(tpwrsf_d);
   	shaped_pulse("H2Osinc", pwHs, two, 2.0e-6, 2.0e-6);
  	obspower(tpwr); obspwrf(4095.0);
  	txphase(zero); 
 
  sim3pulse(2.0*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);

   	obspower(tpwrs); txphase(two); obspwrf(tpwrsf_u);
   	shaped_pulse("H2Osinc", pwHs, two, 2.0e-6 ,2.0e-6);
  	obspower(tpwr); obspwrf(4095.0);
  	txphase(zero); 
 
  zgradpulse(gzlvl3,gt3);
  delay(tauNH-pwN-gt3-pwHs-4.0e-6-PRG_START_DELAY);     
 

  dec2rgpulse(pwN,zero,0.0,0.0);         
  
  dec2power(dpwr2);


/* acquire data */

status(C);
     setreceiver(t14);
}
