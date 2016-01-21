/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*	crinept.c 
	a [15N,1H]-CRINEPT-TROSY using States/TPPI.


       See "Polarization transfer by cross-correlated relaxation in solution NMR
       with very large molecules. Rolan Riek, Gerhard Wider, Konstantin Pervushin 
       and Kurt Wuthrich. PNAS 1999 vol 96, 4918-4923.

       You can also find some optimization notes in...

       "Solution NMR Techniques for Large Molecular and Supramolecular Structures."
       Roland Reik, Jocelyne Fiaux, Eric, Eric B. Bertelsen, Arthur L. Horwich and Kurt
       Wuthrich,JACS 2002 vol 124, 12141-12153.

	Run as four fid experiment, for phase=1,2 and psi2=1,2 
	with array='phase,psi2'. 

	The quartet of fids collected are...
	
	fid1 = inphase(?) & real t1
	fid2 = antiphase(?) & real t1
	fid3 = inphase(?) & imaginary t1
	fid4 = antiphase(?) & imaginary t1

        For Non-VNMR Processing:
	Data is processed by adding fid1 to fid2 and then phase correcting 
	90 degrees for the real t1 point. Imaginary t1 point is likewise
	obtained by subtracting fid4 from fid3 and phase correcting the result 
	by 90 degrees.

        For VNMR Processing:
         wft2d(1,1,1,1,-1,1,1,-1,-1,1,1,-1,-1,-1,-1,-1)

	Set:
	d2	= 0
	tauT	= 3 to 5.4 msec
	psi2	= 1,2  for inphase/antiphase separation
        phase   = 1,2  for States/TPPI

	gt1	= 400 usec
	gzlvl1  = 15 G/cm
       
	Contributed by Mark Rance and Jack Howarth, U. Cincinnati

        adapted for BioPack by G.G Mar 2004
        corr,phincr_d    These are available for fine-tuning. The corr variable is used
                         to shorten final echo delay in order to make first-order phase
                         correction as near to zero as possible. First set to zero, run quick
                         2D and phase F2. Enter "corf2" and note change in sum of rof2+alfa. Reset
                         rof2 and alfa to original values and enter noted change in corr variable.
                         Rerun 2D and rephase in F2. The first-order phase correction (lp) should
                         now be close to zero. If not, repeat process.

                         phincr_d permits additional phase shifting of flip(down) pulse. The
                         BioPack shape, H2Osinc_d.RF has a phase correction built-in, but this
                         may be used for further correction (normally not necessary, so set
                         phincr_d to zero for first tests)

*/

#include <standard.h>

static int
	phi1[2]	= {0,2},
	phi2[1]	= {0},
	rec[2]	= {0,2};

static double   d2_init=0.0;

