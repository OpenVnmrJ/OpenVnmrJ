/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gChsqc_BB.c 

    This pulse sequence will allow one to perform the following experiment:

                 13C-HSQC with sensitivity enhancement,
                 broadband 13C coverage (Methyls to Aromatics) 
                 inspired by Shaka's papers
                 JMR v136 p54  1999
                 JMR v151 p269 2001

    SOME CARE TAKEN TO ACCOUNT FOR "INOVA-TYPE" HIDDEN DELAYS (POWER_DELAY, etc.)

         In uniformly C13 labeled samples,
	 there is a possibility of CA->HB and CB->HA correlations in this HSQC,
         especially with high s/n experiments.
         This is a price one pays for having SE enhancement and Jcc scalar couplings.
	 On the other hand, water suppression is good
 	 and the whole C13 range can be covered in one spectrum.


        	  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nny', dmm = 'ccp'  for C13 decoupling.
    
    Must set phase = 1,2  for States-TPPI acquisition in t1 

    Evgeny Tishchenko, Agilent Technologies December 2011

    

*/
#include <standard.h>
static int   phi1[]  = {1,1,3,3,3,3,1,1},
	     phi2[]  = {0,2},
             phi3[]  = {2},
	     phi4[]  = {0},
             phix[]    ={0},
             rec[]   = {0,2,0,2};
static double   d2_init=0.0;
void pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
	    CT[MAXSTR],
            refocN15[MAXSTR],
            refocCO[MAXSTR], COshape[MAXSTR],
            C180shape[MAXSTR];
 
int         t1_counter,icosel;  		        /* used for states tppi in t1 */

double      tau1,         				         /*  t1 delay */
	    tauch =  getval("tauch"), 	   /* 1/4J   evolution delay */
            tauch1 =  getval("tauch1"),   /* 1/4J or 1/8JC13H   evolution delay */
    	    timeCC =  getval("timeCC"),  /* 13C constant-time if needed*/
	    corrD, corrB, /* small  correction delays */

         dof_dec =  getval("dof_dec"), /*decoupler offset for decoupling during acq - folding */

	pwClvl = getval("pwClvl"),	              /* power for hard C13 pulses */
        pwC180lvl = getval("pwC180lvl"),           /*power levels for 180 shaped pulse */
        pwC180lvlF = getval("pwC180lvlF"),
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */	 
        pwC180 = getval("pwC180"),   /* shaped 180 pulse on C13channl */
 
	sw1 = getval("sw1"),

        pwNlvl = getval("pwNlvl"),
        pwN = getval("pwN"),
 
        pwCOlvl = getval("pwCOlvl"),
        pwCO = getval("pwCO"),

        gstab = getval("gstab"),
        gstab1 = getval("gstab1"), /* recovery for club sandwitch, short*/
        gt0 = getval("gt0"),
        gt1 = getval("gt1"),
	gt2 = getval("gt2"),
        gt3 = getval("gt3"),                               /* other gradients */
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
        gt9 = getval("gt9"),  
	
        gzlvl0 = getval("gzlvl0"),
	gzlvl2 = getval("gzlvl2"),
        gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl9 = getval("gzlvl9");



    getstr("f1180",f1180);
    getstr("C180shape",C180shape);
    getstr("COshape",COshape); getstr("refocCO",refocCO);
    getstr("refocN15",refocN15);
    getstr("CT",CT);


/*   LOAD PHASE TABLE    */

	 
	settable(t2,2,phi2);
	settable(t3,1,phi3);
	settable(t4,1,phi4);
        settable(t10,1,phix);
	settable(t11,4,rec);


/*   INITIALIZE VARIABLES   */

 
 

