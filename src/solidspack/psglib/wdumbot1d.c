/*wdumbot1d.c - A sequence to perform DUMBO with windowed aquisition and tilt
                pulses instead of quadrature detection.

                D.Rice 05/18/06

                This sequence uses tilt pulses to reduce quadrature
                artifacts                                                   */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {2,0,2,0};           // phXprep
static int table2[4] = {0,0,0,0};           // phXwdumbot
static int table3[4] = {2,0,2,0};           // phRec

#define phXprep t1
#define phXwdumbot t2
#define phRec t3

void pulsesequence() {

// Set the Maximum Dynamic Table and v-var Numbers

   settablenumber(20);
   setvvarnumber(30);

// Define Variables and Objects and Get Parameter Values

   WMPA wdumbot = getwdumbot("wdumbotX");
   strncpy(wdumbot.ch,"obs",3);
   putCmd("chXwdumbot='obs'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwXprep") + wdumbot.cycles*(wdumbot.q*wdumbot.pw + 2.0*wdumbot.t1);
   d.dutyoff = d1 + 4.0e-6 +  5.0e-6 + wdumbot.r1 + wdumbot.r2 + 
               at - wdumbot.cycles*(wdumbot.q*wdumbot.pw + 2.0*wdumbot.t1);
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phXprep,4,table1);
   settable(phXwdumbot,4,table2);
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

// Preparation Pulse

   startacq(5.0e-6);
   rcvroff();
   delay(wdumbot.r1);
   rgpulse(getval("pwXprep"), phXprep, 0.0, 0.0);
   xmtrphase(phXzero);
   delay(wdumbot.r2);

// Apply WDUMBO with Tilt Pulses

   decblank(); _blank34(); 
   _wdumbot(wdumbot, phXwdumbot);
   endacq();
   obsunblank(); decunblank(); _unblank34();
}

