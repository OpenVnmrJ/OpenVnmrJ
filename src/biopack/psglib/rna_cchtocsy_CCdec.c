/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* rna_cchtocsy_CCdec

    3D (H)CCH-TOCSY utilizing SE or watergate.

    Correlates ribose 13C resonances of a given ribonucleotide.
    Uses isotropic 13C mixing

    added C2' 13C homonuclear decoupling during 
    t2 evolution period to help with resolution (KD) 

    mixing time = 25-30ms (35 ppm rf field)
          Set sw=5ppm.
          Set sw1=36ppm.
          Set sw2=18ppm
Pulse Sequence: Dayie, JBNMR, 2005, 32:129-39.
    original sequence:  rna_cchtocsyktd_dec2.c
    revised for BioPack, GG, Varian, 1/2008

*/

#include <standard.h>

static int  phi1[2] = {1,3},
	    phi3[8] = {0,0,0,0,2,2,2,2},
	    phi5[4] = {0,0,2,2},
	    phi4[16]= {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
	    phi9[1] = {0},
	    phi10[1] = {1},
	    rec1[8]  = {0,2,2,0, 2,0,0,2};

static double   d2_init=0.0, d3_init=0.0;


pulsesequence()
{

/* DECLARE VARIABLES */

char
        SE[MAXSTR],          /* coherence gradients & sensitivity enhance */
        CT[MAXSTR],                                /* constant time in t1 */
        CCdseq[MAXSTR],
        CChomodec[MAXSTR],          /* Setup for C-imino - C-H6 */

	C13refoc[MAXSTR],                         /* C13 pulse in middle of t1*/
	f1180[MAXSTR],                        /* Flag to start t1 @ halfdwell */
	f2180[MAXSTR];                        /* Flag to start t1 @ halfdwell */

int	icosel,
        ni2 = getval("ni2"),
	t1_counter,
	t2_counter;

double  tau1,                                                /*  t1 delay */
        tau2,                                                /*  t2 delay */
	lambda = 0.94/(4.0*getval("JCH")),        /* 1/4J C-H INEPT delay */
        CTdelay = getval("CTdelay"),     /* total constant time evolution */

        CCdpwr = getval("CCdpwr"),    /*   power level for CC decoupling */
        CCdres = getval("CCdres"),    /*   dres for CC decoupling */
        CCdmf = getval("CCdmf"),      /*   dmf for CC decoupling */

        pwClvl = getval("pwClvl"),              /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */
	compC = getval("compC"),  /* adjustment for C13 amplifier compression */

        pwNlvl = getval("pwNlvl"),                    /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
        pwZa,                                   /* the largest of 2.0*pw and 2.0
*pwN */

        tpwr = getval("tpwr"),    	               /* power for H1 pulses */
        pw = getval("pw"),               /* H1 90 degree pulse length at tpwr */
	compH = getval("compH"),   /* adjustment for H1 amplifier compression */

	ncyc_cc = getval("ncyc_cc"), /* number of DIPSI3 cycles for CC spinlock */

        tof_75,                  /* tof shifted to 7.5 ppm for H4-N4 transfer */
        tof_12,                   /* tof shifted to 12 ppm for H3-N3 transfer */

	dof_80,		 /* dof shifted to 169 ppm for N3-C4 transfer */
	dof_92p5,		 /* dof shifted to 92.5ppm */

/* p_d is used to calculate the isotropic mixing */
        p_d,                 /* 50 degree pulse for DIPSI-3 at rfdC */
        p_d2,                /* 50 degree pulse for DIPSI-3 at rfd */
        rfd,             /* fine C13 power for 10 kHz rf for 500MHz magnet */

	sw1 = getval("sw1"),
        sw2 = getval("sw2"),
        gstab = getval("gstab"),

        pwHs = getval("pwHs"),         /* H1 90 degree pulse length at tpwrs */
        tpwrs,                   /* power for the pwHs ("rna_H2Osinc") pulse */

        pwHs2 = getval("pwHs2"),       /* H1 90 degree pulse length at tpwrs2 */
        tpwrs2,                           /* power for the pwHs2 square pulse */

        gt1 = getval("gt1"),                   /* coherence pathway gradients */
        gzlvl1 = getval("gzlvl1"),
        gzlvl2 = getval("gzlvl2"),

  gzlvl0 = getval("gzlvl0"),
  gzlvl3 = getval("gzlvl3"),
  gt3 = getval("gt3"),
  gzlvl4 = getval("gzlvl4"),
  gt4 = getval("gt4"),
  gzlvl5 = getval("gzlvl5"),
  gzlvl6 = getval("gzlvl6"),
  gt5 = getval("gt5"),
  gzlvlr = getval("gzlvlr");

    getstr("SE",SE);
    getstr("CT",CT);

  getstr("CChomodec",CChomodec);

  getstr("CCdseq",CCdseq);

  getstr("C13refoc",C13refoc);
  getstr("f1180",f1180);
  getstr("f2180",f2180);


/* LOAD PHASE TABLE */
 
	settable(t1,2,phi1);
	settable(t3,8,phi3);
	settable(t4,16,phi4);
	settable(t9,1,phi9);
	settable(t10,1,phi10);
	settable(t5,4,phi5);
	settable(t11,8,rec1);


/* INITIALIZE VARIABLES */

/* different offset values tof=H2O, dof=110ppm, dof2=200ppm */

	tof_75 = tof + 2.5*sfrq;        /* tof shifted to nH2 */
	tof_12 = tof + 8.0*sfrq;	/* tof shifted to nH */
	dof_92p5 = dof - 17.5*dfrq;	/* dof shifted to C1' */
	dof_80 = dof - 30*dfrq;	        /* dof shifted to C6 */

/* 1.9 kHz DIPSI-3 at 500MHz scaled to this sfrq*/
        p_d = (5.0)/(9.0*4.0*1900.0*(sfrq/500.0));

/* 7 kHz DIPSI-3 at 500MHz scaled to this sfrq*/
        p_d2 = (5.0)/(9.0*4.0*7000.0*(sfrq/500.0));
        ncyc_cc = (int) (ncyc_cc + 0.5);
        if (ncyc_cc > 0 )
         {
           printf("CC-mixing time is %f ms.\n",(ncyc_cc*51.8*4*p_d2));
         }
        if( ncyc_cc > 12 )
         {
           text_error("check C->C dipsi-3 time !! ");
           psg_abort(1);
         }
        initval(ncyc_cc,v2);

/* fine C13 power for dipsi-3 isotropic mixing */
        rfd = (compC*4095.0*pwC*5.0)/(p_d2*9.0);
        rfd = (int) (rfd + 0.5);

/* selective H20 one-lobe sinc pulse */
        tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /* needs 1.69 times more */
        tpwrs = (int) (tpwrs);                   /* power than a square pulse */

/* selective H20 square pulse */
        tpwrs2 = tpwr - 20.0*log10(pwHs2/(compH*pw));
        tpwrs2 = (int) (tpwrs2);

  if (2.0*pw > 2.0*pwN) pwZa = 2.0*pw;
  else pwZa = 2.0*pwN;

  if ((CT[A]=='y') && (ni2/(4.0*sw2) > CTdelay))
  { text_error( " ni2 is too big. Make ni2 equal to %d or less.\n",
      ((int)(CTdelay*sw2*4.0)) );                                       psg_abort(1); }




/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2)
         tsadd(t5,1,4);

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2)
        { tsadd(t5,2,4); tsadd(t11,2,4); }

