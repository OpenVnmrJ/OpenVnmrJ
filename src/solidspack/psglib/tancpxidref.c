/*tancpxidref.c - A sequence to form a constant, linear or tangent ramped CP,
                  followed by TOSS4 for sideband suppression.

             CEB 06/28/05 for VNMRJ2.1A-B for the NMR SYSTEM
             Edited  D. Rice 6/28/05
             Edited  D. Rice 10/12/05 tancpx.c
             Edited  D. Rice 01/31/06                                      */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,0,2};           // phH90
static int table2[4] = {0,0,1,1};           // phXhx
static int table3[4] = {1,1,1,1};           // phHhx
static int table4[4] = {1,3,2,0};           // phXref
static int table5[4] = {0,2,1,3};           // phRec

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define phXref t4
#define phRec t5

pulsesequence() {

// Set the Maximum Dynamic Table Number

   settablenumber(10);
   setvvarnumber(30);

// Define Variables and Objects and Get Parameter Values

   CP hx = getcp("HX",0.0,0.0,0,1);
   strncpy(hx.fr,"dec",3);
   strncpy(hx.to,"obs",3);
   putCmd("frHX='dec'\n"); 
   putCmd("toHX='obs'\n");

   WMPA ref = getidref("idrefX");
   strncpy(ref.ch,"obs",3);
   putCmd("chXidref='obs'\n");

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
   d.dutyon = getval("pwH90") + getval("tHX") + ref.pw;
   d.dutyoff = d1 + 4.0e-6  + ref.t1;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = 2.0*ref.rtau - ref.t1 - ref.pw + getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = 2.0*ref.rtau - ref.t1 - ref.pw + getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phH90,4,table1);
   settable(phXhx,4,table2);
   settable(phHhx,4,table3);
   settable(phXref,4,table4);
   settable(phRec,4,table5);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXhx); decphase(phH90);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to X Cross Polarization

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decphase(phHhx);
    _cp_(hx,phHhx,phXhx);

// Interrupted Decoupling

   _dseqon(dec);
   _idref(ref, dec, phXref);

// Begin Acquisition

   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

