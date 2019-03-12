/*onepuldfs.c - A sequence to do direct polarization of quadruple nuclei with
                TPPM and SPINAL decoupling, using a double-frequency sweep
		pulse.

                D.Rice 05/17/06 VNMRJ2.1A-B for the NMR SYSTEM
	        updated with new .h files D.Rice 10/12/05
                D.Rice 04/06/07 Updated to use SHAPES in solidstandard.h   */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,1,1,1};           // phXdfs
static int table2[4] = {0,2,1,3};           // phX90
static int table3[4] = {0,2,1,3};           // phRec

#define phXdfs t1
#define phX90 t2
#define phRec t3

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   SHAPE dfs = getdfspulse("dfsX",0.0,0.0,0,1);
   strncpy(dfs.pars.ch,"obs",3);
   putCmd("chXdfs='obs'\n");

   DSEQ dec = getdseq("H");
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHtppm='dec'\n");
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHspinal='dec'\n");

   dump_shape(dfs);

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwX90") + dfs.pars.t;
   d.dutyoff = d1 + 4.0e-6 + 200.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phXdfs,4,table1);
   settable(phX90,4,table2);
   settable(phRec,4,table3);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXdfs); decphase(zero);
   obspwrf(getval("aXdfs"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// Start Decoupling on H

   _dseqon(dec);

// X Double Frequency Sweep Pulse

   _shape(dfs,phXdfs);
   txphase(phX90);
   obspwrf(getval("aX90"));
   delay(200.0e-6);

// X Direct Polarization

   rgpulse(getval("pwX90"),phX90,0.0,0.0);

// Begin Acquisition

   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}
