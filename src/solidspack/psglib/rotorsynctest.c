/*onepul.c - A sequence to do direct polarization with TPPM and SPINAL
             decoupling.

             D.Rice 07/18/05 VNMRJ2.1A-B for the NMR SYSTEM 
	     updated with new .h files D.Rice 10/12/05              */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,1,3};           // phX90
static int table2[4] = {0,2,1,3};           // phRec

#define phX90 t1
#define phRec t2

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   initval(getval("periods"),v2); 

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Set Phase Tables

   settable(phX90,4,table1);
   settable(phRec,4,table2);
   setreceiver(phRec);

// Begin Sequence

   txphase(phX90); decphase(zero);
   obspwrf(getval("aX90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);

   xgate(1.0);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// Apply a Rotorsync Delay

   rgpulse(getval("pwX90"),phX90,0.0,0.0);
   rotorsync(v2);
   rgpulse(getval("pwX90"),phX90,0.0,0.0);
   xgate(getval("xperiods")); 
   rgpulse(getval("pwX90"),phX90,0.0,0.0);
   delay(10.0e-6); 

// X Direct Polarization

   rgpulse(getval("pwX90"),phX90,0.0,0.0);

// Begin Acquisition

   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   obsunblank(); decunblank(); _unblank34();
}

