/*onepulsfs.c - A sequence to do direct polarization of quadruple nuclei with
                TPPM and SPINAL decoupling, using a single-frequency sweep
		pulse. 

                Used for testing SFS, DFS is recommended. 

                D.Rice 05/17/06 VNMRJ2.1A-B for the NMR SYSTEM
	        updated with new .h files D.Rice 10/12/05
                D.Rice 04/06/07 Updated to use SHAPES in solidstandard.h   
                D. Rice 11/10/08                                            */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,1,1,1};           // phXsfs
static int table2[4] = {0,2,1,3};           // phX90
static int table3[4] = {0,2,1,3};           // phRec

#define phXsfs t1
#define phX90 t2
#define phRec t3

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   SHAPE sfs = getsfspulse("sfsX",0.0,0.0,0,1);
   strncpy(sfs.pars.ch,"obs",3);
   putCmd("chXsfs='obs'\n");

   DSEQ dec = getdseq("H");
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHtppm='dec'\n"); 
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHspinal='dec'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwX90") + sfs.pars.t;
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

   settable(phXsfs,4,table1);
   settable(phX90,4,table2);
   settable(phRec,4,table3);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXsfs); decphase(zero);
   obspwrf(getval("aXsfs"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// Start Decoupling on H

   _dseqon(dec);

// X Double Frequency Sweep Pulse

   if (getval("pwXsfs") > 0.0) {
      _shape(sfs,phXsfs);
   }
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
