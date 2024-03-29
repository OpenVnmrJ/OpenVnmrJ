/*wsam1d.c - A sequence to perform SAMn with windowed aquisition
              and quadrature detection.

              D.Rice 03/04/09                                   */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {2,0,3,1};           // phXprep
static int table2[4] = {0,0,0,0};           // phXwsam
static int table3[4] = {2,0,3,1};           // phRec

#define phXprep t1
#define phXwsam t2
#define phRec t3

void pulsesequence() {

// Set the Maximum Dynamic Table and v-var Numbers

   settablenumber(10);
   setvvarnumber(30);

// Define Variables and Objects and Get Parameter Values

   double aXprep = getval("aXprep");
   double pwXprep = getval("pwXprep");
   double phvXprep = getval("phXprep");

   WMPA wsam = getwsamn("wsamX");
   strcpy(wsam.ch,"obs"); 
   putCmd("chXwsam='obs'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwXprep") + wsam.cycles1*wsam.cycles*wsam.pw;
   d.dutyoff = d1 + 4.0e-6 + 5.0e-6 + wsam.r1 + wsam.r2 +
               at - wsam.cycles1*wsam.cycles*wsam.pw;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phXprep,4,table1);
   settable(phXwsam,4,table2);
   settable(phRec,4,table3);
   setreceiver(phRec);

// Set the Small-Angle Step

   double obsstep = 360.0/(PSD*8192);
   obsstepsize(obsstep);
   int phfXprep = initphase(phvXprep,obsstep);
   int phXzero = initphase(0.0,obsstep);

// Begin Sequence

   xmtrphase(phfXprep); txphase(phXprep);
   obspwrf(aXprep);
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// Standard 90-degree prepX pulse for SAM

   startacq(5.0e-6);
   rcvroff();
   delay(wsam.r1);
   rgpulse(pwXprep, phXprep, 0.0, 0.0);
   xmtrphase(phXzero);
   delay(wsam.r2);

// Apply WSAM Cycles

   decblank(); _blank34();
   _wsamn(wsam, phXwsam);
   endacq();
   obsunblank(); decunblank(); _unblank34();
}
