/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gcacb_tocsy_cmhm_sqA.c               

    (H)CACB_TOCSY_CmHm utilizing gradients and double sensitivity enhancement.
    Single quantum evolution in t1
    
    Uses optional magic-angle gradients.

    Correlates the sidechain aliphatic 13Ca and 13Cb resonances of a given amino acid
    with methyl groups.

    Set dof to 13CO region.

    Uses isotropic 13C mixing.

    Use enough 13C decoupling bandwidth(140ppm) to minimize any artifacts coming from
    other carbons.

        	  CHOICE OF DECOUPLING AND 2D MODES

    	Set dm = 'nny', dmm = 'ccg' (or 'ccw', or 'ccp') for 13C decoupling.
	Set dm3 = 'nnn' for no 2H decoupling, or
		  'nyn'  and dmm3 = 'w' for 2H decoupling. 
  
    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
    t1 [1H]  and t2 [13C].

    2D experiment in t1: wft2d(1,0,-1,0,0,1,0,1) (sensitivity-enhanced)
    2D experiment in t2: wft2d('ni2',1,0,-1,0,0,1,0,1) (sensitivity-enhanced)
    ( or just use wft2da or wft2da('ni2') after setting
      f1coef='1 0 -1 0 0 1 0 1'
      f2coef='1 0 -1 0 0 1 0 1'
     for 3D just use ft3d )
    
    The flag f1180/f2180 should be set to 'y' if t1/t2 is to be started at
    halfdwell time. This will give 90, -180 phasing in f1/f2. If it is set to
    'n' the phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in 13C and f2180='n' for (0,0) in 15N.  f1180='y' is
    ignored if ni=0, and f2180='y' is ignored if ni2=0.

     If 2H decoupling is used, the 2H lock signal may become unstable because
     of 2H saturation.  Check that a 1D spectrum is stable/reproducible as 
     when 2H decoupling is not used.  You might also check this for d2 and d3
     equal to values achieved at say 75% of their maximum.  Remember to return
     d2=d3=0 before starting a 2D/3D experiment.

     gcacb_tocsy_cmhm_sqA.c is a version in which 
     the chemical shift of 13C is labeled solely under
     13C single quantum coherence during t1, whereas in gcacb_tocsy_cmhmA.c
     it is labeled partly under 13C-1H MQ and 13C SQ coherences. The 
     (SQ) scheme is much less sensitive to miscalibration of 1H pulse width.
     In gcacb_tocsy_cmhmA.c, any miscalibration of 1H pulse leads to spurious
     dispersive signals, possibly DQ/ZQ peaks, around the strong methyl correlations.
       

     Submitted by Perttu Permi, U. Helsinki
     (see J.Biomol.NMR, 30, 275-282 (2004) for details)
*/



#include <standard.h>
#include "Pbox_bio.h" 


static double   ofs, bw, ppm,nst,pws,pwme180,me180pwr,me180pwrf; 
static shape Pdec_154p,me180;


static int   phi1[2]  = {1,3},
             phi2[1]  = {0},
             phi3[1]  = {0},
             phi4[1] = {1},
	     rec[2]   = {0,2};

static double   d2_init=0.0;

pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            mag_flg[MAXSTR],                            /*magic angle gradient*/
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
            codecseq[MAXSTR];       /* sequence for 13C' decoupling */
 
int         icosel1,          			  /* used to get n and p type */
            icosel2,
	    t1_counter,  		        /* used for states tppi in t1 */
	    ni2 = getval("ni2");

double      tau1,         				         /*  t1 delay */
            tau2,        				         /*  t2 delay */
	    del = getval("del"),     /* time delays for CH coupling evolution */
	    del1 = getval("del1"),
	    del2 = getval("del2"),
            del3 = getval("del3"),
            del4 = getval("del4"),
            TC = getval("TC"),
            satpwr = getval("satpwr"),
            waltzB1 = getval("waltzB1"),
            spinlock = getval("spinlock"),
            pwco,copwr, cores,codmf,
            kappa,

	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */
	rf0,            	  /* maximum fine power when using pwC pulses */

