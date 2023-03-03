/*spc5inad2d.c - A sequence to provide 13C homonuclear correlation with 
                 a 2Q-1Q presentation using SPC5.

             CEB 06/28/05 for VNMRJ2.1A-B for the NMR SYSTEM
             Edited  D. Rice 6/28/05                                
	     updated with new .h files D.Rice 10/12/05                 */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,1,1,1};           // phH90
static int table2[4] = {0,0,0,0};           // phXhx
static int table3[4] = {0,0,0,0};           // phHhx
static int table4[4] = {3,3,3,3};           // phXmix1
static int table5[4] = {0,1,2,3};           // phXmix2
static int table6[4] = {0,3,2,1};           // phRec

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define phXmix1 t4
#define phXmix2 t5
#define phRec t6

static double d2_init;

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   CP hx = getcp("HX",0.0,0.0,0,1);
   strcpy(hx.fr,"dec");
   strcpy(hx.to,"obs");
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   MPSEQ spc5 = getspc5("spc5X",0,0.0,0.0,0,1);
   MPSEQ spc5ref = getspc5("spc5X",spc5.iSuper,spc5.phAccum,spc5.phInt,1,1);
   strcpy(spc5.ch,"obs");
   putCmd("chXspc5='obs'\n");

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
   d.dutyon = getval("pwH90") + getval("tHX") + spc5.t + spc5ref.t;
   d.dutyoff = d1 + 4.0e-6 + 2.0*getval("tZF");
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = d2_ + getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = d2_ + getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phH90,4,table1);
   settable(phXhx,4,table2);
   settable(phHhx,4,table3);
   settable(phXmix1,4,table4);
   settable(phXmix2,4,table5);
   settable(phRec,4,table6);

// Add STATES-TPPI (STATES + "FAD")

   double obsstep = 360.0/(PSD*8192);
   if (phase1 == 2)
      initval((45.0/obsstep),v1);
   else
      initval(0.0,v1);

   initval((d2*spc5.of[0]*360.0/obsstep),v2);
   obsstepsize(obsstep);
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
   obspwrf(getval("aX90"));

// Mixing with SPC5 Recoupling-Period One

   rgpulse(getval("pwX90"),phXmix1,0.0,0.0);
   obspwrf(getval("aXspc5"));
   decoff();
   xmtrphase(v1); txphase(phXmix1);
   delay(getval("tZF"));
   decpwrf(getval("aHmix"));
   decunblank();
   decon();
   _mpseq(spc5, phXmix1);
   decoff();

// F1 Indirect Period For X

   xmtrphase(v2); txphase(phXmix2);
   _dseqon(dec);
   delay(d2);
   _dseqoff(dec);

// Mixing with SPC5 Recoupling-Period Two 

   decpwrf(getval("aHmix"));
   decunblank();
   decon();
   _mpseq(spc5ref, phXmix2);
   decoff();
   obspwrf(getval("aX90"));
   xmtrphase(zero); txphase(phXmix2);
   delay(getval("tZF"));
   rgpulse(getval("pwX90"),phXmix2,0.0,0.0);

// Begin Acquisition

   _dseqon(dec);
   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

