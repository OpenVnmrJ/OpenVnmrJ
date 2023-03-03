/*spsmall.c - Single-pulse sequence for testing the small angle
              phase shifter.

              Upgraded for SolidsPack 07/03/10 - D. Rice 

              D.Rice 07/18/05 VNMRJ2.1A-B for the NMR SYSTEM 
	      updated with new .h files D.Rice 10/12/05              

              Based on spsmall.c for INOVA - 02/20/02       */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,0,0,0};           // phX90
static int table2[4] = {0,0,0,0};           // phRec

#define phX90 t1
#define phRec t2

void pulsesequence() {

// Set the Maximum Dynamic Table Number

   setvvarnumber(10);

// Define Variables and Objects and Get Parameter Values

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
   d.dutyon = getval("pwX90");
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

   settable(phX90,4,table1);
   settable(phRec,4,table2);
   setreceiver(phRec);

// Set Small-Angle Phase

   double obsstep = 360.0/(PSD*8192);
   int phfX90 = initphase(getval("phfX90"),obsstep);
   obsstepsize(obsstep);

// Begin Sequence

   txphase(phX90); decphase(zero);
   xmtrphase(phfX90);
   obspwrf(getval("aX90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

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

