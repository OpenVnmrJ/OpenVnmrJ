/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*	gnoesyNhsqc_CNfilt.c

	Sequence for F1 13C,15N-filtered NOESY-HSQC(15N) experiment, 
        with option for decoupling of 13C during t2.

	Created: as mf_f13c_noe_hsqc_s13c.c 11/19/2003 by M. Rance, U.Cincinnati
        Modified for BioPack, GG, Varian, November 2005

	Uses isotope-filtering element invented by Stuart et al.,
	J.Am.Chem.Soc. 121, 5346-5347 (1999)

	Set:
	d2	= 0
	tauxh	= 1/(4Jxh)

        satmode = 'y' for presat
	satdly	= length of presaturation period
	satpwr	= power level for presat

	gstab   = gradient recovery delay
	gt1	= gradient pulse to remove initial X nucleus magnetization
	gt2	= cleaning gradient
	gt3	= gradient pulse during initial IzSz period
	gt4	= gradient pulse during second IzSz period (array gzlvl4 for maximum s/n)
	gt5	= cleaning gradient
	gzlvl1 ... gzlvl5

        For gt2=gt3=gt4=gt5, gzlvl2-gzlvl3+gzlvl4+gzlvl5 must =0.

        Half-dwell time delay standard in t1 and t2. Use lp1=-180 
        rp1=90; lp2=-180 rp2=90 for proper phasing.

        To permit adjustment of the filter gradients' strength, the
        parameter gscale can be used. It will multiply gzlvl2,gzlvl3,
        gzlvl4 and gzlvl5 by the same factor. The ratio must be 
        maintained, however.
*/

#include <standard.h>

