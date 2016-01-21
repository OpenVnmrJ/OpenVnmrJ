/*mqmas3qzf2d.c - A sequence to generate a two pulse MQMAS 2D spectrum
                  with a third Z-filter pulse, using a 3Q phase cycle.

                  D. Rice 02/22/06                                    */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[6] = {0,0,0,0,0,0};               // phHdec
static int table2[6] = {0,60,120,180,240,300};      // phf1Xmqmas
static int table3[6] = {0,0,0,0,0,0};               // ph1Xmqmas
static int table4[6] = {0,0,0,0,0,0};               // phf2Xmqmas
static int table5[6] = {0,0,0,0,0,0};               // ph2Xmqmas
static int table6[24] = {0,0,0,0,0,0,1,1,1,1,1,1,   // phXzfsel
                         2,2,2,2,2,2,3,3,3,3,3,3};
static int table7[24] = {1,3,1,3,1,3,2,0,2,0,2,0,   // phRec
                         3,1,3,1,3,1,0,2,0,2,0,2};

#define phHdec t1
#define phf1Xmqmas t2
#define ph1Xmqmas t3
#define phf2Xmqmas t4
#define ph2Xmqmas t5
#define phXzfsel t6
#define phRec t7

static double d2_init;

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   double tXzfselinit = getval("tXzfsel"); // Adjust the Z-filter Delay for the
   double tXzfsel = tXzfselinit - 3.0e-6;  // attenuator switch time.
   if (tXzfsel < 0.0) tXzfsel = 0.0;

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
   d.dutyon = getval("pw1Xmqmas") + getval("pw2Xmqmas") + getval("pwXzfsel");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = d2_ + tXzfselinit + getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = d2_ + tXzfselinit + getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phHdec,6,table1);
   settable(phf1Xmqmas,6,table2);
   settable(ph1Xmqmas,6,table3);
   settable(phf2Xmqmas,6,table4);
   settable(ph2Xmqmas,6,table5);
   settable(phXzfsel,24,table6);
   settable(phRec,24,table7);

   if (phase1 == 2) {
      tsadd(phf1Xmqmas,30,360);
   }
   setreceiver(phRec);
   obsstepsize(1.0);

// Begin Sequence

   xmtrphase(phf1Xmqmas); txphase(ph1Xmqmas); decphase(phHdec);
   obspower(getval("tpwr"));
   obspwrf(getval("aXmqmas"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H Decoupler on Before MQMAS

   _dseqon(dec);

// Two-Pulse MQMAS

   rgpulse(getval("pw1Xmqmas"),ph1Xmqmas,0.0,0.0);
   xmtrphase(phf2Xmqmas); txphase(ph2Xmqmas);
   obsunblank();
   delay(d2);
   rgpulse(getval("pw2Xmqmas"),ph2Xmqmas,0.0,0.0);

// Selective Z-filter Pulse

   txphase(phXzfsel);
   obsblank();
   obspower(getval("dbXzfsel"));
   obspwrf(getval("aXzfsel"));
   delay(3.0e-6);
   obsunblank();
   delay(tXzfsel);
   rgpulse(getval("pwXzfsel"),phXzfsel,0.0,0.0);

// Begin Acquisition

   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

