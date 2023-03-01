/*lgfmcp1d.c - A sequence to provide selective CP using an off-resonance
           Lee-Goldburg spinlock on H and a sinusoidal frequency modulated 
           spinlock on X. 
  
           D. Rice   11/12/08                                          */

#include "solidstandard.h"
#include <standard.h>

// Define Values for Phasetables

static int table1[4] = {1,1,1,1};           // phH90
static int table2[4] = {1,1,3,3};           // phHtilt
static int table3[4] = {0,1,0,1};           // phXsfm
static int table4[4] = {0,0,2,2};           // phHlg
static int table5[4] = {0,1,2,3};           // phRec

#define phH90 t1
#define phHtilt t2
#define phXsfm t3
#define phHlg t4
#define phRec t5

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values 

   SHAPE xsfm = getsfmpulse("sfmX",0.0,0.0,0,1);
   strcpy(xsfm.pars.ch,"obs");
   putCmd("chXsfm = 'obs'\n");

   MPSEQ hlg = getlg("lgH",0,0.0,0.0,0,0);
   hlg.pw[0] = xsfm.pars.t; 
   hlg.array = disarry("pwXsfm",hlg.array); 
   hlg = update_mpseq(hlg,0,0.0,0.0,0);
   strcpy(hlg.ch,"dec");
   putCmd("chHlg = 'dec'\n");

   DSEQ dec = getdseq("H");
   strcpy(dec.t.ch,"dec");
   putCmd("chHtppm='dec'\n");
   strcpy(dec.s.ch,"dec");
   putCmd("chHspinal='dec'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90") + getval("tHX") + getval("pwHtilt");
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
   settable(phXsfm,4,table3);
   settable(phHlg,4,table4);
   settable(phRec,4,table5);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXsfm); decphase(phH90);
   obspwrf(getval("aXsfm")); decpwrf(getval("aH90"));
   obsunblank();decunblank();_unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H 90 and Ramped H to X Cross Polarization with LG Offset

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decunblank();
   decrgpulse(getval("pwHtilt"),phHtilt,0.0,0.0);
   _shapeon(xsfm,phXsfm); _mpseqon(hlg,phHlg);   
    delay(getval("pwXsfm"));
   _shapeoff(xsfm); _mpseqoff(hlg);

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

