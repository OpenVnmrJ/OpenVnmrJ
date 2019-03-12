/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghnco_Jcoca_3DS3.c 

This pulse sequence records 1J(COCa) and 3J(HNCa) couplings in a 1H, 15N, 13CO correlation 
spectrum. The spin-state-selection (referred here as S3) is provided along with the TROSY 
selection.
 
Set 13C carrier (dof) on CO-region. Apply Ca off-resonance.

Set phase=1,2,3,4 phase2=1,2 for S3-edited 3D experiment.

Pulse sequence: P. Permi, P. R. Rosevear and A. Annila, 
		J. Biomol. NMR, 17, 43-54 (2000).

		P. Permi, "Applications for measuring scalar and residual
		dipolar couplings in proteins",
		Acta Universitatis Ouluensis, A354, (2000).
		http://herkules.oulu.fi/isbn9514258223/


For a 1D check, set ni=1, ni2=1, phase=1, phase2=1 and use wft.

For a 2D experiment:

CH: set ni, phase=1,2 phase2=1 and f1coef='1 0 0 0 0 0 -1 0'.
    	Use wft2da. This gives a 13CO-1HN correlation spectrum with
    	1JCOCA splitting in 13C-dimension.

NH: set ni2, phase=1, phase2=1,2 and f2coef='1 0 -1 0 0 1 0 1'.
    	Use wft2da. This gives a 15N-1HN correlation spectrum.


For a 2D S3 experiment

CH: set ni, phase=1,2,3,4 and phase2=1.
    	Use wft2d(1,0,0,0,1,0,0,0,0,0,-1,0,0,0,-1,0) for adding data.
    	This gives the upfield component of 1JCOCA doublet.
         
        Use wft2d(1,0,0,0,-1,0,0,0,0,0,-1,0,0,0,1,0) for subtracting data.
        This gives the downfield component of 1JCOCA doublet.

For a 3D S3 experiment:

	Set ni, ni2, phase=1,2,3,4 phase2=1,2 and f2coef='1 0 -1 0 0 1 0 1'. 
        
        f1coef='1 0 0 0 1 0 0 0 0 0 -1 0 0 0 -1 0' followed by ft3d gives 
        a three-dimensional 13CO, 15N, 1HN correlation spectrum showing
        the upfield component of 1JCOCa doublet in 13C-dimension.

        f1coef='1 0 0 0 -1 0 0 0 0 0 -1 0 0 0 1 0' followed by ft3d gives
        a 3D 13CO, 15N, 1HN correlation spectrum displaying the downfield
        component of 1JCOCA doublet in 13C-dimension.


Written by P. Permi, Univ. of Helsinki (3dabhnco_jcoca_trosy_ns_pp.c)
Modified for BioPack by G.Gray, Sept 2004

*/

#include <standard.h>

static double d2_init = 0.0, d3_init = 0.0;

static int   phi1[1] = {1},  /* 90 for 15N prior to t1 */
             phi2[1] = {0},  /* first 90 for 1H in TROSY scheme */
             phi3[1] = {1},  /*  90 for 15N before acq. */
             phi4[2] = {0,2},  /* 90 13C' prior to t1 */
	     phi5[4] = {0,0,1,1},  
             phi6[8] = {0,0,0,0,2,2,2,2},  /* purge pulse on 13CO */
             phi7[4] = {0,2,2,0};  /* receiver */
             
