/*tancp1dptrfdr.c  Preparation of X with CP, constant time, longitudinal
                   finite-pulse XY4 RFDR and PITHIRDS mixing, with SPINAL
		   or TPPM decoupling

      R. Tycko, J. Chem. Phys, 126,064506 (2007)

                   D. Rice 01/15/08                                   */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,3,1,3};           // phH90
static int table2[4] = {0,0,0,0};           // phHhx
static int table3[8] = {0,0,1,1,2,2,3,3};   // phXhx
static int table4[4] = {0,0,0,0};           // phXdiff
static int table5[4] = {0,0,0,0};           // phXmix
static int table6[8] = {0,2,1,3,2,0,3,1};   // phRec

#define phH90 t1
#define phHhx t2
#define phXhx t3
#define phXdiff t4
#define phXmix t5
#define phRec t6

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   CP hx = getcp("HX",0.0,0.0,0,1);
   strncpy(hx.fr,"dec",3);
   strncpy(hx.to,"obs",3);
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");
   
   MPSEQ pt = getptrfdr("ptrfdrX",0,0.0,0.0,0,1);
   strncpy(pt.ch,"obs",3);
   putCmd("chXptrfdr='obs'\n");
   
   MPSEQ fp = getfprfdr("fprfdrX",pt.iSuper,pt.phAccum,pt.phInt,0,1);
   strncpy(fp.ch,"obs",3);
   putCmd("chXfprfdr='obs'\n");

   DSEQ dec = getdseq("H");
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHtppm='dec'\n"); 
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHspinal='dec'\n");

   DSEQ mix = getdseq("Hmix");
   strncpy(mix.t.ch,"dec",3);
   putCmd("chHmixtppm='dec'\n"); 
   strncpy(mix.s.ch,"dec",3);
   putCmd("chHmixspinal='dec'\n");

// Round tXmix and tXtotal to the element length, based on 36.0*pwXfprfdr.
// Make sure that pwXptrfdr = pwXfprfdr and srate = 1/(3.0*pwXfprfdr.

   double tXtotal = getval("tXtotal");
   tXtotal = roundoff(tXtotal,36.0*getval("pwXptrfdr"));
   double tXmix = getval("tXmix");
   tXmix = roundoff(tXmix,36.0*getval("pwXptrfdr"));
   double tXdiff = tXtotal - tXmix; 

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90") + getval("tHX") + tXmix/3.0; 
   d.dutyoff = d1 + 4.0e-6; 
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d.c3 = d.c3 + (!strcmp(mix.seq,"tppm"));
   d.c3 = d.c3 + ((!strcmp(mix.seq,"tppm")) && (mix.t.a > 0.0));
   d.t3 = (2.0/3.0)*(tXdiff +tXmix);
   d.c4 = d.c4 + (!strcmp(mix.seq,"spinal"));
   d.c4 = d.c4 + ((!strcmp(mix.seq,"spinal")) && (mix.s.a > 0.0));
   d.t4 = (2.0/3.0)*(tXdiff+tXmix);
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0); 

// Create Phasetables

   settable(phH90,4,table1);
   settable(phHhx,4,table2);
   settable(phXhx,8,table3);
   settable(phXdiff,4,table4);
   settable(phXmix,4,table5);
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

// Constant Time Finite-Pulse RFDR Mixing with PITHIRDS

   _dseqon(mix);
   _mpseqon(pt,phXdiff);
   delay(tXdiff);
   _mpseqoff(pt);
   _mpseqon(fp,phXmix);
   delay(tXmix);
   _mpseqoff(fp);
   _dseqoff(mix);

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

