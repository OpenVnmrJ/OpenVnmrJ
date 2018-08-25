/*br24q.c - A sequence to perform BR24 with windowed aquisition.
            and quadrature detection.

            D.Rice 01/12/06                                   */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,1,1,1};           // pw90Xprep
static int table2[4] = {0,0,1,-1};          // pw55Xprep
static int table3[4] = {0,2,3,1};           // phXprep
static int table4[4] = {0,0,0,0};           // phXbr24
static int table5[4] = {0,2,3,1};           // phRec

#define pw90Xprep t1
#define pw55Xprep t2
#define phXprep t3
#define phXbr24 t4
#define phRec t5

#define pwXprep v3
#define adXprep v4

pulsesequence() {

// Set the Maximum Dynamic Table and v-var Numbers

   settablenumber(10);
   setvvarnumber(30);

// Define Variables and Objects and Get Parameter Values

   WMPA br24 = getbr24("br24X");
   strncpy(br24.ch,"obs",3);
   putCmd("chXbr24='obs'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwXprep") + 24.0*br24.cycles*br24.pw;
   d.dutyoff = d1 + 4.0e-6 +  5.0e-6 + br24.r1 + br24.r2 + 
               at - 24.0*br24.cycles*br24.pw;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phXprep,4,table3);
   settable(phXbr24,4,table4);
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
   delay(br24.r1);
   vdelay(NSEC,adXprep);  // Keep Total Time Constant
   xmtron();
   vdelay(NSEC,pwXprep);
   xmtroff();
   xmtrphase(phXzero);
   delay(br24.r2);

// Apply BR24 Cycles

   decblank(); _blank34();
   _br24(br24, phXbr24);
   endacq();
   obsunblank(); decunblank(); _unblank34();
}