void pulsesequence()
{
/* DECLARE VARIABLES */

 char      
 	     f1180[MAXSTR],f2180[MAXSTR],satmode[MAXSTR];

 int	     t1_counter,t2_counter,icosel,first_FID;

 double      /* DELAYS */	
             ni2=getval("ni2"),
             tau1,                      /* t1/2 */
             tau2,                      /* t2/2 */
	     kappa,tauhn,taunco,taucoca,taunca,
             tauhaca,taucaha,taucacb,

	     /* COUPLINGS */
             jhn = getval("jhn"),
             jnco = getval("jnco"),
             jcoca = getval("jcoca"),
             jnca = getval("jnca"),
             jhaca = getval("jhaca"),
             jcaha = getval("jcaha"),
             jcacb = getval("jcacb"),
   
	     /* PULSES */
             pwN = getval("pwN"),               /* PW90 for N-nuc */
             pwC = getval("pwC"),               /* PW90 for C-nuc */
             pwHs = getval("pwHs"),           /* pw for water selective pulse at tpwrs */
             pw90onco,pw180onco,pw180offca,   /* pw's for region-selective pulses */

	     /* POWER LEVELS */
             satpwr = getval("satpwr"),       /* low power level for presat */
             pwClvl = getval("pwClvl"),         /* power level for C hard pulses */ 
             pwNlvl = getval("pwNlvl"),         /* power level for N hard pulses */
             compH  = getval("compH"),          /* compression factor */
             compC  = getval("compC"),          /* compression factor */
             tpwrsf_u = getval("tpwrsf_u"),     /* fine power of first H2O sel pulse    */
             tpwrsf_d = getval("tpwrsf_d"),     /* fine power of second H2O sel pulse    */
             tpwrs,                             /* power level for sel. H2O pulse */
             rf90onco,                          /* power level for CO 90 pulses */ 
             rf180onco,                         /* power level for CO 180 pulses */
             rf180offca,                        /* power level for off-res Ca 180 pulses */
              

             /* GRADIENT DELAYS AND LEVELS */
             gt0 = getval("gt0"), /* gradient time */
             gt1 = getval("gt1"), /* gradient time */
             gt3 = getval("gt3"), /* gradient time */
             gt5 = getval("gt5"),
             gstab  = getval("gstab"),
             gzlvl0 = getval("gzlvl0"), /* level of gradient */
             gzlvl1 = getval("gzlvl1"),
             gzlvl2 = getval("gzlvl2"),
             gzlvl3 = getval("gzlvl3"),
             gzlvl5 = getval("gzlvl5");

/* LOAD VARIABLES */

  getstr("satmode",satmode); 
  getstr("f1180",f1180);
  getstr("f2180",f2180);


/* selective H20 one-lobe sinc pulse */
  tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
  tpwrs = (int) (tpwrs);                   	  /*power than a square pulse */
  if (tpwrsf_d<4095.0) tpwrs=tpwrs+6.0; /* allow for fine power control via tpwrsf_d */


/* check validity of parameter range */

    if ((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
	{
	printf("incorrect Dec1 decoupler flags!  ");
	psg_abort(1);
    } 

    if ((dm2[A] == 'y' || dm2[B] == 'y'))
	{
	printf("incorrect Dec2 decoupler flags!  ");
	psg_abort(1);
    } 

    if ( satpwr > 8 )
    {
	printf("satpwr too large !!!  ");
	psg_abort(1);
    }

    if ( dpwr > 50 )
    {
	printf("don't fry the probe, dpwr too large!  ");
	psg_abort(1);
    }

    if ( dpwr2 > 50 )
    {
	printf("don't fry the probe, dpwr2 too large!  ");
	psg_abort(1);
    }

/* LOAD VARIABLES */

  settable(t1, 1, phi1);
  settable(t2, 1, phi2);
  settable(t3, 1, phi3);
  settable(t4, 2, phi4);
  settable(t5, 4, phi5);
  settable(t6, 8, phi6);
  settable(t7, 4, phi7);

/* INITIALIZE VARIABLES AND POWER LEVELS FOR PULSES */

   tauhn =  ((jhn != 0.0) ? 1/(4*(jhn)) : 2.25e-3);
   taunco = ((jnco !=0.0) ? 1/(4*(jnco)) : 16.6e-3);
   taucoca = ((jcoca !=0.0) ? 1/(4*(jcoca)) : 4.5e-3);
   taunca =  ((jnca  !=0.0)  ? 1/(4*(jnca))  : 12e-3);
   tauhaca = ((jhaca !=0.0)  ? 1/(4*(jhaca)) : 12e-3);
   taucaha = ((jcaha !=0.0)  ? 1/(4*(jcaha)) : 12e-3);
   taucacb = ((jcacb !=0.0)  ? 1/(4*(jcacb)) : 12e-3);

       if((getval("arraydim") < 1.5) || (ix==1))
         first_FID = 1;
       else
         first_FID = 0;

    /* 90 degree pulse on CO, null at Ca 118ppm away */

        pw90onco = sqrt(15.0)/(4.0*118.0*dfrq);
        rf90onco = (4095.0*pwC*compC)/pw90onco;
        rf90onco = (int) (rf90onco + 0.5);
        if(rf90onco > 4095.0)
        {
          if(first_FID)
            printf("insufficient power for pw90onco -> rf90onco (%.0f)\n", rf90onco);
          rf90onco = 4095.0;
          pw90onco = pwC;
        }

    /* 180 degree pulse on CO, null at Ca 118ppm away */

        pw180onco = sqrt(3.0)/(2.0*118.0*dfrq);
        rf180onco = (4095.0*pwC*compC*2.0)/pw180onco;
        rf180onco = (int) (rf180onco + 0.5);
        if(rf180onco > 4095.0)
        {
          if(first_FID)
            printf("insufficient power for pw180onco -> rf180onco (%.0f)\n", rf180onco);
          rf180onco = 4095.0;
          pw180onco = pwC*2.0;
        }
        pw180offca = pw180onco;        rf180offca = rf180onco;
      
/* Phase incrementation for hypercomplex data */
   
         if (phase1 == 2)           /* Hypercomplex in t1 */
     {
	          tsadd(t4, 1, 4);
     }
    
         if (phase1 == 4)           /* Hypercomplex in t1 */
     {
	          tsadd(t4, 1, 4);
     }
   
     kappa=(taunco-tauhn)/(0.5*ni2/sw2)-0.001;
      if (kappa > 1.0) 
     {  
	              kappa=1.0-0.01;
     }   
      
      if (phase2 == 1)      /* Hypercomplex in t2 */
     {
	                icosel = -1;
	                tsadd(t2, 2, 4);
	                tsadd(t3, 2, 4);
     }  
      else icosel = 1;
      
      if (ix == 1)
      printf("semi constant time factor %4.6f\n",kappa);
   
/* calculate modification to phases based on current t1 values
   to achieve States-TPPI acquisition */
 
   if (ix == 1)
      d2_init = d2;
      t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);

      if (t1_counter %2)  /* STATES-TPPI */
      {
        tsadd(t4,2,4);
        tsadd(t7,2,4);
      }

/* calculate modification to phases based on current t2 values 
   to achieve States-TPPI acquisition */ 
       
         if (ix == 1)
            d3_init = d3;
            t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5);
      
            if (t2_counter %2)  /* STATES-TPPI */
     {
	                tsadd(t1,2,4);
	                tsadd(t7,2,4);
     }
   
/* set up so that get (-90,180) phase corrects in F1 if f1180 flag is y */

   tau1 = d2;
   if (f1180[A] == 'y')  tau1 += ( 1.0/(2.0*sw1));
   tau1 = tau1/2.0;

/* set up so that get (-90,180) phase corrects in F2 if f2180 flag is y */
            
   tau2 = d3;
   if(f2180[A] == 'y')  tau2 += ( 1.0/(2.0*sw2) );
   tau2 = tau2/2.0;
   
/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(satpwr);            /* Set power for presaturation    */
   decpower(pwClvl);              /* Set decoupler1 power to pwClvl  */
   decpwrf(rf90onco);
   dec2power(pwNlvl);            /* Set decoupler2 power to pwNlvl */

/* Presaturation Period */

status(B);

  if (satmode[0] == 'y')
    {
      rgpulse(d1,zero,rof1,rof1);
      obspower(tpwr);            /* Set power for hard pulses  */
    }
  else  
    {
      obspower(tpwr);  /* Set power for hard pulses  */
      delay(d1);
    }

rcvroff();

     /* eliminate all magnetization originating on 13C' */
   
     decpwrf(rf90onco);
     decrgpulse(pw90onco,zero,rof1,rof1);
      
     zgradpulse(gzlvl0,gt0);
     delay(gstab);
   
   
  /* transfer from HN to N by INEPT */
   
  /* shaped pulse for water flip-back */
      obspower(tpwrs); obspwrf(tpwrsf_d);
      shaped_pulse("H2Osinc_d",pwHs,one,rof1,0.0);
      obspower(tpwr); obspwrf(4095.0);
  /* shaped pulse */

  rgpulse(pw,zero,rof1,rof1); /* 90 for 1H */

  zgradpulse(gzlvl0*0.8,gt0);
  delay(gstab);
   
  delay(tauhn - gt0 - gstab);        /* 1/4JHN */
   
  sim3pulse(2.0*pw,0.0,2.0*pwN,zero,zero,zero,rof1,rof1);  /* 180 for 1H and 15N */

  delay(tauhn - gt0 - gstab);       /* 1/4JHN */
   
  zgradpulse(gzlvl0*0.8,gt0);
  delay(gstab);

  dec2phase(zero);

  rgpulse(pw,three,rof1,rof1);
      
  zgradpulse(gzlvl3,gt3);
  delay(gstab);
   
/* start transfer from N to CO */
      
  dec2rgpulse(pwN,zero,0.0,0.0);
    
  decphase(zero);

  delay(taunco - gt5 - gstab - POWER_DELAY - 0.5*pw180onco);	 /* 1/4J(NCO) */
	  
  decpwrf(rf180onco); 

  zgradpulse(gzlvl5,gt5);
  delay(gstab);

  sim3pulse(0.0,pw180onco,2.0*pwN,zero,zero,zero,rof1,rof1);  /* 180 for 13C' and 15N */

  zgradpulse(gzlvl5,gt5);
  delay(gstab);

  decpwrf(rf90onco); 
  dec2phase(one);

  delay(taunco - gt5 - gstab - POWER_DELAY - 0.5*pw180onco); 	    /* 1/4J(NCO) */

  dec2rgpulse(pwN,one,0.0,0.0);  /* 90 for 15N */
   
  zgradpulse(gzlvl3*0.8,gt3);
  delay(gstab);
    
  decrgpulse(pw90onco,zero,0.0,0.0);  /* 90 for 13C' */

  decphase(zero);
   
   if ( (phase1 == 3) || (phase1 == 4))
     {
	  decpwrf(rf180offca);    /* Set decoupler1 power to rf180offca */
	  decshaped_pulse("offC3",pw180offca,zero,0.0,0.0);
	   
          decpwrf(rf180onco);           /* Set decoupler1 power to rf180onco */

	  delay(taucoca - gt5 - gstab - 0.5*pw180onco - pw180offca - 2.0*POWER_DELAY);  /* 1/(4JCOCA) */

          zgradpulse(gzlvl5*0.6,gt5);
          delay(gstab);
	
          sim3pulse(0.0,pw180onco,0.0,zero,zero,zero,rof1,rof1);  /* 180 13C' */
  	  decpwrf(rf180offca);              	     /* Set decoupler1 power to rf180offca */
	  decshaped_pulse("offC3",pw180offca,zero,0.0,0.0); 
    
          zgradpulse(gzlvl5*0.6,gt5);
          delay(gstab);

	  delay(taucoca - gt5 - gstab - 0.5*pw180onco - pw180offca - 2.0*POWER_DELAY);   /* 1/(4JCOCA) */

          decpwrf(rf90onco);    /* Set decoupler1 power to rf90onco */
	  decphase(one);

          decrgpulse(pw90onco,one,0.0,0.0); /* 90 for 13C' */

          /* Field crusher gradient */
          zgradpulse(gzlvl3*1.1,gt3);
          delay(gstab);
	  /* Field crusher gradient */

        }

    if (( phase1 == 1) || (phase1 == 2))
       {
	decphase(zero);
        zgradpulse(gzlvl5*0.6,gt5);
        delay(gstab);
	
        decpwrf(rf180offca);   /* Set decoupler1 power to rf180offca */

	delay(0.5*taucoca - gt5 - gstab - POWER_DELAY - 0.5*pw180offca);             /* 1/(8JCOCA) */
	 
	decshaped_pulse("offC3",pw180offca,zero,0.0,0.0); 

	delay(0.5*taucoca - gt5 - gstab - POWER_DELAY - 0.5*pw180offca - 0.5*pw180onco); /* 1/(8JCOCA) */
	
        decpwrf(rf180onco);   /* Set decoupler1 power to rf180onco */

        zgradpulse(gzlvl5*0.6,gt5);
        delay(gstab);

        decrgpulse(pw180onco,zero,0.0,0.0);   /* 180 13C' */

        zgradpulse(gzlvl5*0.6,gt5);
        delay(gstab);
	
	decpwrf(rf180offca);      

	delay(0.5*taucoca - gt5 - gstab - POWER_DELAY - 0.5*pw180onco - 0.5*pw180offca); /* 1/(8JCOCA) */
	      
        decshaped_pulse("offC3",pw180offca,zero,0.0,0.0);
	 
	delay(0.5*taucoca - gt5 - gstab - POWER_DELAY - 0.5*pw180offca); /* 1/(8JCOCA) */

        decpwrf(rf90onco);    /* Set decoupler1 power to rf90onco */

        zgradpulse(gzlvl5*0.6,gt5);
        delay(gstab);

	decphase(zero);

        decrgpulse(pw90onco,zero,0.0,0.0);  /* 90 for 13C' */

        /* Field crusher gradient */
           zgradpulse(gzlvl3*1.1,gt3);
           delay(gstab);
	/* Field crusher gradient */

      }

/* record 13C' shift and 13C'-13Ca coupling frequencies */

     decphase(t4);
     decrgpulse(pw90onco,t4,0.0,0.0);  /* 90 for 13C' */
     decphase(t5);
   
     delay(tau1);
   
     dec2rgpulse(2.0*pwN,zero,0.0,0.0);
   
     delay(tau1);
   
     decpwrf(rf180onco);
     decrgpulse(pw180onco,t5,0.0,0.0);
     
     delay(2.0*pwN);
   
     decphase(zero);
     decpwrf(rf90onco); /* Set decoupler1 power to rf90onco */
     decrgpulse(pw90onco,zero,0.0,0.0);  /* 90 for 13C' */
   
     zgradpulse(gzlvl3*1.3,gt3);
     delay(gstab);
     dec2rgpulse(pwN,t1,0.0,0.0); /* 90 15N */
   
     delay((taunco - tauhn) - kappa*tau2);  /* 1/(4JNCO) - 1/4J(NH) - kt2/2 */
      
     dec2phase(zero);
     dec2rgpulse(2.0*pwN,zero,0.0,0.0);     /* 180 for 15N */
          
     delay((1-kappa)*tau2);                  /* (1-k)t2/2 */
   
     decpwrf(rf180onco);
     decrgpulse(pw180onco,zero,0.0,0.0);     /* 180 for 13C' */
     decpwrf(rf180offca);
   
     delay(taunco - tauhn - gt1 - gstab - pw180onco - pw180offca - 3.0*POWER_DELAY);
      
     zgradpulse(gzlvl1,gt1);
     delay(gstab); 
     decshaped_pulse("offC3",pw180offca,zero,0.0,0.0);
      
     delay(tau2);                            /* t2/2 */
   
   /* start TROSY transfer from N to HN */
      
     txphase(t2);
         
     rgpulse(pw,t2,rof1,rof1);

     zgradpulse(gzlvl5*0.75,gt5);
     delay(gstab);
     dec2phase(zero);
 
     delay(tauhn - gt5 - gstab - POWER_DELAY);

     decpwrf(rf180onco);
     sim3pulse(2.0*pw,pw180onco,2.0*pwN,zero,zero,zero,rof1,rof1);
   
     delay(tauhn - gt5 - gstab - POWER_DELAY);
   
     dec2phase(zero);
      
     zgradpulse(gzlvl5*0.75,gt5);
     delay(gstab);
   
     decphase(t6);
     decpwrf(rf90onco);
     sim3pulse(pw,pw90onco,pwN,one,t6,zero,rof1,rof1);
   
  /* shaped pulse WATERGATE */
     obspower(tpwrs); obspwrf(tpwrsf_u);
     shaped_pulse("H2Osinc_u",pwHs,t2,rof1,0.0);
     obspower(tpwr); obspwrf(4095.0);
  /* shaped pulse */
      
     dec2phase(zero);
     zgradpulse(gzlvl5,gt5);
     delay(gstab);
      
     delay(tauhn - gt5 - gstab - 2.0*POWER_DELAY - pwHs);
   
     sim3pulse(2.0*pw,0.0,2.0*pwN,zero,zero,zero,rof1,rof1);
     
     zgradpulse(gzlvl5,gt5);
     delay(gstab);
   
     dec2phase(t3);
      
     delay(tauhn - gt5 - gstab - 2.0*POWER_DELAY);
   
     decpower(dpwr);
      
     dec2rgpulse(pwN,t3,0.0,0.0);  /* 90 for 15N */
      
     dec2power(dpwr2);
      
     delay((gt1/10.0) - pwN + gstab - POWER_DELAY);
      
     rgpulse(2.0*pw, zero, rof1, rof1);
      
     zgradpulse(icosel*gzlvl2,gt1*0.1);
     delay(gstab);
      
  /* acquire data */
      
   status(C);
     setreceiver(t7);
}
