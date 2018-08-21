/*trapdorycpx.c - Trapdor on H with Y modulation followed by CP to X
                  with SPINAL or TPPM decoupling.

             Edited  D. Rice 01/10/05                                */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,0,2};           // ph1Hhytrap
static int table2[4] = {0,0,0,0};           // phYhytrap
static int table3[4]=  {0,0,0,0};           // ph2Hhytrap
static int table4[4] = {0,0,1,1};           // phXhx
static int table5[4] = {1,1,1,1};           // phHhx
static int table6[4] = {0,2,1,3};           // phRec

#define ph1Hhytrap t1
#define phYhytrap t2
#define ph2Hhytrap t3
#define phXhx t4
#define phHhx t5
#define phRec t6

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
   d.dutyon = getval("pw1Hhytrap") + getval("pw2Hhytrap") + getval("tHX"); 
   d.dutyoff = d1 + 4.0e-6 + getval("t1HYtrap") + getval("t2HYtrap");
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(ph1Hhytrap,4,table1);
   settable(phYhytrap,4,table2);
   settable(ph2Hhytrap,4,table3);
   settable(phXhx,4,table4);
   settable(phHhx,4,table5);
   settable(phRec,4,table6);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXhx); decphase(ph1Hhytrap); dec2phase(phYhytrap);
   obspwrf(getval("aXhx")); decpwrf(getval("aHhytrap")); dec2pwrf(getval("aYhytrap"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// TRAPDOR on H with Y Modulation

   decrgpulse(getval("pw1Hhytrap"),ph1Hhytrap,0.0,0.0);
   decphase(ph2Hhytrap);
   decunblank();
   dec2on();
   delay(getval("t1HYtrap"));
   dec2off();
   decrgpulse(getval("pw2Hhytrap"),ph2Hhytrap,0.0,0.0);
   decphase(phHhx);
   decunblank();
   decphase(phHhx);
   decpwrf(getval("aHhx"));
   delay(getval("t2HYtrap"));

// H to X Cross Polarization

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

