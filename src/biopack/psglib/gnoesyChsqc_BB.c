/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gnoesyChsqc_BB        Agilent Technologies

   This pulse sequence allows one to perform the following experiment:
   NOESY-C13 HSQC 
                 
                 13C-HSQC part has sensitivity enhancement,
                 broadband 13C coverage (Methyls to Aromatics) 
                 
                 inspired by papers
                 JMR v136 p54  1999 -  BIPS and CLUB gradients Shapka et.al.
                 JMR v151 p269 2001
		 Ang. Chemie  v42 pp3938-3941 2003  ZQF filter Keeler et.al

 
             (ET) ZQF filter requires manual adjustments, not fully ready yet

             t1->1H evolution (NOE), t2->13C evolution (HSQC)

         In uniformly C13 labeled samples, for SE-version,
	 there is a possibility of CA->HB and CB->HA correlations in this HSQC,
         especially with in high s/n experiments.

         In this SE experiment, transfer efficiency for CH3/CH2 groups is WORSE 
         than that of NON-SE HSQC

         This is a price one pays for having SE enhancement and Jcc scalar
         couplings.
	 On the other hand, water suppression is good and the
 	 whole C13 range can be covered in one spectrum.


        	  INSTRUCTIONS

    Make sure BioPack is installed and probe file calibrated properly
   
    Run macro "gnoesyChsqc_BB", that will setup 1D experiment 
	(tof on water, dof on CA glycines, dof2 on NH, 15N 13C pulses etc)
    

    For SE='y': maximize 1D signal intensity by adjusting coherence selection
    gradient gzlvl9

    Set required number of transients.

    Run 13C-HSQC (ni2) plane, adjust dof and sw2 to get convenient folding 
    in 13C dimension

    If dof is changed significantly, re-create shaped pulses using button on 
    Pulse Sequence page (this will recreate CO decoupling pulses)

    Run 1H-1H NOESY plane to see whether everything is OK. Set mixing time mix.

   
   SOME OPTIONS

    Set f1180='y' to have 45,-180 phase correction in t2/NOESY 
     (default and recommended)

    Set d1_lock_only='y' to keep lock on only in d1 (default)

    Set phase = 1,2   for States-TPPI  acquisition in t1 (NOESY)
    Set phase2 = 1,2  for Echo-AntiEcho  or States/TPPI acquisition
    in t2  (13C HSQC)
*/
#include <standard.h>

static int   phi1[]  = {1,1,3,3,3,3,1,1},  
	     phi2[]  = {0,2},          /* t2, HSQC phase  t1 evolution  time */
             phi3[]  = {2},      
	     phi4[]  = {0,0,2,2},   /* t4phase, NOE phase t2 evolution time  */
             phix[]  ={0},        /* t10phase , SE-HSQC pphase in rev INEPT */
             rec[]   = {0,2,2,0};

static double   d2_init=0.0;


void pulsesequence()
{
/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR], f2180[MAXSTR],     /* Flag to start t1 @ halfdwell */
	    zqf[MAXSTR],                      /*flag for zero-quantum filter */
            shp_zqf[MAXSTR],
            d1_lock_only[MAXSTR],
            refocN15[MAXSTR],
            refocCO[MAXSTR], COshape[MAXSTR],
            C180shape[MAXSTR],
            SE[MAXSTR];
	 
 
int         t1_counter,icosel;         /* used for states tppi in t1 */

