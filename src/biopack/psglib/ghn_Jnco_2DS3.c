/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* ghn_Jnco_2DS3.c

This pulse sequence records 1J(NCO) and 2J(HNCO) couplings in a 1H-15N correlation spectrum. 
The spin-state-selection (referred here as S3) is provided along with the TROSY selection.

Set 13C carrier at CO-region. To decouple 13Ca during t1, apply 180Ca pulse off-resonance.

Set JNCO to 15 Hz.
abfilter=a for the in-phase data.
abfilter=b for the antiphase data.

For spin-state-selective, interleaved experiment: 
Set phase=1,2 for quadrature detection with abfilter='a' or 'b'.
  [Use wft2d('1 0 -1 0 0 1 0 1') or set f1coef='1 0 -1 0 0 1 0 1' and used wft2da]

Set phase=1,2 and abfilter=a,b  and array='abfilter,phase' for the in-phase and antiphase data. 
   [wft2d(1,0,-1,0,1,0,-1,0,0,1,0,1,0,1,0,1) (add)
    wft2d(1,0,-1,0,-1,0,1,0,0,1,0,1,0,-1,0,-1) (subtract).]

For phase=1,2 and abfilter=a,b  and array='phase,abfilter' use transforms
   [wft2d(1,0,1,0,-1,0,-1,0,1,0,1,0,1,0,1,0) (add)
    wft2d(1,0,-1,0,-1,0,1,0,0,1,0,-1,0,1,0,-1) (subtract).]

Pulse sequence: P. Permi, S. Heikkinen, I. Kilpelainen and A. Annila,
		J. Magn. Reson., 140, 32-40 (1999).
                submitted as abhn_jhnco_trosy_ns_pp.c

		P. Permi, "Applications for measuring scalar and residual
		dipolar couplings in proteins",
		Acta Universitatis Ouluensis, A354, (2000).
		http://herkules.oulu.fi/isbn9514258223/

Written by P. Permi, University of Helsinki


Modified for BioPack by G.Gray, Varian Sept 2004
*/

#include <standard.h>

static double d2_init = 0.0;


static int phi1[2] = {0,2}, /* N-90 before t1 */
           phi2[1] = {0},   /* H-90 after t1 */
           phi3[1] = {1},   /* N-90 before acq. */
           phi4[1] = {0},   /* 180 N in the middle of filter element */
           phi5[4] = {0,0,2,2},	/* 13Ca in t1 */
           phi7[4] = {0,2,0,2};  /* receiver */


