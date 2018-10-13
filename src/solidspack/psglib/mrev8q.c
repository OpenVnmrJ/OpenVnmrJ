/*mrev8q.c - A sequence to perform MREV8 with windowed aquisition and 
             quadrature detection. 

             D.Rice 01/12/06                                        */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,1,1,1};           // pw90Xprep
static int table2[4] = {0,1,0,-1};          // pw45Xprep
static int table3[4] = {0,1,2,3};           // phXprep
static int table4[4] = {0,0,0,0};           // phXmrev8
static int table5[4] = {0,1,2,3};           // phRec

#define pw90Xprep t1
#define pw45Xprep t2
#define phXprep t3
#define phXmrev8 t4
#define phRec t5

#define pwXprep v3
#define adXprep v4

pulsesequence() {

// Set the Maximum Dynamic Table and v-var Numbers

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
 
   settable(phXprep,4,table3);  
   settable(phXmrev8,4,table4);  
   settable(phRec,4,table5);
   setreceiver(phRec);

// Set the Small-Angle Prep Phase

   double obsstep = 360.0/(PSD*8192);
   obsstepsize(obsstep);
   int phfXprep = initphase(getval("phXprep"), obsstep); 
   int phXzero = initphase(0.0, obsstep);

// Set the Realtime pwXprep adXprep

   settable(pw90Xprep,4,table1);
   settable(pw45Xprep,4,table2); 
   int phase90 = (int) (getval("pwXprep")/0.0125e-6);
   int phase45 = (int) (getval("pwXprep")*45.0/(90.0*0.0125e-6));
   tsmult(pw90Xprep,phase90,0);
   tsmult(pw45Xprep,phase45,0);
   getelem(pw90Xprep,ct,v1);
   getelem(pw45Xprep,ct,v2);
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
   delay(mrev8.r1);
   vdelay(NSEC,adXprep);  // Keep Total Time Constant
   xmtron();    
   vdelay(NSEC,pwXprep);
   xmtroff();
   xmtrphase(phXzero);
   delay(mrev8.r2);               
   
// Apply MREV8 Cycles

   decblank(); _blank34(); 
   _mrev8(mrev8, phXmrev8);
   endacq();
   obsunblank(); decunblank(); _unblank34();  
}
