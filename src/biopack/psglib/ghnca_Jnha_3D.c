/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* ghnca_Jnha_3D.c
 
This pulse sequence records an HNCA spectrum with 1JCa(i)-Ha(i)
splitting in F1-dimension and 2JN(i)-Ha(i) splitting in F2-dimension.

Set 13C carrier (dof) at 56ppm.

Set phase=1,2 and phase2=1,2 for 3D-spectrum.
Set dm='nnn', dm2='nnn'
Set f1180='y' and rp1=90 lp1=-180.
Set f2180='y' and rp2=90 lp2=-180.
Set jnca=20

Semi-selective 13C' decoupling is applied during t1 and t2,

Pulse sequence: P. Permi, J. Biomol.NMR, 27, 341-349 (2003).


 
For a 1D check, set ni=1, ni2=1, phase=1, phase2=1 and use wft.

For a 2D experiment:

CH: set ni, phase=1,2 phase2=1 and f1coef='1 0 0 0 0 0 -1 0'.
        Use wft2da. This gives a 13Ca-1HN correlation spectrum.
        The cross peaks are split by 1J(CaHa) and 3JHNHa in
        13C and 1H dimensions, respectively.


NH: set ni2, phase=1, phase2=1,2 and f2coef='1 0 -1 0 0 1 0 1'.
        Use wft2da('ni2'). This gives a 15N-1HN correlation spectrum.


For a 3D experiment:

    Set ni, ni2, phase=1,2 phase2=1,2 f1coef='1 0 0 0 0 0 -1 0' and 
    f2coef='1 0 -1 0 0 0 1 0 1'.
    ft3d gives a three-dimensional 13Ca, 15N, 1HN correlation spectrum 
    displaying 1J(CaHa) doublets in 13Ca dimension. 
    The corresponding 2J(NHa), 3J(NHa) couplings for intraresidual and
    sequential cross peaks can be obtained from 15N dimension (13C-15N 
    plane). 
    3J(HNHa) and 4J(HNHa) couplings can be measured from 1H dimension
    for intraresidual and sequential correlations, respectively.

Written by P. Permi, University of Helsinki
    (3dscthnca_jhan_kaytrosy_ns_pp.c)
Modified for BioPack by G.Gray, Varian, Oct 2004

*/

#include <standard.h>

static double d2_init = 0.0, d3_init = 0.0;

static int   phi1[1]  =  {0},   /* 90 15N prior to t2 */
             phi3[1]  =  {0},   /*  90 for 15N before acq. */
             phi4[2]  =  {0,2}, /* 90 13Ca prior to t1 */
             phi5[4]  =  {0,0,1,1},
             phi6[8]  =  {0,0,0,0,2,2,2,2},
             phi7[4]  =  {0,2,2,0}; /* receiver */ 

