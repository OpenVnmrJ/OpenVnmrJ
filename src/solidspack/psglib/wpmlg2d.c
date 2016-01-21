/*wpmlg2d.c - A 2D sequence to perform PMLG during F1 and PMLG with
              interleaved aquisition during F2.

              D.Rice 04/15/06                                          */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,0,0,0};           // ph1Xprep1
static int table2[4] = {1,1,1,1};           // ph2Xprep1
static int table3[4] = {0,0,0,0};           // phXpmlg
static int table4[4] = {0,0,0,0};           // ph1Xmix
static int table5[4] = {3,3,3,3};           // ph2Xmix
static int table6[4] = {0,2,1,3};           // ph1Xdct
static int table7[4] = {1,1,1,1};           // ph2Xdct
static int table8[4] = {0,0,0,0};           // phXwpmlg
static int table9[4] = {0,2,1,3};           // phRec

#define ph1Xprep1 t1
#define ph2Xprep1 t2
#define phXpmlg t3
#define ph1Xmix t4
#define ph2Xmix t5
#define ph1Xdtct t6
#define ph2Xdtct t7
#define phXwpmlg t8
#define phRec t9

static double d2_init;

pulsesequence() {

// Set the Maximum Dynamic Table and v-var Numbers

   settablenumber(10);
   setvvarnumber(30);

// Define Variables and Objects and Get Parameter Values

   double aXprep1 = getval("aXprep1");  // Define Tilted Pulses using "prep1X".
   double pw1Xprep1 = getval("pw1Xprep1");
   double pw2Xprep1 = getval("pw2Xprep1");
   double srate = getval("srate");
   double tXmix = getval("tXmix"); //Rotor Synchronize the Mix Period using "mixX"
   if (srate > 500) {
      tXmix = roundoff(tXmix,1.0/srate); 
   }

   MPSEQ pmlg = getpmlg("pmlgX",0,0.0,0.0,0,1);
   strncpy(pmlg.ch,"obs",3);
   putCmd("chXpmlg='obs'\n");

   WMPA wpmlg = getwpmlg("wpmlgX");
   strncpy(wpmlg.ch,"obs",3);
   putCmd("chXwpmlg='obs'\n");

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
   d.dutyon = 3.0*getval("pw1Xprep1") + 3.0*getval("pw2Xprep1") + 
              d2_ + 2.0*wpmlg.q*wpmlg.cycles*wpmlg.pw;
   d.dutyoff = d1 + 4.0e-6 + 5.0e-6 + tXmix + wpmlg.r1 + wpmlg.r2 + 
               at - 2.0*wpmlg.q*wpmlg.cycles*wpmlg.pw;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(ph1Xprep1,4,table1);
   settable(ph2Xprep1,4,table2);
   settable(phXpmlg,4,table3);
   settable(ph1Xmix,4,table4);
   settable(ph2Xmix,4,table5);
   settable(ph1Xdtct,4,table6);
   settable(ph2Xdtct,4,table7);
   settable(phXwpmlg,4,table8);
   settable(phRec,4,table9);

//Add STATES

   if (phase1 == 2) {
      tsadd(ph1Xmix,3,4);
   }
   setreceiver(phRec);

// Set the Small-Angle Step

   double obsstep = 360.0/(PSD*8192);
   obsstepsize(obsstep);

// Begin Sequence

   txphase(ph1Xprep1);
   obspwrf(aXprep1);
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// F1 Tilted Preparation Pulse for FSLG or PMLG "prep1X"

   rgpulse(pw1Xprep1, ph1Xprep1, 0.0, 0.0);
   rgpulse(pw2Xprep1, ph2Xprep1, 0.0, 0.0); 

// F1 Evolution with PMLG

   _mpseqon(pmlg,phXpmlg);
   delay(d2); 
   _mpseqoff(pmlg);

// Mixing Period with Untilt Using prep1X

   obspwrf(aXprep1);
   rgpulse(pw2Xprep1, ph2Xmix, 0.0, 0.0);
   rgpulse(pw1Xprep1, ph1Xmix, 0.0, 0.0);
   delay(tXmix);

// Tilted Detection Pulse Using prep1X

   startacq(5.0e-6);
   rcvroff();
   delay(wpmlg.r1);
   rgpulse(pw1Xprep1, ph1Xdtct, 0.0, 0.0);
   rgpulse(pw2Xprep1, ph2Xdtct, 0.0, 0.0);
   delay(wpmlg.r2);

// Apply WPMLG Cycles During Acqusition

   decblank(); _blank34();
   _wpmlg(wpmlg, phXwpmlg);
   endacq();
   obsunblank(); decunblank(); _unblank34();
}

