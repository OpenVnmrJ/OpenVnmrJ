// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif
/* 
 */
/* AdequateAD.c 
    a sequence for proton detected INADEQUATE

REF: B. Reif, M. Koeck, R. Kerssebaum, H. Kang,
         W. Fenical and C. Griesinger : JMR A 118, 282-285 (1996)
     M. Koeck, B. Rief, W. Fenical and C. Griesinger : Tetrahedron Letters,
                                   Vol. 37. No. 3. 363-366 (1996).

    sspul   - option for GRD-90-GRD steady-state pulses
    hsgt    - gradient duration for sspul
    hsglvl  - gradient power level for sspul
    gt1     - first gradient duration
    gzlvl1  - first gradient power level
    gt2     - second gradient duration
    gzlvl2  - second gradient duration
    gt3     - second gradient duration
    gzlvl3  - second gradient duration
    gstab   - gradient recovery delay
    pwxlvl  - decoupler power level for hard decoupler pulses
    pwx     - pulse width for hard decoupler pulses
    dpwr    - power level for decoupling
    dmf     - controlled by dpwr
    j1xh    - one-bond heteronuclear coupling constant (140 for 13C; 90 for 15N)
    jcc     - CC coupling constant (55 - 35 Hz)
    satmode - a y/n flag for transmitter presaturation
    satdly  - the presaturation delay used if satmode = y
    satpwr  - the presaturation power
    satfrq  - the frequency desired for presaturation
    f1180   - a y/n flag for 1/2 dwell starting t1 evolution delay
    nt      - works with nt=1 (nt=2 improves data)
    phase   - use phase=1,2 to select for States-TPPI  
            - transform with wftse = wft2d('ntype',1,0,-1,0,0,1,0,1)
    kappa   - 0 gives C-C Double Quantum evolution in F1
            - 1 gives C-C Single Quantum evolution in F1 (i.e. C13 chemical shifts)
                in this case ni is limited (nimax is handled by the sequence)
    ad180   - adiabatic pulse of length 30*pwx and bandwidth 1/pwx
                 calculated and set inside the sequence
    av180   - av180b refocussing pulse of bandwidth 1.54*B1max (pw = 9.45/bw)
                 calculated and set inside the sequence
    dof     - decoupler offset for pulses (including quaternaries)
    dofdec  - decoupler offset for decoupling during acq. (protonated carbon reg.)

All shapes are created internally. Be sure that to avoid pwxlvl settings with 
compression factors < 0.9.
The following table gives an insight under what conditions the refocussing
pulses give proper performance:

	pwx(square)	gamab1		pw(av180)	C13 range refocussed (kHz)
	8.15 (usec)	30.5 (kHz)	200 (usec)	40.0 (kHz)
	9.15		27.3		225		36
	10.2		24.6		250		32
	11.2		22.3		275		29.3
	12.2		20.5		300		26.5
	13.2		18.9		325		24.5
	14.2		17.5		350		23
	15.2		16.4		375		21
	16.3		15.4		400		20

INOVA implementation: Norbert Zimmermann, BASF AG 08.08.96
      modified: Peter Sandor, Darmstadt, March 1997 - kappa is introduced
      modified: Eriks Kupce, Oxford, October 1998 - adiabatic pulses
      modified: Peter Sandor, Darmstadt, 2002 - all C13 180s are shaped 
      modified: Bert Heise, Darmstadt, 2006 - made VnmrJ compliant
      modified: Bert Heise, Oxford, 2009 - made Chempack compliant
Echo family - NM (April 19, 2007)
*/

#include <standard.h>
#include <chempack.h>