/* CHECK VALIDITY OF PARAMETER RANGES */
/* like in olde good times */

  if((dm[A] == 'y' || dm[B] == 'y'))
  { text_error("incorrect dec1 decoupler flags! Should be 'nnn' or 'nny' "); psg_abort(1); }

  if((dm2[A] != 'n' || dm2[B] != 'n' || dm2[C] != 'n'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nnn' "); psg_abort(1); }

  if( dpwr2 > 45 )
  { text_error("don't fry the probe, DPWR2 too large!  "); psg_abort(1); }
  if( dpwr > 50 )
  { text_error("don't fry the probe, DPWR too large!  "); psg_abort(1); }

  if( (pw > 30.0e-6) )
  { text_error("don't fry the probe, pw too high ! "); psg_abort(1); }

  if( (pwC > 200.0e-6)  )
  { text_error("don't fry the probe, pwC too high ! "); psg_abort(1); }


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-TPPI-Haberkorn element */

if (phase1 == 1)  {tsadd(t10,2,4); icosel = +1;}
       else       {icosel = -1;}
/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1)  d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t2,2,4); tsadd(t11,2,4); }



/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 0.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;

if(CT[A]=='y')   {
                   refocN15[A]='y';
                   refocCO[A]='y';
                   
                   if ( (timeCC-ni/sw1*0.5)*0.5 -pw -pwCO*0.5 -pwN-rof1  <0.2e-6 )
                     {
                       text_error("too many increments in t1 for a constant time"); psg_abort(1);
                     }
                  }

/*temporary*/
/* correction delays */
 
corrB=0.0;
corrD=2.0/M_PI*pwC-pw-rof1;

if (corrD < 0.0) {corrB=-corrD; corrD=0.0;}

 
/* BEGIN PULSE SEQUENCE */

status(A);
	obsoffset(tof);        
	obspower(tpwr);
	decpower(pwClvl); decpwrf(4095.0); decoffset(dof);
 	obspwrf(4095.0);
        delay(d1);
        
        txphase(zero);
        decphase(zero);
	 
	decrgpulse(pwC, zero, rof1, rof1);
	zgradpulse(gzlvl0, gt0);
	delay(2.0*gstab);
	delay(5.0e-3);


/************ H->C13 */
        txphase(zero);
        decphase(zero);
          
   	rgpulse(pw,zero,rof1,rof1);                  
   	
        decpower(pwC180lvl); decpwrf(pwC180lvlF);

        delay(gstab);
	zgradpulse(gzlvl5, gt5);
	delay(tauch - gt5 -gstab);

   
        simshaped_pulse("hard",C180shape,2.0*pw,pwC180,zero,zero, 0.0, 0.0);
 
   	 
        decpower(pwClvl); decpwrf(4095.0);
   	txphase(one);
        delay(tauch - gt5-gstab);
	zgradpulse(gzlvl5, gt5);
	delay(gstab);

 	rgpulse(pw, one, rof1,rof1);
 	
  /*** purge   */  
        dec2power(pwNlvl); dec2pwrf(4095.0);
        txphase(zero);
        decphase(t2);
        delay(gstab);
	zgradpulse(gzlvl2, gt2);
	
        delay(3.0*gstab);
	
	
  /*  evolution on t1 */	