pulsesequence()
{

/*	Declare variables	*/

int
	t1_counter,                     /* used for states tppi in t1 */
	psi2	= getval("psi2"),	/* collection of inphase and antiphase components */
	phase	= getval("phase");	/* collection of indirect with states method */

char    f1180[MAXSTR];                  /* half-dwell time in t1 */

double
	d0	= getval("d0"),
        corr    = getval("corr"),       /* adjustable correction */
	tau1,				/*  t1 delay */
	pwN	= getval("pwN"),	/* hard 15N 90deg pulse length */
	pwNlvl	= getval("pwNlvl"),	/* power level for hard 15N pulses */
	pwC	= getval("pwC"),	/* hard 13C 90deg pulse length */
	pwClvl	= getval("pwClvl"),	/* power level for hard 15N pulses */
	tauT	= getval("tauT"),	/* optimal transfer delay in seconds */

	gt0	= getval("gt0"),
	gt1	= getval("gt1"),	/* coherence pathway gradients */
	gstab   = getval("gstab"),	/* field stabilization delay   */
	gzlvl0	= getval("gzlvl0"),
	gzlvl1	= getval("gzlvl1"),

        pwHs	= getval("pwHs"),      /* H1 90 degree pulse length at tpwrs */ 
        compH	= getval("compH"),      /* H1 amplifier compression factor */ 
        tpwrs	= getval("tpwrs"),    /* power for the pwHs ("H2Osinc") pulse */
       tpwrsf_d	= getval("tpwrsf_d"),    /*fine power for the pwHs ("H2Osinc") pulse */
       phincr_d = getval("phincr_d");    /* small-angle phase adjust for flipback pulse */ 
                                     /* set phincr_d to zero if shape has phase correction 
                                        already built-in (such as H2Osinc_d.RF) */
   if (phincr_d < 0.0) phincr_d=360+phincr_d; 
   initval(phincr_d,v8); 

getstr("f1180",f1180);
tau1 = d2;
if ( ni > 0 )
  {
   if (f1180[A] == 'y')
    tau1 = d2 + 1./(2.*sw1) - 4*pwN/PI - rof1 - 1.0e-6;
   else
    tau1 = d2  - 4*pwN/PI - rof1 - 1.0e-6;
  }
 

/*	Check the gradient parameters.	*/

  if ( fabs(gzlvl0) > 30000 )
        {
        printf( "gzlvl0 is too high !!!\n" );
        psg_abort(1);
        }
  if ( fabs(gzlvl1) > 30000 )
        {
        printf( "gzlvl1 is too high !!!\n" );
        psg_abort(1);
        }

/*	Check other parameters	*/

  if ( pw > 20e-6 )
	{
	printf( "your pw seems too long !!!  ");
	printf( "  pw must be <= 20 usec \n");
	psg_abort(1);
	}
  if ( pwN > 50e-6 )
	{
	printf( "your pwN seems too long !!!  ");
	printf( "  pwN must be <= 50 usec \n");
	psg_abort(1);
	}
  if ( pwC > 20e-6 )
	{
	printf( "your pwC seems too long !!!  ");
	printf( "  pwC must be <= 20 usec \n");
	psg_abort(1);
	}
  if ( ( dm2[A] == 'y' ) || ( dm2[B] == 'y' ) || ( dm2[C] == 'y' ) )
	{
	printf( "no decoupling should be done during experiment !!!\n" );
	psg_abort(1);
	}

  if ( ( dm[A] == 'y' ) || ( dm[B] == 'y' ) || ( dm[C] == 'y' ) )
	{
	printf( "no decoupling should be done on channel 2 !!!\n" );
	psg_abort(1);
	}

/*	Set variables		*/

	settable(t1,    2,	phi1);
	settable(t2,	1,	phi2);
	settable(t3,    2,	rec);

/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for two fids to add */

	if (psi2 == 2)    tsadd(t2,2,4);

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

        if (phase == 2)    tsadd(t1,1,4);                           
         
/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
        { tsadd(t1,2,4); tsadd(t3,2,4); } 


status(A);
	delay(d0);
	obsoffset(tof);
	obspower(tpwrs);
	txphase(zero);
	decphase(zero);
	decpower(pwClvl);
	dec2phase(zero);
	dec2power(pwNlvl);

	delay(d1);


/* for a selective H20 one-lobe sinc pulse */
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
    tpwrs = (int) (tpwrs);                   	  /*power than a square pulse */


status(B);
	dec2rgpulse(pwN,zero,rof1,rof1);

	zgradpulse(gzlvl0,gt0);
	delay(gstab);
        obsstepsize(1.0); 
        xmtrphase(v8); 

        if (tpwrsf_d<4095.0)
        {
         obspower(tpwrs+6.0);
         obspwrf(tpwrsf_d);
   	 shaped_pulse("H2Osinc_d", pwHs, two, rof1, 0.0);
        }
         else
        {
         obspower(tpwrs);
   	 shaped_pulse("H2Osinc_d", pwHs, two, rof1, 0.0);
        }
        xmtrphase(zero);
	obspower(tpwr); obspwrf(4095.0);


        rgpulse(pw, zero, rof1, 1.0e-6);

	zgradpulse(gzlvl1,gt1);
	obspower(tpwrs);
        delay(tauT-gt1-pwHs-4*pw/PI-2*GRADIENT_DELAY-2*POWER_DELAY
		-2.0*SAPS_DELAY-WFG_START_DELAY-7.0e-5);
        obsstepsize(1.0); 
        xmtrphase(v8); 

        if (tpwrsf_d<4095.0)
        {
         obspower(tpwrs+6.0);
         obspwrf(tpwrsf_d);
   	 shaped_pulse("H2Osinc_d", pwHs, three, rof1, 0.0);
        }
         else
        {
         obspower(tpwrs);
   	 shaped_pulse("H2Osinc_d", pwHs, three, rof1, 0.0);
        }
        xmtrphase(zero); obspower(tpwr); obspwrf(4095.0);
        rgpulse(pw, one, rof1, 1.0e-6);

	dec2rgpulse(pwN,t1,rof1,1.0e-6);
	dec2phase(zero);
        if ((tau1-pwC/2.0)>0)
         {
	 delay((tau1-pwC)/2);
	 sim3pulse(0.,2.0*pwC,0.,zero,zero,zero,rof1,1.0e-6);
	 delay((tau1-pwC)/2);
         }
	dec2rgpulse(pwN,zero,rof1,1.0e-6);

	obspower(tpwrs);
        delay(tauT-gt1-pwHs-2*pwN-0.5*pw-2*pw/PI-2*POWER_DELAY
		-2*GRADIENT_DELAY-2.0*SAPS_DELAY-rof1-gstab-7.0e-6);

	zgradpulse(gzlvl1,gt1);
	delay(gstab);
        obsstepsize(1.0); 
        xmtrphase(v8); 

        if (tpwrsf_d<4095.0)
        {
         obspower(tpwrs+6.0);
         obspwrf(tpwrsf_d);
   	 shaped_pulse("H2Osinc_d", pwHs, two, rof1, 0.0);
        }
         else
        {
         obspower(tpwrs);
   	 shaped_pulse("H2Osinc_d", pwHs, two, rof1, 0.0);
        }
	xmtrphase(zero); obspower(tpwr); obspwrf(4095.0);

	sim3pulse(pw,0.,2*pwN,zero,zero,zero,rof1,1.0e-6);

	zgradpulse(gzlvl1,gt1);
	delay(tauT-corr-gt1-2*pwN-2*pw/PI-0.5*pw-2.0*SAPS_DELAY-2*GRADIENT_DELAY-rof1-1.0e-6);
                            /* set corr to a value to obtain lp=0  */

	dec2rgpulse(pwN,t2,rof1,0.0);

status(C);
	setreceiver(t3);
}