static double d2_init = 0.0;
static int phi1[1]   = {0},
           phi2[1]   = {1}, 
           phi4[2]   = {0,2},
           phi5[4]   = {0,0,2,2},
           phi6[4]   = {1,1,3,3},
           phi8[1]   = {0},
           phi9[8]   = {0,0,0,0,2,2,2,2},
           phi10[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
           phi11[1]  = {0},
           phi31[16] = {0,2,2,0,2,0,0,2,2,0,0,2,0,2,2,0};

pulsesequence()
{
/* DECLARE VARIABLES */
 char     satmode[MAXSTR],f1180[MAXSTR],sspul[MAXSTR],comm[MAXSTR],
          pwx180ad[MAXSTR],pwx180adR[MAXSTR],pwx180ref[MAXSTR];
 int	  phase,satmove,maxni,t1_counter,icosel=1,
          prgcycle = (int)(getval("prgcycle")+0.5);
 double   tau1,tauxh, tauxh1, taucc,av180,
          hsgt   = getval("hsgt"), 
          gt1    = getval("gt1"),
          gt2    = getval("gt2"),
          gt3    = getval("gt3"),
          hsglvl = getval("hsglvl"),
          gzlvl1 = getval("gzlvl1"),
          gzlvl2 = getval("gzlvl2"),
          gzlvl3 = getval("gzlvl3"),
          pwx    = getval("pwx"),
          pwxlvl = getval("pwxlvl"),
          j1xh   = getval("j1xh"),
          jcc    = getval("jcc"),
          satdly = getval("satdly"),
          satpwr = getval("satpwr"),
          satfrq = getval("satfrq"),
          sw1    = getval("sw1"),
          gstab  = getval("gstab"),
          kappa  = getval("kappa"),
          dofdec = getval("dofdec"),
	  pwx180 = getval("pwx180"),
	  pwxlvl180  = getval("pwxlvl180");

  getstr("f1180",f1180);
  getstr("satmode",satmode);
  getstr("sspul",sspul);
  getstr("pwx180ad", pwx180ad);
  ni = getval("ni");
  av180 = (double)((int)(1e6*(9.45*4*pwx/1.54+0.5e-6))/1e6); /* round it to 1usec */
  phase = (int) (getval("phase") + 0.5);
  satmove = ( fabs(tof - satfrq) >= 0.1 );
  
/* construct a command line for Pbox */

   if((getval("arraydim") < 1.5) || (ix==1))        /* execute only once */
   { sprintf(comm, "Pbox ad180 -u %s -w \"cawurst-10 %.1f/%.6f\" -s 1.0 -0\n",
             userdir, 1.0/pwx, 30*pwx);
     system(comm);                         /* create adiabatic 180 pulse */
     sprintf(comm, "Pbox av180 -u %s -w \"av180b %.6f\" -s 1.0 -0\n", userdir, av180);
     system(comm);                        /* create refocusing 180 pulse */
   }

/* LOAD VARIABLES */

  settable(t1, 1, phi1); settable(t2, 1, phi2);
  settable(t4, 2, phi4); settable(t5, 4, phi5);
  settable(t6, 4, phi6); settable(t8, 1, phi8);
  settable(t9, 8, phi9); settable(t10, 16, phi10);
  settable(t11, 1, phi11); settable(t31, 16, phi31);

/* INITIALIZE VARIABLES */
   if (j1xh != 0.0) 
    { tauxh = 1/(4*j1xh); tauxh1 = 1/(6*j1xh); }
   else 
    { tauxh = 1.80e-3; tauxh1 = 1.80e-3; }

   if ( jcc != 0.0 ) taucc = 1/(4*jcc);
   else  taucc = 1/(4*40);

/*  Phase incrementation for hypercomplex echo-antiecho 2D data */

    if (phase == 2)  
     { tsadd(t6,2,4); icosel=-1; }
    else icosel=1;
 
/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;                                               
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
     { tsadd(t9,2,4); tsadd(t31,2,4); }

/* Check constant-time conditions */
   if (kappa == 1.0)
      {
        maxni = (int) (taucc*sw1*2.0);
        if (maxni < ni) 
          {abort_message("too many increments! maxni = %d   ",maxni); }
      }

/* SET UP FOR (-90,180) PHASE CORRECTIONS IF f1180='y' */
   tau1 = d2;
   if(f1180[A] == 'y')  tau1 += ( 1.0/(2.0*sw1) ); /* Wait half a dwell time */
   tau1 = tau1/2.0;
   
/* BEGIN ACTUAL PULSE SEQUENCE */

 status(A);
   decpower(pwxlvl); decoffset(dof);     /* Set decoupler power to pwxlvl */

   delay(5.0e-5);
   if (getflag("sspul"))
	steadystate();

   if (satmode[0] == 'y')
     {
        if ((d1-satdly) > 0.02)
                delay(d1-satdly);
        else
                delay(0.02);
        if (getflag("slpsat"))
           {
                shaped_satpulse("relaxD",satdly,v4);
                if (getflag("prgflg"))
                   shaped_purge(v1,v4,v18,v19);
           }
        else
           {
                satpulse(satdly,v4,rof1,rof1);
                if (getflag("prgflg"))
                   purge(v1,v4,v18,v19);
           }

     }
   else
	delay(d1);

   if (getflag("wet"))
     wet4(zero,one);

 status(B);
   rcvroff(); 
   rgpulse(pw,t1,1.0e-6,0.0);    
   txphase(t1); decphase(t1);
   delay(tauxh-15.0*pwx);                        /* delay=1/4J(XH) */
   simshaped_pulse("","ad180",2*pw,30*pwx,t1,t1,0.0,0.0);
   txphase(t2); decphase(t4);
   delay(tauxh-15.0*pwx);
   simpulse(pw,pwx,t2,t4,0.0,0.0);
   decphase(t8);
   delay(taucc);
   decshaped_pulse("av180",av180,t8,0.0,0.0);
   decphase(t9); 
   delay(taucc);
   decrgpulse(pwx,t9,0.0,0.0);
   delay(tau1);
   rgpulse(2*pw,t1,1.0e-6,0.0);
   delay(tau1);
   delay(gt1 + gstab + 2*GRADIENT_DELAY - 2*pw -1.0e-6);
   decshaped_pulse("av180",av180,t11,1.0e-6,0.0);
   zgradpulse(gzlvl1, gt1);
   delay(gstab);
   decrgpulse(pwx*1.3333,t10,1.0e-6,0.0);   /* 120 Grad pulse */
   decphase(t11);
   delay(taucc - kappa*tau1);
   decshaped_pulse("av180",av180,t11,0.0,0.0);
   delay(10.0e-6);
   zgradpulse(gzlvl2,gt2);
   delay(gstab); 
   delay(taucc - 10.0e-6 - gstab - gt2 - 2*GRADIENT_DELAY - 1.0e-6);
   rgpulse(kappa*2*pw,t1,1.0e-6,0.0);
   delay(kappa*tau1); 
   simpulse(pw,pwx,t1,t5,1.0e-6,0.0);
   txphase(t1); decphase(t1);
   delay(tauxh1);                        /* delay=1/4J (XH);1/6J XH,XH2, XH3) */
   simshaped_pulse("","av180",2*pw,av180,t1,t1,0.0,0.0);  
   txphase(t2); decphase(t6);
   delay(tauxh1);
   simpulse(pw,pwx,t2,t6,0.0,0.0);
   txphase(t1); decphase(t1);
   delay(tauxh-15.0*pwx);
   simshaped_pulse("","ad180",2*pw,30*pwx,t1,t1,0.0,0.0);
   txphase(t1);
   delay(tauxh-15.0*pwx-0.5e-6);
   rgpulse(pw,t1,0.0,0.0);
   decpower(dpwr); decoffset(dofdec);  /* lower decoupler power for decoupling */
   delay(gt3 + gstab + 2*GRADIENT_DELAY-POWER_DELAY-1.0e-6);
   rgpulse(2*pw,t1,1.0e-6,0.0);
   zgradpulse(icosel*gzlvl3, gt3);
   delay(gstab);
   rcvron();
 status(C);
   setreceiver(t31);
}
