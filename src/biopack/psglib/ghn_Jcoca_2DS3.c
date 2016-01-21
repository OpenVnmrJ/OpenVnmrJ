/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghn_Jcoca_2DS3.c

This pulse sequence records 1J(CoCa) couplings in a 1H-15N correlation spectrum. 
The spin-state-selection (referred here as S3) is provided along with the TROSY
selection.

Set 13C carrier (dof) on CO-region. Apply Ca off-resonance.
set phase=1,2 and abfilter='a','b' to achieve ab-filtered data sets. 

Lambda defines scaling factor for 1JC'Ca, if set to 1, the 1JC'Ca is
not scaled with respect 15N shift.

Pulse sequence: P. Permi and A. Annila,
		J. Biomol. NMR 16, 221-227 (2000).
                  (as abhnco_jcoca_trosy_ns_pp.c)

                P. Permi, "Applications for measuring scalar and residual
		dipolar couplings in proteins",
		Acta Universitatis Ouluensis, A354, (2000).
		http://herkules.oulu.fi/isbn9514258223/

Written by P. Permi, University of Helsinki
Modified for BioPack by G.Gray, Varian, Sept 2004



For spin-state-selective, interleaved experiment: 
Set phase=1,2 for quadrature detection with abfilter='a' or 'b'.
  [Use wft2d('1 0 -1 0 0 1 0 1') or set f1coef='1 0 -1 0 0 1 0 1' and used wft2da]

Set phase=1,2 and abfilter=a,b  and array='abfilter,phase' for the in-phase and antiphase data. 
   [wft2d(1,0,-1,0,1,0,-1,0,0,-1,0,-1,0,-1,0,-1) (add)
    wft2d(1,0,-1,0,-1,0,1,0,0,-1,0,-1,0,1,0,1) (subtract).]

For phase=1,2 and abfilter=a,b  and array='phase,abfilter' use transforms
   [wft2d(1,0,1,0,-1,0,-1,0,0,-1,0,-1,0,-1,0,-1) (add)
    wft2d(1,0,-1,0,-1,0,1,0,0,-1,0,1,0,-1,0,1) (subtract).]

*/

#include <standard.h>

static double d2_init = 0.0;

static int   phi1[1] = {1},  /* 90 for 15N prior to t1 */
             phi2[1] = {0},  /* first 90 for 1H in TROSY scheme */
             phi3[1] = {1},  /*  90 for 15N ibefore acq. */
             phi4[2] = {0,2},  /* 90 13C' prior to t1 */
	     phi5[4] = {0,0,1,1},  /* pulse in a/b-filter, if abfilter=b */
             phi6[8] = {0,0,0,0,2,2,2,2},  /* purge pulse on 13CO */
             phi7[4] = {0,2,2,0};  /* receiver */
             
