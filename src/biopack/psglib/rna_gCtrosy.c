/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  rna_gCtrosy.c


    This pulse sequence will allow one to perform the following experiment:

    TROSY with gradients for C13/H1 chemical shift correlation with optional
    N15 refocusing and editing spectral regions.


                      NOTE dof MUST BE SET AT 110ppm ALWAYS
                      NOTE dof2 MUST BE SET AT 200ppm ALWAYS


    S3 option: [S3='y']:
    pulse sequence:     Meissner and Sorensen, JMR, 139, 439-442 (1999)
    TROSY method only applied for carbon nuclei. Need to decouple during
    aquisition.

    S3 option: [S3='n']:
    pulse sequence:     according to Weigelt, JACS, 120, 10778 (1998)
    TROSY method applied for carbon and proton nuclei. No decoupling during
    aquisition.
    This option can be used to select upfield (right) and downfield (left)
    component of the downfield (bottom) C13 doublet for RDC measurement.

    Both sequences are using coherence gradients and the Sensitivity Enhancement
    train for better H2O suppression for CH resonances very close to H2O.

    The sequence will function correctly with ribose='y'/aromatic='y'/allC='y'.
    Although, the TROSY enhancement will only be achieved for aromatic nuclei.

    VNMR processing (wft2da):
    f1coef = '1 0 -1 0 0 -1 0 -1' for nonCT
    f1coef = '1 0 -1 0 0 1 0 1' for CT

    THE CT OPTION: [CT='y']:
    This converts the t1 C13 shift evolution to Constant Time and is the recommended
    usage.
    The constant time delay, CTdelay(1/Jcc), can be set for optimum S/N for any type
    of groups, eg 29ms for ribose and 17ms for aromatics. For the allC option
    it is best to set the CTdelay to 1/Jcc(ribose) which equals 2/Jcc(aromatic).
    Note that in some options, CTdelays less than 8ms will generate an error
    message resulting from a negative delay.


                  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nny' or 'nnn', dmm = 'ccg' (or 'ccw', or 'ccp') for 13C decoupling.
    Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for 15N decoupling.

    Must set phase = 1,2  for States-TPPI acquisition in t1 [C13].

    The flag f1180 should be set to 'y' if t1 is to be started at halfdwell
    time. This will give 90, -180 phasing in f1. If it is set to 'n' the
    phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in C13.  f1180='y' is ignored if ni=0.


                  DETAILED INSTRUCTIONS FOR USE OF rna_gCtrosy


    1. Obtain a printout of the Philosopy behind the RnaPack development,
       and General Instructions using the macro:
                                      "printon man('RnaPack') printoff".
       These Detailed Instructions for rna_gCtrosy may be printed using:
                                      "printon man('rna_gCtrosy') printoff".

    2. Apply the setup macro "rna_gCtrosy".  This loads the relevant parameter set
       and also sets ni=0 and phase=1 ready for a 1D spectral check.

    3. Centre H1 frequency on H2O (4.7ppm), N15 frequency on the aromatic N
       region (200 ppm), and C13 frequency on 110ppm.

    4. CHOICE OF C13 REGION:
       ribose='y' gives a spectrum of ribose/C5 resonances centered on dof=85ppm.
       This is a common usage.                               Set sw1 to 55ppm.

       aromatic='y' gives a spectrum of aromatic C2/C6/C8 groups.  dof is shifted
       automatically by the pulse sequence code to 145ppm.  Set sw1 to 30ppm.

       allC='y' gives a spectrum of ribose and aromatic resonances. dof is
       set by the code to 110ppm.                        Set sw1 to 110ppm.

       aromatic_C5='y' gives a spectrum of aromatic C2/C6/C8/C5 groups.  dof is shifted
       automatically by the pulse sequence code to 125ppm.  Set sw1 to 70ppm.


    5. N15 COUPLING:
       Splitting of resonances in the C13 dimension by N15 coupling in N15
       enriched samples is removed by setting N15refoc='y'.  No N15 RF is
       delivered in the pulse sequence if N15refoc='n'.  N15 parameters are
       listed in dg2.

    6. 1/4J DELAY TIMES:
       These are listed in dg/dg2 for possible change by the user. JCH is used
       to calculate the 1/4J time (lambda=1.0*1/4J).
         ribose CH/CH2:           JCH=145-180Hz (180Hz for C1'-H1')
         aromatic CH:             JCH=180-220Hz
         allC:                    JCH=180Hz works best

    7. Wet option: (wet='y'):
       For optimum H2O-suppression use wet='y'.

    8. TROSY peak selection (bottom and right option if S3='n'):

       Type trace='f1' for display of 2D spectrum
       with bottom='y' and right='y' the bottom-right peak of the coupled
       2D gets selected
       with bottom='y' and right='n' the bottom-left peak of the coupled
       2D gets selected
       etc.

       Optimize coherence selection gradient (gzlvl2) for different TROSY-options!!!


        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        @                                                      @
        @   Written for RnaPack by Peter Lukavsky (05/00).     @
        @                                                      @
        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@




