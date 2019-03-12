/*mqmas3qdfszf2d.c - A sequence to generate a two pulse MQMAS 2D spectrum
                  with a third Z-filter pulse, using a 3Q phase cycle.
                  The second pulse of MQMAS is a DFS pulse.

                  Note that parameters pw2Xmqmas and pwXdfs are aliases.

              D. Rice 06/14/06                                             */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[6] = {0,0,1,2,2,3};               // phXmqmas
static int table2[3] = {0,60,30};                   // phfXmqmas
static int table3[6] = {0,0,0,0,0,0};               // phXdfs
static int table4[24] = {0,0,0,0,0,0,1,1,1,1,1,1,   // phXzfsel
                         2,2,2,2,2,2,3,3,3,3,3,3};
static int table5[24] = {3,1,3,1,3,1,0,2,0,2,0,2,   // phRec
                         1,3,1,3,1,3,2,0,2,0,2,0}; 

#define phXmqmas t1
#define phfXmqmas t2
#define phXdfs t3
#define phXzfsel t4
#define phRec t5

static double d2_init;

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   SHAPE dfs = getdfspulse("dfsX",0.0,0.0,0,1);
   strncpy(dfs.pars.ch,"obs",3);
   putCmd("chXdfs='obs'\n");   // Sequence uses pwXdfs and sets pw2Xmqmas

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

   settable(phXmqmas,6,table1);
   settable(phfXmqmas,3,table2);
   settable(phXdfs,6,table3);
   settable(phXzfsel,24,table4);
   settable(phRec,24,table5);

   if (phase1 == 2) {
      tsadd(phXmqmas,1,4);
      tsadd(phRec,2,4);
   }
   setreceiver(phRec);
   obsstepsize(1.0);

// Begin Sequence

   xmtrphase(phfXmqmas); txphase(phXmqmas); decphase(zero);
   obspower(getval("tpwr"));
   obspwrf(getval("aXmqmas"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H Decoupler on Before MQMAS

   _dseqon(dec);

// Two-Pulse MQMAS with 2nd Pulse DFS

   rgpulse(getval("pw1Xmqmas"),phXmqmas,0.0,0.0);
   xmtrphase(zero); txphase(phXdfs);
   delay(d2);

// X Double Frequency Sweep Pulse

   _shape(dfs,phXdfs);

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

