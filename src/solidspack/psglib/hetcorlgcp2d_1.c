/*hetcorlgcp2d_1.c - A sequence to provide correlation between the
               X chemical-shift and H chemical shift obtained with
	       FSLG. Uses an offset (Lee Goldburg) CP for mixing
               D.Rice 12/15/05

	       Allows offset FSLG and tilt pulses to shift the F1
               offset without changing the LG CP and the decoupling
               offset.  The first three tilt pulses and the FSLG
               can be offset from dof with phase coherence.  All
               should use the same offset value. LG CP and decoupling
               are at the dof value.

               Use an offset to move the F1 spectrum away from the
               center glitch and to overcome the effects of phase
               transient.

               Add a mixing period tHmix for spin diffusion mixing of
               cross peaks.

               D. Rice 09/12/08                                    */

#include "solidstandard.h"
#include <standard.h>

// Define Values for Phasetables

static int table1[4] = {1,1,1,1};           // ph1Htilt1
static int table2[4] = {0,0,0,0};           // phHfslg
static int table3[4] = {1,1,1,1};           // phHtilt2
static int table4[4] = {3,3,3,3};           // phHtilt3
static int table5[4] = {1,1,1,1};           // phH90
static int table6[4] = {1,1,3,3};           // ph2Htilt1
static int table7[4] = {0,1,0,1};           // phXhx
static int table8[4] = {0,0,2,2};           // phHhx
static int table9[4] = {0,1,2,3};           // phRec

#define ph1Htilt1 t1
#define phHfslg t2
#define phHtilt2 t3
#define phHtilt3 t4
#define phH90 t5
#define ph2Htilt1 t6
#define phXhx t7
#define phHhx t8
#define phRec t9

static double d2_init;

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   SHAPE p1 = getpulse("tilt1H",0.0,0.0,1,0);
   strcpy(p1.pars.ch,"dec");
   putCmd("chHtilt1='dec'\n");
   p1.pars.array = disarry("xx", p1.pars.array);
   p1 = update_shape(p1,0.0,0.0,1);

   MPSEQ fh = getfslg("fslgH",0,0.0,0.0,1,0);
   strcpy(fh.ch,"dec");
   putCmd("chHfslg='dec'\n");
   double pwHfslg = getval("pwHfslg");   
   fh.nelem = (int) (d2/(2.0*pwHfslg) + 0.1);
   fh.array = disarry("xx", fh.array);
   fh = update_mpseq(fh,0,p1.pars.phAccum,p1.pars.phInt,1);   

   SHAPE p2 = getpulse("tilt2H",0.0,0.0,1,0);
   strcpy(p2.pars.ch,"dec");
   putCmd("chHtilt2='dec'\n");
   p2.pars.array = disarry("xx", p2.pars.array);
   p2 = update_shape(p2,fh.phAccum,fh.phInt,1);

   SHAPE p3 = getpulse("tilt3H",0.0,0.0,1,0);
   strcpy(p3.pars.ch,"dec");
   putCmd("chHtilt3='dec'\n");
   p3.pars.array = disarry("xx", p3.pars.array);
   p3 = update_shape(p3,p2.pars.phAccum,p2.pars.phInt,1); 
   double pwX180 = getval("pwX180");
   double d22 = fh.t/2.0 - pwX180/2.0;
   if (d22 < 0.0) d22 = 0.0;

// CP hx and DSEQ dec Return to the Reference Phase

   CP hx = getcp("HX",0.0,0.0,0,1);
   strcpy(hx.fr,"dec");
   strcpy(hx.to,"obs");
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   DSEQ dec = getdseq("H");
   strcpy(dec.t.ch,"dec");
   putCmd("chHtppm='dec'\n"); 
   strcpy(dec.s.ch,"dec");
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
   d.dutyon = p1.pars.t + d2_ + p2.pars.t + p3.pars.t + getval("pwH90") + 
      getval("pwHtilt1") + getval("tHX");
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

   settable(ph1Htilt1,4,table1);
   settable(phHfslg,4,table2);
   settable(phHtilt2,4,table3);
   settable(phHtilt3,4,table4);
   settable(phH90,4,table5);
   settable(ph2Htilt1,4,table6);
   settable(phXhx,4,table7);
   settable(phHhx,4,table8);
   settable(phRec,4,table9);

//Add States TPPI ("States with "FAD")

   tsadd(phRec,2*d2_index,4);
   if (phase1 == 2) {
      tsadd(phHtilt3,2*d2_index+3,4);
   }
   else {
      tsadd(phHtilt3,2*d2_index,4);
   }
   setreceiver(phRec);

//  Begin Sequence

   txphase(phXhx); decphase(ph1Htilt1);
   obspwrf(getval("aX180")); decpwrf(getval("aHtilt1"));
   obsunblank();decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// Offset H Preparation with a Tilt Pulse

   _shape(p1,ph1Htilt1);

// Offset FSLG Spinlock on H During F1 with Optional pwX180

   _mpseqon(fh,phHfslg);
   delay(d22);
   rgpulse(pwX180,zero,0.0,0.0);
   obspwrf(getval("aX90"));
   txphase(phXhx);
   delay(d22);
   _mpseqoff(fh);

// Offset Continued Tilt to XY plane

   _shape(p2,phHtilt2);

// Offset 90-degree Pulse to Zed and Spin-Diffusion Mix

   _shape(p3,phHtilt3);
   decpwrf(getval("aH90"));
   delay(getval("tHmix"));

// H90, 35-degree Tilt and H-to-X Cross Polarization with LG Offset

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decpwrf(getval("aHtilt1"));
   decrgpulse(getval("pwHtilt1"),ph2Htilt1,0.0,0.0);
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

