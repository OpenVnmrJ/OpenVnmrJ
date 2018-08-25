/*mqmas5qzf2d.c - A sequence to generate a two pulse MQMAS 2D spectrum
                  with a third Z-filter pulse, using a 5Q phase cycle.

                  D. Rice 02/22/06                                 */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[10] = {0,0,0,1,1,2,2,2,3,3};      // ph1Xmqmas
static int table2[5] =  {0,36,72,18,54};            // phfXmqmas
static int table3[10] = {0,0,0,0,0,0,0,0,0,0};      // ph2Xmqmas
static int table4[40] = {0,0,0,0,0,0,0,0,0,0,       // phXzfsel
                         1,1,1,1,1,1,1,1,1,1,
                         2,2,2,2,2,2,2,2,2,2,
			 3,3,3,3,3,3,3,3,3,3};
static int table5[40] = {3,1,3,1,3,1,3,1,3,1,       // phRec
                         0,2,0,2,0,2,0,2,0,2,
                         1,3,1,3,1,3,1,3,1,3,
			 2,0,2,0,2,0,2,0,2,0};

#define ph1Xmqmas t1
#define phfXmqmas t2
#define ph2Xmqmas t3
#define phXzfsel t4
#define phRec t5

static double d2_init;

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   double tXzfselinit = getval("tXzfsel"); // Adjust the Z-filter Delay for the
   double tXzfsel = tXzfselinit - 2.0e-6;  // attenuator switch time. 

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

   settable(ph1Xmqmas,10,table1);
   settable(phfXmqmas,5,table2);
   settable(ph2Xmqmas,10,table3);
   settable(phXzfsel,40,table4);
   settable(phRec,40,table5);

   if (phase1 == 2) {
      tsadd(ph1Xmqmas,1,4);
   }
   setreceiver(phRec);
   obsstepsize(1.0);

// Begin Sequence

   xmtrphase(phfXmqmas); txphase(ph1Xmqmas); decphase(zero);
   obspower(getval("tpwr"));
   obspwrf(getval("aXmqmas"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H Decoupler on Before MQMAS

   _dseqon(dec);

// Two-Pulse MQMAS

   rgpulse(getval("pw1Xmqmas"),ph1Xmqmas,0.0,0.0);
   xmtrphase(zero); txphase(ph2Xmqmas);
   delay(d2);
   rgpulse(getval("pw2Xmqmas"),ph2Xmqmas,0.0,0.0);

// Selective Z-filter Pulse

   txphase(phXzfsel); 
   obsblank();
   obspower(getval("dbXzfsel"));
   obspwrf(getval("aXzfsel"));
   delay(2.0e-6);
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

