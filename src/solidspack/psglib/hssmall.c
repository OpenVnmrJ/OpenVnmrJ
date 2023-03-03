/*hssmall.c - A sequence to perform a small angle phase test with
              HS90.

              D.Rice 09/05/08                                       */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,0,0,0};    // phXhssmall
static int table2[4] = {0,0,0,0};    // phRec

#define phXhssmall t1
#define phRec t2

void pulsesequence() {

// Set the Maximum Dynamic Table Number

   settablenumber(10);
   setvvarnumber(30);

// Define Variables and Objects and Get Parameter Values

   WMPA hs = gethssmall("hssmallX");
   strcpy(hs.ch,"obs");
   putCmd("chXhssmall='obs'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwXprep") + 8.0*hs.cycles*hs.pw;
   d.dutyoff = d1 + 4.0e-6 + 5.0e-6 + hs.r1 + hs.r2 + 
               at - 8.0*hs.cycles*hs.pw;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phXhssmall,4,table1);
   settable(phRec,4,table2);
   setreceiver(phRec);
   obsstepsize(360.0/(PSD*8192));

// Begin Sequence

   txphase(phXhssmall);
   obspwrf(getval("aXhssmall"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);  
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// Apply HSSMALL Cycles

   startacq(5.0e-6);
   rcvroff();
   decblank(); _blank34();
   _hssmall(hs, phXhssmall);
   endacq();
   obsunblank(); decunblank(); _unblank34();
}
