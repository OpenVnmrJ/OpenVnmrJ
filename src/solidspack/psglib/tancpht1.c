/*tancpht1.c - A sequence to measure the 1H T1 with detection through
               a constant, linear or tangent ramped CP.

               D. Rice 3/14/06                                     */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,0,0,0};           // phH180
static int table2[4] = {0,2,0,2};           // phH90
static int table3[4] = {0,0,1,1};           // phXhx
static int table4[4] = {1,1,1,1};           // phHhx
static int table5[4] = {0,2,1,3};           // phRec

#define phH180 t1
#define phH90 t2
#define phXhx t3
#define phHhx t4
#define phRec t5

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

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

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH180") + getval("pwH90") + getval("tHX");
   d.dutyoff = d1 + d2 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phH180,4,table1);
   settable(phH90,4,table2);
   settable(phXhx,4,table3);
   settable(phHhx,4,table4);
   settable(phRec,4,table5);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXhx); decphase(phH180);
   obspwrf(getval("aXhx")); decpwrf(getval("aH180"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H Inversion Pulse

   decrgpulse(getval("pwH180"),phH180,0.0,0.0);
   decphase(phH90);
   decpwrf(getval("aH90"));
   delay(d2);

// H to X Cross Polarization

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decphase(phHhx);
    _cp_(hx,phHhx,phXhx);

// Begin Acquisition

   _dseqon(dec);
   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

