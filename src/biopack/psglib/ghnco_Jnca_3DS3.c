/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* ghnco_Jnca_3DS3.c 

This pulse sequence records 1J(NCa), 2J(NCa), 2J(HNCa) and 3J(HNCa) 
couplings in a 1H-15N-13C' correlation spectrum. 
The spin-state-selection (referred here as S3) is provided along 
with the TROSY selection.

Set lambda to reasonable value to scale up NCa couplings in t2.
 
Set 13C carrier (dof) on CO-region. Apply Ca off-resonance.

Set phase=1,2 phase2=1,2,3,4 for S3-edited 3D-experiment.

Set jcoca=53 and jhn=93.


Pulse sequence: P. Permi, P. R. Rosevear and A. Annila, 
		J. Biomol. NMR, 17, 43-54 (2000).
                 (submitted as 3dabhnco_jnca_trosy.c)

		P. Permi, "Applications for measuring scalar and residual
		dipolar couplings in proteins",
		Acta Universitatis Ouluensis, A354, (2000).
		http://herkules.oulu.fi/isbn9514258223/


For a 1D check, set ni=1, ni2=1, phase=1, phase2=1 and use wft.

For a 2D experiment:

CH: set ni, phase=1,2 phase2=1 and f1coef='1 0 0 0 0 0 -1 0'.
        Use wft2da. This gives a 13CO-1HN correlation spectrum.

NH: set ni2, phase=1, phase2=1,2 and f2coef='1 0 -1 0 0 1 0 1'.
        Use wft2da('ni2'). This gives a 15N-1HN correlation spectrum.
        The cross peaks are split by 1J(NCa) and 2J(NCa), and 2J(HNCa) 
        and 3J(HNCa) couplings in 15N and 1H dimensions, respectively.

        The coefficient lambda defines scaling factor for 1J(NCa) and 
        2J(NCa), if set to 1, the 1(JNCa) and 2J(NCa) are scaled with 
        respect 15N shift by a factor of two. If lambda=0, the 1J(NCa) 
        and 2J(NCa) couplings are not scaled.

For a 2D S3 experiment

NH: set ni2, phase2=1,2,3,4 and phase=1.
        Use wft2d('ni2',1,0,-1,0,1,0,-1,0,0,-1,0,-1,0,-1,0,-1) for 
        adding data. 
        This shows the upfield component of 2J(NCa) doublet, which 
        is further split by 1J(NCa). The corresponding 3J(HNCa) and 
        2J(HNCa) couplings can be measured from 1H dimension
         
        Use wft2d('ni2',1,0,-1,0,-1,0,1,0,0,-1,0,-1,0,1,0,1) for 
        subtracting data.
        This shows the downfield component of 2J(NCa) doublet, which 
        is further split by 1J(NCa). The corresponding 3J(HNCa) and 
        2J(HNCa) couplings can be measured from 1H dimension

