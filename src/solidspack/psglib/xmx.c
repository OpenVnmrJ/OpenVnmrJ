/*xmx.c - A sequence to perform alternating X and -X pulses with 
          windowed acquisition.

          D.Rice 03/26/06                                      */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,0,2};    // phXprep
static int table2[4] = {0,0,0,0};    // phXxmx
static int table3[4] = {0,2,0,2};    // phRec

#define phXprep t1
#define phXxmx t2
#define phRec t3

void pulsesequence() {

// Set the Maximum Dynamic Table Number

   settablenumber(10);
   setvvarnumber(30);

// Define Variables and Objects and Get Parameter Values

   WMPA xmx = getxmx("xmxX");
   strncpy(xmx.ch,"obs",3); 
   putCmd("chXxmx='obs'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwXprep") + 2.0*xmx.cycles*xmx.pw;
   d.dutyoff = d1 + 4.0e-6 + 5.0e-6 + xmx.r1 + xmx.r2 + 
               at - 2.0*xmx.cycles*xmx.pw;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phXprep,4,table1);
   settable(phXxmx,4,table2);
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
   delay(xmx.r1);
   rgpulse(getval("pwXprep"), phXprep, 0.0, 0.0);  
   xmtrphase(phXzero);

// Apply Semi-windowless WHH4 Cycles

   decblank(); _blank34();
   _xmx(xmx, phXxmx);
   endacq();
   obsunblank(); decunblank(); _unblank34();
}

