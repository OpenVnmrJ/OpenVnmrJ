/*pisematest2.c - A calibration experiment for PISEMA that tests
                  the performance of an FSLG spin-lock, using X
                  detection through CP. X decoupling during FSLG
                  if aXlock > 0.0.

                  D. Rice 06-22-07                                    */

#include "solidstandard.h"
#include <standard.h>

// Define Values for Phasetables

static int table1[4] = {0,2,0,2};           // phH90
static int table2[4] = {2,2,2,2};           // ph1Htilt
static int table3[4] = {1,1,1,1};           // phHfslg
static int table4[4] = {0,0,0,0};           // ph2Htilt
static int table5[4] = {3,3,0,0};           // phXhx
static int table6[4] = {3,3,3,3};           // phHhx
static int table7[4] = {1,3,2,0};           // phRec

#define phH90 t1
#define ph1Htilt t2
#define phHfslg t3
#define ph2Htilt t4
#define phXhx t5
#define phHhx t6
#define phRec t7

static double d2_init;

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   double aXlock = getval("aXlock");  // define X decoupling during FSLG
       
   CP hx = getcp("HX",0.0,0.0,0,1);
   strcpy(hx.fr,"dec");
   strcpy(hx.to,"obs");
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");
   
   MPSEQ fh = getfslg("fslgH",0,0.0,0.0,0,1);
   strcpy(fh.ch,"dec");
   putCmd("chHfslg='dec'\n");

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
   d.dutyon = getval("pwH90") + getval("tHX") + 2.0*getval("pwHtilt") + d2_;
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
   settable(ph1Htilt,4,table2);
   settable(phHfslg,4,table3);
   settable(ph2Htilt,4,table4);
   settable(phXhx,4,table5);
   settable(phHhx,4,table6);
   settable(phRec,4,table7);
   setreceiver(phRec);

//  Begin Sequence

   txphase(phXhx); decphase(phH90);
   obspwrf(aXlock); decpwrf(getval("aH90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H Polarization

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);

// Tilt Pulse on H

   decrgpulse(getval("pwHtilt"),ph1Htilt,0.0,0.0);

// FSLG Spinlock on H with X CW Decouple

   if (aXlock > 0.0) xmtron();
   _mpseqon(fh,phHfslg);
   delay(d2);
   _mpseqoff(fh);
   if (aXlock > 0.0) xmtroff();
   obspwrf(getval("aXhx"));

// Tilt-Back Pulse on H

   decrgpulse(getval("pwHtilt"),ph2Htilt,0.0,0.0);

// H to X Cross Polarization

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