void pulsesequence()
{
/* DECLARE VARIABLES */

 char      
 	     f1180[MAXSTR],f2180[MAXSTR];

 int	     t1_counter,t2_counter,icosel,first_FID;

 double      /* DELAYS */	
	    
             tau1,                      /* t1/2 */
             tau2,                      /* t2/2 */  
  
             
	     /* COUPLINGS */ 
             jhn = getval("jhn"), tauhn,
             jnco = getval("jnco"), taunco,
             jcoca = getval("jcoca"), taucoca,
             jnca = getval("jnca"), taunca,
             jhaca = getval("jhaca"), tauhaca,
             jcaha = getval("jcaha"), taucaha,
             jcacb = getval("jcacb"), taucacb,

	     /* PULSES */
             pwN = getval("pwN"),               /* PW90 for N-nuc */
             pwC = getval("pwC"),               /* PW90 for C-nuc */
             pwHs = getval("pwHs"),           /* pw for H2O selective pulse */
   
             /* constants */
             kappa,ni2 = getval("ni2"),
   
	     /* POWER LEVELS */
             satpwr = getval("satpwr"),       /* low power level for presat */
             tpwrs,                 /* power level for selective pulse for water */
             tpwrsf_u = getval("tpwrsf_u"),   /* fine power level for H2O pulse */
             pwClvl = getval("pwClvl"),        /* power level for C hard pulses */ 
             pwNlvl = getval("pwNlvl"),        /* power level for N hard pulses */
             compH  = getval("compH"),         /* amplifier compression         */
             compC  = getval("compC"),         /* amplifier compression         */

	     rf90onca, pw90onca,    /* power level/widthfor Ca 90 hard pulses          */
	     rf180onca,pw180onca,   /* power level/widthfor Ca 180 hard pulses         */
             
   
	     /* GRADIENT DELAYS AND LEVELS */
             gt0 = getval("gt0"),                  /* grad time      */                     
             gt1 = getval("gt1"),                  /* grad time      */                     
             gt3 = getval("gt3"),                  /* grad time      */                     
 	     gt5 = getval("gt5"),                  /* grad time      */               
 	     gstab = getval("gstab"),                  /* grad time      */               
             gzlvl0 = getval("gzlvl0"),               /* level of grad. */
             gzlvl1 = getval("gzlvl1"),
             gzlvl2 = getval("gzlvl2"),
             gzlvl3 = getval("gzlvl3"),
             gzlvl5 = getval("gzlvl5");
   
/* LOAD VARIABLES */

  getstr("f1180",f1180); 
  getstr("f2180",f2180); 


/* check validity of parameter range */

    if ((dm[A] == 'y' || dm[B] == 'y'))
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
  settable(t3, 1, phi3);
  settable(t4, 2, phi4); 
  settable(t5, 4, phi5); 
  settable(t6, 8, phi6); 
  settable(t7, 4, phi7);
   
/* INITIALIZE VARIABLES */

  tauhn =   ((jhn   != 0.0) ? 1/(4*(jhn))   : 2.25e-3);
  taunco =  ((jnco  !=0.0)  ? 1/(4*(jnco))  : 15e-3);
  taucoca = ((jcoca !=0.0)  ? 1/(4*(jcoca)) : 4.5e-3);
  taunca =  ((jnca  !=0.0)  ? 1/(4*(jnca))  : 12e-3);
  tauhaca = ((jhaca !=0.0)  ? 1/(4*(jhaca)) : 12e-3);
  taucaha = ((jcaha !=0.0)  ? 1/(4*(jcaha)) : 12e-3);
  taucacb = ((jcacb !=0.0)  ? 1/(4*(jcacb)) : 12e-3);
   
/* Phase incrementation for hypercomplex data */   

   if (phase2 == 1)      /* Hypercomplex in t2 */
     {
	        icosel = 1;
                tsadd(t3, 2, 4);
     }
   else icosel = -1;
   
   
      if (phase1 == 2)     	/* Hypercomplex in t1 */
        {
          tsadd(t4, 1, 4);
        }
  
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
 *    to achieve States-TPPI acquisition */ 
    
      if (ix == 1)
           d3_init = d3;
         t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5);
   
         if (t2_counter %2)  /* STATES-TPPI */
     {
	        tsadd(t1,2,4);
	        tsadd(t7,2,4);
     }


      /* Calculate correct 13C pulse widths */    

       if((getval("arraydim") < 1.5) || (ix==1))
         first_FID = 1;
       else
         first_FID = 0;

