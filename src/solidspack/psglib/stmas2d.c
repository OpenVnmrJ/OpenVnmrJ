/*stmas2d.c -   A sequence to generate a two pulse STMAS 2D spectrum.

                The initial F1 delay subtracts the internal pulse widths for rotor
                synchronization.  This means that the d2 = 0.0 point is bad and that
                the first dwell should be greater than the sum of the pulses.  Use
                Linear Predicition to fix the first point or set d2 = one dwell.

                See: Ashbrooke, S.E., Wimperis, S.E. Progress in NMR Spectroscopy 45 (2004)
                     53-108.

                     Gan, Z. J. Am Chem. Soc., 122 (2000) 3242-3243.

                D. Rice 05/10/06                                                           */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,0,2};   // ph1Xstmas
static int table2[4] = {0,0,2,2};   // ph2Xstmas
static int table3[4] = {0,2,0,2};   // phRec

#define ph1Xstmas t1
#define ph2Xstmas t2
#define phRec t3

static double d2_init;

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   double pw1Xstmas = getval("pw1Xstmas"); // Get STMAS and echo pulsewidths for synchronization
   double pw2Xstmas = getval("pw2Xstmas"); // of the F1 delay

   double d2init = getval("d2");           // Adjust the F1 delay for the pw1 and pw2 pulses.
   double d2 = d2init - pw1Xstmas/2.0 - pw2Xstmas/2.0;
   if (d2 < 0.0) d2 = 0.0;

   DSEQ dec = getdseq("H");
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHtppm='dec'\n"); 
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHspinal='dec'\n");

// Set Constant-time Period for d2. 

   if (d2_index == 0) d2_init = getval("d2");
   double d2_ = (ni - 1)/sw1 + d2_init;
   putCmd("d2acqret = %f\n",roundoff(d2_,12.5e-9));
   putCmd("d2dwret = %f\n",roundoff(1.0/sw1,12.5e-9));

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pw1Xstmas") + getval("pw2Xstmas");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = d2_ + getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = d2_ + getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(ph1Xstmas,4,table1);
   settable(ph2Xstmas,4,table2);
   settable(phRec,4,table3);

   if (phase1 == 2) {
      tsadd(ph1Xstmas,3,4);
   }
   setreceiver(phRec);

// Begin Sequence

   txphase(ph1Xstmas); decphase(zero);
   obspower(getval("tpwr"));
   obspwrf(getval("aXstmas"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H Decoupler on Before STMAS

   _dseqon(dec);

// Two-Pulse STMAS

   rgpulse(getval("pw1Xstmas"),ph1Xstmas,0.0,0.0);
   txphase(ph2Xstmas);
   delay(d2);
   rgpulse(getval("pw2Xstmas"),ph2Xstmas,0.0,0.0);

// Begin Acquisition

   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

