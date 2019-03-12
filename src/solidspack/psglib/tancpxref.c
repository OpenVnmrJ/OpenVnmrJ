/*tancpxref.c - A sequence to form a constant, linear or tangent ramped CP
                followed by a shaped refocussing pulse, with TPPM or SPINAL
                decoupling.

                D. Rice 03/10/10                                 */

#include "standard.h"
#include "solidstandard.h"
#include "pboxpulse.h"

// Define Values for Phasetables

static int table1[8] = {2,2,2,2,2,2,2,2};           // phH90    
static int table2[8] = {0,3,0,3,2,1,2,1};           // phXhx    
static int table3[8] = {1,1,1,1,1,1,1,1};           // phHhx 
static int table4[8] = {0,0,1,1,0,0,1,1};           // phXshp1
static int table5[8] = {0,1,2,3,2,3,0,1};           // phRec 
#define phH90 t1
#define phXhx t2
#define phHhx t3
#define phXshp1 t4
#define phRec t5

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   CP hx = getcp("HX",0.0,0.0,0,1);
   strncpy(hx.fr,"dec",3);
   strncpy(hx.to,"obs",3);
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   PBOXPULSE shp1 = getpboxpulse("shp1X",0,1); 

   DSEQ dec = getdseq("H");    
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHtppm='dec'\n"); 
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHspinal='dec'\n");

   DSEQ mix = getdseq("Hmix");
   strncpy(mix.t.ch,"dec",3);
   putCmd("chHmixtppm='dec'\n");
   strncpy(mix.s.ch,"dec",3);
   putCmd("chHmixspinal='dec'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90") + getval("tHX") + shp1.pw;
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d.c3 = d.c3 + (!strcmp(mix.seq,"tppm"));
   d.c3 = d.c3 + ((!strcmp(mix.seq,"tppm")) && (mix.t.a > 0.0));
   d.t3 = getval("tau1") + getval("tau2");
   d.t3 = getval("tau1") + getval("tau2");
   d.c4 = d.c4 + (!strcmp(mix.seq,"spinal"));
   d.c4 = d.c4 + ((!strcmp(mix.seq,"spinal")) && (mix.s.a > 0.0));
   d.t4 = getval("tau1") + getval("tau2");
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phH90,8,table1);
   settable(phXhx,8,table2);
   settable(phHhx,8,table3);
   settable(phXshp1,8,table4);
   settable(phRec,8,table5);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXhx); decphase(phH90);
   obspower(tpwr);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to X Cross Polarization

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decphase(phHhx);
   _cp_(hx,phHhx,phXhx);

// Shaped Refocussing Pulse

   _dseqon(mix);
   delay(getval("tau1"));
   _pboxpulse(shp1,phXshp1);
   delay(getval("tau2"));
   _dseqoff(mix);

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

