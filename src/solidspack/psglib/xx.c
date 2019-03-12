/*xx.c - A sequence to perform X pulses with windowed acquisition.

         D. Rice 10/15/06                                       */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,0,2};    // phXprep
static int table2[4] = {0,0,0,0};    // phXxx
static int table3[4] = {0,2,0,2};    // phRec

#define phXprep t1
#define phXxx t2
#define phRec t3

void pulsesequence() {

// Set the Maximum Dynamic Table Number

   settablenumber(10);
   setvvarnumber(30);

// Define Variables and Objects and Get Parameter Values

   WMPA xx = getxx("xxX");
   strncpy(xx.ch,"obs",3);
   putCmd("chXxx='obs'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection
   DUTY d = init_dutycycle();
   d.dutyon = getval("pwXprep") + 2.0*xx.cycles*xx.pw;
   d.dutyoff = d1 + 4.0e-6 + 5.0e-6 + xx.r1 + xx.r2 + 
               at - 2.0*xx.cycles*xx.pw;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phXprep,4,table1);
   settable(phXxx,4,table2);
   settable(phRec,4,table3);
   setreceiver(phRec);

// Set the Small-Angle Prep Phase

   double obsstep = 360.0/(PSD*8192);
   obsstepsize(obsstep);
   int phfXprep = initphase(getval("phXprep"), obsstep);     
   int phXzero = initphase(0.0, obsstep);

// Begin Sequence

   xmtrphase(phfXprep); txphase(phXprep);
   obspwrf(getval("aXprep"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);  
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// Preparation Pulse with Initial Point

   startacq(5.0e-6);
   rcvroff();
   delay(xx.r1);
   rgpulse(getval("pwXprep"), phXprep, 0.0, 0.0);
   xmtrphase(phXzero);

// Apply Semi-windowless WHH4 Cycles

   decblank(); _blank34();
   _xx(xx, phXxx);
   endacq();
   obsunblank(); decunblank(); _unblank34();
}