For a 3D S3 experiment:

        Set ni, ni2, phase=1,2 phase2=1,2,3,4 and f1coef='1 0 0 0 0 0 -1 0'. 
        
        f2coef='1 0 -1 0 1 0 -1 0 0 1 0 1 0 1 0 1' followed by ft3d gives 
        a three-dimensional 13CO, 15N, 1HN correlation spectrum displaying
        the upfield component of 2J(NCa) doublet (further split by 1J(NCa)
        in 15N dimension. The corresponding 3J(HNCa) and 2J(HNCa) couplings
        can be measured from 1H dimension.

        f2coef='1 0 -1 0 -1 0 1 0 0 1 0 1 0 -1 0 -1' followed by ft3d gives
        a three-dimensional 13CO, 15N, 1HN correlation spectrum displaying
        the downfield component of 2J(NCa) doublet (further split by 1J(NCa)
        in 15N dimension. The corresponding 3J(HNCa) and 2J(HNCa) couplings
        can be measured from 1H dimension.


Written by P. Permi, University of Helsinki
Modified for BioPack by G.Gray, Varian, Sept 2004
Signs of last 4 non-zero values of f2coef changed for proper F2 dimension
direction (P.Permi, April 2006).

*/

#include <standard.h>

static double d2_init = 0.0, d3_init = 0.0;

static int   phi1[1] = {1},  /* 90 for 15N prior to t1 */
             phi2[1] = {0},  /* first 90 for 1H in TROSY scheme */
             phi3[1] = {1},  /*  90 for 15N before acq. */
             phi4[2] = {0,2},  /* 90 13C' prior to t1 */
	     phi5[4] = {0,0,1,1},  /* 180 on 13C' after t1 to ensure flat baseline */
             phi6[8] = {0,0,0,0,2,2,2,2}, /* purge pulse on 13CO */
             phi7[4] = {0,2,2,0};  /* receiver */
             
pulsesequence()
{
/* DECLARE VARIABLES */

 char      
 	     f1180[MAXSTR],f2180[MAXSTR],satmode[MAXSTR];

 int	     t1_counter,t2_counter,icosel,first_FID;

 double      /* DELAYS */
             tau1,                      /* t1/2 */
             tau2,                      /* t2/2 */


	     /* COUPLINGS */ 
             jhn = getval("jhn"),tauhn,
             jnco = getval("jnco"),taunco,
             jcoca = getval("jcoca"),taucoca,
             jnca = getval("jnca"),taunca,
             jhaca = getval("jhaca"),tauhaca,
             jcaha = getval("jcaha"),taucaha,
             jcacb = getval("jcacb"),taucacb,
   
	     /* PULSES */
             pwN = getval("pwN"),               /* PW90 for N-nuc */
             pwC = getval("pwC"),               /* PW90 for C-nuc */
             pwHs = getval("pwHs"),           /* pw for selective water pulse at twprsl */

	     /* POWER LEVELS */
             satpwr = getval("satpwr"),       /* low power level for presat */
             tpwrsf_d = getval("tpwrsf_d"),   /* fine power level for flipback */
             tpwrsf_u = getval("tpwrsf_u"),   /* fine power level for flipback */
             tpwrs,ni2,                 /* power level for selective pulse for water */
             pwClvl = getval("pwClvl"),         /* power level for C hard pulses */ 
             compC = getval("compC"),           /* amplifier compression factor  */ 
             compH = getval("compH"),           /* amplifier compression factor  */ 
             pwNlvl = getval("pwNlvl"),         /* power level for N hard pulses */
             rf90onco,pw90onco,                 /* power/width for CO 90 pulses */ 
             rf180onco,pw180onco,               /* power/width for CO 180 pulses */
             rf180offca,pw180offca,             /* power/width for off-res Ca 180 pulses */

             /* CONSTANTS */
             sw1 = getval("sw1"),
             sw2 = getval("sw2"),
             kappa,                             /* semi constant-time factor */
   	     lambda = getval("lambda"),         /* scaling factor for JNCa */ 

	     /* GRADIENT DELAYS AND LEVELS */
             gt0 = getval("gt0"), /* gradient time */
             gt1 = getval("gt1"), /* gradient time */
             gt3 = getval("gt3"), /* gradient time */
             gt5 = getval("gt5"),
             gstab = getval("gstab"),
             gzlvl0 = getval("gzlvl0"), /* level of gradient */
             gzlvl1 = getval("gzlvl1"),
             gzlvl2 = getval("gzlvl2"),
             gzlvl3 = getval("gzlvl3"),
             gzlvl5 = getval("gzlvl5");


/* selective H20 one-lobe sinc pulse */
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
    tpwrs = (int) (tpwrs);                   	  /*power than a square pulse */
    if (tpwrsf_d<4095.0) tpwrs=tpwrs+6;   /* nominal tpwrsf_d ~ 2048 */
         /* tpwrsf_d,tpwrsf_u can be used to correct for radiation damping  */

/* LOAD VARIABLES */
  ni = getval("ni");
  ni2 = getval("ni2");
  getstr("satmode",satmode); 
  getstr("f1180",f1180);
  getstr("f2180",f2180);

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

/* INITIALIZE VARIABLES AND POWER LEVELS FOR PULSES */

   tauhn = ((jhn != 0.0) ? 1/(4*(jhn)) : 2.25e-3);
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
   
     kappa=(taunco - tauhn - gt1 - gstab)/((double) ni2/sw2)-0.001;
      if (kappa > 1.0) 
     {  
	              kappa=1.0-0.01;
     }   

       if (( phase2 == 1) || (phase2 == 3)) /* Hypercomplex in t2 */
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


status(B);
  if (satmode[0] == 'y')
    {
      rgpulse(d1,zero,rof1,0.0);
      obspower(tpwr);            /* Set power for hard pulses  */
    }
  else  
    {
      obspower(tpwr);                 /* Set power for hard pulses  */
      delay(d1);
    }


     rcvroff();
     /* eliminate all magnetization originating on 13C */
    
     decrgpulse(pw90onco,zero,rof1,rof1);
      
     zgradpulse(gzlvl0,gt0);
     delay(gstab);
   
  /* transfer from HN to N by INEPT */
   
  /* shaped pulse for water flip-back */
     obspower(tpwrs); obspwrf(tpwrsf_d);
     shaped_pulse("H2Osinc_d",pwHs,one,rof1,0.0);
     obspower(tpwr); obspwrf(4095.0);
  /* shaped pulse */

  rgpulse(pw,zero,rof1,0.0); /* 90 for 1H */

  zgradpulse(gzlvl0*1.1,gt0);
  delay(gstab);
   
  delay(tauhn - gt0 - gstab);        /* 1/(4JHN) */
   
  sim3pulse(2.0*pw,0.0,2.0*pwN,zero,zero,zero,rof1,rof1);  /* 180 for 1H and 15N */

  zgradpulse(gzlvl0*1.1,gt0);
  delay(gstab);
   
  delay(tauhn - gt0 - gstab);        /* 1/(4JHN) */

  rgpulse(pw,three,rof1,0.0);
      
  zgradpulse(gzlvl3*1.5,gt3);
  delay(gstab);
   
/* start transfer from N to CO */
      
  dec2rgpulse(pwN,zero,0.0,0.0);
    
  delay(taunco - gt0 - gstab - POWER_DELAY - 0.5*pw180onco);	 /* 1/(4JNCO) */
	  
  decpwrf(rf180onco);          /* Set decoupler1 power to rf180onco */

  zgradpulse(gzlvl0*1.3,gt0);
  delay(gstab);

  sim3pulse(0.0,pw180onco,2.0*pwN,zero,zero,zero,rof1,rof1);  /* 180 for 13C' and 15N */

  zgradpulse(gzlvl0*1.3,gt0);
  delay(gstab);

  decpwrf(rf90onco);            /* Set decoupler1 power to rf90onco */

  delay(taunco - gt0 - gstab - POWER_DELAY - 0.5*pw180onco);   /* 1/(4JNCO) */

  dec2rgpulse(pwN,one,0.0,0.0);  /* 90 for 15N */
   
  zgradpulse(gzlvl3*1.3,gt3);
  delay(gstab);
   
/* record 13C' shift */

     decrgpulse(pw90onco,t4,0.0,0.0);  /* 90 for 13C' */
   
     delay(tau1);
   
     decpwrf(rf180offca);      
     sim3shaped_pulse("","offC3","",0.0,pw180offca,2.0*pwN,zero,zero,zero,0.0,0.0); /* 180 for 13CA and 15N */
   
     delay(tau1);
   
     decpwrf(rf180onco);
     decrgpulse(pw180onco,t5,0.0,0.0);

     decpwrf(rf180offca);
     delay(pwN - 0.5*pw180offca);
     sim3shaped_pulse("","offC3","",0.0,pw180offca,0.0,zero,zero,zero,0.0,0.0); /* 180 for 13CA */
     delay(pwN - 0.5*pw180offca);
   
     decphase(zero);
     decpwrf(rf90onco); /* Set decoupler1 power to rf90onco */
     decrgpulse(pw90onco,zero,0.0,0.0);  /* 90 for 13C' */
   
     zgradpulse(gzlvl3*1.1,gt3);
     delay(gstab);

      if ( (phase2 == 3) || (phase2 == 4))
     {
	  decphase(zero);
	  decrgpulse(pw90onco,zero,0.0,0.0);  /* 90 for 13C' */
	
	  decpwrf(rf180offca);    /* Set decoupler1 power to rf180offca */
	  decshaped_pulse("offC3",pw180offca,zero,0.0,0.0); 
	   
          decpwrf(rf180onco);           /* Set decoupler1 power to rf180onco */

	  delay(taucoca - gt0 - gstab - 0.5*pw180onco - pw180offca - 2.0*POWER_DELAY);  /* 1/(4JCOCA) */

          zgradpulse(gzlvl0*0.9,gt0);
          delay(gstab);
	
          sim3pulse(0.0,pw180onco,0.0,zero,zero,zero,rof1,rof1);  /* 180 13C' */
  	  decpwrf(rf180offca);              	     /* Set decoupler1 power to rf180offca */
	  decshaped_pulse("offC3",pw180offca,zero,0.0,0.0); 
    
          zgradpulse(gzlvl0*0.9,gt0);
          delay(gstab);

	  delay(taucoca - gt0 - gstab - 0.5*pw180onco - pw180offca - 2.0*POWER_DELAY);   /* 1/(4JCOCA) */

          decpwrf(rf90onco);    /* Set decoupler1 power to rf90onco */

          decrgpulse(pw90onco,one,0.0,0.0); /* 90 for 13C' */

          /* Field crusher gradient */
          zgradpulse(gzlvl3*0.5,gt0);
          delay(gstab);
	  /* Field crusher gradient */

        }

    if (( phase2 == 1) || (phase2 == 2))
       {
	  
	decphase(zero);
	decrgpulse(pw90onco,zero,0.0,0.0);  /* 90 for 13C' */
	  
        zgradpulse(gzlvl0*0.9,gt0);
        delay(gstab);
	
        decpwrf(rf180offca);   /* Set decoupler1 power to rf180offca */

	delay(0.5*taucoca - gt0 - gstab - POWER_DELAY - 0.5*pw180offca); /* 1/(8JCOCA) */
	 
	decshaped_pulse("offC3",pw180offca,zero,0.0,0.0); 

	delay(0.5*taucoca - gt0 - gstab - POWER_DELAY - 0.5*pw180offca - 0.5*pw180onco); /* 1/(8JCOCA) */
	
        decpwrf(rf180onco);   /* Set decoupler1 power to rf180onco */

        zgradpulse(gzlvl0*0.9,gt0);
        delay(gstab);

        sim3pulse(0.0,pw180onco,0.0,zero,zero,zero,rof1,rof1);   /* 180 13C' */

        zgradpulse(gzlvl0*0.9,gt0);
        delay(gstab);
	
	decpwrf(rf180offca);      

	delay(0.5*taucoca - gt0 - gstab - POWER_DELAY - 0.5*pw180onco - 0.5*pw180offca); /* 1/(8JCOCA) */
	      
        decshaped_pulse("offC3",pw180offca,zero,0.0,0.0);
	 
	delay(0.5*taucoca - gt0 - gstab - POWER_DELAY - 0.5*pw180offca); /* 1/(8JCOCA) */

        decpwrf(rf90onco);         /* Set decoupler1 power to rf90onco */

        zgradpulse(gzlvl0*0.9,gt0);
        delay(gstab);

        decrgpulse(pw90onco,zero,0.0,0.0);  /* 90 for 13C' */

        /* Field crusher gradient */
           zgradpulse(gzlvl3*0.5,gt3);
           delay(gstab);
	/* Field crusher gradient */

      }  
 
     dec2phase(t1);
     dec2rgpulse(pwN,t1,0.0,0.0); /* 90 15N */

   if (lambda>0.0)
     {
     delay(lambda*tau2);  /* (lambda)t2/2 */
   
     decpwrf(rf180offca);
     sim3shaped_pulse("","offC3","",0.0,pw180offca,2.0*pwN,zero,zero,zero,rof1,rof1);

     delay(lambda*tau2);  /* (lambda)t2/2 */
     }
     delay(tau2);
     delay(taunco - tauhn - pw180onco);   

     decpwrf(rf180onco);
     decrgpulse(pw180onco,zero,0.0,0.0);     /* 180 for 13C' */

     delay((1-kappa)*tau2);                  /* (1-k)t2/2 */

     dec2rgpulse(2.0*pwN,zero,0.0,0.0);     /* 180 for 15N */
   
     delay((taunco - tauhn - gt1 - gstab) - kappa*tau2);  /* 1/(4JNCO) - kt2/2 */
          
     zgradpulse(gzlvl1,gt1);
     delay(gstab);
   
   /* start TROSY transfer from N to HN */
      
     sim3pulse(pw,0.0,0.0,t2,zero,zero,rof1,0.0); 
          
     zgradpulse(gzlvl5,gt5);
     delay(gstab);
 
     delay(tauhn - gt5 - gstab - POWER_DELAY);
   
      
     decpwrf(rf180onco);
     sim3pulse(2.0*pw,pw180onco,2*pwN,zero,zero,zero,rof1,rof1);
   
     delay(tauhn - gt5 - gstab - POWER_DELAY);
   
      
     zgradpulse(gzlvl5,gt5);
     delay(gstab);
   
     decpwrf(rf90onco);
     sim3pulse(pw,pw90onco,pwN,one,t6,zero,rof1,0.0);
   
  /* shaped pulse WATERGATE */
         obspower(tpwrs); obspwrf(tpwrsf_u);
         shaped_pulse("H2Osinc_u",pwHs,t2,rof1,0.0);
         obspower(tpwr); obspwrf(4095.0);
  /* shaped pulse */
   
     zgradpulse(gzlvl5,gt5);
     delay(gstab);
      
     delay(tauhn - gt5 - gstab - 2.0*POWER_DELAY - pwHs);
   
     sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,rof1,rof1);
   
      
     delay(tauhn - gt5 - gstab - 2.0*POWER_DELAY);
   
     decpower(dpwr);
      
     zgradpulse(gzlvl5,gt5);
     delay(gstab);
      
     dec2rgpulse(pwN,t3,0.0,0.0);  /* 90 for 15N */
      
     dec2power(dpwr2);
      
     delay((gt1/10.0) -pwN + gstab - POWER_DELAY);
      
     rgpulse(2.0*pw, zero, rof1, rof1);
      
     zgradpulse(gzlvl2*icosel,gt1/10.0);
     delay(gstab);
      
  /* acquire data */
      
   status(C);
     setreceiver(t7);
}
