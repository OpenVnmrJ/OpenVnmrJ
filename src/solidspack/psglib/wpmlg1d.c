/*wpmlg1d.c - A sequence to perform PMLG with windowed aquisition
              and quadrature detection.

              D.Rice 04/15/06                                  */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {2,0,3,1};           // ph1Xprep1
static int table2[4] = {1,1,1,1};           // ph2Xprep1
static int table3[4] = {0,0,0,0};           // phXwpmlg
static int table4[4] = {2,0,3,1};           // phRec

#define ph1Xprep1 t1
#define ph2Xprep1 t2
#define phXwpmlg t3
#define phRec t4

void pulsesequence() {

// Set the Maximum Dynamic Table and v-var Numbers

   settablenumber(10);
   setvvarnumber(30);

// Define Variables and Objects and Get Parameter Values

   double aXprep1 = getval("aXprep1");  // Define Tilted Pulses using "prep1X".
   double pw1Xprep1 = getval("pw1Xprep1");
   double pw2Xprep1 = getval("pw2Xprep1");
   double phXprep1 = getval("phXprep1");

   WMPA wpmlg = getwpmlg("wpmlgX");
   strncpy(wpmlg.ch,"obs",3); 
   putCmd("chXwpmlg='obs'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pw1Xprep1") + getval("pw2Xprep1") + 2.0*wpmlg.q*wpmlg.cycles*wpmlg.pw;
   d.dutyoff = d1 + 4.0e-6 + 5.0e-6 + wpmlg.r1 + wpmlg.r2 + 
               at - 2.0*wpmlg.q*wpmlg.cycles*wpmlg.pw;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(ph1Xprep1,4,table1);
   settable(ph2Xprep1,4,table2);
   settable(phXwpmlg,4,table3);
   settable(phRec,4,table4);
   setreceiver(phRec);

// Set the Small-Angle Step

   double obsstep = 360.0/(PSD*8192);
   obsstepsize(obsstep);
   int phfXprep1 = initphase(phXprep1, obsstep);
   int phXzero = initphase(0.0, obsstep);

// Begin Sequence

   xmtrphase(phfXprep1); txphase(ph1Xprep1);
   obspwrf(aXprep1);
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// Tilted Preparation Pulse for FSLG or PMLG "prep1X"

   startacq(5.0e-6);
   rcvroff();
   delay(wpmlg.r1);
   rgpulse(pw1Xprep1, ph1Xprep1, 0.0, 0.0);
   rgpulse(pw2Xprep1, ph2Xprep1, 0.0, 0.0);
   xmtrphase(phXzero);
   delay(wpmlg.r2);

// Apply WPMLG Cycles

   decblank(); _blank34();
   _wpmlg(wpmlg, phXwpmlg);
   endacq();
   obsunblank(); decunblank(); _unblank34();
}
