/*mrev8.c - A sequence to perform MREV8 with windowed aquisition.
 
              D.Rice 09/05/08                                       */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,0,2};    // phXprep 
static int table2[4] = {0,0,0,0};    // phXmrev8
static int table3[4] = {0,2,0,2};    // phRec

#define phXprep t1
#define phXmrev8 t2
#define phRec t3

void pulsesequence() {

// Set the Maximum Dynamic Table Number

   settablenumber(10);
   setvvarnumber(30);
  
// Define Variables and Objects and Get Parameter Values

   WMPA mrev8 = getmrev8("mrev8X");
   strncpy(mrev8.ch,"obs",3); 
   putCmd("chXmrev8='obs'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");
   
// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwXprep") + 24.0*mrev8.cycles*mrev8.pw;
   d.dutyoff = d1 + 4.0e-6 + 5.0e-6 + mrev8.r1 + mrev8.r2 + 
               at - 24.0*mrev8.cycles*mrev8.pw;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);
// Set Phase Tables
 
   settable(phXprep,4,table1); 
   settable(phXmrev8,4,table2);
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
   delay(mrev8.r1);    
   rgpulse(getval("pwXprep"), phXprep, 0.0, 0.0); 
   delay(mrev8.r2);          
   xmtrphase(phXzero);
   
// Apply MREV8 Cycles

   decblank(); _blank34(); 
   _mrev8(mrev8, phXmrev8);
   endacq();
   obsunblank(); decunblank(); _unblank34();  
}
