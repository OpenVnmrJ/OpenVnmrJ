/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 
   hNcaNH with double-TROSY feature
   Use for 2H-13C-15N labeled proteins. 

  Correlations: 
   t1(ni)   : N[i],N[i-1],N[i+1]
   t2(ni2)  : N[i]
   t3(np/at): HN[i]

  Depending upon tmeCN and size of the protein: 
    [default] timeCN 14-16ms    auto-peak N[i] has different sign than cross-peaks N[i +/-1] 
              timeCN 20-24ms    auto-peak N[i] almost absent, cross-peaks have larger intensity
   
   Sensitivity is about 2/3 of HNCACB experiment
             (See Supplementary for  hNCAnh experiment paper)

   Reference hNCAnH:   Frueh, Arthanari, Koglin, Walsh, Wagner      JACS 2009 v 131 pp 12880-12881
   Reference hNcaNH:   Frueh, Sun, Vosburg, Walsh, Hoch and Wagner, JACS 2006 v 128 pp 5757-5763

   Phase cycle slightly modified to utilize steady-state 15N magnetization 


   This is an "A" style of the pulse sequence programming: 
      all shaped pulses (except proton) are generated automatically on-the-flight by Pbox calls,
      right before experiment starts. User needs to have only three reference (calibration)
      parameters per channel: pw90, pw90 power level and compression factor (see BioPack manual)

   For any automatically generated shaped pulse,shape parameters are determined by a single
   "Pbox input string parameter" (see Pbox manual).
   
     Example: 180-degree shaped pulse on CO while sitting on CA will be  
              determined by "Pbox input string" parameter 
              
              CO180offCA_in_str = 'gaus 60p 120p 0 0 180', i.e.
              Gaussian shape, covering 60ppm, offset by 120ppm,
              spin state 0, pulse phase 0, flip angle on resonance 180 degrees.

     NOTE: due to this particular pulse sequence implementation peculiarities,  
           arraying  string parameter "whatever_pulse_in_str" will not work:
           experiment will run with whatever_pulse_in_str = first-element-of-array 

   SOME CARE TAKEN FOR INOVA-TYPE AP DELAYS; NOT TESTED ON INOVA SYSTEMS
   
    
    exp_mode='t' for trosy in the double-trosy hNcaNH

    exp_mode='R' to check signal in 1D and test relaxation losses during 2*timeCN  (~2*16ms=32ms) transfers
    
    set dm3='nyn' for deuterium decoupling on CA during 2*timeCN transfer


 How to set up:

  1a)  Set exp_mode='R', timeCN=0.0001 (=100us), run 1D expriment
  1b)  Set exp_mode='R', timeCN=0.0001 (=100us),
       array  gzlvl6 (coherence-selection decoding gradient) for max transfer efficiency
       at this stage, 1D sensitivity should be almost the same as regular TROSY-HNCA


  1c) [recommended,  can result in up to 5-10% signal-to-noise increase, depending on console/amplifier vintage.]

      Check whether there is a need to account for a small phase shift due to different power levels used for 
      shaped 180 pulse on CA and hard 13C pw90.

      Set exp_mode='R', timeCN=0.0001 (=100us),
      Record several 1Ds with slightly different phases 
      for the shaped 180-degree pulse  on CA in the middle of  2*timeCN period:
      
       Example:

        -> run 1D with  CA180n_in_str = 'q3 18p  0  0 0 180' 
           (shaped 180-degree q3 pulse on CA covering 18ppm  with no small phase shift) 
            note 1D intensity, call it "I0" 

        -> run 1D with  CA180n_in_str = 'q3 18p  0  0 10 180' 
            (shaped 180 on CA pulse with 10degree small phase shift) note 1D intensity, call it "I+"

        -> run 1D with -10degree small shift, call it "I-"

           Chose whatever gives better S/N 
       
        NOTE: due to this particular pulse sequence implementation peculiarities,  
              arraying string parameter "CA180n_in_str" will not work:
              experiment will run with CA180n_in_str = first-element-of-array 

  2a) [optional] Check 2H decoupler efficiency

      Set exp_mode='R', timeCN=0.016, dm3='nnn','nyn'. Run arrayed 1D 
      There should be a noticable difference between 2H-decouled and coupled spectra.


  2b) [optional] Check relaxation during 2*timeCN
      Set exp_mode='R',  array timeCN=0.0001, 0.003, 0.006 ... up to 0.025 (25ms)
      Run this arrayed 1D to get an idea of CA* R2*.
      That would help to choose timeCN, see supplemetary for the original paper. 
      Typical timeCN starting value for a large protein with CA* R2*=40 1/s is about 16-20ms
  

  3) set exp_mode='t', timeCN around 16ms, dm3='nyn' and run as 3D with normal (linear)
     sampling or with NUS (Non-Uniform Sampling, recommended)

  timeTN1 = first N[i]-> Ca[i,i-1] tranfer 11-13ms, 1/(4*JNCa)          (12.4ms)
  timeCN  = Ca[i,i-1]-> N[i,i+1,i-1] transfer, 16-25ms          
  timeTN  = last NCa->N transfer, shortened by 1/(4JHN) = timeTN1-tauNH (10ms) 
