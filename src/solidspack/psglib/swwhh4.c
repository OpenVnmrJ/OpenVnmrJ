/*swwhh4.c - A sequence to perform semi-windowless WhHuHa4 with
             windowed aquisition.

             D.Rice 01/12/06                                          */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,1,1,1};           // pw90Xprep
static int table2[4] = {0,0,1,-1};          // pw55Xprep
static int table3[4] = {0,2,3,1};           // phXprep
static int table4[4] = {0,0,0,0};           // phXswwhh4
static int table5[4] = {0,2,3,1};           // phRec

#define pw90Xprep t1
#define pw55Xprep t2
#define phXprep t3
#define phXswwhh4 t4
#define phRec t5

#define pwXprep v3
#define adXprep v4

void pulsesequence() {

// Set the Maximum Dynamic Table Number

   settablenumber(10);
   setvvarnumber(30);
  
// Define Variables and Objects and Get Parameter Values

   WMPA swwhh4 = getswwhh4("swwhh4X");
   strncpy(swwhh4.ch,"obs",3); 
   putCmd("chXswwhh4='obs'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwXprep") + 4.0*swwhh4.cycles*swwhh4.pw;
   d.dutyoff = d1 + 4.0e-6 + 5.0e-6 + swwhh4.r1 + swwhh4.r2 +
               at - 4.0*swwhh4.cycles*swwhh4.pw;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phXprep,4,table3);
   settable(phXswwhh4,4,table4);
   settable(phRec,4,table5);
   setreceiver(phRec);

// Set the Small-Angle Prep Phase

   double obsstep = 360.0/(PSD*8192);
   obsstepsize(obsstep);
   int phfXprep = initphase(getval("phXprep"), obsstep);
   int phXzero = initphase(0.0, obsstep);

// Set the Realtime pwXprep adXprep

   settable(pw90Xprep,4,table1);
   settable(pw55Xprep,4,table2);
   int phase90 = (int) (getval("pwXprep")/0.0125e-6);
   int phase55 = (int) (getval("pwXprep")*54.7/(90.0*0.0125e-6));
   tsmult(pw90Xprep,phase90,0);
   tsmult(pw55Xprep,phase55,0);
   getelem(pw90Xprep,ct,v1);
   getelem(pw55Xprep,ct,v2);
   add(v1,v2,pwXprep);
   sub(v1,v2,adXprep);

// Begin Sequence

   xmtrphase(phfXprep); txphase(phXprep);
   obspwrf(getval("aXprep"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// Incremented Preparation Pulse

   startacq(5.0e-6);
   rcvroff();
   delay(swwhh4.r1);
   vdelay(NSEC,adXprep);  // Keep Total Time Constant
   xmtron();
   vdelay(NSEC,pwXprep);
   xmtroff();
   xmtrphase(phXzero);
   delay(swwhh4.r2);

// Apply Semi-windowless WHH4 Cycles

   decblank(); _blank34();
   _swwhh4(swwhh4, phXswwhh4);
   endacq();
   obsunblank(); decunblank(); _unblank34();
}