/* selective H20 one-lobe sinc pulse */
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
    tpwrs = (int) (tpwrs);                   	  /*power than a square pulse */
    if (tpwrsf_u<4095.0) tpwrs=tpwrs+6;   /* nominal tpwrsf_u ~ 2048 */
         /* tpwrsf_u can be used to correct for radiation damping  */

    /* 90 degree pulse on Ca, null at CO 118ppm away */  

        pw90onca = sqrt(15.0)/(4.0*118.0*dfrq);
        rf90onca = (4095.0*pwC*compC)/pw90onca;
        rf90onca = (int) (rf90onca + 0.5);
        if(rf90onca > 4095.0)
        {
          if(first_FID)
            printf("insufficient power for pw90onca -> rf90onca (%.0f)\n", rf90onca);
          rf90onca = 4095.0;
          pw90onca = pwC;
        }


    /* 180 degree pulse on CA, null at Co 118ppm away */  

        pw180onca = sqrt(3.0)/(2.0*118.0*dfrq);
        rf180onca = (4095.0*pwC*compC*2.0)/pw180onca;
        rf180onca = (int) (rf180onca + 0.5);
        if(rf180onca > 4095.0)
        {
          if(first_FID)
            printf("insufficient power for pw180onca -> rf180onca (%.0f)\n", rf180onca);
          rf180onca = 4095.0;
          pw180onca = pwC*2.0;
        }

    /* set up so that get (-90,180) phase corrects in F1 if f1180 flag is y */

   tau1 = d2;
   if (f1180[A] == 'y')  tau1 += ( 1.0/(2.0*sw1) - (4.0/PI)*pw90onca - 2.0*pwN - PRG_START_DELAY - PRG_STOP_DELAY - 4.0*POWER_DELAY);
   tau1 = tau1/2.0;
   
    /* set up so that get (-90,180) phase corrects in F2 if f2180 flag is y */
         
      tau2 = d3;
      if(f2180[A] == 'y')  tau2 += ( 1.0/(2.0*sw2) );
      tau2 = tau2/2.0;


    if(ni2 > 1)
           kappa = (double)(t2_counter*(2.0*taunca + pw180onca + PRG_START_DELAY + 2*POWER_DELAY)) / ( (double) (ni2-1) );
      else kappa = 0.0;
   
       if (ix == 1)
         printf("semi-constant time factor %4.6f\n",kappa);


/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);

   obspower(satpwr);            /* Set power for presaturation    	*/
   decpower(pwClvl);             /* Set decoupler1 power to pwClvl	*/
   dec2power(pwNlvl);            /* Set decoupler2 power to pwNlvl 	*/

/* Presaturation Period */

  if (satmode[0] == 'y')
  {
    rgpulse(d1,zero,rof1,0.0);
    obspower(tpwr);                	      /* Set power for hard pulses  */
  }
  else  
  {
    obspower(tpwr);                	      /* Set power for hard pulses  */
    delay(d1);
  }

if (dm3[B] == 'y') lk_hold();

  rcvroff();
  /* eliminate all magnetization originating on 13Ca */

  decpwrf(rf90onca); 
  decrgpulse(pw90onca,zero,0.0,0.0);
   
  zgradpulse(gzlvl0,gt0);
  delay(gstab);

  /* transfer from HN to N by INEPT */

   rgpulse(pw,zero,rof1,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl0*0.9,gt0);
   delay(gstab);

   delay(tauhn - gt0 - gstab); /* 1/4JHN */
   
   sim3pulse(2.0*pw,0.0,2.0*pwN,zero,zero,zero,rof1,rof1);

   delay(tauhn - gt0 - gstab); /* 1/4JHN */

   
   zgradpulse(gzlvl0*0.9,gt0);
   delay(gstab-rof1);

  rgpulse(pw,three,rof1,0.0);
   
         /* shaped pulse for water flip-back */
         obspower(tpwrs); obspwrf(tpwrsf_u);
         shaped_pulse("H2Osinc_u",pwHs,zero,rof1,0.0);
         obspower(tpwr); obspwrf(4095.0);
         /* shaped pulse */
   
   zgradpulse(gzlvl3,gt3);
   delay(gstab);

/* start transfer from N to CO */

   dec2rgpulse(pwN,t1,0.0,0.0);


   /* decoupling on for carbonyl carbon */
         decpwrf(4095.0);
         decpower(dpwr);
         decprgon(dseq,1.0/dmf,dres);
         decon();
   /* decoupling on for carbonyl carbon */

   delay(tau2);

  /* co-decoupling off */
         decoff();
         decprgoff();
  /* co-decoupling off */

   decpower(pwClvl);
   decpwrf(rf180onca);
   decrgpulse(pw180onca,zero,0.0,0.0);

   /* decoupling on for carbonyl carbon */
         decpwrf(4095.0);
         decpower(dpwr);
         decprgon(dseq,1.0/dmf,dres);
         decon();
   /* decoupling on for carbonyl carbon */

   delay((1-kappa)*tau2);

   dec2rgpulse(2.0*pwN,t5,0.0,0.0);

   delay(2.0*taunca + pw180onca + 2.0*POWER_DELAY + 2.0*PRG_START_DELAY - kappa*tau2); /* 1/(4JNCA) */
			

  /* co-decoupling off */
         decoff();
         decprgoff();
  /* co-decoupling off */

   decpower(pwClvl);
   decpwrf(rf90onca);
     
