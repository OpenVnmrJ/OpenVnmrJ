/*lgcp2d.c - A sequence to provide selective CP using an off-resonance
           Lee-Goldburg spinlock. This sequence arrays the contact time
           d2 so that the data can be transformed in F1. tHlg = tXlg = the
           maximum contact time.  lgcp2d can be used to provide a normal
           CP with pwHtilt = 0.0.  Use a pwHtilt = 35 degree flip angle for a
           Lee-Goldburg CP.

           D.Rice 01/30/06 - lgcp2d 08/28/07                               */

#include "solidstandard.h"
#include <standard.h>

// Define Values for Phasetables

static int table1[4] = {1,1,1,1};           // phH90
static int table2[4] = {1,1,3,3};           // phHtilt
static int table3[4] = {0,1,0,1};           // phXlg
static int table4[4] = {0,0,2,2};           // phHlg
static int table5[4] = {0,1,2,3};           // phRec

#define phH90 t1
#define phHtilt t2
#define phXlg t3
#define phHlg t4
#define phRec t5

static double d2_init;

pulsesequence() {

// Set Constant-time Period for d2. 

   if (d2_index == 0) d2_init = getval("d2");
   double d2_ = (ni - 1)/sw1 + d2_init;
   putCmd("d2acqret = %f\n",roundoff(d2_,12.5e-9));
   putCmd("d2dwret = %f\n",roundoff(1.0/sw1,12.5e-9));

// Define Variables and Objects and Get Parameter Values 

   MPSEQ hlg = getlg("lgH",0,0.0,0.0,0,0);
   strncpy(hlg.ch,"dec",3);
   putCmd("chHlg = 'dec'\n"); 
   hlg.pw[0] = d2_ + 1.0/sw1;
   hlg = update_mpseq(hlg,0,0.0,0.0,0);

   MPSEQ  xlg = getlg("lgX",0,0.0,0.0,0,0);
   strncpy(xlg.ch,"obs",3);
   putCmd("chXlg = 'obs'\n");
   xlg.pw[0] = d2_ + 1.0/sw1;
   xlg = update_mpseq(xlg,0,0.0,0.0,0);

   DSEQ dec = getdseq("H");
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHtppm='dec'\n");
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHspinal='dec'\n");

   printf("hithere\n");


//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90") + getval("tHX") + getval("pwHtilt") + d2_;
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

   settable(phH90,4,table1);
   settable(phHtilt,4,table2);
   settable(phXlg,4,table3);
   settable(phHlg,4,table4);
   settable(phRec,4,table5);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXlg); decphase(phH90);
   obspwrf(getval("aXlg")); decpwrf(getval("aH90"));
   obsunblank();decunblank();_unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H 90 and Ramped H to X Cross Polarization with LG Offset

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decunblank();
   decrgpulse(getval("pwHtilt"),phHtilt,0.0,0.0);

// F1 Evolution of Contact Time - Dps displays d2_. 

   _mpseqon(xlg,phXlg); _mpseqon(hlg,phHlg);
   delay(d2);
   _mpseqoff(xlg); _mpseqoff(hlg);

// Begin Acquisition

   obsblank(); _blank34();
   _dseqon(dec);
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