/*  Set up f1180  */

    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0))
        { tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;


        icosel=1;

  if (SE[A]=='y')
   {
        if (phase2 == 2)
        {
                tsadd(t10,2,4);
                icosel = -1;
        }
   }

/*  Set up f2180  */
       tau2 = d3;
  if((f2180[A] == 'y') && (ni2 > 1.0))
   {
        tau2 += ( 1.0 / (2.0*sw2) );
        if(tau2 < 0.2e-6) tau2 = 0.0;
   }

        tau2 = tau2/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */
  if( ix == 1) d3_init = d3;

        t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );

  if(t2_counter % 2)
   {
        tsadd(t3,2,4);
        tsadd(t11,2,4);
   }


/* CHECK VALIDITY OF PARAMETER RANGE */


    if( sfrq > 610 )
        { printf("Power Levels at 750/800 MHz may be too high for probe");
          psg_abort(1); }

    if( dpwrf < 4095 )
        { printf("reset dpwrf=4095 and recalibrate C13 90 degree pulse");
          psg_abort(1); }

    if( dpwrf2 < 4095 )
        { printf("reset dpwrf2=4095 and recalibrate N15 90 degree pulse");
          psg_abort(1); }

    if((dm[A] == 'y' || dm[B] == 'y'))
    {
        printf("incorrect dec1 decoupler flag! Should be 'nny' or 'nnn' ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y'))
    {
        printf("incorrect dec2 decoupler flags! Should be 'nnn' or 'nny' ");
        psg_abort(1);
    }

    if( ((dm[C] == 'y') && (dm2[C] == 'y') && (at > 0.18)) )
    {
        text_error("check at time! Don't fry probe !! ");
        psg_abort(1);
    }

    if( dpwr > 50 )
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

   if( dpwr2 > 50 )
    {
        printf("don't fry the probe, DPWR2 too large!  ");
        psg_abort(1);
    }

    if( pw > 20.0e-6 )
    {
        printf("dont fry the probe, pw too high ! ");
        psg_abort(1);
    }

    if( pwC > 40.0e-6 )
    {
        printf("dont fry the probe, pwC too high ! ");
        psg_abort(1);
    }

    if( pwN > 100.0e-6 )
    {
        printf("dont fry the probe, pwN too high ! ");
        psg_abort(1);
    }

    if (gzlvlr > 500 || gzlvlr < -500)
    {
	text_error(" RDt1-gzlvlr must be -500 to 500 (0.5G/cm) \n");
	psg_abort(1);
    }

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);

        obspower(tpwr);
	obsstepsize(0.5);
        decpower(pwClvl);
	decstepsize(0.5);
	obsoffset(tof);
        dec2power(pwNlvl);
	dec2stepsize(0.5);

        decoffset(dof_80);	/* Preset the carbon frequency for the C1' carbons */

        txphase(zero);
        decphase(zero);
        dec2phase(zero);

        delay(d1);
        rcvroff();

        dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 and C13 magnetization*/
        decrgpulse(pwC, zero, 0.0, 0.0);
        zgradpulse(gzlvl0, 0.5e-3);
        delay(1.0e-4);
        dec2rgpulse(pwN, one, 0.0, 0.0);
        decrgpulse(pwC, one, 0.0, 0.0);
        zgradpulse(0.7*gzlvl0, 0.5e-3);
        delay(5.0e-4);

	delay(lambda);

	simpulse(2*pw, 2*pwC, zero, zero, 0.0, 0.0);
	dec2phase(t5);

	delay(lambda - SAPS_DELAY);

	simpulse(pw, pwC, zero, t5, 0.0, 0.0); /* 2x, -2x*/
	dec2phase(zero);
        txphase(one);

	zgradpulse(gzlvl5,gt5);
	delay(lambda - SAPS_DELAY - gt5);

	simpulse(2*pw, 2*pwC, one, zero, 0.0, 0.0);

	zgradpulse(gzlvl5,gt5);
        delay(lambda - 2*SAPS_DELAY - gt5 - 2*POWER_DELAY);


	decpwrf(4095.0);


	txphase(zero);
	decphase(zero);

  if (C13refoc[A]=='y')
   {

        if (tau1 > (2.0*GRADIENT_DELAY + pwN + 0.64*pw + 5.0*SAPS_DELAY))
        {
        zgradpulse(gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
        delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw) - SAPS_DELAY);
        sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
        zgradpulse(-1.0*gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
        delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
        }
        else if (tau1 > (0.64*pw + 0.5*SAPS_DELAY))
        delay(2.0*tau1 - 2.0*0.64*pw - SAPS_DELAY );
   }
  else
   {
        if (tau1 > (2.0*GRADIENT_DELAY + pwN + 0.64*pw + 5.0*SAPS_DELAY))
        {
        zgradpulse(gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
        delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw) - SAPS_DELAY);
        dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
        zgradpulse(-1.0*gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
        delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
        }
        else if (tau1 > (0.64*pw + 0.5*SAPS_DELAY))
        delay(2.0*tau1 - 2.0*0.64*pw - SAPS_DELAY );
   }


	decrgpulse(pwC,three,0.0,0.0);   /* flip transferred 13C-magn. to z */
	decrgpulse(pwC,one,0.0,0.0);   /* flip transferred 13C-magn. to z */

        decphase(zero);
        decpwrf(rfd);

        starthardloop(v2);
    decrgpulse(6.4*p_d2,zero,0.0,0.0);
    decrgpulse(8.2*p_d2,two,0.0,0.0);
    decrgpulse(5.8*p_d2,zero,0.0,0.0);
    decrgpulse(5.7*p_d2,two,0.0,0.0);
    decrgpulse(0.6*p_d2,zero,0.0,0.0);
    decrgpulse(4.9*p_d2,two,0.0,0.0);
    decrgpulse(7.5*p_d2,zero,0.0,0.0);
    decrgpulse(5.3*p_d2,two,0.0,0.0);
    decrgpulse(7.4*p_d2,zero,0.0,0.0);

    decrgpulse(6.4*p_d2,two,0.0,0.0);
    decrgpulse(8.2*p_d2,zero,0.0,0.0);
    decrgpulse(5.8*p_d2,two,0.0,0.0);
    decrgpulse(5.7*p_d2,zero,0.0,0.0);
    decrgpulse(0.6*p_d2,two,0.0,0.0);
    decrgpulse(4.9*p_d2,zero,0.0,0.0);
    decrgpulse(7.5*p_d2,two,0.0,0.0);
    decrgpulse(5.3*p_d2,zero,0.0,0.0);
    decrgpulse(7.4*p_d2,two,0.0,0.0);

    decrgpulse(6.4*p_d2,two,0.0,0.0);
    decrgpulse(8.2*p_d2,zero,0.0,0.0);
    decrgpulse(5.8*p_d2,two,0.0,0.0);
    decrgpulse(5.7*p_d2,zero,0.0,0.0);
    decrgpulse(0.6*p_d2,two,0.0,0.0);
    decrgpulse(4.9*p_d2,zero,0.0,0.0);
    decrgpulse(7.5*p_d2,two,0.0,0.0);
    decrgpulse(5.3*p_d2,zero,0.0,0.0);
    decrgpulse(7.4*p_d2,two,0.0,0.0);

    decrgpulse(6.4*p_d2,zero,0.0,0.0);
    decrgpulse(8.2*p_d2,two,0.0,0.0);
    decrgpulse(5.8*p_d2,zero,0.0,0.0);
    decrgpulse(5.7*p_d2,two,0.0,0.0);
    decrgpulse(0.6*p_d2,zero,0.0,0.0);
    decrgpulse(4.9*p_d2,two,0.0,0.0);
    decrgpulse(7.5*p_d2,zero,0.0,0.0);
    decrgpulse(5.3*p_d2,two,0.0,0.0);
    decrgpulse(7.4*p_d2,zero,0.0,0.0);
        endhardloop();


	decphase(t3);

        decpwrf(4095.0);

        decrgpulse(pwC,three,0.0,0.0);  /* flip transferred 13C-magnetization to z */
        decoffset(dof_92p5);	/* Preset the carbon frequency for the C1' carbon */

	decrgpulse(pwC,t3,0.0,0.0);  /* 4x,-4x  flip transferred 13C-magnetization to x */


if (SE[A]=='y') 
{
 /*****************     CONSTANT TIME EVOLUTION      *****************/
      if (CT[A]=='y') {
     /***************/

        initval(90.0, v9);
        decstepsize(1.0);
        dcplrphase(v9);
        decphase(t9);
        delay(CTdelay/2.0 - tau2);

    decrgpulse(2.0*pwC, t9, 0.0, 0.0);
    dcplrphase(zero);
    decphase(t10);

          if (tau2 < gt1 + gstab)
               {delay(CTdelay/2.0 - pwZa - gt1 - gstab);
                 zgradpulse(icosel*gzlvl1, gt1);        /* 2.0*GRADIENT_DELAY */
                delay(gstab - 2.0*GRADIENT_DELAY);
                sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
                delay(tau2);}

          else {delay(CTdelay/2.0 - pwZa);
                sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
                delay(tau2 - gt1 - gstab);
                 zgradpulse(icosel*gzlvl1, gt1);        /* 2.0*GRADIENT_DELAY */
                delay(gstab - 2.0*GRADIENT_DELAY);}


     /***************/
                      }
     /********************************************************************/

     /*****************         NORMAL EVOLUTION         *****************/
      else            {
     /***************/

if (CChomodec[A]=='y')

    {
    decpower(CCdpwr); decphase(zero);
    decprgon(CCdseq,1.0/CCdmf,CCdres);
    decon();  /* CC decoupling on */
    }

decphase(zero);
delay(tau2);

         sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

         decphase(t9);
         delay(gt1 + gstab - pwZa);
         delay(tau2);

if 	(CChomodec[A]=='y')
        {
        decoff(); decprgoff();        /* CC decoupling off */
        decpower(pwClvl);
        }

         decrgpulse(2.0*pwC, t9, 0.0, 0.0);

          zgradpulse(icosel*gzlvl1, gt1);               /* 2.0*GRADIENT_DELAY */
         decphase(t10);
         delay(gstab - 2.0*GRADIENT_DELAY);


     /***************/
                      }
     /********************************************************************/

/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */

        decrgpulse(pwC, zero, 0.0, 0.0);

        decphase(zero);
        zgradpulse(gzlvl5, gt5);
        delay(lambda - 0.5*pwC - gt5);

        simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

        zgradpulse(gzlvl5, gt5);
        txphase(one);
        decphase(t10);
        delay(lambda  - 0.5*pwC - gt5);

        simpulse(pw, pwC, one, t10, 0.0, 0.0);

        txphase(zero);
        decphase(zero);
        zgradpulse(gzlvl6, gt5);
        delay(lambda - 0.5*pwC - gt5);

        simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);
        txphase(two);
        zgradpulse(gzlvl6, gt5);
        delay(lambda - 0.5*pwC - gt5);

        simpulse(pw, pwC, two, zero, 0.0, 0.0);
        txphase(zero);
        delay(lambda - 0.5*pwC);

        simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

        dcplrphase(zero);                                       /* SAPS_DELAY */
        zgradpulse(gzlvl2, gt1/4.0);                   /* 2.0*GRADIENT_DELAY */
        delay(lambda - gt1/4.0 - 0.5*pwC - 2.0*GRADIENT_DELAY - 2*POWER_DELAY - SAPS_DELAY);

}

