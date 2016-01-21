/*onepulcpm.c - A sequence to do direct polarization with CPM (eDroopy)
                decoupling. 
   
                D. Rice 10/26/20

                                                                       */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,1,3};           // phX90
static int table2[4] = {0,0,0,0};           // phHcpm
static int table3[4] = {0,2,1,3};           // phRec

#define phX90 t1
#define phHcpm t2
#define phRec t3

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   int decmode = getval("decmode");
   SHAPE cpm;
   if (decmode > 1) {
      cpm = getcpm("cpmH",0.0,0.0,0,1);
      strncpy(cpm.pars.ch,"dec",3);
      putCmd("chHcpm ='dec'\n");
   }

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwX90");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = 0;
   d.c1 = d.c1 + (decmode > 0);
   d.t1 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phX90,4,table1);
   settable(phHcpm,4,table2);
   settable(phRec,4,table3);
   setreceiver(phRec);

// Begin Sequence

   txphase(phX90); decphase(zero);
   obspwrf(getval("aX90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// X Direct Polarization

   rgpulse(getval("pwX90"),phX90,0.0,0.0);

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

