/*stmasdqfspltse2d.c - A sequence to generate a two pulse STMAS 2D spectrum
                    with a third selective echo pulse and a fourth selective echo
                    for removal of the CT-CT correlation, using a split t1.

                    The initial F1 delay subtracts the internal pulse widths for rotor
                    synchronization.  This means that the d2 = 0.0 point is bad and that
                    the first dwell should be greater than the sum of the pulses.  Use
                    Linear Prediction to fix the first point or set d2 = one dwell.

                See: Ashbrooke, S.E., Wimperis, S.E. Progress in NMR Spectroscopy 45 (2004)
                     53-108. (Figure 45)

                D. Rice 05/10/06                                                         */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,0,0,0};          // ph1Xstmas
static int table2[8] = {0,0,1,1,2,2,3,3};  // ph1Xechsel
static int table3[8] = {0,1,0,1,0,1,0,1};  // phf1Xechsel
static int table4[32] = {0,0,1,1,2,2,3,3,
                         1,1,2,2,3,3,0,0,
                         2,2,3,3,0,0,1,1,
                         3,3,0,0,1,1,2,2}; // ph2Xstmas
static int table5[8] = {0,1,0,1,0,1,0,1};  // phf2Xstmas
static int table6[128] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                          2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
                          3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3};
                                           // ph2Xechsel
static int table7[64] = {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,
                        2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1};
                                           // phRec

//static int table7[64] = {0,0,0,0,0,0,0,0,3,3,3,3,3,3,3,3,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,
//                         2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,3,3,3,3,3,3,3,};
                                           // phRec

#define ph1Xstmas t1
#define ph1Xechsel t2
#define phf1Xechsel t3
#define ph2Xstmas t4
#define phf2Xstmas t5
#define ph2Xechsel t6
#define phRec t7

static double d2_init;

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   double ival = getval("ival");                 //Obtain the K-values from ival and mval using
   double mval = getval("mval");                 //Table 3 of Ashbrook & Wimperis
   double k0 = 1.0;
   double k1 = 0.0;
   double k2 = 0.0;
   if ((ival == 1.5) && (mval == 1.5)) {
      k0 = 9.0/17.0;
      k1 = 8.0/17.0;
   }
   if (ival == 2.5) {
      if (mval == 1.5) {
         k0 = 24.0/31.0;
         k2 = 7.0/31.0;
      }
      else {
         k0 = 9.0/17.0;
         k1 = 8.0/17.0;
      }
   }
   if (ival == 3.5) {
      if (mval == 1.5) {
         k0 = 45.0/73.0;
         k2 = 28.0/73.0;
      }
      else if (mval == 2.5) {
         k0 = 45.0/68.0;
         k1 = 23.0/68.0;
      }
      else {
         k0 = 5.0/17.0;
         k1 = 12.0/17.0;
      }
   }
   if (ival == 4.5) {
      if (mval == 1.5) {
         k0 = 72.0/127.0;
         k2 = 55.0/127.0;
      }
      else if (mval == 2.5) {
         k0 = 18.0/19.0;
         k2 = 1.0/19.0;
      }
      else if (mval == 3.5) {
         k0 = 8.0/17.0;
         k1 = 9.0/17.0;
      }
      else {
         k0 = 9.0/34.0;
         k1 = 25.0/34.0;
      }
   }

   double d2init = getval("d2");           // Define the Split d2 in the Pulse Sequence
   double d20 = k0*d2init;
   double d21 = k1*d2init;
   double d22 = k2*d2init;

   double pw1Xstmas = getval("pw1Xstmas"); // Get STMAS and echo pulsewidths for synchronization
   double pw2Xstmas = getval("pw2Xstmas"); // of the F1 delay
   double pwXechsel = getval("pwXechsel");

   double tXechselinit = getval("tXechsel"); // Adjust the selective echo delay for the
   double tXechsel = tXechselinit - 3.0e-6;  // attenuator switch time.
   if (tXechsel < 0.0) tXechsel = 0.0;

                                          // Adjust the F1 delay for the attenuator
   d20 = d20 - 6.0e-6 - pw1Xstmas/2.0 - pw2Xstmas/2.0 - pwXechsel;
   if (d20 < 0.0) d20 = 0.0;              // switch time, pulses and one selective echo delay

   DSEQ dec = getdseq("H");
   strcpy(dec.t.ch,"dec");
   putCmd("chHtppm='dec'\n"); 
   strcpy(dec.s.ch,"dec");
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
   d.dutyon = getval("pw1Xstmas") + getval("pw2Xstmas") + getval("pwXechsel");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = d2_ + tXechsel + getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = d2_ + tXechsel + getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(ph1Xstmas,4,table1);
   settable(ph1Xechsel,8,table2); 
   settable(phf1Xechsel,8,table3);
   settable(ph2Xstmas,32,table4);
   settable(phf2Xstmas,8,table5);
   settable(ph2Xechsel,128,table6);
   settable(phRec,64,table7);

   if (phase1 == 2) {
      tsadd(ph1Xstmas,1,4);
   }
   setreceiver(phRec);
   obsstepsize(45.0);

// Begin Sequence

   txphase(ph1Xstmas); decphase(zero);
   obspower(getval("tpwr"));
   obspwrf(getval("aXstmas"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H Decoupler on Before STMAS

   _dseqon(dec);

// First Pulse of Two-Pulse STMAS

   rgpulse(getval("pw1Xstmas"),ph1Xstmas,0.0,0.0);  

// F1 delay

   xmtrphase(phf1Xechsel);
   txphase(ph1Xechsel);
   obsblank();
   obspower(getval("dbXechsel"));
   obspwrf(getval("aXechsel"));
   delay(3.0e-6);
   obsunblank();
   delay(d20);

// First Selective Echo pulse for DQF

   rgpulse(getval("pwXechsel"),ph1Xechsel,0.0,0.0);
   
// Second Pulse of Two-Pulse STMAS

   xmtrphase(phf2Xstmas);
   txphase(ph2Xstmas);
   obsblank();
   obspower(getval("tpwr"));
   obspwrf(getval("aXstmas"));
   delay(3.0e-6);
   obsunblank();
   rgpulse(getval("pw2Xstmas"),ph2Xstmas,0.0,0.0);

// Tau Delay and Second Selective Echo Pulse

   xmtrphase(zero);
   txphase(ph2Xechsel);
   obsblank();
   obspower(getval("dbXechsel"));
   obspwrf(getval("aXechsel"));
   delay(3.0e-6);
   obsunblank();
   delay(d21 + tXechsel);
   rgpulse(getval("pwXechsel"),ph2Xechsel,0.0,0.0);
   delay(d22);

// Begin Acquisition

   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

