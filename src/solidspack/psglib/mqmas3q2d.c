/*mqmas3q2d.c - A sequence to generate a two pulse MQMAS 2D spectrum
                using a 3Q phase cycle.

                D. Rice 06/14/06                                    */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[24] = {0,60,120,180,240,300,90,150,210,270,330,30,
                        180,240,300,0,60,120,270,330,30,90,150,210};
                                                    // phf1Xmqmas
static int table2[24] = {1,1,1,1,1,1,2,2,2,2,2,2,
                        3,3,3,3,3,3,0,0,0,0,0,0};   // ph2Xmqmas
static int table3[24] = {0,2,0,2,0,2,1,3,1,3,1,3,
                        2,0,2,0,2,0,3,1,3,1,3,1};   // phRec

#define phf1Xmqmas t1
#define ph2Xmqmas t2
#define phRec t3

static double d2_init;

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

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
   d.dutyon = getval("pw1Xmqmas") + d2_ + getval("pw2Xmqmas");
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

   settable(phf1Xmqmas,24,table1);
   settable(ph2Xmqmas,24,table2);
   settable(phRec,24,table3);

   if (phase1 == 2) {
      tsadd(phf1Xmqmas,30,360);
   }
   setreceiver(phRec);
   obsstepsize(1.0);

// Begin Sequence

   xmtrphase(phf1Xmqmas); decphase(zero);
   obspower(getval("tpwr"));
   obspwrf(getval("aXmqmas"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H Decoupler on Before MQMAS

   _dseqon(dec);

// Two-Pulse MQMAS with DFS Conversion

   rgpulse(getval("pw1Xmqmas"),zero,0.0,0.0);
   xmtrphase(zero); txphase(ph2Xmqmas);
   delay(d2);
   rgpulse(getval("pw2Xmqmas"),ph2Xmqmas,0.0,0.0);

// Begin Acquisition

   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

