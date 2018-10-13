/*hetcorlgcp2d.c - A sequence to provide correlation between the
               X chemical-shift and H chemical shift obtained with
	       FSLG. Uses a nonselective ramped CP for mixing.
               
               D.Rice 12/15/05                                  */

#include "solidstandard.h"
#include <standard.h>

// Define Values for Phasetables

static int table1[4] = {1,1,1,1};           // phHtilt
static int table2[4] = {0,0,0,0};           // phHfslg
static int table3[4] = {3,3,3,3};           // phHtilt2
static int table4[4] = {1,1,1,1};           // phH90
static int table5[4] = {1,1,3,3};           // phHtilt3 
static int table6[4] = {0,1,0,1};           // phXhx 
static int table7[4] = {0,0,2,2};           // phHhx 
static int table8[4] = {0,1,2,3};           // receiver

#define phHtilt t1
#define phHfslg t2
#define phHtilt2 t3
#define phH90 t4
#define phHtilt3 t5
#define phXhx t6
#define phHhx t7
#define phRec t8

static double d2_init;

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   double pwX90 = getval("pwX90");
   double d22 = d2/2.0 - pwX90;
   if (d22 < 0.0) d22 = 0.0;

   MPSEQ fh = getfslg("fslgH",0,0.0,0.0,0,1);
   strncpy(fh.ch,"dec",3);
   putCmd("chHfslg='dec'\n");

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
   d.dutyon = 3.0*getval("pwHtilt") + d2_ + getval("pwH90") + getval("tHX");
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
   settable(phH90,4,table4);
   settable(phHtilt3,4,table5);
   settable(phXhx,4,table6);
   settable(phHhx,4,table7);
   settable(phRec,4,table8);

//Add STATES TPPI ("States with "FAD")

   tsadd(phRec,2*d2_index,4);     
   if (phase1 == 2) {
      tsadd(phHtilt3,2*d2_index+3,4);
      tsadd(phHhx,2*d2_index+3,4);
   }
   else {
      tsadd(phHtilt3,2*d2_index,4);
      tsadd(phHhx,2*d2_index,4);
   }
   setreceiver(phRec);

//  Begin Sequence

   txphase(phXhx); decphase(phHtilt);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));
   obsunblank();decunblank();_unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H Preparation with a Tilt Pulse

   decrgpulse(getval("pwHtilt"),phHtilt,0.0,0.0);

// FSLG spinlock on H and Reverse Tilt to Zed

   _mpseqon(fh,phHfslg);
   delay(d22);
   rgpulse(2.0*pwX90,zero,0.0,0.0);
   txphase(phXhx);
   delay(d22);
   _mpseqoff(fh);
   decpwrf(getval("aH90"));
   decrgpulse(getval("pwHtilt"),phHtilt2,0.0,0.0);

// H 90 and Ramped H to X Cross Polarization with LG Offset

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decrgpulse(getval("pwHtilt"),phHtilt3,0.0,0.0);
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
