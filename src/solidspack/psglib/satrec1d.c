/*satrec1d.c - A sequence to measure T1 by saturation recovery using direct
             polarization with TPPM and SPINAL decoupling during acquisition.

	     V. Zorin	04/28/09                                           */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,0,0,0};           // phXsat
static int table2[4] = {0,2,1,3};           // phX90
static int table3[4] = {0,2,1,3};           // phRec

#define phXsat t1
#define phX90 t2
#define phRec t3

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   MPSEQ sat = getsat("satX",0.0,0.0,0,1);
   strcpy(sat.ch,"obs");
   putCmd("chXsat='obs'\n"); 

   DSEQ dec = getdseq("H");
   strcpy(dec.t.ch,"dec");
   putCmd("chHtppm='dec'\n"); 
   strcpy(dec.s.ch,"dec");
   putCmd("chHspinal='dec'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("qXsat")*getval("pwXsat") + getval("pwX90");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phXsat,4,table1);
   settable(phX90,4,table2);
   settable(phRec,4,table3);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXsat); decphase(zero);
   obspwrf(getval("aXsat"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// X saturation pulses

   _mpseq(sat,phXsat);
   txphase(phX90);
   obspwrf(getval("aX90"));
   delay(d2);

// X Direct Polarization

   rgpulse(getval("pwX90"),phX90,0.0,0.0);

// Begin Acquisition

   _dseqon(dec);
   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

