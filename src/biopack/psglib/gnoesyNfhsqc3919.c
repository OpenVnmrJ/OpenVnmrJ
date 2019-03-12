/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  a simple NOE-N15 fast HSQC with 3919

   no bells no whistles
  - C13 refocusing via BIP, now 13C offset should be somewhere between CH3 and CO, ~90ppm

  VNMRS/DD2 vesrion only!!!

    ET Agilent 2014

  fast-HSQC: Mori, Abeygunawardana, Johnson, van Zijl JMR B 1995 v 108, pp94-98
  BIP pulses JMR v136 p54  1999;       JMR v151 p269 2001


  temporary, Jan 2014 - modified to have 0 dead times in t1 and t2


template used 
gnoesyNfhsqcA.c    "3D noesy Fast N15 hsqc using 3919 watergate suppression"

  

*/


#include <standard.h>


static int  

	   phi1[]  = {0,0,2,2},
           phi2[]  = {0,2,0,2},
          
          rec[]  = {0,2,2,0};

                                
 


void pulsesequence()
{
  int        ni2=getval("ni2");

  char	        
            C180shape[MAXSTR],
            C13refoc[MAXSTR],
	    f1180[MAXSTR],   		       /* Flag to start t1 @ halfdwell */
	    f2180[MAXSTR];   		       /* Flag to start t2 @ halfdwell */


  double    tauxh=getval("tauxh"),

            tau1, tau2, 
	    tauWG=getval("tauWG"),
	    mix=getval("mix"),

            gzlvl1=getval("gzlvl1"),
            gt1=getval("gt1"),

            gzlvl2=getval("gzlvl2"),
            gt2=getval("gt2"),
     
            gzlvl3=getval("gzlvl3"),
            gt3=getval("gt3"),

            gzlvl6=getval("gzlvl6"),	/* gradients during H1 indirect evolution, tau1 */
            gt6=getval("gt6"),		/* set gzlvl6 to zero for no gradients */

            gstab=getval("gstab"),			/* gradient recovery delay */
         
            pwN = getval("pwN"),
            pwNlvl = getval("pwNlvl"),  

            
            C180pw=getval("C180pw"),      
            C180pwr=getval("C180pwr"),      /* C13 decoupling, bip */

            sw1 = getval("sw1"),
            sw2 = getval("sw2"),
                                
       
            pwClvl = getval("pwClvl"), 	         /* coarse power for C13 pulse */
            pwC = getval("pwC");      /* C13 90 degree pulse length at pwClvl */
           

    getstr("C13refoc",C13refoc);
    getstr("C180shape",C180shape);

    getstr("f1180",f1180);
    getstr("f2180",f2180);
    
/* check validity of parameter range */

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
    { text_error("incorrect Dec1 decoupler flags!  "); psg_abort(1); } 

    if((dm2[A] == 'y' || dm2[B] == 'y') )
    { text_error("incorrect Dec2 decoupler flags!  "); psg_abort(1); } 



      settable(t1,  4, phi1);
      settable(t2,  4, phi2);

      settable(t11, 4, rec); 
 
/*  Calculate modifications to phases for States-TPPI acquisition          */

    if (phase1 == 2)    {tsadd(t1 ,1,4);}
    if (d2_index % 2)  	{tsadd(t1,2,4); tsadd(t11,2,4); }


    if (phase2 == 2)    {tsadd(t2 ,1,4);}
    if (d3_index % 2)  	{tsadd(t2,2,4); tsadd(t11,2,4); }

/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 0.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1 )); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;

    tau2 = d3;
    if((f2180[A] == 'y') && (ni2 > 0.0)) 
	{ tau2 += ( 1.0 / (2.0*sw2) ); if(tau2 < 0.2e-6) tau2 = 0.0; }
    tau2 = tau2/2.0;



         
                           /* sequence starts!! */
   status(A);
     
     obspower(tpwr);
     
     dec2power(pwNlvl);
     decpower(pwClvl);
     decoffset(dof); 
     dec2offset(dof2); 
       decpower(C180pwr);

     dec2phase(zero);


     delay(d1);
     
   status(B);

    /* t1, protons +mix*/

     initval(45.0,v1);
     obsstepsize(1.0);
     txphase(t1);
     xmtrphase(v1);
    
        parallelstart("obs"); /* symmetric 1H */
             delay(gstab);
             rgpulse(pw, t1, rof1, rof1);
             xmtrphase(zero);
	       delay(tau1*2.0);
                 

                 rgpulse(2.0*pw,zero,rof1,rof1);
                 /*obsunblank();
                 rgpulse(pw,one,rof1,0.0); 
                 rgpulse(2.0*pw,zero,0.0,0.0);
                 rgpulse(pw,one,0.0,rof1);
                 obsblank();
                 */
              rgpulse(pw, zero, rof1, rof1);
              delay( mix-gt1 -gstab);
              
        parallelstart("dec2");  /* N15 dec */

             delay(gstab+tau1+pw+ rof2*2.0 -pwN);
             dec2rgpulse(pwN, zero, 0.0, 0.0);
             delay(tau1+mix-gt1 -gstab);
   

        parallelstart("dec");  /* C13 dec */
              delay(gstab + tau1+pw+rof2*2.0  + pwN +2e-6); 

              /* avoiding simultaneous hard pulses on 13C an 15N, awkward */

              if ( (C13refoc[A]=='y') )    { decshaped_pulse(C180shape, C180pw, zero, 0.0, 0.0); }
                                 

        parallelend();



      
     zgradpulse(gzlvl1,gt1);
     delay(gstab);
     

     /* H->HzNz */


    	rgpulse(pw, zero, rof1, rof1);	
		       
        delay(gstab); 	
	zgradpulse(gzlvl2, gt2);
	delay(tauxh - gt2 -gstab);

   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

        delay(tauxh - gt2 -gstab);
        zgradpulse(gzlvl2, gt2);
        delay(gstab);

 	rgpulse(pw, one, rof1, rof1);

        delay(gstab);
        zgradpulse(gzlvl6, gt6);
       /* delay(gstab);*/

      /* t2 evolution*/
    
        parallelstart("obs"); /* symmetric 1H */
              
              delay(gstab + tau2-2.0*pw-rof1 + pwN);  
              obsunblank();
              rgpulse(pw,one,rof1,0.0); 
              rgpulse(2.0*pw,zero,0.0,0.0);
              rgpulse(pw,one,0.0,rof1);
              obsblank(); 
               
              
        parallelstart("dec2");  /* N15 evolution */
             delay(gstab);

             dec2rgpulse(pwN, t2, 0.0, 0.0);
             delay(tau2*2.0);
             
             dec2rgpulse(pwN*2.0, zero, 0.0, 0.0); 
             dec2rgpulse(pwN, zero, 0.0, 0.0); 
             delay(gstab-pwN*2.0);       

        parallelstart("dec");  /* C13 dec */
              delay(gstab + tau2-C180pw*0.5 + pwN); 
              /* avoiding simultaneous hard pulses on 13C an 15N, awkward */

              if ( (C13refoc[A]=='y') && (tau2*2.0>C180pw+2.0e-6))    { decshaped_pulse(C180shape, C180pw, zero, 0.0, 0.0); }
                                 

        parallelend();


     zgradpulse(gzlvl6, gt6);
     delay(gstab);

	 

/* back to protons */
     rgpulse(pw, two, rof1, rof1);
     delay(gstab-rof1);
     zgradpulse(gzlvl3,gt3);
     delay(tauxh-gt3-gstab-rof1);
     
 	
       rgpulse(pw*0.231,one,rof1,rof1);     
       delay(tauWG-2.0*rof1);
       rgpulse(pw*0.692,one,rof1,rof1);
       delay(tauWG-2.0*rof1);
       rgpulse(pw*1.462,one,rof1,rof1);

       delay(tauWG/2.0-pwN -rof1);
       dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);     
       delay(tauWG/2.0-pwN -rof1);

       rgpulse(pw*1.462,three,rof1,rof1);
       delay(tauWG-2.0*rof1);
       rgpulse(pw*0.692,three,rof1,rof1);
       delay(tauWG-2.0*rof1);
       rgpulse(pw*0.231,three,rof1,rof1);   

     delay(tauxh-gt3-gstab-rof1);
     zgradpulse(gzlvl3,gt3);   
  
     dec2power(dpwr2);
     delay(gstab+2.0/3.1415*pw);

   status(C);
     setreceiver(t11);   
}
