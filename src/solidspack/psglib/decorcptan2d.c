/*decorcptan2d.c  A sequence to measure XY correlation through YX ramped CP
                following direct polarization of Y.

                D. Rice 04/13/06                                          */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,0,0,0};           // phY90
static int table2[4] = {1,3,1,3};           // phYyx
static int table3[4] = {0,0,0,0};           // phXyx
static int table4[4] = {0,2,0,2};           // phRec

#define phY90 t1
#define phYyx t2
#define phXyx t3
#define phRec t4

static double d2_init;

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   CP yx = getcp("YX",0.0,0.0,0,1);
   strncpy(yx.fr,"dec2",4);
   strncpy(yx.to,"obs",3);
   putCmd("frYX='dec2'\n");
   putCmd("toYX='obs'\n");

   DSEQ dec = getdseq("H");
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHtppm='dec'\n"); 
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHspinal='dec'\n");

   DSEQ dec2 = getdseq("Y");
   strncpy(dec2.t.ch,"dec2",4);
   putCmd("chYtppm='dec2'\n"); 
   strncpy(dec2.s.ch,"dec2",4);
   putCmd("chYspinal='dec2'\n");

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
   d.dutyon = getval("pwY90") + getval("tYX");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = d2_ + getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = d2_ + getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Create Phasetables

   settable(phY90,4,table1);
   settable(phYyx,4,table2);
   settable(phXyx,4,table3);
   settable(phRec,4,table4);
   
   if (phase1 == 2)
      tsadd(phYyx,1,4);

// Begin Sequence

   setreceiver(phRec);
   txphase(phXyx); decphase(zero); dec2phase(phY90);
   obspwrf(getval("aXyx")); dec2pwrf(getval("aY90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// Direct Polarization of Y

   dec2rgpulse(getval("pwY90"),phY90,0.0,0.0);

// F1 Indirect period for Y

   _dseqon(dec);
   delay(d2);

// Y to X Cross Polarization

   dec2phase(phYyx);
   _unblank34();
   _cp_(yx,phYyx,phXyx);

// Begin Acquisition with Simulatneous H and Y decoupling

   _dseqon(dec2);
   obsblank();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec); _dseqoff(dec2);
   obsunblank(); decunblank(); _unblank34();
}