/* real time here */
if(CT[A]=='n')  
     {  
 	     decrgpulse(pwC, t2, 0.0, 0.0);
             decphase(zero);
 	     decpower(pwC180lvl); decpwrf(pwC180lvlF);
 	      
 	     delay(tau1);
             obsunblank();
             rgpulse(pw,one,rof1,0.0); 
             rgpulse(2.0*pw,zero,0.0,0.0);
             rgpulse(pw,one,0.0,rof1);
             obsblank(); 

             if(refocN15[A]=='y') dec2rgpulse(2.0*pwN,zero,0.0,0.0); /*n15 refocusing */

     	     if(refocCO[A]=='y') 
  	            { 
  	              decpower(pwCOlvl);
			decshaped_pulse(COshape,pwCO,zero, 3.0e-6, 3.0e-6);
  	              decpower(pwC180lvl);
        	    }


  	    delay(tau1);
        

 	  /*************** CODING with CLUB sandwitches and BIPS */
 	  

  	 zgradpulse(gzlvl3*icosel, gt3); 
  	 delay(gstab1);
    	 decshaped_pulse(C180shape,pwC180,zero, 0.0, 0.0);
    	 zgradpulse(-gzlvl3*icosel, gt3);

    	 delay(gstab1 +rof1*2.0+pw*4.0 +pwC*4.0/M_PI +2.0*POWER_DELAY+2.0*PWRF_DELAY);

         if(refocN15[A]=='y') delay(2.0*pwN); /*n15 refocusing */
         if(refocCO[A]=='y') /* ghost CO pulse */
              { 
                decpower(pwCOlvl);
                decshaped_pulse(COshape,pwCO,zero, 3.0e-6, 3.0e-6);
  		decpower(pwC180lvl);
              }

               
	zgradpulse(-gzlvl3*icosel, gt3);
  	delay(gstab1);
	decshaped_pulse(C180shape,pwC180,zero, 0.0, 0.0);
 	zgradpulse( gzlvl3*icosel, gt3);
    	decpower(pwClvl); decpwrf(4095.0);
   	delay(gstab1);
  }      /* end of if bracket  for real-time  */       
    /*^^^^^^^ end of real time */   
        

/* CONSTANT TIME VESION: */
if(CT[A]=='y')  
     {      
      decrgpulse(pwC, t2, 0.0, 0.0);
            
      /* timeCC-t1 evolution */
      decpower(pwC180lvl); decpwrf(pwC180lvlF);
      delay((timeCC-tau1)*0.5 -2.0*pw -pwCO*0.5 -pwN-rof1);

      obsunblank();
      rgpulse(pw,one,rof1,0.0); 
      rgpulse(2.0*pw,zero,0.0,0.0);
      rgpulse(pw,one,0.0,rof1);
      obsblank(); 
            
      if(refocN15[A]=='y') dec2rgpulse(2.0*pwN,zero,0.0,0.0); /*n15 refocusing */
      if(refocCO[A]=='y' ) 
	{ 
          decpower(pwCOlvl);
	  decshaped_pulse(COshape,pwCO,zero, 3.0e-6, 3.0e-6);
  	  decpower(pwC180lvl);
        }   
          delay((timeCC-tau1)*0.5 -2.0*pw -pwCO*0.5 -pwN-rof1);
 /* end of timeCC-t1 evolution */
 /* 180 on carbons in T1 */

	decshaped_pulse(C180shape,pwC180,zero, 0.0, 0.0);

/* timeCC+t1 evolution  + encoding */
 
        delay((timeCC+tau1)*0.5 -2.0*pw -pwCO*0.5 -pwN -rof1);
 	     
        obsunblank();
        rgpulse(pw,one,rof1,0.0); 
        rgpulse(2.0*pw,zero,0.0,0.0);
        rgpulse(pw,one,0.0,rof1);
        obsblank(); 
        if(refocN15[A]=='y') dec2rgpulse(2.0*pwN,zero,0.0,0.0); /*n15 refocusing */
        if(refocCO[A]=='y' )  
         { 
 	  decpower(pwCOlvl);
	  decshaped_pulse(COshape,pwCO,zero, 3.0e-6, 3.0e-6);
  	  decpower(pwC180lvl);
         }  

        delay((timeCC+tau1)*0.5 -2.0*pw -pwCO*0.5 -pwN -rof1);

        zgradpulse(-gzlvl3*icosel, gt3*2.0); /* coding */
        delay(gstab +pwC*4.0/M_PI);
        decshaped_pulse(C180shape,pwC180,zero, 2e-6, 2e-6); /* ghost BIP pulse */
        zgradpulse(gzlvl3*icosel, gt3*2.0); /* coding */
        delay(gstab);
 
  } /*^^^^^^^ end of CONSTANT  time  bracket*/   

        /*reverse INPET */

        
 
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
        decpower(dpwr);	decpwrf(4095.0); decoffset(dof_dec);
        dec2power(dpwr2); /* POWER_DELAY EACH */
	delay(gstab);
status(C);
	setreceiver(t11);
}		 