void pulsesequence()
{
/* DECLARE VARIABLES */

 char      
             f1180[MAXSTR],satmode[MAXSTR],abfilter[MAXSTR];

 int	     phase,t1_counter,icosel,ni,first_FID;

 double      /* DELAYS */
             tauhn,
             taunco,
             tau1,                               /* t1/2 */

             /* COUPLINGS */
             jhn = getval("jhn"),
             jnco = getval("jnco"),
   
             /* PULSES */
             pw180offca = getval("pw180offca"), /* PW180 for off-res ca nucleus @ rf180offca */
             pw90onco,                          /* PW90 for on-res co nucleus @ rf90onco */
             pw180onco,                         /* PW180 for on-res co nucleus @ rf90onco */
             pwN = getval("pwN"),               /* PW90 for N-nuc */
             pwC = getval("pwC"),               /* PW90 for C-nuc */
             pwHs = getval("pwHs"),             /* pw for water selective pulse  */

             /* POWER LEVELS */
             satpwr = getval("satpwr"),       /* low power level for presat */
             pwClvl = getval("pwClvl"),         /* power level for C hard pulses */ 
             pwNlvl = getval("pwNlvl"),         /* power level for N hard pulses */
             rf90onco,                          /* power level for CO 90 pulses */
             rf180onco,                         /* power level for CO 180 pulses */
             rf180offca,                        /* power level for off-res Ca 180 pulses */

             compH = getval("compH"),       /* adjustment for C13 amplifier compression */
             compC = getval("compC"),       /* adjustment for C13 amplifier compression */

             /* CONSTANTS */
             sw1 = getval("sw1"),
             dof = getval("dof"),
             kappa,
             dofca,
             tpwrsf_d = getval("tpwrsf_d"), /* fine power adustment for first soft pulse*/
             tpwrsf_u = getval("tpwrsf_u"), /* fine power adustment for first soft pulse*/
             tpwrs,	  	              /* power for the pwHs ("H2Osinc") pulse */

             /* GRADIENT DELAYS AND LEVES */
             gstab = getval("gstab"),
             gt0 = getval("gt0"),             /* gradient time */
             gt1 = getval("gt1"),             /* gradient time */
             gt3 = getval("gt3"),
             gt5 = getval("gt5"),
             gzlvl0 = getval("gzlvl0"),       /* level of gradient */
             gzlvl1 = getval("gzlvl1"),       /* level of gradient */
             gzlvl2 = getval("gzlvl2"),
             gzlvl3 = getval("gzlvl3"),
             gzlvl5 = getval("gzlvl5");

/* selective H20 one-lobe sinc pulse */
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
    tpwrs = (int) (tpwrs);                   	  /*power than a square pulse */

/* 180 degree pulse on Ca, null at CO 118ppm away */
    rf180offca = (compC*4095.0*pwC*2.0)/pw180offca;
    rf180offca = (int) (rf180offca + 0.5);


/* LOAD VARIABLES */

  ni = getval("ni");
  phase = (int) (getval("phase") + 0.5);
  getstr("satmode",satmode); 
  getstr("f1180",f1180); 
  getstr("abfilter",abfilter);

/* check validity of parameter range */

    if (dm[A] == 'y')
	{
	printf("incorrect Dec1 decoupler flags!  ");
	psg_abort(1);
    } 

    if (dm2[A] == 'y')
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

  settable(t1, 2, phi1);
  settable(t2, 1, phi2);
  settable(t3, 1, phi3);
  settable(t4, 1, phi4);
  settable(t5, 4, phi5);
  settable(t7, 4, phi7);

/* INITIALIZE VARIABLES AND POWER LEVELS FOR PULSES */

  tauhn = 1/(4.0*95.0);  /* initialize */
  taunco = 1/(4.0*15.0);  /* initialize */
  tauhn = ((jhn != 0.0) ? 1/(4*(jhn)) : 2.25e-3);
  taunco = ((jnco !=0.0) ? 1/(4*(jnco)) : 16.6e-3);

  kappa=(gt1 +gstab + (4.0/PI)*pwN + WFG_START_DELAY + pw180offca)/(0.5*ni/sw1)-0.001;

  if (kappa > 1.0) 
     {  
                              kappa=1.0-0.01;
     }

  if (ix == 1)
  printf("semi-constant-time factor %4.6f\n",kappa);

  dofca = dof - 118.0*dfrq;

       if((getval("arraydim") < 1.5) || (ix==1))
         first_FID = 1;
       else
         first_FID = 0;

/* 90 degree pulse on CO, null at Ca 118ppm away */

        pw90onco = sqrt(15.0)/(4.0*118.0*dfrq);
        rf90onco = (compC*4095.0*pwC)/pw90onco;
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
        rf180onco = (compC*4095.0*pwC*2.0)/pw180onco;
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

  if (phase == 1)      /* Hypercomplex in t2 */
     {
	                        icosel = 1;
	                        tsadd(t2, 2, 4);
	                        tsadd(t3, 2, 4);
     }  
      else icosel = -1;
   
/* calculate modification to phases based on current t1 values
   to achieve States-TPPI acquisition */
  
   if(ix == 1)
      d2_init = d2;
      t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);

      if(t1_counter %2) 
      {
        tsadd(t1, 2, 4);
        tsadd(t7, 2, 4);
      }

/* set up so that get (-90,180) phase corrects in F1 if f1180 flag is y */

   tau1 = d2;
   if (f1180[A] == 'y')  tau1 += ( 1.0/(2.0*sw1));
   tau1 = tau1/2.0;
   
/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(satpwr);            /* Set power for presaturation    */
   decpower(pwClvl);              /* Set decoupler1 power to pwClvl  */
   decpwrf(rf180onco);
   dec2power(pwNlvl);            /* Set decoupler2 power to pwNlvl */
   obsoffset(tof);
   decoffset(dof);
   dec2offset(dof2);

/* Presaturation Period */

 if (satmode[0] == 'y')
   {
     rgpulse(d1,zero,rof1,rof1);
     obspower(tpwr);               /* Set power for hard pulses  */
   }
 else  
   {
     obspower(tpwr);               /* Set power for hard pulses  */
     delay(d1);
   }

