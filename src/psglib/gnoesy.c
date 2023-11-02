// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* gnoesy - made from gtnnoesy.c -
   made from tnnoesy -  2D cross relaxation experiment.  It can be performed in
            either phase-sensitive or absolute value mode.  Either
            TPPI or the hypercomplex method can be used to achieve
            F1 quadrature in a phase-sensitive presentation.  No
            attempt is made to suppress J-cross peaks in this pulse
            sequence.
                TRANSMITTER SOLVENT SATURATION ONLY
                ASSUMES ON-RESONANCE SOLVENT (tof is at solvent position)

 
        gzlvl1 = gradient amplitude (-32768 to +32768; use 30000)
        gt1 = gradient time (duration) in seconds (0.003)   
        gzlvl2 = gradient amplitude (-32768 to +32768; use 30000)
        gt2 = gradient time (duration) in seconds (0.012)   
        gzlvl3 = gradient amplitude (-32768 to +32768; use 30000)
        gt3 = gradient time (duration) in seconds (0.003)   
        grise = gradient rise and fall time (in seconds; 0.00001)
        gstab = optional delay for stability (in seconds)   
        phase = 1 (selects echo N-type coherence selection; default) 
              = 2 (selects antiecho P-type coherence selection) 
              = 1,2 (selects phase sensitive acquisition (N,P))
	d1  =  (if too short, you lose signal intensity,
                     but you don't seem to create artifacts)
	Phase cycling: can run all pulses at zero phase,
                     but you may get a bit of t1=0 axial peaks
 
           for working with organic samples try:
                gzlvl1 = gzlvl2 = gzlvl3 = 10000
                gt1 = gt3 = 0.003
                gt2 = 0.012    

           for working with H2O sample try:
                gzlvl1 = gzlvl2 = gzlvl3 = 30000
                gt1 = gt3 = 0.003
                gt2 = 0.012
  PROCESSING: 
        process N-type data with wft2d(1,0,0,1)
        process P-type data with wft2d(1,0,0,-1)
                   the ('t2dc') argument to wft2d may be useful
 
        process phase sensitive data (phase = 1,2) with:
                wft2d(1,0,0,1,0,1,1,0) (wft2dnp)
 

	PAKeifer 920924 - made gnoesy from gtnnoesy
	PAK 921027 - additional documentation
*/


#include <standard.h>

static int ph1[2] = {0,2};
static int ph2[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2};
static int ph3[16] = {0,0,1,1,2,2,3,3,0,0,1,1,2,2,3,3};
static int ph4[16] = {0,2,1,3,2,0,3,1,2,0,3,1,0,2,1,3};

void pulsesequence()
{
   double          mix, 
                   gzlvl1,gt1,
                   gzlvl2,gt2,
                   gzlvl3,gt3,
                   grise,gstab;
   int             iphase,icosel;
   char            sspul[MAXSTR];


/* LOAD VARIABLES */
   mix = getval("mix");
   iphase = (int) (getval("phase") + 0.5);
   getstr("sspul", sspul);
  gzlvl1 = getval("gzlvl1");
  gt1 = getval("gt1");
  gzlvl2 = getval("gzlvl2");
  gt2 = getval("gt2");
  gzlvl3 = getval("gzlvl3");
  gt3 = getval("gt3");
  grise = getval("grise");
  gstab = getval("gstab");

  icosel = 1; 
  if (iphase == 2)
   icosel = -1;


   settable(t1, 2, ph1);
   settable(t2, 16, ph2);
   settable(t3, 16, ph3);
   settable(t4, 16, ph4);


/* CHECK CONDITIONS */
   if ((rof1 < 9.9e-6) && (ix == 1))
      fprintf(stdout,"Warning:  ROF1 is less than 10 us\n");


   assign(zero,oph);  

/* BEGIN THE ACTUAL PULSE SEQUENCE */
   status(A);
      if (sspul[A] == 'y')
       {
        rgpulse(200*pw, zero, rof1, 0.0e-6);
        rgpulse(200*pw, one, 0.0e-6, rof1);
       }
      delay(d1);
   status(B);
      rcvroff();
      rgpulse(pw, t1, rof1, 1.0e-6);
   delay(gt1 + grise + grise + 24.4e-6 - 1.0e-6 - rof1);
   rgpulse(pw*2.0, t1, rof1, 1.0e-6);
        if (d2 > 0.0)
         delay(d2 - 1.0e-6 - rof1 - (4.0*pw/3.14159265358979323846));
     zgradpulse(gzlvl1,gt1+grise);
     delay(grise);

      rgpulse(pw, t2, rof1, 1.0e-6);
   status(C);
     zgradpulse(gzlvl2,gt2+grise);
     delay(grise);
      hsdelay(mix-gt2);
   status(D);
      rgpulse(pw, t3, rof1, rof2);
     delay(gt3 + grise + grise + gstab + 24.4e-6 - rof2 - rof1);
      rgpulse(pw*2.0, t3, rof1, rof2);
     zgradpulse(gzlvl3*icosel,gt3+grise);
     rcvron();
     delay(grise);
     delay(gstab);
     setreceiver(t4);
}