*/

#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */	     
static int   
                     
             phx[]  ={0},   
             phy[]  ={1},   
             psi1[] ={2},
             psi2[] ={3},
             psi2c[]={1}, /* psi2 for first 13C 90 in reverse inept */
 
	     phi1[]  = {0,0},	phi2h[] = {0,0,0,0},  /* HSQC first part, regular hNcaNH */


             phi2[]  = {0,0,1,1},  /* first  N15 in  first N->Ca tranfer, t1, double trosy, default   */
             
            /* phi2 = 2 phi6=1 phi7=0 rec=+ */
            /* phi2 = 1 phi6=3 phi7=2 rec=+ */
            /* phi2 = 0 phi6=1 phi7=0 rec=-  for utilizing 15N steady state*/


             phi6[]  = {1,1,3,3}, 
            /* last pw90 on proton in the first inept, should go with phi2 for using 15N steady-state */ 
             

	     phi7[]  = {0,0,2,2}, /*flip backs and 180 in 1st trosy, double-trosy filter */	 
             phi8[]  = {2,2,0,0},  /* phi8 is a reverse of of ph7 */     
	     phi3[]  = {0,2,0,2}, /* 90 on ca */	
             phi4[]  = {1,1,1,1,1,1,1,1}, /* first 90 pwN in TROSY t2 */
	     phi5[]  = {0,0},  
             rec[]   = {2,0,0,2,2,0,0,2};


static double   d2_init=0.0, relaxTmax;

static shape CA180, CA180n, CA90, CO180offCA;


/* xxxxxxxxxxxxxxxxxxxxxxxxx    c13pulse via Pbox  returns element duration xxxxxxx */

double dec_c13_shpulse(shape pwxshape, codeint phase) 
{
 
 decpower(pwxshape.pwr);
 decshaped_pulse(pwxshape.name, pwxshape.pw, phase, 0.0, 0.0);
 decpower(getval("pwClvl"));
 return(pwxshape.pw + 2.0*POWER_DELAY +WFG_STOP_DELAY + WFG_START_DELAY);
}


pulsesequence()
{
/* DECLARE AND LOAD VARIABLES */

char	 
        CA90_in_str[MAXSTR],     
  	CA180_in_str[MAXSTR],  CA180n_in_str[MAXSTR],       
        CO180offCA_in_str[MAXSTR],   
        RFpars[MAXSTR],       
        exp_mode[MAXSTR],   /* flag to run 3D, or 2D time-shared 15N TROSY /13C HSQC-SE*/   
        
 
	f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
	f2180[MAXSTR],
	f3180[MAXSTR];			    /* do TROSY on N15 and H1 */
 
int         icosel;      			  /* used to get n and p type */
     




double  x,y,z, t2max, t1max,tpwrs,
        tau1, tau2,   /*evolution times in indirect dimensions */
             

        tpwrsf_d = getval("tpwrsf_d"), /* fine power adustment for first soft pulse(down)*/
        tpwrsf_u = getval("tpwrsf_u"), /* fine power adustment for second soft pulse(up) */
        pwHs = getval("pwHs"),                     /* H1 90 degree pulse length at tpwrs */
        compH =getval("compH"),
        ni2=getval("ni2"),
        tauNH=getval("tauNH"),    /* 1/(4Jhn), INEPTs, 2.4ms*/
        tauNH1=getval("tauNH1"),    /* 1/(4Jhn), TROSY in CN CT, 2.7ms*/

	timeTN1=getval("timeTN1"),  /* CT time for (first) N->CA*N transfer */
        timeTN=getval("timeTN"),    /* CT time for last SE TROSY */ 
        timeCN=getval("timeCN"),    /* CT time for CA -> N transfer, middle */
 
 

	pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
	pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
        compC = getval("compC"),
        dfrq = getval("dfrq"),

      	              
	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
        

	gstab = getval("gstab"),
	g6bal= getval("g6bal"), /* balance of the decoding gradient
				 around last 180 pulse on 1H
				 g6bal=1.0 : full g6 is on the right side of the last pw180 on 1H
				 g6bal=0.0:  full g6 is on the left side*/



  	gt0 = getval("gt0"),     
        gt1 = getval("gt1"),
        gt2 = getval("gt2"),
 	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
        gt6 = getval("gt6"),
 	gt7 = getval("gt7"),
	gt8 = getval("gt8"),
	gt9 = getval("gt9"),
	gt10 = getval("gt10"),

	gzlvl0 = getval("gzlvl0"),
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),	
        gzlvl6 = getval("gzlvl6"),
	gzlvl7 = getval("gzlvl7"),
	gzlvl8 = getval("gzlvl8"),
	gzlvl9 = getval("gzlvl9"),
	gzlvl10 = getval("gzlvl10"),
        gzlvl11 = getval("gzlvl11");

    getstr("f1180",f1180);
    getstr("f2180",f2180);

    getstr("exp_mode",exp_mode);
 
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));          /*needs 1.69 times more*/
    tpwrs = (int) (tpwrs);                               /*power than a square pulse */

    if (tpwrsf_d<4095.0)
      tpwrs=tpwrs+6.0;  /* add 6dB to let tpwrsf_d control fine power ~2048*/