double      tau1, tau2,   /*  t1 delay */
            ni2=getval("ni2"),
      				        
	    tauch =  getval("tauch"), 	   /* 1/4J   evolution delay */
            tauch1 =  getval("tauch1"),   /* 1/4J or 1/8JCH   evolution delay */

	    mix =  getval("mix"),

            corrD, corrB, /* small  correction delays */
        
        pw_zqf = getval("pw_zqf"),      /* zeroquantum filter parameters */
        tpwr_zqf = getval("tpwr_zqf"),

       dof_dec =  getval("dof_dec"),

	pwClvl = getval("pwClvl"),           /* power for hard C pulses */
        pwC180lvl = getval("pwC180lvl"),  /*power levels for 180 shaped pulse */
        pwC180lvlF = getval("pwC180lvlF"),
        pwC = getval("pwC"),          /* C 90 degree pulse length at pwClvl */	 
        pwC180 = getval("pwC180"),   /* shaped 180 pulse on Cchannle */
 
	sw1 = getval("sw1"),

        pwNlvl = getval("pwNlvl"),
        pwN = getval("pwN"),
 
        pwCOlvl = getval("pwCOlvl"),
        pwCO = getval("pwCO"),

        gstab = getval("gstab"),
        gstab1 = getval("gstab1"), /* recovery for club sandwitch, short*/
        gt0 = getval("gt0"),
	gt2 = getval("gt2"),
        gt3 = getval("gt3"),                               /* other gradients */
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),

        gt6 = getval("gt6"),
        gt9 = getval("gt9"),  
	
        gzlvl0 = getval("gzlvl0"),
	gzlvl2 = getval("gzlvl2"),
        gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),

	gzlvl6 = getval("gzlvl6"),
	gzlvl9 = getval("gzlvl9"),


        gzlvlr = getval("gzlvlr");

    getstr("f1180",f1180);
    getstr("f2180",f2180);

    getstr("C180shape",C180shape);

    getstr("COshape",COshape); getstr("refocCO",refocCO);
    getstr("refocN15",refocN15);
    getstr("SE",SE);
    getstr("zqf",zqf);getstr("shp_zqf",shp_zqf);
    getstr("d1_lock_only",d1_lock_only);

/*   LOAD PHASE TABLE    */

	 
	settable(t2,2,phi2);
	settable(t3,1,phi3);
	settable(t4,4,phi4);
        settable(t10,1,phix);
	settable(t11,4,rec);


/*   INITIALIZE VARIABLES   */

 
 

