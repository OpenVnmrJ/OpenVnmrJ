/*tancpxt1.c - A sequence to measure the X inversion recovery T1 with a preparation
             using a constant, linear or tangent ramped CP, with SPINAL or TPPM 
             decoupling.
                                                            
             The 8-member phase cycle produces an exponential decay with a time 
             constant T1.                                                           */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[8] = {0,2,2,0,0,2,2,0};     // phH90
static int table2[8] = {0,0,0,0,1,1,1,1};     // phXhx
static int table3[8] = {1,1,1,1,1,1,1,1};     // phHhx
//static int table4[8] = {2,2,0,0,3,3,1,1};     // ph1X90
static int table4[8] = {3,3,1,1,0,0,2,2};     // ph1X90
static int table5[8] = {2,2,0,0,3,3,1,1};     // ph2X90
static int table6[8] = {0,2,2,0,1,3,3,1};     // phRec

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define ph1X90 t4
#define ph2X90 t5
#define phRec t6

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
   d.dutyon = getval("pwH90") + getval("tHX") + 2.0*getval("pwX90");
   d.dutyoff = d1 + 4.0e-6 + d2;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phH90,8,table1);
   settable(phXhx,8,table2);
   settable(phHhx,8,table3);
   settable(ph1X90,8,table4);
   settable(ph2X90,8,table5);
   settable(phRec,8,table6);
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

// X Inversion with T1 Recovery and Detection

   obspwrf(getval("aX90"));
   rgpulse(getval("pwX90"),ph1X90,0.0,0.0);
   txphase(phXhx);
   obsunblank();
   delay(d2);
   rgpulse(getval("pwX90"),ph2X90,0.0,0.0);

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