/*   LOAD PHASE TABLE    */
	
        
        settable(t1,1,phi1);

        settable(t2,4,phi2); /* default double trosy */
		 if (exp_mode[A] == 'h') {settable(t2,4,phi2h);}; /*option for regular hNcaNH */
                
        settable(t3,4,phi3);
        settable(t4,8,phi4);
        settable(t5,2,phi5);
        settable(t6,4,phi6);
        
        settable(t7,4,phi7);
        settable(t8,4,phi8);

	settable(t21,1,psi1);  /*trosy and SE hsqc in reverse INPET */
	settable(t22,1,psi2);
        settable(t23,1,psi2c);

          
         settable(t31,8,rec); 
	 



/* some checks */

 if((dm2[A] == 'y') || (dm2[B] == 'y') || (dm2[C] == 'y') || (dm2[D] == 'y'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nnnn' "); psg_abort(1); }

     if ( dm3[A] == 'y' || dm3[C] == 'y' )
       { printf("incorrect dec3 decoupler flags! Should be 'nyn' or 'nnn' ");
							             psg_abort(1);}	
    if ( dpwr3 > 56 )
       { printf("dpwr3 too large! recheck value  "); psg_abort(1);}

    if ( (dm3[B] == 'y' )  && (timeCN*2.0 > 60.0e-3) )
       { printf("too lond time for 2H decoupling, SOL ");psg_abort(1);}


/*   INITIALIZE VARIABLES   */
 

/* PHASES AND INCREMENTED TIMES */

 
    if (phase1 == 2)  {tsadd(t2 ,1,4);}
    if(d2_index % 2)  {tsadd(t2,2,4); tsadd(t31,2,4); }  
   
    /*   Ntrosy , last part */
/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

      if (phase2 == 1)    {				                      icosel =  1;  }
            else 	  {  tsadd(t21,2,4);  tsadd(t22,2,4); tsadd(t23,2,4); icosel = -1;  }

    if(d3_index % 2) 	{ tsadd(t4,2,4); tsadd(t5,2,4); tsadd(t31,2,4); }   /* ECHO-ANTIECHO + STATES-TPPI t1, TROSY last step */

 
/* setup semi-constant time in t2 (ni) */

    t2max=(ni2-1.0)/sw2; 
    tau2 = d3;  
    
    if((f2180[A] == 'y') && (ni2 > 0.0)) 
          {tau2 +=  0.5/sw2 ; t2max+= 0.5/sw2; }

    if(tau2 < 0.2e-6) {tau2 = 0.0;}

    if( t2max < timeTN*2.0) {t2max=2.0*timeTN;}; /* if not enough  ni2 increments, then just regular CT in t2/ni2 CN */
        

    
/* setting up semi-CT on t1 (ni) dimension */
    tau1  = d2; 
    t1max=(ni-1.0)/sw1;
  
    if((f1180[A] == 'y') && (ni > 0.0)) 
          {tau1 +=  0.5/sw1 ; t1max+= 0.5/sw1; }  
	
  
    if( t1max < timeTN1*2.0) {t1max=2.0*timeTN1;}; /* if not enough  ni increments, then just regular CT in t1/ni CN */

   



    
 if(FIRST_FID)                                            /* call Pbox */
        {
         getstr("CA180_in_str",CA180_in_str);  getstr("CA180n_in_str",CA180n_in_str);
         getstr("CA90_in_str",CA90_in_str);    getstr("CO180offCA_in_str",CO180offCA_in_str);

	 strcpy(RFpars,             "-stepsize 0.5 -attn i");

            CA180 =  pbox("et_CA180_auto", CA180_in_str, RFpars, dfrq, compC*pwC, pwClvl);
	    CA180n = pbox("et_CA180n_auto", CA180n_in_str, RFpars, dfrq, compC*pwC, pwClvl);
            CA90  =  pbox("et_CA90_auto", CA90_in_str, RFpars, dfrq, compC*pwC, pwClvl);
            CO180offCA = pbox("et_CO180offCA_auto", CO180offCA_in_str, RFpars, dfrq, compC*pwC, pwClvl);
       
             printf("t1max is %f\n",t1max);
	     printf("t2max is %f\n",t2max);
	 };





/* BEGIN PULSE SEQUENCE */

status(A);

	obspower(tpwr);
	decpower(pwClvl);
	dec2power(pwNlvl);

	txphase(zero);
        decphase(zero);
        dec2phase(zero);

	 delay(d1); 

        zgradpulse(gzlvl2, gt2);
	delay(gstab*3.0);

 if (exp_mode[B]=='n')  /* test for steady-state 15N */
           {
		dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
           }

       /* Hz -> HzXz INEPT */

   	rgpulse(pw,zero,rof1,rof1);                 /* 1H pulse excitation */

        zgradpulse(gzlvl0, gt0);
	delay(tauNH -gt0);

   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

   	delay(tauNH - gt0 -gstab);
	zgradpulse(gzlvl0, gt0);
	delay(gstab);

 	rgpulse(pw, t6, rof1, rof1);

       /* on HzNz now */
      /* water flipback*/
        obspower(tpwrs); obspwrf(tpwrsf_u);
 	shaped_pulse("H2Osinc",pwHs,zero,rof1,rof1);
	obspower(tpwr); obspwrf(4095.0);

       /* purge */
       
	zgradpulse(gzlvl3, gt3);
        dec2phase(t2);
	
	delay(gstab*2.0);
        

/*  HzNz -> NzCAz  +t1 evolution*/

       

	dec2rgpulse(pwN, t2, 0.0, 0.0);

      /* double-trosy hNcaNH */

		
		delay(tauNH1  -pwHs-4.0*rof1 -pw  -2.0*POWER_DELAY -WFG_STOP_DELAY-WFG_START_DELAY);
                
        	  	obspower(tpwrs); obspwrf(tpwrsf_d);
 			shaped_pulse("H2Osinc",pwHs,two,rof1,rof1);
			obspower(tpwr); obspwrf(4095.0);
      
        		rgpulse(pw, zero, rof1, rof1);   
        	        rgpulse(pw, t7, rof1, rof1);

        		obspower(tpwrs); obspwrf(tpwrsf_u);
 			shaped_pulse("H2Osinc",pwHs,t8,rof1,rof1);
			obspower(tpwr); obspwrf(4095.0);
              		
        	dec_c13_shpulse(CO180offCA,zero);  

        	delay(tau1*0.5);
		dec_c13_shpulse(CO180offCA,zero); dec2phase(zero);

       		delay( timeTN1 -tauNH1 
                       -pwHs  -4.0*rof1 -pw  -2.0*POWER_DELAY -WFG_STOP_DELAY-WFG_START_DELAY
                       -CA180.pw -2.0*CO180offCA.pw  -3.0*(2.0*POWER_DELAY +WFG_STOP_DELAY +WFG_START_DELAY) 
                      ); 

		dec_c13_shpulse(CA180,zero);

        

		delay(tau1*0.5 -timeTN1*tau1/t1max); 

                dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
       
		delay( timeTN1 -tau1*timeTN1/t1max);

       		dec2rgpulse(pwN, zero, 0.0, 0.0);

/*   on CAzNz now */

/* purge */
       zgradpulse(gzlvl7, gt7);
       delay(gstab);	

	if(dm3[B] == 'y'){  dec3unblank();
                             if(1.0/dmf3>900.0e-6) {
						     dec3power(dpwr3+6.0);
						     dec3rgpulse(0.5/dmf3, one, 1.0e-6, 0.0e-6);
						     dec3power(dpwr3);
						     }
			   else dec3rgpulse(1.0/dmf3, one, 1.0e-6,0.0e-6);
  			   dec3phase(zero);       setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
	                    }	




 
	/* dec_c13_shpulse(CA90,t3);*/
	decrgpulse(pwC,t3,0.0,0.0);

	 /*decrgpulse(pwC*2.0,zero,rof1,rof1);*/
	decphase(zero);
	delay(timeCN-2.0*pwN - POWER_DELAY -WFG_START_DELAY);
        if (exp_mode[A]=='R')  /* test CA.N relaxation rate  */
           {
	    delay(2.0*pwN);
           } else
        {dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);}
	

	dec_c13_shpulse(CA180n,zero);
	delay(timeCN-  POWER_DELAY -WFG_STOP_DELAY);

	/*dec_c13_shpulse(CA90,zero);*/

	decrgpulse(pwC,zero,0.0,0.0);
 

 		if(dm3[B] == 'y')  {                     
		                    setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3);                  
					 if(1.0/dmf3>900.0e-6) {
							     dec3power(dpwr3+6.0);
							     dec3rgpulse(0.5/dmf3, three, 1.0e-6, 0.0e-6);
							     dec3power(dpwr3);
							     }
						else dec3rgpulse(1.0/dmf3, three, 1.0e-6, 0.0e-6);
				      dec3blank(); delay(PRG_START_DELAY);
			   	     }


       zgradpulse(gzlvl5, gt5);
	dec2phase(t4);
       delay(gstab);	



       /*   t2 (N) evolution  + back to NH*/

   	dec2rgpulse(pwN, t4, 0.0, 0.0);
        dec2phase(zero);
	      

        delay(timeTN- timeTN*tau2/t2max);

	dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);

        delay(tau2*0.5  - timeTN*tau2/t2max);

        dec_c13_shpulse(CA180,zero);

  
       delay(timeTN - CA180.pw -2.0*CO180offCA.pw  -gt4-gstab -pwHs-3.0*rof1
              +4.0*pwN/3.1415-pw
             -8.0*POWER_DELAY -4.0*WFG_STOP_DELAY-4.0*WFG_START_DELAY
             -2.0*GRADIENT_DELAY);

       dec_c13_shpulse(CO180offCA,zero);
   
       delay(tau2*0.5);
       
       dec_c13_shpulse(CO180offCA,zero);
 


       zgradpulse(gzlvl4, gt4);
       delay(gstab);	  
    

       /*Water flipback (flipdown actually ) */
        obspower(tpwrs); obspwrf(tpwrsf_d);                         
 	shaped_pulse("H2Osinc",pwHs,three,rof1,rof1);
	obspower(tpwr);  obspwrf(4095.0); 
     



