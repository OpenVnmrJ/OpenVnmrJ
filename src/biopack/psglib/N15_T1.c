/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 15N   T1 relaxation Varian/Agilent
    N15_T1.c

     HSQC:          	Kay, Keifer and Saarinen, JACS, 114, 10663 (1992)
   
    relaxation times:   Kay et al, JMR, 97, 359 (1992)
			Farrow et al, Biochemistry, 33, 5984 (1994)
    TROSY:		Weigelt, JACS, 120, 10778 (1998)
    
    water_sat='y' (default) saturate/dephase all water

    water_sat='n' try to keep water in +Z,
                   see paper: Cheng&Tjandra, JMR v213, pp151-157 (2011)
                       

   Evgeny TishchenkoT, Agilent technologies, May 2012: 
    used BioPack gNhsqc.c as template, removed  majority of options.
    This sequence is essentially the same as N15T1_lek_pfg_sel_enh.c from Lewis Kay.
*/

#include <standard.h>
static int    
	     phi1[4]  = {1,3,1,3}, /* first 90 in 15N t1 */	
             phi2[4]  = {0,0,0,0},
	     phi3[4]  = {1,1,3,3}, /* last 90 on 15N in first inept, +Nz/-Nz */
	     phi10[1]  = {0},       /* Echo-anti-echo in reverse double inept */
	
             rec[4]   = {0,2,2,0};
void pulsesequence()
{

/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
	    C13refoc[MAXSTR],		/* C13 sech/tanh pulse in middle of t1*/
	    Cshape[MAXSTR],                       /* CACO inversion 180 pulse */
            water_sat[MAXSTR];            /* saturate/non-saturate water flag */
 
int         icosel;          			  /* used to get n and p type */
double      tau1,         				         /*  t1 delay */
            tpwrs,

        tpwrsf_d = getval("tpwrsf_d"), /* fine power adustment for first soft pulse(down)*/
        tpwrsf_u = getval("tpwrsf_u"), /* fine power adustment for second soft pulse(up) */
        pwHs = getval("pwHs"),                     /* H1 90 degree pulse length at tpwrs */
        compH =getval("compH"),

	    lambda = 1.0/(4.0*getval("JNH")), 	           /* 1/4J H1 evolution delay */
	    tNH = 1.0/(4.0*getval("JNH")),       	  /* 1/4J N15 evolution delay */
	    tau_cp= getval("tau_cp"),          /* 1/4 of overall relaxation increment */
	    ncyc=getval("ncyc"),         	 /* number of pulsed cycles in relaxT */
            pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
            pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
            C180pw=getval("C180pw"), 
            C180pwr=getval("C180pwr"),       /* 13C decoupling pulse parameters */
   
            compN = getval("compN"),       /* adjustment for N15 amplifier compression */
            compC = getval("compC"),       /* adjustment for C13 amplifier compression */

	    pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
            pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
	 	 

            sw1 = getval("sw1"),
 

	gt1 = getval("gt1"), 
        gt2 = getval("gt2"),
 		              /* coherence pathway gradients */
       
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

	gt0 = getval("gt0"),				   /* other gradients */
	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),

	gt6 = getval("gt6"),
	gt7 = getval("gt7"),

	gstab = getval("gstab"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),
        gzlvl6 = getval("gzlvl6"),
        gzlvl7 = getval("gzlvl7");

        getstr("f1180",f1180);
    
        getstr("C13refoc",C13refoc);
        getstr("Cshape",Cshape);
        getstr("water_sat",water_sat);
        tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));          /*needs 1.69 times more*/
        tpwrs = (int) (tpwrs);                               /*power than a square pulse */

        if (tpwrsf_d<4095.0)
         tpwrs=tpwrs+6.0;  /* add 6dB to let tpwrsf_d control fine power ~2048*/


/*   LOAD PHASE TABLE    */
	
        settable(t1,4,phi1);
        settable(t2,4,phi2);
        settable(t3,4,phi3);
	settable(t10,1,phi10);
	settable(t31,4,rec); 

