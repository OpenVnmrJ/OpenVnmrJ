/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* gNTSR1.c  Off-resonance Trosy selected R1rho experiment  

   minimum step cycling is 8
   
   tau1 timing corrected for regular experiment (4*pwN/PI correction added)
   tau1 timing corrected for TROSY experiment
   f1180 flag for starting t1 at half-dwell

   C13refoc='y' uses refocussing 13C STUD or composite pulse in t1
     power and pulse width are automatically calculated for either one
   C13shape flag='y' adiabatic refocussing pulse in t1 
   C13shape flag='n' composite refocussing pulse in t1 

   wtg3919='y' does water suppression by watergate 3919

   Coded by N.Murali, Varian, Palo Alto
   Ref: T.I. Igumenova and A.G. Palmer III JACS 2006, 128, 8110-8111 

   The off-resonance B1 field is supplied by the waveform contained in
   the variable "slshape". It is a "spinlock"-type waveform and uses
   the power slpwr2, dmf of sldmf2 and res of sldres2. An example 
   waveform may be created using Pbox with the input string
     {tanhtan 0.004 0.0}
   The waveform .DEC file will have the proper power/dmf/dres values

   See Mulder et.al, JMR, 131, 351-357 (1998) for details on off-resoance
   pulse.
*/


#include <standard.h>

static double d2_init = 0.0;

static int phi1[2] = {0,2},
           phi2[8] = {0,0,0,0,2,2,2,2},
           phi3[4] = {3,3,1,1}, 
           phi4[1] = {1},
           phi5[1] = {2},
           rec[8]  = {0,2,2,0,2,0,0,2};
                                

void pulsesequence()
{
  int       t1_counter;
  char	    C13refoc[MAXSTR],	      /* C13 refocussing pulse in middle of t1 */
	    C13shape[MAXSTR],   /* choose between sech/tanh or composite pulse */
            wtg3919[MAXSTR],
            slshape[MAXSTR],
	    f1180[MAXSTR];   		       /* Flag to start t1 @ halfdwell */
  double    tauxh, tau1,
            gsign,
            icosel,
            gzlvl0=getval("gzlvl0"),
            gzlvl1=getval("gzlvl1"),
            gzlvl2=getval("gzlvl2"),
            gzlvl3=getval("gzlvl3"),
 	    gzlvl4=getval("gzlvl4"),
/*          gzlvl5=getval("gzlvl5"), */
            gt0=getval("gt0"),
            gt1=getval("gt1"),
            gt2=getval("gt2"),
            gt3=getval("gt3"),
            gt4=getval("gt4"),
/*          gt5=getval("gt5"), */
	    gstab=getval("gstab"),	      /* recovery delay after gradient */
            del = getval("del"),
            del1 = getval("del1"),
            del2 = getval("del2"), 
            Troe = getval("Troe"),
            slpwr2 = getval("slpwr2"),
            sldmf2 = getval("sldmf2"),
            sldres2 = getval("sldres2"),
            JNH = getval("JNH"),
            pwN = getval("pwN"),
            pwNlvl = getval("pwNlvl"),  
            sw1 = getval("sw1"),
            sfrq = getval("sfrq"),
            tof = getval("tof"),
            pwClvl = getval("pwClvl"), 	         /* coarse power for C13 pulse */
            pwC = getval("pwC"),       /* C13 90 degree pulse length at pwClvl */
            rfst = 4095.0,	            /* fine power for the stCall pulse */
            compC = getval("compC");   /* adjustment for C13 amplifier compr-n */


/* INITIALIZE VARIABLES */

    getstr("C13refoc",C13refoc);
    getstr("C13shape",C13shape);
    getstr("wtg3919",wtg3919);
    getstr("slshape",slshape);
    getstr("f1180",f1180);
    
    tauxh =  1/(4*(JNH));

    if ( (C13refoc[A]=='y') && (C13shape[A]=='y') )
    {
      /* 180 degree adiabatic C13 pulse from 0 to 200 ppm */
      rfst = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35));   
      rfst = (int) (rfst + 0.5);
      if ( 1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
      { 
        text_error( " Not enough C13 RF. pwC must be %f usec or less.\n", 
	          (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) );    
	psg_abort(1); 
      }
    }


