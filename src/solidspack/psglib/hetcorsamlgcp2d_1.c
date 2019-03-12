/*hetcorsamlgcp2d_1.c - A sequence to provide correlation between the
               X chemical-shift and H chemical shift obtained with
	       SAMn. Uses an offset (Lee Goldburg) CP for mixing
               D.Rice 12/15/05

	       Allows offset SAM and tilt pulses to shift the F1
               offset without changing the LG CP and the decoupling
               offset.  The first two 90-degree pulses and the FSLG
               can be offset from dof with phase coherence.  All
               should use the same offset value. LG CP and decoupling
               are at the dof value.

               Use an offset to move the F1 spectrum away from the
               center glitch and to overcome the effects of phase
               transient.

               Add a mixing period tHmix for spin diffusion mixing of
               cross peaks.

               V. Zorin - D. Rice 03/03/09                           */

#include "solidstandard.h"
#include <standard.h>

// Define Values for Phasetables

static int table1[4] = {1,1,1,1};           // ph1H90
static int table2[4] = {0,0,0,0};           // phHsam
static int table3[4] = {3,3,3,3};           // ph2H90
static int table4[4] = {1,1,1,1};           // ph3H90
static int table5[4] = {1,1,3,3};           // phHtilt
static int table6[4] = {0,1,0,1};           // phXhx
static int table7[4] = {0,0,2,2};           // phHhx
static int table8[4] = {0,1,2,3};           // phRec

#define ph1H90 t1
#define phHsam t2
#define ph2H90 t3
#define ph3H90 t4
#define phHtilt t5
#define phXhx t6
#define phHhx t7
#define phRec t8

static double d2_init;

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   SHAPE p1 = getpulse("90H",0.0,0.0,1,0);
   strncpy(p1.pars.ch,"dec",3);
   putCmd("chH90='dec'\n");
   p1.pars.array = disarry("xx", p1.pars.array);
   p1 = update_shape(p1,0.0,0.0,1);

   MPSEQ sh = getsamn("samH",0,0.0,0.0,1,0);
   strncpy(sh.ch,"dec",3);
   putCmd("chHsam='dec'\n");
   double pwHsam = getval("pwHsam");
   sh.nelem = (int) (d2/(pwHsam) + 0.1);
   sh.array = disarry("xx", sh.array);
   sh = update_mpseq(sh,0,p1.pars.phAccum,p1.pars.phInt,1);  

   SHAPE p2 = getpulse("90H",0.0,0.0,2,0);
   strncpy(p2.pars.ch,"dec",3);
   putCmd("chH90='dec'\n");
   p2.pars.array = disarry("xx", p2.pars.array);
   p2 = update_shape(p2,sh.phAccum,sh.phInt,2);
   double pwX180 = getval("pwX180");
   double d22 = sh.t/2.0 - pwX180/2.0;
   if (d22 < 0.0) d22 = 0.0;

// CP hx and DSEQ dec Return to the Reference Phase

   CP hx = getcp("HX",0.0,0.0,0,1);
   strncpy(hx.fr,"dec",3);
   strncpy(hx.to,"obs",3);
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   DSEQ dec = getdseq("H");
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHtppm='dec'\n"); 
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHspinal='dec'\n");

// Set Constant-time Period for d2. 

   if (d2_index == 0) d2_init = getval("d2");
   double d2_ = (ni - 1)/sw1 + d2_init;
   putCmd("d2acqret = %f\n",roundoff(d2_,12.5e-9));
   putCmd("d2dwret = %f\n",roundoff(1.0/sw1,12.5e-9));

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = p1.pars.t + d2_ + p2.pars.t + getval("pwH90") + getval("pwHtilt") +
              getval("tHX");
   d.dutyoff = d1 + 4.0e-6 + getval("tHmix");
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(ph1H90,4,table1);
   settable(phHsam,4,table2);
   settable(ph2H90,4,table3);
   settable(ph3H90,4,table4);
   settable(phHtilt,4,table5);
   settable(phXhx,4,table6);
   settable(phHhx,4,table7);
   settable(phRec,4,table8);

//Add STATES TPPI ("States with "FAD")

   tsadd(phRec,2*d2_index,4);
   if (phase1 == 2) {
      tsadd(ph2H90,2*d2_index+3,4);
   }
   else {
      tsadd(ph2H90,2*d2_index,4);
   }
   setreceiver(phRec);

//  Begin Sequence

   txphase(phXhx); decphase(ph1H90);
   obspwrf(getval("aX180")); decpwrf(getval("aH90"));
   obsunblank();decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// Offset H Preparation with a Tilt Pulse

   _shape(p1,ph1H90);

// Offset SAMn Spinlock on H During F1 with Optional pwX180

   _mpseqon(sh,phHsam);
   delay(d22);
   rgpulse(pwX180,zero,0.0,0.0);
   obspwrf(getval("aX90"));
   txphase(phXhx);
   delay(d22);
   _mpseqoff(sh);

// Offset 90-degree Pulse to Zed and Spin-Diffusion Mix

   _shape(p2,ph2H90);
   decpwrf(getval("aH90"));
   delay(getval("tHmix"));

// H90, 35-degree Tilt and H-to-X Cross Polarization with LG Offset

   decrgpulse(getval("pwH90"),ph3H90,0.0,0.0);
   decunblank(); 
   decrgpulse(getval("pwHtilt"),phHtilt,0.0,0.0);
   decphase(phHhx);
   _cp_(hx,phHhx,phXhx);

// Begin Acquisition

   obsblank(); _blank34();
   _dseqon(dec);
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

