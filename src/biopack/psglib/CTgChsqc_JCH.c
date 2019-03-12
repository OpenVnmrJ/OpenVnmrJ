/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* CTgChsqc_JCH.c    This pulse sequence will allow one to perform the 
                     following experiment:

   13C-1H CT HSQC   with gradient coherence selection and JCH measurements
                (N.Tjandra and Ad Bax JMR 124 512-515 1997)
                
                  modJCH - flag to use JCH modulation
                  timeCT constant-time for 13C evolution,  1/JCC ca 13-14ms

                  timeJCH - "delta" for JCH evoution,  
                             total JCH evoution is 2*(timeCT-timeJCH)


		for a pseudo-3D spectrum:
			set modJCH='y'
			set ni large enough to resolve CA in the indirect 
                          dimension (or to fill full CT time)
			array timeJCH from 0.0 to 2.4ms
			set phase = 1,2 array='timeJCH,phase'

     ADJUST gzlv3/lgzlvl4 (CODING/DECODING GRADIENTS) FOR MAX INTENSITY  

        	  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nny', dmm = 'ccg' (or 'ccw', or 'ccp') for C13 decoupling.
    Set dm2 = 'nyn', dmm2 = 'cpc' (or 'cwc', or 'cgc') for N15 decoupling.

    Must set phase = 1,2  for States-TPPI acquisition in t1 
    
    The flag f1180 should be set to 'y' if t1 is to be started at halfdwell
    time. This will give 90, -180 phasing in f1 and fpmult=1. 

    FOR INOVA THERE STILL MIGHT BE AP DELAYS THAT ARE NOT CORRECTED 
*/

#include <standard.h>
  
static int   phi1[]  = {1,1,3,3,3,3,1,1},
	     phi2[]  = {0,2,0,2},
             phi3[]  = {0,0,1,1},
	     phi4[]  = {0,0,0,0},
             phix[]   ={0,0,0,0},
             rec[]   = {0,2,2,0};

static double   d2_init=0.0;


void pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */

            CACB180shape[MAXSTR],
            CO180shape[MAXSTR],
             modJCH[MAXSTR];
	    
 
int         t1_counter,icosel;		     /* used for states tppi in t1 */

double      tau1,         				         /*  t1 delay */
	    tauxh =  getval("tauxh"), 	   /* 1/4J   evolution delay */
 

            timeCT  =  getval("timeCT"),  /* constant-time for CC evolution, ca 26ms */
            timeJCH =  getval("timeJCH"), /* time for  JCH evolution */


 

	pwClvl = getval("pwClvl"),	              /* power for hard X pulses */
        CACB180pwr = getval("CACB180pwr"),           /*power levels for 180 shaped pulse */
        CACB180pwrf = getval("CACB180pwrf"),
        pwC = getval("pwC"),          /* X 90 degree pulse length at pwClvl */	 
        CACB180pw = getval("CACB180pw"),   /* shaped 180 pulse on Xchannle */
	sw1 = getval("sw1"),

        CO180pw = getval("CO180pw"),
        CO180pwr = getval("CO180pwr"),

        gstab = getval("gstab"),
        gstab1 = getval("gstab1"), /* recovery for club sandwitch, short*/
        gt0 = getval("gt0"),
	gt2 = getval("gt2"),
        gt3 = getval("gt3"),                               /* other gradients */
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
        gt6 = getval("gt6"),
	
        gzlvl0 = getval("gzlvl0"),
	gzlvl2 = getval("gzlvl2"),
        gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl6 = getval("gzlvl6");


    getstr("f1180",f1180);
    getstr("CACB180shape",CACB180shape);
    getstr("CO180shape",CO180shape);
 
    getstr("modJCH",modJCH);


/*   LOAD PHASE TABLE    */

	 
	settable(t2,4,phi2);
	settable(t3,4,phi3);
	settable(t4,4,phi4);
        settable(t10,4,phix);
	settable(t11,4,rec);



/* CHECK VALIDITY OF PARAMETER RANGES */

  if((dm[A] == 'y' || dm[B] == 'y'))
  { text_error("incorrect dec1 decoupler flags! Should be 'nnn' or 'nny' "); psg_abort(1); }

   
  if( dpwr2 > 50 )
  { text_error("don't fry the probe, DPWR2 too large!  "); psg_abort(1); }
  if( dpwr > 50 )
  { text_error("don't fry the probe, DPWR too large!  "); psg_abort(1); }

  if( (pw > 20.0e-6) )
  { text_error("don't fry the probe, pw too high ! "); psg_abort(1); }
 
  if( (pwC > 200.0e-6)  )
  { text_error("don't fry the probe, pwC too high ! "); psg_abort(1); }

 if( ( 0.5*ni/sw1 > timeCT - CO180pw)  ) {
        printf("Too many increments, set ni to less then %d\n",(int)( (timeCT-CO180pw)*sw1*2.0)  );
            psg_abort(1); }
 