/* check validity of parameter range */

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
    { text_error("incorrect Dec1 decoupler flags!  "); psg_abort(1); } 

    if((dm2[A] == 'y' || dm2[B] == 'y') )
    { text_error("incorrect Dec2 decoupler flags!  "); psg_abort(1); } 

    if( dpwr > 0 )
    { text_error("don't fry the probe, dpwr too large!  "); psg_abort(1); }

    if( dpwr2 > 50 )
    { text_error("don't fry the probe, dpwr2 too large!  "); psg_abort(1); }

    if ((dm2[C] == 'y'))
    { text_error("Choose  dm2='nnn' ! "); psg_abort(1); }

/* LOAD VARIABLES */

    if(ix == 1) d2_init = d2;
    t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);
    
/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;

/* LOAD PHASE TABLES */

      gsign = 1.0;

      assign(one,v7); 
      assign(three,v8);
      settable(t1, 2, phi1);
      settable(t2, 8, phi2);
      settable(t3, 4, phi3);
      settable(t4, 1, phi4);
      settable(t5, 1, phi5);  
      settable(t6, 8, rec);

      if ( phase1 == 1 ) icosel = -1;				/* Hypercomplex in t1 */
       else                 
        { tsadd(t4, 2, 4); tsadd(t5, 2, 4); icosel = +1;}                      
        
                                   
    if(t1_counter %2)          /* calculate modification to phases based on */
    { tsadd(t3,2,4); tsadd(t6,2,4); }   /* current t1 values */

         
                           /* sequence starts!! */
   status(A);
     
     obspower(tpwr);
     dec2power(pwNlvl);
     decpower(pwClvl);
     if (C13shape[A]=='y') 
	decpwrf(rfst);
     else
	decpwrf(4095.0);
     delay(d1);
     dec2rgpulse(pwN,zero,0.0,0.0);		/* destroy N15 Magnetization */
     zgradpulse(gzlvl0*1.2,gt0);
     delay(10e-4);
     
   status(B);

     rgpulse(pw, zero, rof1, 0.0);
     zgradpulse(gzlvl0,gt0);
     txphase(zero);
     dec2phase(zero);
     delay(tauxh-gt0);               /* delay=1/4J(XH)   */

     sim3pulse(2*pw,0.0,2*pwN,zero,zero,t1,rof1,0.0);

     zgradpulse(gzlvl0,gt0);
     dec2phase(t2);
     delay(tauxh-gt0 );               /* delay=1/4J(XH)   */
  
     rgpulse(pw, three, rof1, 0.0);

     zgradpulse(gzlvl4,gt4);
     delay(gstab); 

     /* S3E module */

     dec2rgpulse(pwN,t1,rof1,0.0);
     zgradpulse(gzlvl0,gt0);
     delay(del1-gt0);
     getelem(t1,ct,v3);
     add(v3,three,v3);
     dec2rgpulse(pwN,v3,rof1,0.0);
     sim3pulse(2*pw,0.0,2*pwN,zero,zero,t1,rof1,0.0);
     dec2rgpulse(pwN,v3,rof1,0.0);
     zgradpulse(gzlvl0,gt0);
     delay(del1-gt0);
     dec2stepsize(45.0);
     initval(3.0,v1);
     dcplr2phase(v1);
     dec2rgpulse(pwN,t2,rof1,0.0);
     dcplr2phase(zero);
     dec2stepsize(90.0);

     /* S3CT module */

     obsoffset(tof+(3.5*sfrq));            /* proton carrier moved to middle of amide region */
     zgradpulse(gzlvl4, gt4);
     delay(gstab); delay(del);
     dec2power(slpwr2);
     dec2unblank();
     dec2on();
     dec2prgon(slshape,1/sldmf2,sldres2);
     delay(Troe/2.0);
     dec2prgoff();
     dec2off();
     dec2blank();
     zgradpulse(gzlvl4,gt4);
     delay(gstab); delay(del);
     dec2power(pwNlvl);
     dec2rgpulse(pwN,zero,rof1,0.0);
     zgradpulse(gzlvl0,gt0);
     delay(tauxh-gt0);
     sim3pulse(2.0*pw,0.0,2.0*pwN,zero,zero,zero,rof1,0.0);
     zgradpulse(gzlvl0,gt0);
     delay(tauxh-gt0);
     sim3pulse(2*pw,0.0,pwN,zero,zero,one,rof1,0.0);
     zgradpulse(gzlvl4, gt4);
     delay(gstab); delay(del);
     dec2power(slpwr2);
     dec2unblank();
     dec2on();
     dec2prgon(slshape,1/sldmf2,sldres2);
     delay(Troe/2.0);
     dec2prgoff();
     dec2off();
     dec2blank();
     zgradpulse(gzlvl4,gt4);
     delay(gstab); delay(del);
     dec2power(pwNlvl);
     obsoffset(tof);

     /* End of S3CT module */

     decphase(zero);
            
     dec2rgpulse(pwN, t3, 0.0, 0.0);              

       if ( (C13shape[A]=='y') && (C13refoc[A]=='y') 
	   && (tau1 > 0.5e-3 +WFG2_START_DELAY ) )
         {
         delay(tau1 -0.5e-3 -WFG2_START_DELAY );     
         decshaped_pulse("stC200", 1.0e-3, zero, 0.0, 0.0);
         delay(tau1 -0.5e-3 -WFG2_STOP_DELAY );
         }
       else if ( (C13shape[A]!='y') && (C13refoc[A]=='y') 
	        && (tau1 > 2.0*pwC +SAPS_DELAY ) )
         {
         delay(tau1 -2.0*pwC -SAPS_DELAY ); 
	 decrgpulse(pwC, zero, 0.0, 0.0);
	 decphase(one);
         decrgpulse(2.0*pwC, one, 0.0, 0.0);  
	 decphase(zero);
	 decrgpulse(pwC, zero, 0.0, 0.0);
         delay(tau1 -2.0*pwC -SAPS_DELAY ); 
         }
       else 
         delay(2.0*tau1 );

     zgradpulse(gzlvl1,gt1);				/* Encoding gradient */
     delay(gstab);
     dec2rgpulse(2.0*pwN,zero,0.0,0.0);
     delay(gt1+gstab);

     rgpulse(pw, t4, 0.0, 0.0);         
     zgradpulse(gzlvl0,gt0);
     delay(tauxh -gt0);
     sim3pulse(2.0*pw,0.0,2.0*pwN,zero,zero,zero,0.0,0.0);
     zgradpulse(gzlvl0,gt0);
     delay(tauxh -gt0);       
     sim3pulse(pw,0.0,pwN,zero,zero,three,0.0,0.0);
     zgradpulse(gzlvl3,gt3);
     txphase(v7); dec2phase(zero);
     delay(del2-gt3);
     dec2rgpulse(2.0*pwN,zero,0.0,0.0);
     
     if(wtg3919[0] == 'y')
     {     
       delay(tauxh-del2);            /*check if this is needed (-2.5*d3-2.385*pw);*/	
       rgpulse(pw*0.231,v7,0.0,0.0);     
       delay(d3);
       rgpulse(pw*0.692,v7,0.0,0.0);
       delay(d3);
       rgpulse(pw*1.462,v7,0.0,0.0);

       delay(d3/2.0);
       txphase(v8);
       delay(d3/2.0);

       rgpulse(pw*1.462,v8,0.0,0.0);
       delay(d3);
       rgpulse(pw*0.692,v8,0.0,0.0);
       delay(d3);
       rgpulse(pw*0.231,v8,0.0,0.0);
       zgradpulse(gzlvl3,gt3);
       delay(del2-gt3-2.0*(2.5*d3+2.385*pw));
       dec2rgpulse(pwN,t5,0.0,0.0);
       delay(tauxh-del2);
       zgradpulse(gzlvl2*icosel,gt2);
       delay(gstab);

      }
     
     else
     { 
       delay(tauxh-del2) ;
       rgpulse(2.0*pw,zero,0.0,0.0);
       zgradpulse(gzlvl3,gt3);
       delay(del2-gt3);
       dec2rgpulse(pwN,t5,0.0,0.0);
       delay(tauxh-del2);
       zgradpulse(gzlvl2*icosel,gt2);
       delay(gstab);
       rgpulse(2.0*pw,zero,0.0,0.0);			/* pulse for refousing gt2+gstab */
       delay(gt2+gstab);
       delay(rof2);
       rgpulse(2.0*pw,two,0.0,0.0);			/* pulse for matching CT pathway with wtg */
       delay(rof2);
      }
        
     dec2power(dpwr2);

   status(C);
     setreceiver(t6);   
}



