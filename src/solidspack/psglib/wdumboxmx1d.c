/*wdumboxmx1d.c - A sequence to perform DUMBO with windowed aquisition
               and quadrature detection. Uses a Z-rotation supercycle. 

               D.Rice 03/26/09                                      */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {2,0,1,3};           // phXprep
static int table2[4] = {0,0,0,0};           // phXwdumbo
static int table3[4] = {2,0,1,3};           // phRec

#define phXprep t1
#define phXwdumbo t2
#define phRec t3

pulsesequence() {

// Set the Maximum Dynamic Table and v-var Numbers

   settablenumber(10);
   setvvarnumber(30);

// Define Variables and Objects and Get Parameter Values
 
   double aXprep = getval("aXprep");
   double pwXprep = getval("pwXprep");
   double phvXprep = getval("phXprep");

   WMPA wdumbo = getwdumboxmx("wdumboX");
   strncpy(wdumbo.ch,"obs",3);
   putCmd("chXwdumbo='obs'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwXprep") + wdumbo.q*wdumbo.cycles*wdumbo.pw;
   d.dutyoff = d1 + 4.0e-6 +  5.0e-6 + wdumbo.r1 + wdumbo.r2 + 
               at - wdumbo.q*wdumbo.cycles*wdumbo.pw;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phXprep,4,table1);
   settable(phXwdumbo,4,table2);
   settable(phRec,4,table3);
   setreceiver(phRec);

// Set the Small-Angle Step

   double obsstep = 360.0/(PSD*8192);
   obsstepsize(obsstep);
   int phfXprep = initphase(phvXprep, obsstep);
   int phXzero = initphase(0.0, obsstep);

// Begin Sequence

   xmtrphase(phfXprep); txphase(phXprep);
   obspwrf(aXprep);
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// Standard 90-degree prepX pulse for DUMBOxmx

   startacq(5.0e-6);
   rcvroff();
   delay(wdumbo.r1);
   rgpulse(pwXprep, phXprep, 0.0, 0.0);
   xmtrphase(phXzero);
   delay(wdumbo.r2);

// Apply WDUMBO

   decblank(); _blank34();
   _wdumbo(wdumbo, phXwdumbo);
   endacq();
   obsunblank(); decunblank(); _unblank34();
}