/* CHECK VALIDITY OF PARAMETER RANGES */

 
  if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
  { text_error("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1); }

  if((dm2[A] == 'y' || dm2[B] == 'y'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1); }

  if( dpwr2 > 50 )
  { text_error("don't fry the probe, DPWR2 too large!  ");   	    psg_abort(1); }

  if( pw > 50.0e-6 )
  { text_error("dont fry the probe, pw too high ! ");               psg_abort(1); } 
  
  if( pwN > 100.0e-6 )
  { text_error("dont fry the probe, pwN too high ! ");              psg_abort(1); }

/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */
     icosel = -1; 
     if (phase1 == 1)  {tsadd(t10,2,4); icosel = +1;}
           

/*  Set up f1180  */
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;


/* Calculate modifications to phases for States-TPPI acquisition          */
 if(d2_index % 2) { tsadd(t1,2,4); tsadd(t31,2,4); }

/* BEGIN PULSE SEQUENCE */

status(A);

	obspower(tpwr);
	decpower(pwClvl);
 	dec2power(pwNlvl);
	txphase(zero);
        decphase(zero);
        dec2phase(zero);
	delay(d1);
 status(B);
	dec2rgpulse(pwN, zero, 0.0, 0.0);   /*destroy N15 magnetization*/
	zgradpulse(gzlvl0, 0.5e-3);
	delay(1.0e-4);
   	rgpulse(pw,zero,rof1,rof1);                 /* 1H pulse excitation */
        delay(gstab);
        zgradpulse(gzlvl0, gt0);
	delay(lambda - gt0 -gstab);
   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
        txphase(one);
	delay(lambda - gt0 -gstab);
        zgradpulse(gzlvl0, gt0);
        delay(gstab);
 	rgpulse(pw, one, rof1, rof1); /* on NzHz now */
	if(water_sat[A]=='n')         /* water to -Z */
	    {
        	obspower(tpwrs); obspwrf(tpwrsf_d);
 		shaped_pulse("H2osinc",pwHs,two,rof1,rof1);
		obspower(tpwr); obspwrf(4095.0);
	    }
	/* purge */ 
        zgradpulse(gzlvl3, gt3);
	delay(gstab);
   	 
      /* HzNz-> Nz */

	dec2rgpulse( pwN, zero, 0.0, 0.0);
        delay(gstab);
        zgradpulse(gzlvl6, gt0);
        delay(lambda - gt0 -gstab);
        sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
	dec2phase(t3);
	delay(lambda - gt0 -gstab);
        zgradpulse(gzlvl6, gt0);
        delay(gstab);
        dec2rgpulse( pwN, t3, 0.0, 0.0);

	/* purge */ 
        zgradpulse(gzlvl7, gt7);
	dec2phase(t3);
	delay(gstab);

   /* T1 relaxation delay */
         
        if(ncyc > 0) 
        {
          initval(ncyc,v4);
          loop(v4,v5);
          initval(2.0,v8); 
          loop(v8,v9);
          if(water_sat[A]=='n')         /* water to +Z */
	    {
 	     delay(tau_cp-pw -rof1 -pwHs - 2.0*rof1-2.0*POWER_DELAY-WFG_START_DELAY -WFG_STOP_DELAY );
             obspower(tpwrs); obspwrf(tpwrsf_d);
             shaped_pulse("H2osinc",pwHs,two,rof1,rof1);
             obspower(tpwr); obspwrf(4095.0);
	     rgpulse(2.0*pw, zero, rof1, rof1);
             obspower(tpwrs); obspwrf(tpwrsf_u);
 	     shaped_pulse("H2osinc",pwHs,two,rof1,rof1);
	     obspower(tpwr);
	     delay(tau_cp-pw -rof1 -pwHs - 2.0*rof1-2.0*POWER_DELAY-WFG_START_DELAY -WFG_STOP_DELAY );
	    }
   	   else /* just don't bother about water */
            {
             delay(tau_cp-pw -rof1);
	     rgpulse(2.0*pw, zero,  rof1, rof1);
	     delay(tau_cp-pw -rof1);
            }
          endloop(v9); /* repeat two times */
          endloop(v5);
        }
/* 15N evolution, t1 */
	txphase(zero);
	dec2phase(t1);
        dec2rgpulse( pwN, t1, 0.0, 0.0);
 	delay(tau1);
    	rgpulse(2.0*pw, zero, 0.0, 0.0);
        if(C13refoc[A]=='y')
         { 
          decpower(C180pwr);
 	  decshaped_pulse(Cshape, C180pw, zero, 0.0, 0.0);
          decpower(pwClvl);
         } 
    	delay(tau1);

       /* coding */ 
        delay(lambda  -gt1 -gstab -2.0*POWER_DELAY - C180pw); 
        if(C13refoc[A]!='y')  delay(2.0*POWER_DELAY + C180pw); 
        zgradpulse(-icosel*gzlvl1, gt1); delay(gstab);
    	dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
        rgpulse(2.0*pw, zero, 0.0, 0.0);
        zgradpulse(icosel*gzlvl1, gt1);  delay(gstab); 	/* 2.0*GRADIENT_DELAY */
        delay(lambda  -gt1 -gstab);
    	dec2phase(t10);

/*  reverse INEPT  */
        sim3pulse(pw, 0.0, pwN, zero, zero, t10, 0.0, 0.0);
	txphase(zero);
	dec2phase(zero);
        delay(gstab);
	zgradpulse(gzlvl4, gt5);
	delay(lambda  -gt5 -gstab);
	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
	txphase(one);
	dec2phase(one);
	delay(lambda - gt5 -gstab);
        zgradpulse(gzlvl4, gt5);
        delay(gstab);
	sim3pulse(pw, 0.0, pwN, one, zero, one, 0.0, 0.0);
	txphase(zero);
	dec2phase(zero);
        delay(gstab);
	zgradpulse( gzlvl5, gt5);
	delay(lambda - gt5 -gstab);
	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
	delay(lambda - gt5 -gstab);
        zgradpulse(gzlvl5, gt5);
        delay(gstab);
        rgpulse(pw, zero, 0.0, 0.0); 
	delay(gt2  +gstab - 0.65*pw + 2.0*GRADIENT_DELAY + 2.0*POWER_DELAY);
	rgpulse(2.0*pw, zero, rof1, rof1);
	dec2power(dpwr2); decpower(dpwr);				       /* POWER_DELAY */
        zgradpulse( gzlvl2, gt2);		/* 2.0*GRADIENT_DELAY */
        delay(gstab -10.0e-6);
statusdelay(C,10.0e-6);		
	setreceiver(t31);
}		 
