/*sammy2d.c -  A sequence to provide correlation between the
               chemical-shift and 1H-dipolar interaction using
               a pair of magic-sandwich spinlocks (SAMMY).

               Uses a modified SAMMY sequence based on a personal
               communication by Alex Nevzorov.  Changes from the paper
               are:
               1.  invert the X phase at the center of each MS cycle.
                   That means the X durations are 6.0*pwX90, 12.0*pwX90, 6.0*pwX90.
               2.  set each of four H spinlocks = 3.5*pwH90 and the two delays
                   = 3.0*pwH90 each. The four explicit pwH90's fill 2 MS cycles.
               3.  del1 and del2 remain the same but should be zero.
    
      Alexander A. Nevzorov and Stanley J. Opella, J. Magn. Reson.
      164 (2003) 182-186.

               D. Rice 6/23/07                                             */

#include "solidstandard.h"
#include <standard.h>

// Define Values for Phasetables

static int table1[4] = {0,2,0,2};           // phH90
static int table2[4] = {3,3,0,0};           // phXhx
static int table3[4] = {3,3,3,3};           // phHhx
static int table4[4] = {1,1,1,1};           // phHsmyd
static int table5[4] = {3,3,0,0};           // phXsmyo 
static int table6[4] = {1,3,2,0};           // phRec

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define phHsmyd t4
#define phXsmyo t5
#define phRec t6

static double d2_init;

pulsesequence() {

//Define Variables and Objects and Get Parameter Values 

   CP hx = getcp("HX",0.0,0.0,0,1);
   strncpy(hx.fr,"dec",3);
   strncpy(hx.to,"obs",3);
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   MPSEQ sd = getsammyd("smydH",0,0.0,0.0,0,1);
   strncpy(sd.ch,"dec",3);
   putCmd("chHsmyd='dec'\n");

   MPSEQ so = getsammyo("smyoX",0,0.0,0.0,0,1);
   strncpy(so.ch,"obs",3);
   putCmd("chXsmyo='obs'\n");

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
   d.dutyon = getval("pwH90") + getval("tHX") + d2_;
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
   settable(phXhx,4,table2);
   settable(phHhx,4,table3);
   settable(phHsmyd,4,table4);
   settable(phXsmyo,4,table5);
   settable(phRec,4,table6);
   setreceiver(phRec);
    
// Begin Sequence

   txphase(phXhx); decphase(phH90);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to X Cross Polarization

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decphase(phHhx);
   _cp_(hx,phHhx,phXhx);

// SAMMY Spinlocks on X and H

   _mpseqon(sd,phHsmyd); _mpseqon(so,phXsmyo);
   delay(d2);
   _mpseqoff(sd); _mpseqoff(so);

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
