/*phtran.c - phase detected pulse for phase transient tuning

             C. Bronnimann 3/30/05
	     Edited by D. Rice 2/6/06                                  */

#include <standard.h>
#include "solidstandard.h"

static int table1[4] = {0,0,0,0};  //phTune
static int table2[4] = {0,0,0,0};  //phRec

#define phTune t1
#define phRec t2

void pulsesequence(){

//Define Variables and Get Parameter Values

   double pwTune = getval("pwTune");
   pwTune = pwTune*6.0;
   at = pwTune*2.0;
   char atval[MAXSTR]; 
   sprintf(atval,"at = %f\n", at);
   putCmd(atval);
   int chTune = (int) getval("chTune");
   if ((chTune < 1) || (chTune > 4)) {
         abort_message("chTune(%d) must be between 1 and 4\n", chTune);
   }

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Set Phase Tables

   settable(phTune,4,table1);
   settable(phRec,4,table2);
   setreceiver(t2);

//Begin Sequence

   obspwrf(getval("aTune"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2e-6); sp1off(); delay(2.0e-6);

//Begin Phase Detected Pulse

   set4Tune(chTune,getval("gain"));
   delay(1.0e-4);
   ShapedXmtNAcquire("phtran",pwTune,phTune,6.0e-6,chTune);
   obsunblank(); decunblank(); _unblank34();
}