if( (modJCH[A]=='y') &&  (timeCT-timeJCH-CO180pw-gt3-gstab-2.0*GRADIENT_DELAY-4.0*pw-6.0*rof1 <0.2e-6) )

  { printf("Too long timeJCH\n"); psg_abort(1); }


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


/* BEGIN PULSE SEQUENCE */

status(A);
	obsoffset(tof);        
	obspower(tpwr);
        decpower(pwClvl); decpwrf(4095.0);
 	obspwrf(4095.0);

        delay(d1);
        
         txphase(zero);
         decphase(zero);
 
	decrgpulse(pwC, zero, rof1, rof1);
	zgradpulse(gzlvl0, gt0);
	delay(2.0*gstab);
	delay(5.0e-3);


/************ H->X */
         txphase(zero);
         decphase(zero);
          
   	rgpulse(pw,zero,rof1,rof1);                  
   	
        decpower(CACB180pwr); decpwrf(CACB180pwrf);

        delay(gstab);
	zgradpulse(gzlvl5, gt5);
	delay(tauxh - gt5 -gstab);

   
        simshaped_pulse("hard",CACB180shape,2.0*pw,CACB180pw,zero,zero, WFG_STOP_DELAY, WFG_START_DELAY);

   	/* simpulse(2.0*pw,2.0*pwC, zero,zero, 0.0, 0.0);*/
   	 
        decpower(pwClvl); decpwrf(4095.0);
   	txphase(one);
        delay(tauxh - gt5-gstab);
	zgradpulse(gzlvl5, gt5);
	delay(gstab);

 	rgpulse(pw, one, rof1,rof1);
 	
  /*** purge   */  
  
        txphase(zero);
        decphase(t2);
        delay(gstab);
	zgradpulse(gzlvl2, gt2);
	
       delay(3.0*gstab);
	
	
  /*  evolution on t1 */	
         
       status(B);
      

               decrgpulse(pwC, t2, 0.0, 0.0);

              decpower(CO180pwr);  
              decshaped_pulse(CO180shape,CO180pw,zero, 0.0, 0.0); /*ghost CO 180 */

              decphase(zero);   decpower(CACB180pwr);  decpwrf(CACB180pwrf);
	         
              delay(timeCT-tau1-CO180pw);

         decshaped_pulse(CACB180shape,CACB180pw,t3, WFG_STOP_DELAY, WFG_START_DELAY);



	       delay(timeJCH);
               
               if(modJCH[A]=='y'){

                    rgpulse(pw,one,rof1,rof1);
                    rgpulse(2.0*pw,zero,rof1,rof1); /*90y180x90y*/
                    rgpulse(pw,one,rof1,rof1); 
                   }
        
               delay(timeCT-timeJCH - CO180pw
                                   -gt3 -gstab -2.0*GRADIENT_DELAY
                                   -4.0*pw -6.0*rof1);

               zgradpulse(gzlvl3, gt3); 
               delay(gstab);

               /* 180 on CARBONYLS */
              
               if(modJCH[A]=='n'){

                    rgpulse(pw,one,rof1,rof1);
                    rgpulse(2.0*pw,zero,rof1,rof1); /*90y180x90y*/
                    rgpulse(pw,one,rof1,rof1); 
                   }
              decpower(CO180pwr);  decpwrf(4095.0);
              decshaped_pulse(CO180shape,CO180pw,zero, 0.0, 0.0);
              
              delay(tau1);
             
            
             decpower(pwClvl);  
      
            decrgpulse(pwC, zero, 0.0, 0.0);
 
status(A);


            
           /* purge */
            zgradpulse(gzlvl6,gt6);
            
            delay(gstab);
            
            dec2power(dpwr2); 
            /*reverse INPET */

        
 
  
          rgpulse(pw,zero,rof1,rof1);
	  delay(gstab);  
          zgradpulse(-0.9*gzlvl4*icosel,gt4);
	  delay(tauxh - gt4 - gstab); 

	  decpower(CACB180pwr); decpwrf(CACB180pwrf);
         
            simshaped_pulse("hard",CACB180shape,2.0*pw,CACB180pw,zero,zero, WFG_STOP_DELAY, WFG_START_DELAY); 
	            
         delay(tauxh - gt4 - gstab+rof1);  
	   zgradpulse(gzlvl4*icosel,gt4);

           decpower(dpwr); 	decpwrf(4095.0);
            
	delay(gstab);
       
	
				       

        
status(C);
	setreceiver(t11);
}		 
