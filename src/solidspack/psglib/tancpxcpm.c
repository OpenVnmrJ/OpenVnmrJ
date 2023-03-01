/*tancpxs.c - A sequence to do cross polarization cpm (eDroopy)
              decoupling.

              D. Rice 10/26/20
                                                                 */
#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,0,2};           // phH90
static int table2[4] = {0,0,1,1};           // phXhx
static int table3[4] = {1,1,1,1};           // phHhx
static int table4[4] = {1,1,1,1};           // phHcpm
static int table5[4] = {0,2,1,3};           // phRec

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define phHcpm t4
#define phRec t5

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   int decmode = getval("decmode");

   CP hx = getcp("HX",0.0,0.0,0,1); 
   strcpy(hx.fr,"dec");
   strcpy(hx.to,"obs");
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   SHAPE cpm;
   if (decmode > 1) {
      cpm = getcpm("cpmH",0.0,0.0,0,1);
      strcpy(cpm.pars.ch,"dec");
      putCmd("chHcpm ='dec'\n");
   }

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90") + getval("tHX");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = 0;
   d.c1 = d.c1 + (decmode > 0);
   d.t1 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phH90,4,table1);
   settable(phXhx,4,table2);
   settable(phHhx,4,table3);
   settable(phHcpm,4,table4);
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

// Begin Acquisition

   if (decmode > 0){ decpwrf(getval("aHcpm")); decon(); if (decmode > 1){ _shapeon(cpm,phHcpm);}}
   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   if (decmode > 0){ decoff(); if (decmode > 1){ _shapeoff(cpm);}}
   obsunblank(); decunblank(); _unblank34();
}

