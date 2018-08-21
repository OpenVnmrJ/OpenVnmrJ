/*onepulxyr.c - A sequence to do direct polarization of both X
                and Y with TPPM and SPINAL decoupling.

             D.Rice 07/18/05 VNMRJ2.1A-B for the NMR SYSTEM
	     updated with new .h files D.Rice 10/12/05

	     XY Receive D.Rice 12/31/07

	     Uses X obs, Y dec and H dec2. Use probeConnect to set
	     dec to the second observe and dec2 to highband              */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,1,3};           // phX90
static int table2[4] = {0,2,1,3};           // phY90
static int table3[4] = {0,2,1,3};           // phRec

#define phX90 t1
#define phY90 t2
#define phRec t3

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   extern int NUMch;

   DSEQ dec2 = getdseq("H");
   strncpy(dec2.t.ch,"dec2",3);
   putCmd("chHtppm='dec2'\n"); 
   strncpy(dec2.s.ch,"dec2",3);
   putCmd("chHspinal='dec2'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwX90");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec2.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec2.seq,"tppm")) && (dec2.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec2.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec2.seq,"spinal")) && (dec2.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phX90,4,table1);
   settable(phY90,4,table2);
   settable(phRec,4,table3);
   setreceiver(phRec);

// Begin Sequence

   txphase(phX90); decphase(phY90); decphase(zero);
   obspwrf(getval("aX90")); decpwrf(getval("aY90"));
   obsunblank(); decunblank(); dec2unblank(); if (NUMch > 3) dec3unblank();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// X and Y Direct Polarization

   simpulse(getval("pwX90"),getval("pwY90"),phX90,phY90,0.0,0.0);

// Begin Acquisition

   _dseqon(dec2);
   obsblank(); decblank(); if (NUMch > 3) dec3blank();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec2);
   obsunblank(); decunblank(); dec2unblank(); if (NUMch > 3) dec3unblank();
}