pulsesequence()
{
/* DECLARE VARIABLES */

 char      
 	     f1180[MAXSTR],
	     abfilter[MAXSTR];	/* set 'a' for inphase and 'b' for antiphase */


 int	     t1_counter,icosel,first_FID;

 double      /* DELAYS */
             tau1,                      /* t1/2 */
             
	     /* COUPLINGS */ 
             jhn = getval("jhn"),
             jnco = getval("jnco"),
             jcoca = getval("jcoca"),
             jnca = getval("jnca"),
             jhaca = getval("jhaca"),
             jcaha = getval("jcaha"),
             jcacb = getval("jcacb"),
             tauhn,taunco,taucoca,taunca,tauhaca,taucaha,taucacb,
   
	     /* PULSES */
             pwN = getval("pwN"),               /* PW90 for N-nuc */
             pwC = getval("pwC"),               /* PW90 for C-nuc */
             pwHs = getval("pwHs"),

             /* POWER LEVELS */
             satpwr = getval("satpwr"),         /* low power level for presat */
             tpwrsf_d = getval("tpwrsf_d"),    /* fine power for first flipback pulse */
             tpwrsf_u = getval("tpwrsf_u"),    /* fine power for second flipback pulse */
             compH  = getval("compH"),          /* amplifier compression factor  */
             compC  = getval("compC"),          /* amplifier compression factor  */
             tpwrs,
             pwClvl = getval("pwClvl"),         /* power level for C hard pulses */ 
             pwNlvl = getval("pwNlvl"),         /* power level for N hard pulses */
             rf90onco,pw90onco,               /* power level/pw for CO 90 pulses */ 
             rf180onco,pw180onco,             /* power level/pw for CO 180 pulses */
             rf180offca,pw180offca,           /* power level/pw for CA 180 pulses */

             /* CONSTANTS */
             kappa,
             lambda = getval("lambda"),      /* scaling factor for 1JC'Ca */
             dofca,

	     /* GRADIENT DELAYS AND LEVES */
             gt1 = getval("gt1"),                    /* gradient time */
             gt0 = getval("gt0"),
             gt3 = getval("gt3"),
             gt5 = getval("gt5"),
             gstab  = getval("gstab"),
             gzlvl0 = getval("gzlvl0"),              /* level of gradient */
             gzlvl1 = getval("gzlvl1"),
             gzlvl2 = getval("gzlvl2"),
             gzlvl3 = getval("gzlvl3"),
             gzlvl5 = getval("gzlvl5");

/* LOAD VARIABLES */

  getstr("f1180",f1180); 
  getstr("abfilter",abfilter);

/* check validity of parameter range */

    if ((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
	{
	printf("incorrect Dec1 decoupler flags!  ");
	psg_abort(1);
    } 

    if ((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' ))
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

/* INITIALIZE VARIABLES */

   tauhn = ((jhn != 0.0) ? 1/(4*(jhn)) : 2.25e-3);
   taunco = ((jnco !=0.0) ? 1/(4*(jnco)) : 16.6e-3);
   taucoca = ((jcoca !=0.0) ? 1/(4*(jcoca)) : 4.5e-3);
   taunca =  ((jnca  !=0.0)  ? 1/(4*(jnca))  : 12e-3);
   tauhaca = ((jhaca !=0.0)  ? 1/(4*(jhaca)) : 12e-3);
   taucaha = ((jcaha !=0.0)  ? 1/(4*(jcaha)) : 12e-3);
   taucacb = ((jcacb !=0.0)  ? 1/(4*(jcacb)) : 12e-3);

   dofca = dof - 118.0*dfrq;

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
      


/* selective H20 one-lobe sinc pulse */
  tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
  tpwrs = (int) (tpwrs);                   	  /*power than a square pulse */
  if (tpwrsf_d<4095.0) tpwrs=tpwrs+6.0; /* allow for fine power control via tpwrsf_d */



/* Phase incrementation for hypercomplex data */

   if (abfilter[0] == 'b')
     {
	         tsadd(t1, 1, 4);
	         tsadd(t4, 1, 4);
     }
   
     kappa=(taunco-tauhn)/(0.5*ni/sw1)-0.001;
      if (kappa > 1.0) 
     {  
	              kappa=1.0-0.01;
     }   
      
      if (phase1 == 1)      /* Hypercomplex in t1 */
     {
	                icosel = -1;
	                tsadd(t2, 2, 4);
	                tsadd(t3, 2, 4);
     }  
      else icosel = 1;   
      
      if (ix == 1)
      printf("semi constant-time factor %4.6f\n",kappa);
   
/* calculate modification to phases based on current t1 values
   to achieve States-TPPI acquisition */ 
 
   if (ix == 1)
      d2_init = d2;
      t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);

      if (t1_counter %2)  /* STATES-TPPI */
      {
        tsadd(t1,2,4);
        tsadd(t7,2,4);
      }

/* set up so that get (-90,180) phase corrects in F1 if f1180 flag is y */

   tau1 = d2;
   if (f1180[A] == 'y')  tau1 += ( 1.0/(2.0*sw1));
   tau1 = tau1/2.0;
   

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(satpwr);            /* Set power for presaturation    */
   decpower(pwClvl);            /* Set decoupler1 power to pwClvl  */
   decpwrf(rf90onco);
   dec2power(pwNlvl);           /* Set decoupler2 power to pwNlvl */

/* Presaturation Period */

status(B);

  if (satmode[0] == 'y')
    {
      rgpulse(d1,zero,rof1,rof1);
      obspower(tpwr);              /* Set power for hard pulses  */
    }
  else  
    {
      obspower(tpwr);              /* Set power for hard pulses  */
      delay(d1);
    }


  rcvroff();
     /* eliminate all magnetization originating on 13C */
   
     decpwrf(rf90onco); 
     decrgpulse(pw90onco,zero,0.0,0.0);
      
     zgradpulse(gzlvl0*1.5,gt0);
     delay(gstab);

  /* transfer from HN to N by INEPT */

  /* shaped pulse for water flip-back */
      obspower(tpwrs); obspwrf(tpwrsf_d);
      shaped_pulse("H2Osinc_d",pwHs,one,rof1,0.0);
      obspower(tpwr); obspwrf(4095.0);
  /* shaped pulse */

  rgpulse(pw,zero,rof1,rof1); /* 90 for 1H */

  zgradpulse(gzlvl0,gt0);
  delay(gstab);
   
  delay(tauhn - gt0 - gstab);        /* 1/4JHN */
   
  sim3pulse(2.0*pw,0.0,2.0*pwN,zero,zero,zero,rof1,rof1);  /* 180 for 1H and 15N */

  delay(tauhn - gt0 - gstab);       /* 1/4JHN */
   
  zgradpulse(gzlvl0,gt0);
  delay(gstab);

  dec2phase(zero);

  rgpulse(pw,three,rof1,rof1);
      
  zgradpulse(gzlvl3,gt3);
  delay(gstab);
   
/* start transfer from N to CO */
      
  dec2rgpulse(pwN,zero,0.0,0.0);
    
  delay(taunco - gt0 - gstab - POWER_DELAY - 0.5*pw180onco);	 /* 1/4J(NCO) */
	  
  decpwrf(rf180onco);              	/* Set decoupler1 power to rf180onco */

  zgradpulse(gzlvl0,gt0);
  delay(gstab);

  sim3pulse(0.0,pw180onco,2.0*pwN,zero,zero,zero,rof1,rof1);  /* 180 for 13C' and 15N */

  zgradpulse(gzlvl0,gt0);
  delay(gstab);

  decpwrf(rf90onco);            /* Set decoupler1 power to rfco90 */

  delay(taunco - gt0 - gstab - POWER_DELAY - 0.5*pw180onco); 	    /* 1/4J(NCO) */

  dec2rgpulse(pwN,one,0.0,0.0);  /* 90 for 15N */
   
  zgradpulse(gzlvl3*1.2,gt3);
  delay(gstab);
    
  decrgpulse(pw90onco,zero,0.0,0.0);  /* 90 for 13C' */

  decphase(zero);
	
      if (abfilter[0] == 'b')
        {
	  decpwrf(rf180offca);    /* Set decoupler1 power to rf180offca */
	  decshaped_pulse("offC3",pw180offca,zero,0.0,0.0); 
	   
          decpwrf(rf180onco);           /* Set decoupler1 power to rf180onco */

	  delay(taucoca - gt5 - gstab - 0.5*pw180onco - pw180offca - 2.0*POWER_DELAY);  /* 1/4J(COCA) */

          zgradpulse(gzlvl5*0.6,gt5);
	  delay(gstab);
	
          decrgpulse(pw180onco,zero,0.0,0.0);  /* 180 13C' */
  	  decpwrf(rf180offca);              	     /* Set decoupler1 power to rf180offca */
	  decshaped_pulse("offC3",pw180offca,zero,0.0,0.0); 
    
          zgradpulse(gzlvl5*0.6,gt5);
	  delay(gstab);

	  delay(taucoca - gt5 - gstab - 0.5*pw180onco - pw180offca - 2.0*POWER_DELAY);   /* 1/4J(NCO) */

          decpwrf(rf90onco);    /* Set decoupler1 power to rf90onco */
	  decphase(one);

          decrgpulse(pw90onco,one,0.0,0.0); /* 90 for 13C' do not touch alpha's */

          /* Field crusher gradient */
          zgradpulse(gzlvl3*1.4,gt3);
	  delay(gstab);
	  /* Field crusher gradient */

        }

    if (abfilter[0] == 'a')
      {

	decphase(zero);
        zgradpulse(gzlvl5*0.6,gt5);
	delay(gstab);
	
        decpwrf(rf180offca);   /* Set decoupler1 power to rf180offca */

	delay(0.5*taucoca - gt5 - gstab - POWER_DELAY - 0.5*pw180offca);                   /* 1/8J(COCA) */
	 
	decshaped_pulse("offC3",pw180offca,zero,0.0,0.0); 

	delay(0.5*taucoca - gt5 - gstab - POWER_DELAY - 0.5*pw180offca - 0.5*pw180onco);     /* 1/8J(COCA) */
	
        decpwrf(rf180onco);   /* Set decoupler1 power to rf180onco */

        zgradpulse(gzlvl5*0.6,gt5);
	delay(gstab);

        decrgpulse(pw180onco,zero,0.0,0.0);   /* 180 13C' */

        zgradpulse(gzlvl5*0.6,gt5);
	delay(gstab);
	
	decpwrf(rf180offca);      

	delay(0.5*taucoca - gt5 - gstab - POWER_DELAY - 0.5*pw180onco - 0.5*pw180offca);      /* 1/8J(COCA) */
	      
        decshaped_pulse("offC3",pw180offca,zero,0.0,0.0);
	 
	delay(0.5*taucoca - gt5 - gstab - POWER_DELAY - 0.5*pw180offca); /* 1/8J(COCA) */

        decpwrf(rf90onco);

        zgradpulse(gzlvl5*0.6,gt5);
	delay(gstab);

	decphase(zero);

        decrgpulse(pw90onco,zero,0.0,0.0);  /* 90 for 13C' */

        /* Field crusher gradient */
        zgradpulse(gzlvl3*1.4,gt3);
	delay(gstab);
	/* Field crusher gradient */

      }

/* record 13C' and 13Ca coupling frequencies */

     decphase(t4);
     decrgpulse(pw90onco,t4,0.0,0.0);  /* 90 for 13C' */
     decphase(t5);
   
     decpwrf(rf180offca);      
     decshaped_pulse("offC3",pw180offca,zero,0.0,0.0);
   
     delay(lambda*tau1);
   
     decpwrf(rf180onco);
     decrgpulse(pw180onco,t5,0.0,0.0);
     
     decpwrf(rf180offca); /* Set decoupler1 power to rf180offca */
     decshaped_pulse("offC3",pw180offca,zero,0.0,0.0);
   
     delay(lambda*tau1);
   
     decphase(zero);
     decpwrf(rf90onco); /* Set decoupler1 power to rf90onco */
     decrgpulse(pw90onco,zero,0.0,0.0);  /* 90 for 13C' */
   
     zgradpulse(gzlvl3*0.5,gt3);
     delay(gstab);
   
     dec2phase(t1);
     dec2rgpulse(pwN,t1,0.0,0.0); /* 90 15N */
   
     delay((taunco - tauhn) - kappa*tau1);  /* 1/4J(NCO) - kt1/2 */
      
     dec2phase(zero);
     dec2rgpulse(2.0*pwN,zero,0.0,0.0);     /* 180 for 15N */
          
     delay((1-kappa)*tau1);                      /* (1-k)t1/2 */
   
     decpwrf(rf180onco);
     decrgpulse(pw180onco,zero,0.0,0.0);     /* 180 for 13C' */
     decpwrf(rf180offca);
   
     delay(taunco - tauhn - gt1 - gstab - pw180onco - pw180offca - 3.0*POWER_DELAY);
      
     zgradpulse(gzlvl1,gt1);
     delay(gstab);

     decshaped_pulse("offC3",pw180offca,zero,0.0,0.0);
      
     delay(tau1);                            /* t1/2 */
   
   /* start TROSY transfer from N to HN */
      
     txphase(t2);
     rgpulse(pw,t2,rof1,rof1);
          
     zgradpulse(gzlvl5,gt5);
     delay(gstab);
 
     delay(tauhn - gt5 - gstab - POWER_DELAY);
   
     decpwrf(rf180onco);
     sim3pulse(2.0*pw,pw180onco,2*pwN,zero,zero,zero,rof1,rof1);
   
     delay(tauhn - gt5 - gstab - POWER_DELAY);
   
      
     zgradpulse(gzlvl5,gt5);
     delay(gstab);
   
     decpwrf(rf90onco);
     sim3pulse(pw,pw90onco,pwN,one,t6,zero,rof1,rof1);
   
         /* shaped pulse WATERGATE */
          obspower(tpwrs); obspwrf(tpwrsf_u);
          shaped_pulse("H2Osinc_u",pwHs,t2,rof1,0.0);
          obspower(tpwr); obspwrf(4095.0);
         /* shaped pulse */
   
          zgradpulse(gzlvl5*0.85,gt5);
          delay(gstab);
      
          delay(tauhn - gt5 - gstab - 2.0*POWER_DELAY - pwHs);
   
          sim3pulse(2.0*pw,0.0,2.0*pwN,zero,zero,zero,rof1,rof1);
   
      
          delay(tauhn - gt5 - gstab - 2.0*POWER_DELAY);
 
          zgradpulse(gzlvl5*0.85,gt5);
          delay(gstab);
          decpower(dpwr);
      
          dec2rgpulse(pwN,t3,0.0,0.0);  /* 90 for 15N */
      
          dec2power(dpwr2);
      
          delay((gt1/10.0) - pwN + gstab - POWER_DELAY);
      
          rgpulse(2.0*pw, zero, rof1, rof1);
      
          zgradpulse(icosel*gzlvl2,gt1/10.0);
          delay(gstab);
      /* acquire data */
      
   status(C);
          setreceiver(t7);
}
