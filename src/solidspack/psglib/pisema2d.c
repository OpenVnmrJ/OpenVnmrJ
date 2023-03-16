/*pisema2d.c - A sequence to provide correlation between the
               chemical-shift and 1H-dipolar interaction in the
	       spin-locked rotating frame.
               
               CEB 10/12/05 for VNMRJ2.1A-B for the NMR SYSTEM
               Edited  D. Rice 10/15/05                         */

#include "solidstandard.h"
#include <standard.h>

// Define Values for Phasetables

static int table1[4] = {0,2,0,2};           // phH90
static int table2[4] = {3,3,0,0};           // phXhx
static int table3[4] = {3,3,3,3};           // phHhx
static int table4[4] = {2,2,2,2};           // phHtilt
static int table5[4] = {1,1,1,1};           // phHfslg
static int table6[4] = {3,3,0,0};           // phXlock
static int table7[4] = {1,3,2,0};           // phRec

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define phHtilt t4
#define phHfslg t5
#define phXlock t6
#define phRec t7

static double d2_init;

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   CP hx = getcp("HX",0.0,0.0,0,1);
   strcpy(hx.fr,"dec");
   strcpy(hx.to,"obs");
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   MPSEQ fh = getfslg("fslgH",0,0.0,0.0,0,1);
   strcpy(fh.ch,"dec");
   putCmd("chHfslg='dec'\n");

   MPSEQ fx = getfslg("fslgX",0,0.0,0.0,0,1);
   strcpy(fx.ch,"obs");
   putCmd("chXfslg='obs'\n");

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
   d.dutyon = getval("pwH90") + getval("tHX") + d2_;
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

   settable(phH90,4,table1);
   settable(phXhx,4,table2);
   settable(phHhx,4,table3);
   settable(phHtilt,4,table4);
   settable(phHfslg,4,table5);
   settable(phXlock,4,table6);
   settable(phRec,4,table7);
   setreceiver(phRec);

//  Begin Sequence

   txphase(phXhx); decphase(phH90);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to X Cross Polarization

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decphase(phHhx);
   _cp_(hx,phHhx,phXhx);

// Tilt Pulse on H with Continued X Spinlock

   xmtron(); decon();
   decpwrf(getval("aH90")); obspwrf(getval("aXhx"));
   decrgpulse(getval("pwHtilt"),phHtilt,0.0,0.0);

// FSLG Spinlocks on X and H

   xmtron(); decon(); 
   _mpseqon(fh,phHfslg); _mpseqon(fx,phXlock);
   delay(d2);
   _mpseqoff(fh); _mpseqoff(fx);

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
