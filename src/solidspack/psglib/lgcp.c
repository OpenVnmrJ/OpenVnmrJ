/*lgcp.c - A sequence to provide selective CP using an off-resonance
           Lee-Goldburg spinlock. lgcp can be used to provide a normal
           CP with pwHtilt = 0.0.  Use a 35 degree flip angle for a
           Lee-Goldburg CP.

           D.Rice 01/30/06                                          */

#include "solidstandard.h"
#include <standard.h>

// Define Values for Phasetables

static int table1[4] = {1,1,1,1};           // phH90
static int table2[4] = {1,1,3,3};           // phHtilt
static int table3[4] = {0,1,0,1};           // phXhx
static int table4[4] = {0,0,2,2};           // phHhx
static int table5[4] = {0,1,2,3};           // phRec

#define phH90 t1
#define phHtilt t2
#define phXhx t3
#define phHhx t4
#define phRec t5

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

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

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90") + getval("tHX") + getval("pwHtilt");
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
   settable(phHtilt,4,table2);
   settable(phXhx,4,table3);
   settable(phHhx,4,table4);
   settable(phRec,4,table5);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXhx); decphase(phH90);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));
   obsunblank();decunblank();_unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H90, 35-degree Tilt and H-to-X Cross Polarization with LG Offset

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
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

