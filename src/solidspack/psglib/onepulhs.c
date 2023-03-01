/*onepulhs.c - A sequence to do direct polarization with TPPM and SPINAL
             decoupling following enhancement with a hyperbolic secant 
             inversion pulse.

             D.Rice 07/18/05 VNMRJ2.1A-B for the NMR SYSTEM
	     updated with new .h files D.Rice 10/12/05              */

#include "standard.h"
#include "solidstandard.h"
#include "pboxpulse.h"

// Define Values for Phasetables

static int table1[4] = {0,0,0,0};           // phXshp1
static int table2[4] = {0,2,1,3};           // phX90
static int table3[4] = {0,2,1,3};           // phRec

#define phXshp1 t1
#define phX90 t2
#define phRec t3

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   PBOXPULSE shp1 = getpboxpulse("shp1X",0,1);
   strcpy(shp1.ch,"obs");
   putCmd("chHshp1 ='obs'\n");
   shp1.t1 = 0.0;
   shp1.t2 = 0.0;

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
   d.dutyon = getval("pwX90") + shp1.pw;
   d.dutyoff = d1 + 4.0e-6 + 200e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);

// Set Phase Tables

   settable(phXshp1,4,table1);
   settable(phX90,4,table2);
   settable(phRec,4,table3);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXshp1); decphase(zero);
   obspwrf(getval("aXshp1"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// Start Decoupling on H

   _dseqon(dec);

// X Hyperbolic Secant Pulse

   _pboxpulse(shp1,phXshp1);
   txphase(phX90);
   obspower(getval("tpwr")); 
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

