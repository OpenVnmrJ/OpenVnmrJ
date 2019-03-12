/*stmasspltse2d1.c - A split echo sequence to generate a two pulse STMAS 2D spectrum
                with a third selective echo pulse.

                See: Ashbrooke, S.E., Wimperis, S.E. Progress in NMR Spectroscopy 45 (2004)
                     53-108.

                D. Rice 05/10/06                                                           */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,0,0,0};       // ph1Xstmas
static int table2[4] = {0,1,2,3};       // ph2Xstmas
static int table3[16] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3};   // phXechsel
static int table4[8] = {0,0,0,0,2,2,2,2};   // phRec

#define ph1Xstmas t1
#define ph2Xstmas t2
#define phXechsel t3
#define phRec t4

static double d2_init;

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values
                                             
   double ival = getval("ival");                 //Obtain the K-values from ival and mval
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

   double d2init = getval("d2");
   double d20 = k0*d2init;
   double d21 = k1*d2init;
   double d22 = k2*d2init;

   double pw1Xstmas = getval("pw1Xstmas");  
   double pw2Xstmas = getval("pw2Xstmas");  

   double tXechselinit = getval("tXechsel");
   double tXechsel = tXechselinit - 3.0e-6; 
   if (tXechsel < 0.0) tXechsel = 0.0;
                                        
   d20 = d20 - pw1Xstmas/2.0 - pw2Xstmas/2.0;
   if (d20 < 0.0) d20 = 0.0;

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
   settable(ph2Xstmas,4,table2);
   settable(phXechsel,16,table3);
   settable(phRec,8,table4);

   if (phase1 == 2) {
      tsadd(ph1Xstmas,1,4);
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
   delay(d20);
   rgpulse(getval("pw2Xstmas"),ph2Xstmas,0.0,0.0);

// Selective Echo Pulse

   txphase(phXechsel);
   obsblank();
   obspower(getval("dbXechsel"));
   obspwrf(getval("aXechsel"));
   delay(3.0e-6);
   obsunblank();
   delay(d21 + tXechsel);
   rgpulse(getval("pwXechsel"),phXechsel,0.0,0.0);
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

