/*tancp2drad.c  DP with F1 X, uses RAD Mixing for X

                D. Rice 01/30/06                                    */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,0,0,0};           // phH90
static int table2[4] = {3,3,3,3};           // phHhx
static int table3[4] = {0,0,0,0};           // phXhx
static int table4[4] = {3,1,1,3};           // phXmix1
static int table5[4] = {1,1,3,3};   // phXmix2 {0,2,1,3,2,0,3,1};
static int table6[4] = {1,3,1,3};   // phRec {0,2,1,3,2,0,3,1};

#define phH90 t1
#define phHhx t2
#define phXhx t3
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

   DSEQ dec = getdseq("H");
   strcpy(dec.t.ch,"dec");
   putCmd("chHtppm='dec'\n"); 
   strcpy(dec.s.ch,"dec");
   putCmd("chHspinal='dec'\n");

// Set Mixing Period to N Rotor Cycles 

   double taur=1,mix,srate; 
   mix =  getval("tXmix"); 
   srate =  getval("srate");
   if (srate >= 500.0)
      taur = roundoff((1.0/srate), 0.125e-6);
   else {
      printf("ABORT: Spin Rate (srate) must be greater than 500\n");
      psg_abort(1);
   }
   mix = roundoff(mix,taur);
   mix = mix - getval("pwX90"); 
   if (mix < 0.0) mix = 0.0; 

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
   d.dutyon = getval("pwH90") + getval("tHX") + getval("pwX90") + mix;
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = d2_ +  getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = d2_ +  getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0); 

// Create Phasetables

   settable(phH90,4,table1);
   settable(phHhx,4,table2);
   settable(phXhx,4,table3);
   settable(phXmix1,4,table4);
   settable(phXmix2,4,table5);
   settable(phRec,4,table6);
   setreceiver(phRec);

   if (phase1 == 2)
      tsadd(phXhx,1,4);

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

// F2 Indirect Period for X

   txphase(phXmix1);
   obspwrf(getval("aX90"));
   _dseqon(dec);
   delay(d2);
   _dseqoff(dec);

// RAD(DARR) Mixing For X

   decpwrf(getval("aHmix"));
   decunblank(); decon();
   rgpulse(getval("pwX90"),phXmix1,0.0,0.0);
   txphase(phXmix2);
   obsunblank();
   delay(mix);
   rgpulse(getval("pwX90"),phXmix2,0.0,0.0);
   decoff();

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

