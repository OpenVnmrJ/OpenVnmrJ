/*jmascacosh2d.c - A J-based CACO 2D experiment using CP to X followed
                   by refocused constant time INEPT X mixing, with 
                   two-level SPINAL or TPPM decoupling during INEPT 
                   and acquisition. Uses combined shaped inversion pulses
                   to selectively refocus CA and CO. 
                                                                        */
#include "standard.h"
#include "solidstandard.h"
#include "pboxpulse.h"

// Define Values for Phasetables

static int table1[4] = {1,1,3,3};           // phH90
static int table2[4] = {0,0,0.0};           // phHhx
static int table3[4] = {0,2,0,2};           // phXhx
static int table4[4] = {0,2,0,2};           // ph1Xshcaco
static int table5[4] = {1,3,1,3};           // ph1X90
static int table6[4] = {0,0,0,0};           // ph2X90
static int table7[4] = {2,2,2,2};           // ph2Xshcaco
static int table8[4] = {0,0,0,0};           // ph3X90
static int table9[4] = {2,2,2,2};           // ph4X90
static int table10[4] = {0,0,2,2};          // phRec

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define ph1Xshcaco t4
#define ph1X90 t5
#define ph2X90 t6
#define ph2Xshcaco t7
#define ph3X90 t8
#define ph4X90 t9
#define phRec t10

static double d2_init;

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   CP hx = getcp("HX",0.0,0.0,0,1);
   strncpy(hx.fr,"dec",3);
   strncpy(hx.to,"obs",3);
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   PBOXPULSE shca = getpboxpulse("shcaX",0,1);
   strncpy(shca.ch,"obs",3);
   putCmd("chXshca ='obs'\n");

   PBOXPULSE shco = getpboxpulse("shcoX",0,1);
   strncpy(shco.ch,"obs",3);
   putCmd("chXshco ='obs'\n");

   PBOXPULSE shcaco = combine_PBOXPULSE(shca,shco,0,1); 

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

   double shcacolen = (shcaco.pw + 2.0*shcaco.t2)/2.0;
   double d22 = d2/2.0;

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
   d.dutyon = getval("pwH90") + getval("tHX") + 2.0*shcaco.pw + 
              4.0*getval("pwX90");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = d2_ + getval("tZF") - getval("pwX90") + getval("rd") + 
          getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = d2_ + getval("tZF") - getval("pwX90") + getval("rd") + 
          getval("ad") + at;
   d.c3 = d.c3 + (!strcmp(mix.seq,"tppm"));
   d.c3 = d.c3 + ((!strcmp(mix.seq,"tppm")) && (mix.t.a > 0.0));
   d.t3 = 2.0*getval("taua") + 2.0*getval("taub") + 2.0*shcaco.t1 + 
          2.0*shcaco.t2 - 4.0*shcacolen - 2.5*getval("pwX90") - 6.0e-6;
   d.c4 = d.c4 + (!strcmp(mix.seq,"spinal"));
   d.c4 = d.c4 + ((!strcmp(mix.seq,"spinal")) && (mix.s.a > 0.0));
   d.t4 = 2.0*getval("taua") + 2.0*getval("taub") + 2.0*shcaco.t1 + 
          2.0*shcaco.t2 - 4.0*shcacolen - 2.5*getval("pwX90") - 6.0e-6;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Up 2D
   
   int errval = (int) ((getval("taua") - shcacolen)*2.0*sw1);
   if ((getval("taua") - ni/(2.0*sw1) - shcacolen) < 0.0) { 
     text_error("Error:ni is too large. Make ni equal to %d or less.\n",errval); 
     psg_abort(1);
   }

// Set Phase Tables

   settable(phH90,4,table1);
   settable(phXhx,4,table2);
   settable(phHhx,4,table3);
   settable(ph1Xshcaco,4,table4);
   settable(ph1X90,4,table5);
   settable(ph2X90,4,table6);
   settable(ph2Xshcaco,4,table7);
   settable(ph3X90,4,table8);
   settable(ph4X90,4,table9);
   settable(phRec,4,table10);
   setreceiver(phRec);

// States Acquisition

   if (phase1 == 2)       
      tsadd(phXhx,3,4);

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
   decphase(zero);

// Begin F1 Refocused INEPT

   _dseqon(mix); 
   txphase(ph1Xshcaco); 
   obspwrf(shcaco.a);
   delay(getval("taua") - d22 - shcacolen);
   _pboxpulse(shcaco,ph1Xshcaco); 
   obsblank();
   obspower(getval("tpwr"));
   delay(3.0e-6);
   obsunblank();
   obsunblank(); 
   txphase(ph1X90); 
   obspwrf(getval("aX90"));
   delay(getval("taua") + d22 - shcacolen - getval("pwX90") - 3.0e-6);
   rgpulse(getval("pwX90"),ph1X90,0.0,0.0); 
   rgpulse(getval("pwX90"),ph2X90,0.0,0.0);
   txphase(ph2Xshcaco); 
   obspwrf(shcaco.a);
   obsunblank();
   delay(getval("taub") - shcacolen - getval("pwX90"));
   _pboxpulse(shcaco,ph2Xshcaco);
   obsblank();
   obspower(getval("tpwr"));
   delay(3.0e-6);
   obsunblank();
   txphase(ph3X90); 
   obspwrf(getval("aX90"));
   obsunblank(); 
   delay(getval("taub") - shcacolen - getval("pwX90")/2.0 - 3.0e-6);
   rgpulse(getval("pwX90"),ph3X90,0.0,0.0); 
   txphase(ph4X90);
   obsunblank();
   _dseqoff(mix);
   _dseqon(dec); 
   delay(getval("tZF") - getval("pwX90"));
   rgpulse(getval("pwX90"),ph4X90,0.0,0.0);
   
// Begin Acquisition

   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}