else
{
        decphase(zero);
        zgradpulse(gzlvl5,gt5);

        delay(lambda - gt5);
        simpulse(2*pw,2*pwC,zero,zero,0.0,0.0);
        zgradpulse(gzlvl5,gt5);
        delay(lambda - gt5);

	decrgpulse(pwC,zero,0.0,0.0);

	zgradpulse(gzlvl3,gt3);
	delay(gstab);

        obspower(tpwrs);
        shaped_pulse("rna_H2Osinc", pwHs, zero, 5.0e-4, 0.0);
        obspower(tpwr);

	rgpulse(pw, zero, 2*rof1, 0.0);
	txphase(two);
	obspower(tpwrs2);
	
	zgradpulse(gzlvl4,gt4);
        delay(gstab - 2*SAPS_DELAY - 2*POWER_DELAY - GRADIENT_DELAY);

        rgpulse((lambda-gstab-gt4-pwC), two, 0.0, 0.0);
        simpulse(pwC,pwC,two,three,0.0,0.0);
        simpulse(2*pwC,2*pwC,two,zero,0.0,0.0);
        simpulse(pwC,pwC,two,three,0.0,0.0);
        rgpulse((pwHs2-2*pwC-(lambda-gstab-gt4-pwC)), two, 0.0, 0.0);

        txphase(zero);
        obspower(tpwr);
        rgpulse(2*pw, zero, 0.0, 0.0);
        obspower(tpwrs2);

        rgpulse(pwHs2, two, 4.0e-6, 0.0);
        decphase(t4);

        zgradpulse(gzlvl4,gt4);
        delay(gstab-2*pwC-2*SAPS_DELAY - POWER_DELAY - GRADIENT_DELAY);

        decrgpulse(pwC,t4,0.0,0.0);
        decrgpulse(pwC,zero,0.0,0.0);
}
        dec2power(dpwr2);               /* 2*POWER_DELAY */
        decpower(dpwr);

status(C);

 setreceiver(t11);
}