/* CHECK VALIDITY OF PARAMETER RANGES */

  if((dm[A] == 'y' || dm[B] == 'y'))
  { text_error("incorrect dec1 decoupler flags! Should be 'nnn' or 'nny' "); psg_abort(1); }

  if((dm2[A] != 'n' || dm2[B] != 'n' || dm2[C] != 'n'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nnn' "); psg_abort(1); }

  if( dpwr2 > 45 )
  { text_error("don't fry the probe, DPWR2 too large!  "); psg_abort(1); }
  if( dpwr > 50 )
  { text_error("don't fry the probe, DPWR too large!  "); psg_abort(1); }

  if( (pw > 20.0e-6) )
  { text_error("don't fry the probe, pw too high ! "); psg_abort(1); }

  if( (pwC > 200.0e-6)  )
  { text_error("don't fry the probe, pwC too high ! "); psg_abort(1); }


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-TPPI-Haberkorn element */

if (phase2 == 1)  { if(SE[A]=='y'){ tsadd(t10,2,4);}  
                    icosel = +1;}
       else       { icosel = -1; if(SE[A]=='n') {tsadd(t2,1,4);}   }

if (phase1 == 2)  {tsadd(t4 ,1,4);}
/* Calculate modifications to phases for States-TPPI acquisition          */

    
    
   if(d3_index % 2) 	{ tsadd(t2,2,4); tsadd(t11,2,4); }
   if(d2_index % 2)     { tsadd(t4,2,4); tsadd(t11,2,4); }


/*  Set up f1180, f2180  */
   
    tau2 = d3;
    if((f2180[A] == 'y') && (ni2 > 0.0)) 
	{ tau2 += ( 1.0 / (2.0*sw2) ); if(tau2 < 0.2e-6) tau2 = 0.0; }
    tau2 = tau2/2.0;


    tau1 = d2;
    tau1 += -pw*4.0/M_PI-rof1*2.0;

    if((f1180[A] == 'y') && (ni > 0.0)) 
          {tau1 += ( 1.0 / (2.0*sw1) );  }
    if(tau1 < 0.2e-6) {tau1 = 0.0;}
    tau1 = tau1/2.0;


 
corrB=0.0;
corrD=2.0/M_PI*pwC-pw-rof1;

if (corrD < 0.0) {corrB=-corrD; corrD=0.0;}


/* BEGIN PULSE SEQUENCE */

status(A);
	obsoffset(tof);        
	obspower(tpwr); decoffset(dof);
	
 	obspwrf(4095.0);

        delay(d1);

 	/* if(d1_lock_only[A]=='y'){lk_hold();} */

         txphase(zero);
         decphase(zero);
	 
  dec2power(pwNlvl); dec2pwrf(4095.0);
 decpower(pwC180lvl); decpwrf(pwC180lvlF);

/************************* t2 evolution, 1H */
        initval(1.0, v10);
        obsstepsize(45.0);
        xmtrphase(v10);     /*45 degree phase for water */

if(tau1*2.0<pwC180+pwN*2.0) 
       {
        rgpulse(pw,t4,rof1,rof1);    
        xmtrphase(zero);   
        delay(tau1);
        delay(tau1);
        rgpulse(pw,zero,rof1,rof1);
        decshaped_pulse(C180shape,pwC180,zero, 0.0, 0.0);
        dec2rgpulse(2.0*pwN,zero,0.0,0.0);
        } 
else
        {
        rgpulse(pw,t4,rof1,rof1);    
        xmtrphase(zero);   
        delay(tau1-pwN-pwC180*0.5);
        decshaped_pulse(C180shape,pwC180,zero, 0.0, 0.0);
        dec2rgpulse(2.0*pwN,zero,0.0,0.0);
        delay(tau1-pwN-pwC180*0.5);
        rgpulse(pw,zero,rof1,rof1);
        }
      /* NOESY period */

       if (SE[A]=='n'){ zgradpulse(gzlvl0*.9, gt0);} else {delay(gt0);}
  
      /* zero-quantum option */
       if(zqf[A]=='y') 
 	{
	 	rgradient('z',gzlvlr); 
          	obspower(tpwr_zqf); 
          	shaped_pulse(shp_zqf,pw_zqf,zero,rof1,rof1);
      		obspower(tpwr);
                rgradient('z',0.0);
	}
	else {delay(pw_zqf);};

        delay(mix-gt0*2.0-4.0*gstab - pw_zqf);

        decpower(pwClvl); decpwrf(4095.0);
	decrgpulse(pwC, zero, rof1, rof1);
	zgradpulse(gzlvl0, gt0);

	delay(4.0*gstab);
	 


/************ H->C */
        txphase(zero);
        decphase(zero);
   	rgpulse(pw,zero,rof1,rof1);                  
   	
        decpower(pwC180lvl); decpwrf(pwC180lvlF);

        delay(gstab);
	zgradpulse(gzlvl6, gt6);
	delay(tauch - gt6 -gstab);

   
        simshaped_pulse("hard",C180shape,2.0*pw,pwC180,zero,zero, 0.0, 0.0);
 
   	 
        decpower(pwClvl); decpwrf(4095.0);
   	txphase(one);
        delay(tauch - gt6-gstab);
	zgradpulse(gzlvl6, gt6);
	delay(gstab);

 	rgpulse(pw, one, rof1,rof1);
 	
  /*** purge   */  
      
        txphase(zero);
        decphase(t2);
        delay(gstab);
	zgradpulse(gzlvl2, gt2);
	
       delay(3.0*gstab);
	
	
  /*  evolution on t2 */	

      decrgpulse(pwC, t2, 0.0, 0.0);
      decphase(zero);
      decpower(pwC180lvl); decpwrf(pwC180lvlF);
 	      
      delay(tau2);
      obsunblank();
      rgpulse(pw,one,rof1,0.0); 
      rgpulse(2.0*pw,zero,0.0,0.0);
      rgpulse(pw,one,0.0,rof1);
      obsblank(); 

      if(refocN15[A]=='y') dec2rgpulse(2.0*pwN,zero,0.0,0.0);
      /*n15 refocusing */

   if(refocCO[A]=='y')  
     { 
       decpower(pwCOlvl);
       decshaped_pulse(COshape,pwCO,zero, 3.0e-6, 3.0e-6);
       decpower(pwC180lvl);
     }

    delay(tau2);
      
  /*************** CODING with CLUB sandwitches and BIPS */
	  

   if (SE[A]=='y'){ zgradpulse(gzlvl3*icosel, gt3);  delay(gstab1);}
          
   decshaped_pulse(C180shape,pwC180,zero, 0.0, 0.0);

   if (SE[A]=='y'){ zgradpulse(-gzlvl3*icosel, gt3); delay(gstab1);}

  delay(rof1*2.0+pw*4.0 +pwC*4.0/M_PI +2.0*POWER_DELAY+2.0*PWRF_DELAY);

   if(refocN15[A]=='y') delay(2.0*pwN); /*n15 refocusing */
   if(refocCO[A]=='y') /* ghost CO pulse */
    { 
     decpower(pwCOlvl);
     decshaped_pulse(COshape,pwCO,zero, 3.0e-6, 3.0e-6);
     decpower(pwC180lvl);
    }

               
  if (SE[A]=='y'){ zgradpulse(-gzlvl3*icosel, gt3); delay(gstab1);}

  decshaped_pulse(C180shape,pwC180,zero, 0.0, 0.0);
  if (SE[A]=='y'){zgradpulse( gzlvl3*icosel, gt3); delay(gstab1);}
  decpower(pwClvl); decpwrf(4095.0);
   		
        /*reverse INPET */

 if (SE[A]=='y'){  /*______SE version ____*/
 
	simpulse(pw, pwC, zero, t10, 0.0, 0.0);
	delay(gstab);  
	zgradpulse(gzlvl4,gt4);decpower(pwC180lvl); decpwrf(pwC180lvlF);
	delay(tauch - gt4 - gstab -corrD-pwC180 -POWER_DELAY-PWRF_DELAY);
	  

       decshaped_pulse(C180shape,pwC180,zero, 0.0, 0.0);
       delay(corrD);
       rgpulse(2.0*pw,zero,rof1,rof1);
	zgradpulse(gzlvl4,gt4);
	delay(tauch - gt4 - gstab -corrB-pwC180 -POWER_DELAY-PWRF_DELAY); 
        
	delay(gstab);
            
	decphase(one); txphase(one);
        decshaped_pulse(C180shape,pwC180,zero, 0.0, 0.0);
        decpower(pwClvl); decpwrf(4095.0); 
        delay(corrB);
       

    simpulse(pw, pwC, one, one, 0.0, 0.0);

        decpower(pwC180lvl); decpwrf(pwC180lvlF);
	delay(gstab-POWER_DELAY-PWRF_DELAY -WFG_START_DELAY);  
	zgradpulse(gzlvl5,gt5);
	delay(tauch1 - gt5 - gstab);

	simshaped_pulse("hard",C180shape,2.0*pw,pwC180,zero,zero, 0.0, 0.0);

	delay(tauch1- gt5 - gstab -  WFG_STOP_DELAY);
	zgradpulse(gzlvl5,gt5);
	delay(gstab-rof1);    
	rgpulse(pw, zero,rof1,rof1);

    /* echo and decoding */

       delay(gstab+gt9-rof1+10.0e-6 + 2.0*POWER_DELAY+PWRF_DELAY +2.0*GRADIENT_DELAY); decoffset(dof_dec);
       rgpulse(2.0*pw, zero,rof1,rof1);
        delay(10.0e-6);
       zgradpulse(gzlvl9,gt9);
         

         
        decpower(dpwr);	decpwrf(4095.0);decoffset(dof_dec);

        dec2power(dpwr2); /* POWER_DELAY EACH */
	delay(gstab);
      } /* end of SE version */
 
else
      { /*non-SE version */

          decrgpulse(pwC, zero, 0.0, 0.0);
          zgradpulse(-gzlvl2,gt2);delay( gstab);

        rgpulse(pw,zero,rof1,rof1);

	  zgradpulse(gzlvl5,gt5); decoffset(dof);
	  delay(tauch1 - gt5 -rof1);
          simshaped_pulse("hard",C180shape,2.0*pw,pwC180,zero,zero, 0.0, 0.0);
          zgradpulse(gzlvl5,gt5);
	  delay(tauch1 - gt5 -2.0*POWER_DELAY+PWRF_DELAY -rof1) ;
          
         decpower(dpwr);	decpwrf(4095.0);decoffset(dof_dec);

        dec2power(dpwr2);
       rgpulse(pw,zero,rof1,rof2);
}	
status(C);
	setreceiver(t11);
/*
        acquire(np, 1.0/sw);
        if(d1_lock_only[A]=='y')
          lk_sample();
*/
}		 
