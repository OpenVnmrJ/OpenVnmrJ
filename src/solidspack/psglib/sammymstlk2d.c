/*sammymstlk2d.c -  A sequence to provide correlation between the
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

static int table1[4] = {1,3,1,3};           // phH90
static int table2[4] = {0,0,0,0};           // phHlock
static int table3[4] = {3,3,3,3};           // phHcomp
static int table4[4] = {0,0,0,0};           // ph1Xhx
static int table5[4] = {2,2,2,2};           // ph2Xhx
static int table6[4] = {0,0,0,0};           // ph1Hhx
static int table7[4] = {2,2,2,2};           // ph2Hhx
static int table8[4] = {0,2,0,2};           // phHsmyd
static int table9[4] = {0,2,0,2};           // phXsmyo 
static int table10[4] = {0,0,0,0};          // phHdec 
static int table11[4] = {1,3,1,3};          // phRec

#define phH90 t1
#define phHlock t2
#define phHcomp t3
#define ph1Xhx t4
#define ph2Xhx t5
#define ph1Hhx t6
#define ph2Hhx t7
#define phHsmyd t8
#define phXsmyo t9
#define phHdec t10
#define phRec t11

static double d2_init;

pulsesequence() {

//Define Variables and Objects and Get Parameter Values 
       double tHX3 = (getval("tHX"))/3.0;   //Define MOIST CP in the Sequence

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

   d2 = sd.nelem*sd.telem; 

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
   d.dutyon = getval("pwH90") + getval("pwHlock")+ getval("tHX") + d2_;
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
   settable(phHlock,4,table2);
   settable(phHcomp,4,table3);
   settable(ph1Xhx,4,table4);
   settable(ph2Xhx,4,table5);
   settable(ph1Hhx,4,table6);
   settable(ph2Hhx,4,table7);
   settable(phHsmyd,4,table8);
   settable(phXsmyo,4,table9);
   settable(phHdec,4,table10);
   settable(phRec,4,table11);
   setreceiver(phRec);
    
// Begin Sequence

   txphase(ph1Xhx); decphase(phH90);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H 90-degree Pulse 

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);

// Prelock with Compensation Pulse

   decunblank(); decon(); 
   if (getval("onHlock") > 0) {
      decphase(phHlock);  
      delay(getval("pwHlock"));
      decphase(phHcomp);
      decpwrf(getval("aHcomp"));
      delay(getval("pwHcomp"));
   }

 // H to X MOIST Cross Polarization

   xmtron();
   decphase(ph1Hhx);
   decpwrf(getval("aHhx"));
   delay(tHX3);
   txphase(ph1Xhx); decphase(ph1Hhx);
   delay(tHX3);
   txphase(ph2Xhx); decphase(ph2Hhx);
   delay(tHX3);
   xmtroff(); decoff();

// SAMMY Spinlocks on X and H

   _mpseqon(sd,phHsmyd); _mpseqon(so,phXsmyo);
   delay(d2);
   _mpseqoff(sd); _mpseqoff(so);
   decphase(phHdec);

// Begin Acquisition

   obsblank(); _blank34();
   _dseqon(dec);
   _decdoffset(getval("rd"),getval("ofHdec"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   decoffset(dof);
   obsunblank(); decunblank(); _unblank34();
}