/* reverse double INEPT */


/* 90 */  
   rgpulse(pw, t21, rof1, rof1);  
   zgradpulse(gzlvl11, gt1);		 
       
  delay(tauNH   -gt1 -rof1 
        -CA180.pw -2.0*POWER_DELAY - WFG_STOP_DELAY- WFG_START_DELAY
        );
  dec_c13_shpulse(CA180,zero);

  sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

  delay(tauNH  - gt1 -gstab);
   zgradpulse(gzlvl11, gt1);	
	 
   delay(gstab);
 

 
 /* 90 */ 

  sim3pulse(pw, 0.0, pwN, one, zero, zero, 0.0, 0.0);
  zgradpulse(gzlvl1, gt1);		 
  delay(tauNH  -gt1);

  sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
 

  delay(tauNH -POWER_DELAY  -gt1- gstab);
  zgradpulse(gzlvl1, gt1);
dec2phase(t22);		 
   delay(gstab);


              
  sim3pulse(0.0,0.0, pwN, one, zero, t22, 0.0, 0.0);  
 
   


   
 
	zgradpulse(-(1.0-g6bal)*gzlvl6*icosel, gt6); /* 2.0*GRADIENT_DELAY */
        delay( gstab  -pwN*0.5 +pw*(2.0/3.1415-0.5) );

        rgpulse(2.0*pw, zero, rof1, rof1);

	dec2power(dpwr2); decpower(dpwr);	
			      
        zgradpulse(g6bal*gzlvl6*icosel, gt6);		/* 2.0*GRADIENT_DELAY */
        delay(gstab +2.0*POWER_DELAY );
   status(C);
	setreceiver(t31);
}	 