/* p_d is used to calculate the isotropic mixing on the Cab region            */
        p_d,                  	       /* 50 degree pulse for DIPSI-2 at rfd  */
        rfd,                    /* fine power for 7 kHz rf for 500MHz magnet  */
	ncyc = getval("ncyc"), 			  /* no. of cycles of DIPSI-3 */


   compC = getval("compC"),         /* adjustment for C13 amplifier compression */


	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

        pwHd,                           /* H1 90 degree pulse length at tpwrd */
        tpwrd,                             /*rf for WALTZ decoupling */

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),

	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
	gzcal = getval("gzcal"),               /* G/cm to DAC coversion factor*/
        gstab = getval("gstab"),
        gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

	gt3 = getval("gt3"),				   /* other gradients */
	gt5 = getval("gt5"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl6 = getval("gzlvl6");

    getstr("mag_flg",mag_flg);
    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("codecseq",codecseq);

/*   LOAD PHASE TABLE    */

	settable(t1,2,phi1);
	settable(t2,1,phi2);
	settable(t3,1,phi3);
	settable(t4,1,phi4);
	settable(t11,2,rec);

        

/*   INITIALIZE VARIABLES   */

    if( dpwrf < 4095 )
	{ printf("reset dpwrf=4095 and recalibrate C13 90 degree pulse");
	  psg_abort(1); }

    /* maximum fine power for pwC pulses */
	rf0 = 4095.0;

   /* dipsi-3 decoupling on CbCa */	
 	p_d = (5.0)/(9.0*4.0*spinlock); /* DIPSI-3*/
 	rfd = (compC*4095.0*pwC*5.0)/(p_d*9.0);
	rfd = (int) (rfd + 0.5);
  	ncyc = (int) (ncyc + 0.5);


   /* power level and pulse time for WALTZ 1H decoupling */
        pwHd = 1/(4.0 * waltzB1) ;    
        tpwrd = tpwr - 20.0*log10(pwHd/(pw));
        tpwrd = (int) (tpwrd + 0.5);

/* activate auto-calibration flags */
setautocal();
  if (autocal[0] == 'n')
  {
    codmf= getval("codmf");
    pwco = 1.0/codmf; /* pw for 13C' decoupling field */
    copwr = getval("copwr"); /* power level for 13C' decoupling */
    cores = getval("cores"); /* power level for 13C' decoupling */
  }
  else        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
  {
    strcpy(codecseq,"Pdec_154p");
    if(FIRST_FID)                                            /* call Pbox */
    {
      ppm = getval("dfrq");
      bw=20.0*ppm; ofs=154*ppm;
      Pdec_154p = pbox_Dsh("Pdec_154p", "WURST2", bw, ofs, compC*pwC, pwClvl);
      bw=30*ppm; ofs=0.0*ppm; nst = 1000; pws = 0.001;
      me180 = pbox_makeA("me180", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
    }

    copwr = Pdec_154p.pwr; pwco = 1.0/Pdec_154p.dmf;
    cores = Pdec_154p.dres;
    pwme180 = me180.pw; me180pwr= me180.pwr; me180pwrf = me180.pwrf;

  }
/* CHECK VALIDITY OF PARAMETER RANGES */

    if( gt1 > 0.5*del - 1.0e-4)
    {
        printf(" gt1 is too big. Make gt1 less than %f.\n", (0.5*del - 1.0e-4));
        psg_abort(1);
    }

    if( dm[A] == 'y' )
    {
        printf("incorrect dec1 decoupler flag! Should be 'nny' or 'nnn' ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[C] == 'y'))
    {
        printf("incorrect dec2 decoupler flags! Should be 'nnn' ");
        psg_abort(1);
    }
    if((dm3[A] == 'y' || dm3[C] == 'y'))
    {
        printf("incorrect dec3 decoupler flags! Should be 'nnn' or 'nyn' ");
        psg_abort(1);
    }

    if( dpwr > 52 )
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

    if( pw > 50.0e-6 )
    {
        printf("dont fry the probe, pw too high ! ");
        psg_abort(1);
    } 
  
    if( pwN > 100.0e-6 )
    {
        printf("dont fry the probe, pwN too high ! ");
        psg_abort(1);
    } 
 

/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    icosel1 = 1; icosel2 = 1;
    if (phase1 == 2) 
	{ tsadd(t2,2,4); icosel1 = -1;}
    if (phase2 == 2) 
	{ tsadd(t4,2,4); icosel2 = -1; tsadd(t2,2,4);}

/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;


/*  Set up f2180  */

    tau2 = d3;
    if((f2180[A] == 'y') && (ni2 > 1.0)) 
	{ tau2 += ( 1.0 / (2.0*sw2) ); if(tau2 < 0.2e-6) tau2 = 0.0; }
    tau2 = tau2/2.0;


/* Calculate modifications to phases for States-TPPI acquisition  */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t1,2,4); tsadd(t11,2,4); }

   
   if(ni > 1)
           kappa = (double)(t1_counter*(del2)) / ( (double) (ni-1) );
      else kappa = 0.0;   

/*   BEGIN PULSE SEQUENCE   */

status(A);

        decoffset(dof-140*dfrq);
	obspower(tpwr);
	decpower(pwClvl);
 	dec2power(pwNlvl);
	decpwrf(rf0);
	obsoffset(tof);
	txphase(zero);
	delay(1.0e-5);

  if (satmode[A] == 'y')
    {
      obspower(satpwr);
      txphase(zero);
      rgpulse(d1,zero,20.0e-6,20.0e-6);
      obspower(tpwr);            /* Set power for hard pulses  */
    }
  else  
    {
      obspower(tpwr); /* Set power for hard pulses  */ 
      delay(d1);
    }

	decrgpulse(pwC, zero, 0.0, 0.0);	   /*destroy C13 magnetization*/
	zgradpulse(gzlvl1, 0.5e-3);
	delay(gstab);
	decrgpulse(pwC, one, 0.0, 0.0);
	zgradpulse(0.7*gzlvl1, 0.5e-3);
	delay(1.1*gstab);

   if(dm3[B] == 'y')				  /*optional 2H decoupling on */
        { 
          dec3unblank();
          dec3rgpulse(1/dmf3, one, 0.0, 0.0); 
          dec3unblank();
          setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
         } 
	rgpulse(pw, zero, 0.0, 0.0);                    /* 1H pulse excitation */

	zgradpulse(gzlvl3, gt3);
        decphase(zero);
	delay(0.5*del - gt3);

	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl3, gt3);
        txphase(one);
        decphase(t1);
	delay(0.5*del - gt3);

        rgpulse(pw,one,0.0,0.0);
        zgradpulse(1.8*gzlvl3, gt3);
        txphase(zero);
        delay(150e-6);
	decrgpulse(pwC, t1, 0.0, 0.0);
        
      /* decoupling on for carbonyl carbon */
         decpwrf(4095.0);
         decpower(copwr);
         decprgon(codecseq,pwco,cores);
         decon();
      /* decoupling on for carbonyl carbon */

        delay(tau1);

        dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
 
        zgradpulse(icosel1*gzlvl4, gt1);

	delay(0.5*del2 - 2.0*pwN - gt1 - 2.0*pw);

        rgpulse(2.0*pw,zero,0.0,0.0);

        delay(tau1 - (kappa*tau1));

      /* co-decoupling off */
         decoff();
         decprgoff();
      /* co-decoupling off */
         decpower(pwClvl);

	decrgpulse(2.0*pwC, zero, 0.0, 0.0);

      /* decoupling on for carbonyl carbon */
         decpwrf(4095.0);
         decpower(copwr);
         decprgon(codecseq,pwco,cores);
         decon();
      /* decoupling on for carbonyl carbon */

	delay(0.5*del2 - kappa*tau1);

      /* co-decoupling off */
         decoff();
         decprgoff();
      /* co-decoupling off */
         decpower(pwClvl);

        decphase(t2);

	decrgpulse(pwC, t2, 0.0, 0.0);

	decpwrf(rfd);
	delay(2.0e-6);
	initval(ncyc, v2);
	starthardloop(v2);
     decrgpulse(4.9*p_d,one,0.0,0.0);
     decrgpulse(7.9*p_d,three,0.0,0.0);
     decrgpulse(5.0*p_d,one,0.0,0.0);
     decrgpulse(5.5*p_d,three,0.0,0.0);
     decrgpulse(0.6*p_d,one,0.0,0.0);
     decrgpulse(4.6*p_d,three,0.0,0.0);
     decrgpulse(7.2*p_d,one,0.0,0.0);
     decrgpulse(4.9*p_d,three,0.0,0.0);
     decrgpulse(7.4*p_d,one,0.0,0.0);
     decrgpulse(6.8*p_d,three,0.0,0.0);
     decrgpulse(7.0*p_d,one,0.0,0.0);
     decrgpulse(5.2*p_d,three,0.0,0.0);
     decrgpulse(5.4*p_d,one,0.0,0.0);
     decrgpulse(0.6*p_d,three,0.0,0.0);
     decrgpulse(4.5*p_d,one,0.0,0.0);
     decrgpulse(7.3*p_d,three,0.0,0.0);
     decrgpulse(5.1*p_d,one,0.0,0.0);
     decrgpulse(7.9*p_d,three,0.0,0.0);

     decrgpulse(4.9*p_d,three,0.0,0.0);
     decrgpulse(7.9*p_d,one,0.0,0.0);
     decrgpulse(5.0*p_d,three,0.0,0.0);
     decrgpulse(5.5*p_d,one,0.0,0.0);
     decrgpulse(0.6*p_d,three,0.0,0.0);
     decrgpulse(4.6*p_d,one,0.0,0.0);
     decrgpulse(7.2*p_d,three,0.0,0.0);
     decrgpulse(4.9*p_d,one,0.0,0.0);
     decrgpulse(7.4*p_d,three,0.0,0.0);
     decrgpulse(6.8*p_d,one,0.0,0.0);
     decrgpulse(7.0*p_d,three,0.0,0.0);
     decrgpulse(5.2*p_d,one,0.0,0.0);
     decrgpulse(5.4*p_d,three,0.0,0.0);
     decrgpulse(0.6*p_d,one,0.0,0.0);
     decrgpulse(4.5*p_d,three,0.0,0.0);
     decrgpulse(7.3*p_d,one,0.0,0.0);
     decrgpulse(5.1*p_d,three,0.0,0.0);
     decrgpulse(7.9*p_d,one,0.0,0.0);

     decrgpulse(4.9*p_d,three,0.0,0.0);
     decrgpulse(7.9*p_d,one,0.0,0.0);
     decrgpulse(5.0*p_d,three,0.0,0.0);
     decrgpulse(5.5*p_d,one,0.0,0.0);
     decrgpulse(0.6*p_d,three,0.0,0.0);
     decrgpulse(4.6*p_d,one,0.0,0.0);
     decrgpulse(7.2*p_d,three,0.0,0.0);
     decrgpulse(4.9*p_d,one,0.0,0.0);
     decrgpulse(7.4*p_d,three,0.0,0.0);
     decrgpulse(6.8*p_d,one,0.0,0.0);
     decrgpulse(7.0*p_d,three,0.0,0.0);
     decrgpulse(5.2*p_d,one,0.0,0.0);
     decrgpulse(5.4*p_d,three,0.0,0.0);
     decrgpulse(0.6*p_d,one,0.0,0.0);
     decrgpulse(4.5*p_d,three,0.0,0.0);
     decrgpulse(7.3*p_d,one,0.0,0.0);
     decrgpulse(5.1*p_d,three,0.0,0.0);
     decrgpulse(7.9*p_d,one,0.0,0.0);

     decrgpulse(4.9*p_d,one,0.0,0.0);
     decrgpulse(7.9*p_d,three,0.0,0.0);
     decrgpulse(5.0*p_d,one,0.0,0.0);
     decrgpulse(5.5*p_d,three,0.0,0.0);
     decrgpulse(0.6*p_d,one,0.0,0.0);
     decrgpulse(4.6*p_d,three,0.0,0.0);
     decrgpulse(7.2*p_d,one,0.0,0.0);
     decrgpulse(4.9*p_d,three,0.0,0.0);
     decrgpulse(7.4*p_d,one,0.0,0.0);
     decrgpulse(6.8*p_d,three,0.0,0.0);
     decrgpulse(7.0*p_d,one,0.0,0.0);
     decrgpulse(5.2*p_d,three,0.0,0.0);
     decrgpulse(5.4*p_d,one,0.0,0.0);
     decrgpulse(0.6*p_d,three,0.0,0.0);
     decrgpulse(4.5*p_d,one,0.0,0.0);
     decrgpulse(7.3*p_d,three,0.0,0.0);
     decrgpulse(5.1*p_d,one,0.0,0.0);
     decrgpulse(7.9*p_d,three,0.0,0.0);
	endhardloop();

        txphase(one);
	decpwrf(rf0);
        decphase(t3);
        obspower(tpwrd);
        decrgpulse(pwC,t3,0.0,0.0);
        decoffset(dof - 155*dfrq);
        rgpulse(pwHd,one,0.0,2.0e-6);
        txphase(zero);
        obsunblank();
        obsprgon("waltz16", pwHd, 90.0);              /* PRG_START_DELAY */
        xmtron();

	delay(TC - OFFSET_DELAY - POWER_DELAY - PRG_START_DELAY - tau2);

	decrgpulse(2.0*pwC, zero, 0.0, 0.0);

        delay(TC + tau2 - POWER_DELAY - PRG_STOP_DELAY - 2*gt1 - gstab - 2.0*pw);

        xmtroff();
        obsprgoff();
        obsblank();
        rgpulse(pwHd,three,2.0e-6,0.0);
        obspower(tpwr);

    if (mag_flg[A] =='y')
        magradpulse(gzcal*icosel2*gzlvl2, gt1);
    else
        zgradpulse(icosel2*gzlvl2, gt1);
        delay(gstab/2.0);
        rgpulse(2.0*pw,zero,0.0,0.0);
    if (mag_flg[A] =='y')
        magradpulse(gzcal*icosel2*gzlvl2, gt1);
    else
        zgradpulse(icosel2*gzlvl2, gt1);
        delay(gstab/2.0);

        decphase(zero);
        simpulse(0.0,pwC, two, zero, 0.0, 0.0);

        zgradpulse(gzlvl5, gt5);
        delay(0.5*del1 - gt5);

        simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

        zgradpulse(gzlvl5, gt5);
        txphase(one);
        decphase(t4);
        delay(0.5*del1 - gt5);

	simpulse(pw, pwC, one, t4, 0.0, 0.0);

	zgradpulse(gzlvl6, gt5);
	txphase(zero);
	decphase(zero);
	delay(0.5*del4 - gt5);

	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl6, gt5);
	delay(0.5*del4 - gt5);

	simpulse(pw,pwC,zero,zero,0.0,0.0);
        zgradpulse(2.3*gzlvl6, gt1);

   if (autocal[A] == 'y')
       {
        decpower(me180pwr); decpwrf(me180pwrf);
	delay(0.5*del3 - gt1 - 0.0005 -2.0*POWER_DELAY- WFG2_START_DELAY);
	simshaped_pulse("","me180",2.0*pw,0.001, zero, zero, 0.0, 0.0);
        decpwrf(rf0);
        decphase(zero);

       }
   else
       {
	delay(0.5*del3 - 0.5*pwC - gt1);
	simpulse(2.0*pw,2.0*pwC, zero, zero, 0.0, 0.0);
       }


   decpower(dpwr);
        if (mag_flg[A] == 'y')
            magradpulse(gzcal*((2.3*gzlvl6)+gzlvl1), gt1);
        else
            zgradpulse(((2.3*gzlvl6)+gzlvl1), gt1);

   if (autocal[A] == 'y')
   {
     if(dm3[B] == 'y')
       delay(0.5*del3 - 0.0005 -gt1 -1/dmf3 - 2.0*GRADIENT_DELAY - 2.0*POWER_DELAY);
        else
       delay(0.5*del3 -  0.0005 -gt1 - 2.0*GRADIENT_DELAY - 2.0*POWER_DELAY);
   }
   else
   {
     if(dm3[B] == 'y') 
      delay(0.5*del3  - gt1 -1/dmf3 - 2.0*GRADIENT_DELAY - POWER_DELAY);
         else
      delay(0.5*del3  - gt1 - 2.0*GRADIENT_DELAY - POWER_DELAY);
   }

   if(dm3[B] == 'y')			         /*optional 2H decoupling off */
        {
          dec3rgpulse(1/dmf3, three, 0.0, 0.0); 
          setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3);
          dec3blank();
        }
 if (dm3[B]=='y') lk_sample();
status(C); 
 setreceiver(t11);
}
