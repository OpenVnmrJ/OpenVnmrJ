/*dcptan3drad.c  DCP with F1 13C, F2 15N and F3 13C, uses RAD Mixing for 13C

            CEB 06/28/05 for VNMRJ2.1A-B for the NMR SYSTEM
            Edited  D. Rice 6/28/05
	    updated with new .h files D.Rice 10/12/05                       */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,0,2};           // phH90
static int table2[4] = {1,1,1,1};           // phHhy
static int table3[4] = {1,1,1,1};           // phY90
static int table4[4] = {1,1,1,1};           // phYhy
static int table5[4] = {1,1,1,1};           // phHyx
static int table6[4] = {1,1,1,1};           // phYyx
static int table7[4] = {0,0,1,1};           // phXyx
static int table8[4] = {3,3,2,2};           // phXmix1
static int table9[4] = {1,1,2,2};           // phXmix2
static int table10[4] = {0,2,3,1};          // phRec

#define phH90 t1
#define phHhy t2
#define phY90 t3
#define phYhy t4
#define phHyx t5
#define phYyx t6
#define phXyx t7
#define phXmix1 t8
#define phXmix2 t9
#define phRec t10

static double d2_init;
static double d3_init;

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   CP hy = getcp("HY",0.0,0.0,0,1);
   strncpy(hy.fr,"dec",3);
   strncpy(hy.to,"dec2",4);
   putCmd("frHY='dec'\n"); 
   putCmd("toHY='dec2'\n");

   CP yx = getcp("YX",0.0,0.0,0,1);
   strncpy(yx.fr,"dec2",4);
   strncpy(yx.to,"obs",3);
   putCmd("frYX='dec2'\n");
   putCmd("toYX='obs'\n");
   
   MPSEQ spc5 = getspc5("spc5X",0,0.0,0.0,0,1);
   MPSEQ spc5ref = getspc5("spc5X",spc5.iSuper,spc5.phAccum,spc5.phInt,1,1);   
   strncpy(spc5.ch,"obs",3);
   putCmd("chXspc5='obs'\n");

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

// Set Constant-time Period for d3. 

   if (d3_index == 0) d3_init = getval("d3");
   double d3_ = (ni - 1)/sw1 + d3_init;
   putCmd("d3acqret = %f\n",roundoff(d3_,12.5e-9));
   putCmd("d3dwret = %f\n",roundoff(1.0/sw2,12.5e-9));

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwY90") + getval("pwH90") + getval("tHY") + getval("tYX")
              + 2.0*getval("pwX90") + spc5.t + spc5ref.t;
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = d2 +  d3 + getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = d2 +  d3 + getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Create Phasetables 

   settable(phH90,4,table1);
   settable(phHhy,4,table2);
   settable(phY90,4,table3);
   settable(phYhy,4,table4);
   settable(phHyx,4,table5);
   settable(phYyx,4,table6);
   settable(phXyx,4,table7);
   settable(phXmix1,4,table8);
   settable(phXmix2,4,table9);
   settable(phRec,4,table10);
   setreceiver(phRec);

   if (phase1 == 2)
      tsadd(phXyx,1,4);

   if (phase2 == 2)
      tsadd(phYhy,1,4);
      
// Begin Sequence

   txphase(phXyx); decphase(phH90); dec2phase(phY90);
   obspwrf(getval("aXyx")); decpwrf(getval("aH90")); dec2pwrf(getval("aY90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to Y Cross Polarization with a Y Prepulse 

   dec2rgpulse(getval("pwY90"),phY90,0.0,0.0);
   dec2phase(phYhy);
   dec2pwrf(getval("aYyx"));
   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decphase(phHhy);
   _cp_(hy,phHhy,phYhy);

// F1 Indirect Period For Y

    _dseqon(dec);
    delay(d3);
    _dseqoff(dec);

// Y to X Cross Polarization 

   decphase(phHyx); dec2phase(phYyx);
   decpwrf(getval("aHyx"));
   decunblank(); decon();
   _cp_(yx,phYyx,phXyx);
   decphase(phHhy);
   decoff();

// F2 Indirect Period for X

   obspwrf(getval("aX90"));
   _dseqon(dec);
   delay(d2);
   _dseqoff(dec);

 // Mixing with SPC5 Recoupling

   rgpulse(getval("pwX90"),phXmix1,0.0,0.0);
   obspwrf(getval("aXspc5"));
   xmtrphase(v1); txphase(phXmix1);
   delay(getval("tZF"));
   decpwrf(getval("aHmix"));
   decon();
   _mpseq(spc5, phXmix1);
   xmtrphase(v2); txphase(phXmix2);
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