static int
	phi1[16]= {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
	phi2[2]	= {0,2},
	phi3[8] = {0,0,0,0,2,2,2,2},
	phi4[4] = {0,0,2,2},
	phi5[1]	= {0},
	phi6[1] = {0},
	phir[16]= {0,0,2,2,2,2,0,0,2,2,0,0,0,0,2,2};


pulsesequence()
{

/*	Declare variables	*/

char
        satmode[MAXSTR];

int
	t1_counter = 0,
	t2_counter = 0,
	ni	= getval("ni"),
	ni2	= getval("ni2"),
	phase	= getval("phase"),
	phase2	= getval("phase2");

double
	pi	= 3.1415926,
	rg1	= 2.0e-6,
	rg2	= 2.0e-6,
	d0	= getval("d0"),
	d2_init = 0.0,
	d3_init = 0.0,
	satdly	= getval("satdly"),
	satpwr	= getval("satpwr"),
	satfrq	= getval("satfrq"),
	mix	= getval("mix"),
	tauch	= getval("tauch"),
	taunh	= getval("taunh"),
	tauxh	= getval("tauxh"),
	taua,
	taua0,
	taub,
	tauc,
	tauc0,
	taug,
	dtau,
	tau1,
	tau2,
	tau2b,
	pwC	= getval("pwC"),
	pwN	= getval("pwN"),
	pwClvl	= getval("pwClvl"),
	pwNlvl	= getval("pwNlvl"),
	gt1	= getval("gt1"),
	gt2	= getval("gt2"),
	gt3	= getval("gt3"),
	gt4	= getval("gt4"),
	gt5	= getval("gt5"),
	gt6	= getval("gt6"),
	gt7	= getval("gt7"),
	gt8	= getval("gt8"),
	gt9	= getval("gt9"),
	gt10	= getval("gt10"),
	gt11	= getval("gt11"),
	gzlvl1	= getval("gzlvl1"),
	gzlvl2	= getval("gzlvl2"),
	gzlvl3	= getval("gzlvl3"),
	gzlvl4	= getval("gzlvl4"),
	gzlvl5	= getval("gzlvl5"),
	gzlvl6	= getval("gzlvl6"),
	gzlvl7	= getval("gzlvl7"),
	gzlvl8	= getval("gzlvl8"),
	gzlvl9	= getval("gzlvl9"),
	gzlvl10 = getval("gzlvl10"),
	gzlvl11 = getval("gzlvl11"),
	gscale= getval("gscale"),
	gstab= getval("gstab");

getstr("satmode",satmode);

gzlvl2=gscale*gzlvl2;
gzlvl3=gscale*gzlvl3;
gzlvl4=gscale*gzlvl4;
gzlvl5=gscale*gzlvl5;
if ( ni > 0 )
  tau1 = d2 + 1./(2.*sw1);
else
  tau1 = d2;

tau2	= d3 + 1./(2.*sw2) - 4*pwC/pi - 2*pw - 4.0e-6;
if (tau2<0.0) tau2=0.0;
tau2b	= d3 + 1./(2.*sw2) - 4*pwC/pi - 2*pw - 6.0e-6;

/*	Parameter checks	*/

  if ( pw > 20e-6 )
	{
	printf( "your pw seems too long !!!  ");
	printf( "  pw must be <= 20 usec \n");
	psg_abort(1);
	}
  if ( pwC > 30e-6 )
	{
	printf( "your pwC seems too long !!!  ");
	printf( "  pwC must be <= 30 usec \n");
	psg_abort(1);
	}
  if ( pwN > 50e-6 )
        {
        printf( "your pwN seems too long !!!  ");
        printf( "  pwN must be <= 50 usec \n");
        psg_abort(1);
        }
  if ( dm2[C] == 'y' )
	{
	if ( at > 0.21 ) 
	   {
	   printf( " acquisition time is too long for decoupling !!!\n" );
	   psg_abort(1);
	   }
	if ( dpwr > 45 )
	   {
	   printf( "incorrect power for dpwr !!! " );
	   printf( "  must limit dpwr <= 45 \n");
	   psg_abort(1);
	   }
	}
  if ( ( dm2[A] == 'y' ) || ( dm2[B] == 'y' ) )
	{
	printf( "no decoupling should be done during status periods A or B !!!\n" );
	psg_abort(1);
	}
  if ( gt1 > 1.5e-3 )
	{
	printf( "gt1 is too long !!!\n" );
	psg_abort(1);
	}
  if ( gt2 > 1e-3 )
	{
	printf( "gt2 is too long !!!\n" );
	psg_abort(1);
	}
  if ( gt3 > 1e-3 )
	{
	printf( "gt3 is too long !!!\n" );
	psg_abort(1);
	}
  if ( gt4 > 1e-3 )
	{
	printf( "gt4 is too long !!!\n" );
	psg_abort(1);
	}
  if ( gt5 > 1e-3 )
        {
        printf( "gt5 is too long !!!\n" );
        psg_abort(1);
        }
  if ( gt6 > 1e-3 )
        {
        printf( "gt6 is too long !!!\n" );
        psg_abort(1);
        }
  if ( gt7 > 1e-3 )
        {
        printf( "gt7 is too long !!!\n" );
        psg_abort(1);
        }
  if ( gt8 > 1e-3 )
        {
        printf( "gt8 is too long !!!\n" );
        psg_abort(1);
        }
  if ( gt9 > 1.5e-3 )
        {
        printf( "gt9 is too long !!!\n" );
        psg_abort(1);
        }
  if ( gt10 > 1.5e-3 )
        {
        printf( "gt10 is too long !!!\n" );
        psg_abort(1);
        }
  if ( gt11 > 1e-3 )
        {
        printf( "gt11 is too long !!!\n" );
        psg_abort(1);
        }


/*	Set variables		*/

settable(t1,   16,	phi1);
settable(t2,    2,	phi2);
settable(t3,    8,	phi3);
settable(t4,    4,      phi4);
settable(t5,	1,	phi5);
settable(t6,    1,      phi6);
settable(t60,  16,	phir);

/*	Phase incrementation for hypercomplex data	*/

  if ( phase == 2 )
	{
	tsadd(t1,1,4);
	}

  if ( phase2 == 2 )
        {
        tsadd(t4,1,4);
        }

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2)
        { tsadd(t1,2,4); tsadd(t60,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2)
        { tsadd(t4,2,4); tsadd(t60,2,4); }

/* Calculate delay values for initial filter period.  */

taua0 = 600.0e-6;
tauc0 = 2*pwC+gt5+102.0e-6+SAPS_DELAY+2*GRADIENT_DELAY+rg1+rg2+2*pw/pi;
if ( ni > 2)
  dtau = (taunh - taua0)/((double)(ni*2-1));
else
  dtau = 0.;
taua = 1.5*tauch + 0.5*tauc0 - t1_counter*dtau;
taub = 0.5*(tauch - tauc0) + t1_counter*dtau;
tauc = tauc0 + tau1 - 2*t1_counter*dtau;
taug = 0.5*(taunh + taub - taua);


/* Determine real-time variable v3 to use in the event that ni2=0 */

mod4(ct,v2);
hlv(v2,v3);

  initval(0.0,v6);

/*	Begin the actual pulse sequence		*/

status(A);
	delay(d0);
	txphase(zero);
	decphase(zero);
	decpower(pwClvl);
	dec2phase(zero);
	dec2power(pwNlvl);

   if ( satmode[A] == 'y' )
	{
	obspower(satpwr);
	obsoffset(satfrq);
	rgpulse(satdly,zero,rg1,rg2);
	obspower(tpwr);
	obsoffset(tof);
	delay(1.0e-3);
	}
   else
	{
	delay(d1);
	}

status(B);

	decrgpulse(pwC,zero,rg1,rg2);

	zgradpulse(gzlvl1,gt1);
	delay(500.0e-6);

        rgpulse(pw,t1,20.0e-6,rg2);

        zgradpulse(gzlvl2,gt2);

	delay(tauch-pwN-pw-gt2-2*GRADIENT_DELAY-2*(rg1+rg2)-2*pw/pi);

	dec2rgpulse(pwN,zero,rg1,rg2);
	simpulse(2*pw,0.666*pwC,zero,zero,rg1,rg2);

	zgradpulse(gzlvl3,gt3);
	delay(taua-pw-2.333*pwC-gt3-2*GRADIENT_DELAY-rg1-rg2-1.0e-6);

	decrgpulse(pwC,zero,rg1,0.);
	simpulse(2*pw,2.666*pwC,zero,one,1.0e-6,0.);
	decrgpulse(pwC,zero,1.0e-6,rg2);

	zgradpulse(gzlvl4,gt4);
	delay(taub-taug-2.333*pwC-pwN-gt4-2*GRADIENT_DELAY-rg1-rg2-1.0e-6);

	dec2rgpulse(2*pwN,zero,rg1,rg2);

	delay(taug-2*pwN-rg1-2*rg2);

	dec2rgpulse(pwN,t2,rg1,rg2);
	decrgpulse(pwC,t2,rg1,0.);
	decrgpulse(pwC,one,1.0e-6,rg2);

	xmtrphase(v6);
	delay(tauc-2*pwC-gt5-101.0e-6-SAPS_DELAY-2*GRADIENT_DELAY-rg1-rg2-2*pw/pi);
	zgradpulse(gzlvl5,gt5);
	delay(100.0e-6-rg1);

	rgpulse(pw,t3,rg1,rg2);

	zgradpulse(gzlvl6,gt6);
	delay(0.5*mix-2*pw-gt6-rg1-rg2-SAPS_DELAY-2*GRADIENT_DELAY-OFFSET_DELAY);

	rgpulse(pw,zero,rg1,1.0e-6);
	rgpulse(2*pw,one,1.0e-6,1.0e-6);
	rgpulse(pw,zero,1.0e-6,rg2);

	delay(0.5*mix-2*pw-pwN-pwC-gt7-1.0e-3-3*(rg1+rg2)-2*GRADIENT_DELAY);

	decrgpulse(pwC,zero,rg1,rg2);
	dec2rgpulse(pwN,zero,rg1,rg2);
	zgradpulse(gzlvl7,gt7);
	delay(1.0e-3);

	rgpulse(pw,zero,rg1,rg2);

	zgradpulse(gzlvl8,gt8);
	txphase(zero);
	delay(tauxh-gt8-2.333*pwN-rg1-rg2-2.0e-6);		/* delay 1/(4J) */

	dec2rgpulse(pwN,zero,rg1,1.0e-6);
	sim3pulse(2*pw,0.0,2.667*pwN,zero,zero,one,1.0e-6,1.0e-6);
	dec2rgpulse(pwN,zero,1.0e-6,rg2);

	zgradpulse(gzlvl8,gt8);
	txphase(one);
	delay(tauxh-gt8-2.333*pwN-rg1-rg2-2.0e-6);		/* delay 1/(4J) */

	rgpulse(pw,one,rg1,rg2);

	zgradpulse(gzlvl9,gt9);			/* IzSz state	*/
	txphase(t5);
	dec2phase(t4);
	delay(gstab);


  if ( ni2 == 0 )
    {
    ifzero(v3);
	delay(4*pwN+8.0e-6);
    elsenz(v3);
	dec2rgpulse(pwN,zero,2.0e-6,1.0e-6);
	dec2rgpulse(2*pwN,one,1.0e-6,1.0e-6);
	dec2rgpulse(pwN,zero,1.0e-6,2.0e-6);
    endif(v3);
    }

  else

    {
        dec2rgpulse(pwN,t4,rg1,2.0e-6);

    if ( ((0.5*tau2b) > (2*pwC+2.0e-6)) )
	{
	delay(0.5*tau2b-2*pwC-2.0e-6);
	decrgpulse(2*pwC,zero,1.0e-6,1.0e-6);
	rgpulse(2*pw,t5,2.0e-6,2.0e-6);
	dec2phase(t6);
	delay(0.5*tau2b);
	}
    else if ( (tau2b > 4.0e-7) )
	{
	delay(0.5*tau2b);
        rgpulse(2.0*pw,t5,2.0e-6,2.0e-6);
	dec2phase(t6);
        delay(0.5*tau2b);
        }
    else
	{
	dec2phase(t6);
	delay(0.5*tau2);
	rgpulse(2*pw,t5,1.0e-6,1.0e-6);
	delay(0.5*tau2);
	}

        dec2rgpulse(pwN,t6,1.0e-6,rg2);
    }

   if ( ni2 > 0 )
	zgradpulse(gzlvl10,gt10);
   else
	zgradpulse(-gzlvl10,gt10);

        decpower(dpwr);
	txphase(zero);
	dec2phase(zero);
	delay(gstab);

	rgpulse(pw,zero,rg1,rg2);

	zgradpulse(gzlvl11,gt11);
	delay(tauxh-gt11-2.333*pwN-rg1-rg2-2.0e-6);

	dec2rgpulse(pwN,zero,rg1,1.0e-6);
	sim3pulse(2*pw,0.0,2.667*pwN,zero,zero,one,1.0e-6,1.0e-6);
	dec2rgpulse(pwN,zero,1.0e-6,rg2);

	zgradpulse(gzlvl11,gt11);
	dec2power(dpwr2); 
	delay(tauxh-gt11-2.333*pwN-rg1-rof2-POWER_DELAY-2.0e-6);

	rgpulse(pw,two,rg1,rof2);

/*	Acquire the FID		*/

status(C);

	setreceiver(t60);
}
