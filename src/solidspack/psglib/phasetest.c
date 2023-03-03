/*phasetest.c - A sequence to test phase transitions of two
                back-to-back waveforms. 

                                                              */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,0,0,0};           // ph1X901
static int table2[4] = {1,1,1,1};           // ph2X902
static int table3[4] = {0,2,1,3};           // phRec

#define phX901 t1
#define phX902 t2
#define phRec t3

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   SHAPE p1 = getpulse("901X",0.0,0.0,0,1);
   strcpy(p1.pars.ch,"obs");
   putCmd("chX901='obs'\n");

   SHAPE p2 = getpulse("902X",0.0,0.0,0,1);
   strcpy(p1.pars.ch,"obs");
   putCmd("chX902='obs'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");
  
// Dutycycle Protection
   DUTY d = init_dutycycle();
   d.dutyon = getval("pwX901") + getval("pwX902");
   d.dutyoff = d1 + 4.0e-6;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phX901,4,table1);
   settable(phX902,4,table2);
   settable(phRec,4,table3);
   setreceiver(phRec);

// Begin Sequence

   txphase(phX901);
   obspwrf(getval("aX901"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// Back-to-Back Pulses

   rgpulse(getval("pwX901"),phX901,0.0,0.0);
   txphase(phX901);
   obspwrf(getval("aX902"));
   obsunblank(); 
   rgpulse(getval("pwX902"),phX902,0.0,0.0);

// Begin Acquisition
  
   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   obsunblank(); decunblank(); _unblank34();
}

