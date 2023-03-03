/*hetcortancp2d.c - A sequence to provide correlation between the
               X chemical-shift and 1H- chenmical shift obtained with
	       FSLG. Uses a nonselective ramped CP for mixing.

               D.Rice 12/15/05
	       V.Zorin 24/01/13                                       */

#include "solidstandard.h"
#include <standard.h>

// Define Values for Phasetables

static int table1[4] = {1,1,1,1};           // phHtilt
static int table2[4] = {0,0,0,0};           // phHfslg
static int table3[4] = {1,1,1,1};	    // phHtilt2 
static int table4[4] = {0,1,0,1};           // phXhx
static int table5[4] = {0,0,2,2};           // phHhx
static int table6[4] = {0,1,2,3};           // phRec

#define phHtilt t1
#define phHfslg t2
#define phHtilt2 t3
#define phXhx t4
#define phHhx t5
#define phRec t6

static double d2_init;

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   MPSEQ fh = getfslg("fslgH",0,0.0,0.0,0,1);
   strcpy(fh.ch,"dec");
   putCmd("chHfslg='dec'\n");

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
   d.dutyon = getval("pwH90") + getval("pwHtilt") + d2_ + getval("tHX");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phHtilt,4,table1);
   settable(phHfslg,4,table2);
   settable(phHtilt2,4,table3);
   settable(phXhx,4,table4);
   settable(phHhx,4,table5);
   settable(phRec,4,table6);
   setreceiver(phRec);

//STATES

   if (phase1 == 2) tsadd(phHhx,3,4);

//  Begin Sequence

   txphase(phXhx); decphase(phHtilt);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));
   obsunblank();decunblank();_unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H Preparation tilt

   decrgpulse(getval("pwHtilt"),phHtilt,0.0,0.0);

// FSLG spinlock on H and tilt to XY-plane

   _mpseqon(fh,phHfslg);
   delay(d2);
   _mpseqoff(fh);
   decpwrf(getval("aH90"));
   decrgpulse(getval("pwH90")-getval("pwHtilt"),phHtilt2,0.0,0.0);

// Ramped H to X Cross Polarization

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
