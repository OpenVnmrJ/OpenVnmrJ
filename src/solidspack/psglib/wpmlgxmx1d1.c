/*wpmlgxmx1d1.c - A sequence to perform PMLG with windowed aquisition
              and quadrature detection. Uses a Z-rotation supercycle and shapes in the loops.
	      CAUTION: shapes in the loop require VNMRJ 3.1 or higher
		       use wmplgxmx1d.c for older versions

              V.Zorin 08/10
		      11/06/10                                 */

#include "standard.h"
#include "solidstandard.h"


// Define Values for Phasetables

static int table1[4] = {2,0,3,1};           // phXprep
static int table2[4] = {0,0,0,0};           // phXwpmlg
static int table3[4] = {2,0,3,1};           // phRec

#define phXprep t1
#define phXwpmlg t2
#define phRec t3

void pulsesequence() {

// Set the Maximum Dynamic Table and v-var Numbers

   settablenumber(10);
   setvvarnumber(30);

// Define Variables and Objects and Get Parameter Values

   double aXprep = getval("aXprep");
   double pwXprep = getval("pwXprep");
   double phvXprep = getval("phXprep");

   WMPSEQ wpmlg = getwpmlgxmx1("wpmlgX");
   strcpy(wpmlg.wvsh.mpseq.ch,"obs"); 
   putCmd("chXwpmlg='obs'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwXprep") + wpmlg.cycles*wpmlg.wvsh.mpseq.t;
   d.dutyoff = d1 + 4.0e-6 + 5.0e-6 + wpmlg.r1 + wpmlg.r2 +
               at - wpmlg.cycles*wpmlg.wvsh.mpseq.t;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phXprep,4,table1);
   settable(phXwpmlg,4,table2);
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

// Standard 90-degree prepX pulse for PMLGxmx"

   startacq(5.0e-6);
   rcvroff();
   delay(wpmlg.r1);
   rgpulse(pwXprep, phXprep, 0.0, 0.0);
   xmtrphase(phXzero);
   delay(wpmlg.r2);

// Apply WPMLG Cycles

   decblank(); _blank34();
   _wpmlg1(wpmlg, phXwpmlg);
   endacq();
   obsunblank(); decunblank(); _unblank34();
}