/* t1 evolution period for 13Ca */

 if(dm3[B] == 'y')                         /*optional 2H decoupling on */
   { dec3unblank(); dec3rgpulse(1/dmf3, one, 0.0, 0.0); 
   dec3unblank(); setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);}

   decrgpulse(pw90onca,t4,0.0,0.0);

   /* decoupling on for carbonyl carbon */
         decpwrf(4095.0);
         decpower(dpwr);
         decprgon(dseq,1.0/dmf,dres);
         decon();
   /* decoupling on for carbonyl carbon */

   delay(tau1);
   dec2rgpulse(2.0*pwN,zero,0.0,0.0);   
   delay(tau1);

  /* co-decoupling off */
         decoff();
         decprgoff();
  /* co-decoupling off */

   decpower(pwClvl);
   decpwrf(rf90onca); /* Set decoupler1 power to rf90onca */
   decrgpulse(pw90onca,zero,0.0,0.0); /* 90 for Ca */

   /* decoupling on for carbonyl carbon */
         decpwrf(4095.0);
         decpower(dpwr);
         decprgon(dseq,1.0/dmf,dres);
         decon();
   /* decoupling on for carbonyl carbon */

   if(dm3[B] == 'y')                        /*optional 2H decoupling off */
     {dec3rgpulse(1/dmf3, three, 0.0, 0.0); dec3blank(); 
     setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3); dec3blank();}

if (dm3[B] == 'y')  lk_sample();
   
     delay(2.0*taunca - POWER_DELAY - PRG_STOP_DELAY - gt1 - gstab);

  /* co-decoupling off */
         decoff();
         decprgoff();
  /* co-decoupling off */
   
     decpower(pwClvl);
   zgradpulse(gzlvl1,gt1);
   delay(gstab);

   /* start TROSY transfer from N to HN using PEP-TROSY */
         
      sim3pulse(pw,0.0,pwN,zero,zero,t3,rof1,0.0); 
       
      zgradpulse(gzlvl5,gt5);
      delay(gstab);

      delay(tauhn - gt5 - gstab);   /* 1/4J (HN) */

      sim3pulse(2.0*pw,0.0,2.0*pwN,zero,zero,zero,rof1,rof1);  
   
      delay(tauhn - gt5 - gstab);  /* 1/4J (HN)  */
   
      zgradpulse(gzlvl5,gt5);
      delay(gstab);
      decpwrf(rf90onca);       /* Set decoupler1 power to rf90onca */

      sim3pulse(pw,pw90onca,pwN,one,t6,one,rof1,rof1);  /* 90 for 1H, 13Ca and 15N */
   
      zgradpulse(gzlvl5*0.8,gt5);
      delay(gstab);
   
      delay(tauhn - gt5 - gstab);  /* 1/4J (HN) */
   
      sim3pulse(2.0*pw,0.0,2.0*pwN,zero,zero,zero,rof1,rof1);   /* 180 for 1H and 15N */  
      
      delay(tauhn - gt5 - gstab - POWER_DELAY);  /* 1/4J (HN)  */
   
      zgradpulse(gzlvl5*0.8,gt5);
      delay(gstab -rof1);

      sim3pulse(pw,0.0,pwN,zero,zero,zero,rof1,0.0);  /* 90 for 15N */
   
      dec2power(dpwr2);
      decpower(dpwr);
       
      delay((gt1/10.0) + gstab - pwN - rof1 - 2.0*POWER_DELAY);
   
      rgpulse(2.0*pw, zero, rof1, rof1);
   
      zgradpulse(gzlvl2*icosel,gt1/10.0);
      delay(gstab);

   /* acquire data */
   
status(C);
       setreceiver(t7);
}