if (tpwrsf_d<4095.0) tpwrs=tpwrs+6.0; /* allow for fine power control via tpwrsf_d */

  rcvroff();
  dec2phase(zero);
  obspwrf(tpwrsf_d); obspower(tpwrs);
  shaped_pulse("H2Osinc_d", pwHs, one, rof1, rof1);
  obspower(tpwr); obspwrf(4095.0);

  rgpulse(pw,zero,rof1,0.0);

  zgradpulse(gzlvl0,gt0);
  delay(gstab);
   
  delay(tauhn - gt0 - gstab);   /* delay for 1/4JHN coupling */
   
  sim3pulse(2.0*pw,0.0,2.0*pwN,zero,zero,zero,rof1,rof1);

  delay(tauhn - gt0 - gstab);  /* delay for 1/4JHN coupling */
   
  zgradpulse(gzlvl0,gt0);
  delay(gstab -rof1);

  dec2phase(zero);
   
  rgpulse(pw,three,rof1,0.0);
            
  zgradpulse(gzlvl3,gt3);
  delay(gstab);
   
  dec2rgpulse(pwN,zero,0.0,0.0);

  if (abfilter[0] == 'b')
    {
	
        zgradpulse(gzlvl5*1.2,gt5);
	delay(gstab);

	delay(0.5*taunco - gt5 - gstab); /* 1/8J(NCO) */

        sim3pulse(0.0*pw,0.0*pw180onco,0.0,zero,zero,zero,rof1,rof1);

	delay(0.5*taunco - gt5 - gstab);     /* 1/8J(NCO) */

        zgradpulse(gzlvl5*1.2,gt5);
	dec2phase(t4);
        delay(gstab);
	
        sim3pulse(0.0,pw180onco,2.0*pwN,zero,zero,t4,rof1,rof1);
     
        zgradpulse(gzlvl5*1.2,gt5);
	delay(gstab);

	delay(0.5*taunco - gt5 - gstab);     /* 1/8J(NCO) */

        sim3pulse(0.0*pw,0.0*pw180onco,0.0,zero,zero,zero,rof1,rof1);
       
	delay(0.5*taunco - gt5 - gstab - POWER_DELAY);  /* 1/8J(NCO) */

        decpwrf(rf180offca);   
        zgradpulse(gzlvl5*1.2,gt5);
	delay(gstab);

	dec2phase(one);

	dec2rgpulse(pwN,one,0.0,0.0);  /* purge for 15N */
    }

  if (abfilter[0] == 'a')
    {
        zgradpulse(gzlvl5*1.2,gt5);
	delay(gstab);
	
	delay(0.5*taunco - gt5 - gstab - 0.5*pw180onco);   /* 1/8J(NCO) */

	sim3pulse(0.0*pw,pw180onco,0.0,zero,zero,zero,rof1,rof1); /* 180 CO */
	
	delay(0.5*taunco - gt5 - gstab - 0.5*pw180onco);   /* 1/8J(NCO) */
	
        zgradpulse(gzlvl5*1.2,gt5);
	dec2phase(t4);
	delay(gstab);

        sim3pulse(0.0,0.0,2.0*pwN,zero,zero,t4,rof1,rof1); /* 180 15N */

        zgradpulse(gzlvl5*1.2,gt5);
	delay(gstab);
	
	delay(0.5*taunco - gt5 - gstab - 0.5*pw180onco); /* 1/8J(NCO) */
	
	sim3pulse(0.0*pw,pw180onco,0.0,zero,zero,zero,rof1,rof1);  /* 180 CO */

	delay(0.5*taunco - gt5 - gstab - 0.5*pw180onco - POWER_DELAY); /* 1/8J(NCO) */

        decpwrf(rf180offca);   
        zgradpulse(gzlvl5*1.2,gt5);
	delay(gstab);	

        decphase(t5); 
       	dec2phase(zero);
       
       	dec2rgpulse(pwN,zero,0.0,0.0); /* purge for 15N */
    }

        zgradpulse(gzlvl3*1.3,gt3);
        delay(gstab);

       	dec2rgpulse(pwN,t1,0.0,0.0); /* read for 15N */
   
       delay(tau1);
       
       decshaped_pulse("offC3",pw180offca,t5,0.0,0.0);
       
       delay((1-kappa)*tau1);
   
       zgradpulse(gzlvl1,gt1);
       delay(gstab);

       dec2phase(zero);
       dec2rgpulse(2.0*pwN,zero,0.0,0.0);

       delay(gt1 + gstab + (4.0/PI)*pwN + WFG_START_DELAY + pw180offca - kappa*tau1);

 
   sim3pulse(pw,0.0,0.0,t2,zero,zero,rof1,rof1);  /* Pulse for 1H  */ 
 
   zgradpulse(gzlvl5,gt5);
   delay(gstab);

  delay(tauhn - gt5 - gstab);  /* delay=1/4J (NH) */

  sim3pulse(2.0*pw,0.0,2.0*pwN,zero,zero,zero,rof1,rof1);

  txphase(zero); 
  dec2phase(zero);

  delay(tauhn - gt5 - gstab); /* 1/4J (NH)  */

  zgradpulse(gzlvl5,gt5);
  delay(gstab);

  txphase(one);

  sim3pulse(pw,0.0,pwN,one,zero,zero,rof1,rof1); /* 90 for 1H and 15N */
  obspower(tpwrs); obspwrf(tpwrsf_u);
  shaped_pulse("H2Osinc_u", pwHs, t2, rof1, 0.0);
  obspower(tpwr); obspwrf(4095.0);
   
  dec2phase(zero);

  zgradpulse(gzlvl5*0.8,gt5);
  delay(gstab);

  delay(tauhn -gt5 -gstab -2.0*POWER_DELAY -pwHs -WFG_START_DELAY); /* delay=1/4J (NH) */

  sim3pulse(2.0*pw,0.0,2.0*pwN,zero,zero,zero,rof1,rof1);

  txphase(zero); 
  dec2phase(t3);

  delay(tauhn - gt5 - gstab - 2.0*POWER_DELAY);   /* 1/4J (NH)  */

  zgradpulse(gzlvl5*0.8,gt5);
  delay(gstab);

  dec2rgpulse(pwN,t3,0.0,0.0);

  decpower(dpwr);
  dec2power(dpwr2);
   
  delay((gt1/10.0) + gstab - 2.0*POWER_DELAY);
         
  rgpulse(2.0*pw, zero, 0.0, 0.0);
         
  zgradpulse(gzlvl2*icosel,gt1*0.1);
  delay(gstab);

/* acquire data */

     setreceiver(t7);
}