*/

#include <standard.h>
  

/* Chess - CHEmical Shift Selective Suppression */
Chess(pulsepower,pulseshape,duration,phase,rx1,rx2,gzlvlw,gtw,gswet)
double pulsepower,duration,rx1,rx2,gzlvlw,gtw,gswet;
  codeint phase;
  char* pulseshape;
{
  obspwrf(pulsepower);
  shaped_pulse(pulseshape,duration,phase,rx1,rx2);
  zgradpulse(gzlvlw,gtw);
  delay(gswet);
}

/* Wet4 - Water Elimination */
Wet4(phaseA,phaseB)
  codeint phaseA,phaseB;
{
  double finepwr,gzlvlw,gtw,gswet,gswet2,wetpwr,wetpw,dz;
  char   wetshape[MAXSTR];
  getstr("wetshape",wetshape);    /* Selective pulse shape (base)  */
  wetpwr=getval("wetpwr");        /* User enters power for 90 deg. */
  wetpw=getval("wetpw");        /* User enters power for 90 deg. */
  dz=getval("dz");
  finepwr=wetpwr-(int)wetpwr;     /* Adjust power to 152 deg. pulse*/
  wetpwr=(double)((int)wetpwr);
  if (finepwr==0.0) {wetpwr=wetpwr+5; finepwr=4095.0; }
  else {wetpwr=wetpwr+6; finepwr=4095.0*(1-((1.0-finepwr)*0.12)); }
  rcvroff();
  obspower(wetpwr);         /* Set to low power level        */
  gzlvlw=getval("gzlvlw");      /* Z-Gradient level              */
  gtw=getval("gtw");            /* Z-Gradient duration           */
  gswet=getval("gswet");        /* Post-gradient stability delay */
  gswet2=getval("gswet2");        /* Post-gradient stability delay */
  Chess(finepwr*0.5059,wetshape,wetpw,phaseA,20.0e-6,rof2,gzlvlw,gtw,gswet);
  Chess(finepwr*0.6298,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/2.0,gtw,gswet);
  Chess(finepwr*0.4304,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/4.0,gtw,gswet);
  Chess(finepwr*1.00,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/8.0,gtw,gswet2);
  obspower(tpwr); obspwrf(tpwrf);     /* Reset to normal power level   */
  rcvron();
  delay(dz);
}
	     
static int   

/*  phx etc also enable TROSY phase changes */
             phx[1]={0},   phy[1]={1},   ph_x[1]={2},	ph_y[1]={3},

	     phi8[4]  = {0,1,2,3},
             phi4[4]  = {0,0,2,2},
	     recT[4]  = {0,2,2,0},

             phi3[2]  = {1,3},
             recT1[2]  = {0,2};

static double   d2_init=0.0;


pulsesequence()
{

/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
	    wet[MAXSTR],     /* Flag to select optional WET water suppression */
	    S3[MAXSTR],    /* spin-state-selective coherence transfer element */
            CT[MAXSTR],                                /* constant time in t1 */
	    SHAPE[MAXSTR],
            ribose[MAXSTR],                         /* ribose CHn groups only */
            aromatic[MAXSTR],                     /* aromatic CHn groups only */
	    aromatic_C5[MAXSTR],                  /* aromatic CHn groups only */
            allC[MAXSTR],                       /* ribose and aromatic groups */
            rna_stCshape[MAXSTR],     /* calls sech/tanh pulses from shapelib */
            N15refoc[MAXSTR],                    /* N15 pulse in middle of t1 */
	    bottom[MAXSTR],
	    right[MAXSTR];
 
int         t1_counter,  		        /* used for states tppi in t1 */
	    icosel;                               /* used to get n and p type */

double      tau1,         				         /*  t1 delay */
	    lambda = 1.0/(4.0*getval("JCH")), 	   /* 1/4J H1 evolution delay */
            CTdelay = getval("CTdelay"),     /* total constant time evolution */

   pwClvl = getval("pwClvl"),                   /* coarse power for C13 pulse */
   pwC = getval("pwC"),               /* C13 90 degree pulse length at pwClvl */
   rfC,                           /* maximum fine power when using pwC pulses */
 compC = getval("compC"),         /* adjustment for C13 amplifier compression */

/* Sech/tanh inversion pulses automatically calculated by macro "rna_cal"     */
/* and string parameter rna_stCshape calls them from your shapelib.           */
   rfst = 0.0,          /* fine power for the rna_stCshape pulse, initialised */
   dofa,       /* dof shifted to 85 or 125ppm for ribose and aromatic spectra */

        pwHs = getval("pwHs"),          /* H1 90 degree pulse length at tpwrs */
        tpwrs,                        /* power for the pwHs ("H2Osinc") pulse */
	compH = getval("compH"),   /* adjustment for H1 amplifier compression */

        pwNlvl = getval("pwNlvl"),                    /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
        sw1 = getval("sw1"),

 grecov = getval("grecov"),   /* Gradient recovery delay, typically 150-200us */

        gt1 = getval("gt1"),                   /* coherence pathway gradients */
        gzlvl1 = getval("gzlvl1"),
        gzlvl2 = getval("gzlvl2"),

	gt0 = getval("gt0"),				   /* other gradients */
        gt3 = getval("gt3"),
	gt5 = getval("gt5"),
	gzlvl0 = getval("gzlvl0"),
        gzlvl3 = getval("gzlvl3"),
	gzlvl5 = getval("gzlvl5");

    getstr("f1180",f1180);
    getstr("wet",wet);
    getstr("CT",CT);
    getstr("S3",S3);
    getstr("SHAPE",SHAPE);
    getstr("ribose",ribose);
    getstr("aromatic",aromatic);
    getstr("aromatic_C5",aromatic_C5);
    getstr("allC",allC);
    getstr("N15refoc",N15refoc);
    getstr("bottom",bottom);
    getstr("right",right);


/*   LOAD PHASE TABLE    */
	
  if (S3[A] == 'y')
   {    settable(t3,4,phi4);
        settable(t2,1,phx);
        settable(t1,1,phx);
        settable(t4,1,phx);
        settable(t9,4,phi8);
        settable(t11,1,ph_y);
        settable(t12,4,recT);
   }
  else
   {
        settable(t3,2,phi3);
        settable(t1,1,phx);

        if (bottom[A]=='y')
        settable(t4,1,phy);
        else
        settable(t4,1,ph_y);
        if (right[A]=='y')
        settable(t10,1,phx);
        else
        settable(t10,1,ph_x);

        settable(t9,1,phx);
        settable(t11,1,phy);
        settable(t12,2,recT1); 
   }


/*   INITIALIZE VARIABLES   */

/* maximum fine power for pwC pulses */
        rfC = 4095.0;
	dofa=dof;

/* optional refocusing of N15 coupling for N15 enriched samples */
  if (N15refoc[A]=='n')  pwN = 0.0;

/* selective H20 one-lobe sinc pulse */
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
    tpwrs = (int) (tpwrs);                        /*power than a square pulse */

/*  RIBOSE spectrum only, centered on 85ppm. */
  if (ribose[A]=='y')
   {
        dofa = dof - 25.0*dfrq;

        /* 50ppm sech/tanh inversion */
        rfst = (compC*4095.0*pwC*4000.0*sqrt((7.5*sfrq/600+3.85)/0.41));
        rfst = (int) (rfst + 0.5);
        strcpy(rna_stCshape, "rna_stC50");
   }

/*  ALL AROMATIC spectrum only, centered on 125ppm */
  if (aromatic_C5[A]=='y')
   {
        dofa = dof + 15*dfrq;

        /* 30ppm sech/tanh inversion */
        rfst = (compC*4095.0*pwC*4000.0*sqrt((12.0*sfrq/600.0+3.85)/0.41));
        rfst = (int) (rfst + 0.5);
        strcpy(rna_stCshape, "rna_stC80");
   }

/*  AROMATIC spectrum only, centered on 145ppm */
  if (aromatic[A]=='y')
   {
        dofa = dof + 35*dfrq;

        /* 30ppm sech/tanh inversion */
        rfst = (compC*4095.0*pwC*4000.0*sqrt((4.5*sfrq/600.0+3.85)/0.41));
        rfst = (int) (rfst + 0.5);
        strcpy(rna_stCshape, "rna_stC30");
   }


/*  TOTAL CARBON spectrum, centered on 110ppm */
  if (allC[A]=='y')
   {
        dofa = dof;

        /* 140ppm sech/tanh inversion */
        rfst = (compC*4095.0*pwC*4000.0*sqrt((21.0*sfrq/600.0+7.0)/0.35));
        rfst = (int) (rfst + 0.5);
        strcpy(rna_stCshape, "rna_stC140");
        if ( 1.0/(4000.0*sqrt((21.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n",
            (1.0e6/(4000.0*sqrt((21.0*sfrq/600.0+7.0)/0.35))) ); psg_abort(1); }
   }



/* CHECK VALIDITY OF PARAMETER RANGES */

  if ((CT[A]=='y') && (S3[A]=='n') && (ni/(2.0*sw1) > CTdelay))
  { text_error( " ni is too big. Make ni equal to %d or less.\n",
      ((int)(CTdelay*sw1*2.0)) ); psg_abort(1); }

  if ( (CT[A]=='y') && (S3[A]=='y') && ((ni/(4.0*sw1)) > (CTdelay/2.0 - lambda)) )
  { text_error( " ni is too big. Make ni equal to %d or less.\n",
      ( (int)(4.0*sw1*(CTdelay/2.0 - lambda)) ) ); psg_abort(1); }

  if((dm3[A] == 'y' || dm3[C] == 'y' ))
  { text_error("incorrect dec1 decoupler flags! Should be 'nyn' or 'nnn' "); psg_abort(1); }

  if((dm2[A] == 'y' || dm2[B] == 'y'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nnn' or 'nny' "); psg_abort(1); }

  if( (dm2[C] == 'y') && (N15refoc[A] == 'n') )
  { text_error(" don't need to decouple if N15refoc is 'n'!! "); psg_abort(1); }

  if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ) && (S3[A]=='n'))
  { text_error("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1); }

/*
  if((dm[C] == 'n') && (S3[A] == 'y'))
  { text_error("incorrect dec1 decoupler flags! Should be 'nny' "); psg_abort(1); }
*/

  if( dpwr > 50 )
  { text_error("don't fry the probe, DPWR too large!  "); psg_abort(1); }

  if( dpwr2 > 50 )
  { text_error("don't fry the probe, DPWR2 too large!  "); psg_abort(1); }

  if( (pw > 20.0e-6) && (tpwr > 56) )
  { text_error("don't fry the probe, pw too high ! "); psg_abort(1); }

  if( (pwC > 40.0e-6) && (pwClvl > 56) )
  { text_error("don't fry the probe, pwC too high ! "); psg_abort(1); }

  if( (pwN > 100.0e-6) && (pwNlvl > 56) )
  { text_error("don't fry the probe, pwN too high ! "); psg_abort(1); }


/*  CHOICE OF PULSE SEQUENCE */

  if ( ((ribose[A]=='y') && (allC[A]=='y')) || ((ribose[A]=='y') && (aromatic[A]=='y'))
       || ((aromatic[A]=='y') && (allC[A]=='y')) || ((ribose[A]=='y') && (aromatic_C5[A]=='y'))
       || ((aromatic_C5[A]=='y') && (allC[A]=='y')) || ((aromatic[A]=='y') && (aromatic_C5[A]=='y')) )
   { text_error("Choose  ONE  of  ribose='y'  OR  aromatic='y'  OR  aromatic_C5='y' OR  allC='y' ! ");
        psg_abort(1); }


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

  if (S3[A]=='y')
   {
 	if (phase1 == 1)                 icosel = +1;
        else          {  tsadd(t11,2,4); icosel = -1;  }
   }
  else
   {
        if (phase1 == 1)                                  icosel = +1;
        else          {  tsadd(t4,2,4);  tsadd(t10,2,4);  icosel = -1;  }
   }
          

/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;



/* Calculate modifications to phases for States-TPPI acquisition  */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t3,2,4); tsadd(t12,2,4); } 


/* BEGIN PULSE SEQUENCE */

status(A);

        obspower(tpwr);
        decpower(pwClvl);
        decpwrf(rfC);
        dec2power(pwNlvl);
        obsoffset(tof);
        decoffset(dofa);
        dec2offset(dof2);
        txphase(zero);
        decphase(zero);
        dec2phase(zero);

	delay(d1);

  if (wet[A] == 'y')
    {
        Wet4(zero,one);             /* WET solvent suppression */
        delay(1.0e-4);
        obspower(tpwr);
    }

        rcvroff();
	txphase(t1);
 
   	rgpulse(pw,t1,0.0,0.0);                 /* 1H pulse excitation */
	txphase(one);
   	decphase(zero);
if (SHAPE[A] == 'y')
{
	decpwrf(rfst);

	zgradpulse(gzlvl0, gt0);
	delay(lambda - gt0 - WFG2_START_DELAY - 0.5e-3 + 70.0e-6);

        simshaped_pulse("", rna_stCshape, 2.0*pw, 1.0e-3, one, zero, 0.0, 0.0);
   	txphase(one);
        decpwrf(rfC);

	zgradpulse(gzlvl0, gt0);
	delay(lambda - gt0 - 0.5e-3 + 70.0e-6);
}
else
{
        zgradpulse(gzlvl0, gt0);
        delay(lambda - gt0);

        simpulse(2.0*pw, 2.0*pwC, one, zero, 0.0, 0.0);
        txphase(one);

        zgradpulse(gzlvl0, gt0);
        delay(lambda - gt0);
}

 	rgpulse(pw, one, 0.0, 0.0);

  if (wet[A] == 'y')
    {
	obspower(tpwrs);

	if(S3[A]=='y')
	{
        txphase(t2);
        shaped_pulse("rna_H2Osinc", pwHs, t2, 5.0e-5, 0.0);
	}
	else
	{
        txphase(two);
        shaped_pulse("rna_H2Osinc", pwHs, two, 5.0e-5, 0.0);
	}

        obspower(tpwr);
    }

	zgradpulse(gzlvl3, gt3);
	decphase(t3);
	delay(grecov);

   	decrgpulse(pwC, t3, 0.0, 0.0);
	txphase(t4);
	decphase(zero);


/*  xxxxxxxxxxxxxxxxxxx    OPTIONS FOR C13 EVOLUTION    xxxxxxxxxxxxxxxxxxxx  */

     /*****************     CONSTANT TIME EVOLUTION      *****************/
      if (CT[A]=='y') {
     /***************/

    decphase(t9);

    delay(CTdelay/2.0 - lambda - tau1);

    decrgpulse(2.0*pwC, t9, 2.0e-6, 0.0);
    decphase(zero);

          if (tau1 < gt1 + grecov)
               {delay(CTdelay/2.0 - lambda - 2.0*pwN - gt1 - grecov);
                zgradpulse(gzlvl1, gt1);         /* 2.0*GRADIENT_DELAY */
                delay(grecov - 2.0*GRADIENT_DELAY);
                dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
                delay(tau1);}

          else {delay(CTdelay/2.0 - lambda - 2.0*pwN);
                dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
                delay(tau1 - gt1 - grecov);
                zgradpulse(gzlvl1, gt1);         /* 2.0*GRADIENT_DELAY */
                delay(grecov - 2.0*GRADIENT_DELAY);}

     /***************/
                      }
     /********************************************************************/

     /*****************         NORMAL EVOLUTION         *****************/
      else            {
     /***************/

 	 decphase(t9);
	 delay(tau1);

         dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);

         delay(gt1 + grecov - 2.0*pwN);
         delay(tau1);

         decrgpulse(2.0*pwC, t9, 0.0, 0.0);

         zgradpulse(gzlvl1, gt1); 
         decphase(zero);
         delay(grecov - 2.0*GRADIENT_DELAY);

     /***************/
                      }
     /********************************************************************/

/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */


	rgpulse(pw, t4, 0.0, 0.0);
	txphase(zero);
        zgradpulse(gzlvl5, gt5);
	delay(lambda - gt5 - pwC);

        simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

        zgradpulse(gzlvl5, gt5);

  if(S3[A]=='y')
   {
        txphase(t11);
        delay(lambda  - gt5 - pwC);

        simpulse(pw, pwC, t11, zero, 0.0, 0.0);
        txphase(zero);

        delay(lambda - pwC);

        simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

        zgradpulse(icosel*gzlvl2, gt1/4);
        delay(lambda - gt1/4 - 2.0*POWER_DELAY - pwC);

        dec2power(dpwr2);
        decpower(dpwr);
        rcvron();
status(C);
   }
  else
   {
        decphase(t11);
        delay(lambda - gt5 - pwC);

        simpulse(pw, pwC, zero, t11, 0.0, 0.0);
        decphase(zero);

        zgradpulse(1.5*gzlvl5, gt5);
        delay(lambda - gt5 - pwC);

        simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);
	decphase(t10);

        zgradpulse(1.5*gzlvl5, gt5);
        delay(lambda - gt5 - pwC);

        decrgpulse(pwC, t10, 0.0, 0.0);

	zgradpulse(-gzlvl2, gt1/8);
	delay(grecov/2 + POWER_DELAY);

        rgpulse(2.0*pw, zero, 0.0, rof2);

	zgradpulse(icosel*gzlvl2, gt1/8);
        dec2power(dpwr2);
        rcvron();
statusdelay(C,grecov/2-rof2);
   }

        setreceiver(t12);
}
