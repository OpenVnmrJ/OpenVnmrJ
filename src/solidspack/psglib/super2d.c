/*super2d.c - SUPER for 2D Separation of X Shift Tensors in F1
              with SPINAL or TPPM decoupling.  

              D. Rice 03/18/09
                                                              
              This sequence uses a phase cycle derived from information 
              on the web site of K. Schmidt-Rohr, Iowa State University. 
              All other code is specific to the Varian NMR System.
                                                                        */
#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,3,1,3};           // phH90
static int table2[8] = {0,0,1,1,2,2,3,3};   // phXhx
static int table3[4] = {0,0,0,0};           // phHhx
static int table4[4] = {1,1,1,1};           // phHdec
static int table5[4] = {0,0,0,0};           // phXsuper
static int table6[8] = {1,1,2,2,3,3,0,0};   // ph1X90
static int table7[8] = {3,3,0,0,1,1,2,2};   // ph2X90
static int table8[8] = {1,1,2,2,3,3,0,0};   // ph1Xtoss
static int table9[8] = {3,3,0,0,1,1,2,2};   // ph2Xtoss
static int table10[16] = {1,1,2,2,3,3,0,0,
                         3,3,0,0,1,1,2,2};  // ph3Xtoss
static int table11[16] = {2,2,1,1,0,0,3,3,
                         0,0,3,3,2,2,1,1};  // ph4Xtoss
static int table12[8] = {0,2,1,3,2,0,3,1};  // phRec

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define phHdec t4
#define phXsuper t5
#define ph1X90 t6
#define ph2X90 t7
#define ph1Xtoss t8
#define ph2Xtoss t9
#define ph3Xtoss t10
#define ph4Xtoss t11
#define phRec t12

#define tXinczf t14

static double d2_init;

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   CP hx = getcp("HX",0.0,0.0,0,1);
   strcpy(hx.fr,"dec");
   strcpy(hx.to,"obs");
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   MPSEQ super = getsuper("superX",0,0.0,0.0,0,1);
   strcpy(super.ch,"obs");
   putCmd("chXsuper='obs'\n");
   putCmd("chXtoss='obs'\n");

   double tauR = 0.0; 
   if (getval("srate") > 500.0) { 
      tauR = 1.0/getval("srate");
   }
   else {
      printf("Set srate>500 for TOSS4 in this sequence");
      psg_abort(1);
   }

   int delaytable[4];
   int i;
   for (i = 0; i < 4; i++) {
      delaytable[i] = (int) roundoff(tauR*(i+1)/50.0e-9,1.0); 
   }
   settable(tXinczf,4,delaytable);

   DSEQ dec = getdseq("H"); 
   strcpy(dec.t.ch,"dec");
   putCmd("chHtppm='dec'\n"); 
   strcpy(dec.s.ch,"dec");
   putCmd("chHspinal='dec'\n");

   DSEQ mix = getdseq("Hmix");
   strcpy(mix.t.ch,"dec");
   putCmd("chHmixtppm='dec'\n"); 
   strcpy(mix.s.ch,"dec");
   putCmd("chHmixspinal='dec'\n");

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
   d.dutyon = getval("pwH90") + getval("tHX") + d2_ + 2.0*getval("pwX90") + 
              4.0*getval("pwXtoss"); 
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = 2.2408*tauR + - 4.0*getval("pwXtoss") + getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = 2.2408*tauR + - 4.0*getval("pwXtoss") + getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phH90,4,table1);
   settable(phXhx,8,table2);
   settable(phHhx,4,table3);
   settable(phHdec,4,table4);
   settable(phXsuper,4,table5);
   settable(ph1X90,8,table6);
   settable(ph2X90,8,table7);
   settable(ph1Xtoss,8,table8);
   settable(ph2Xtoss,8,table9);
   settable(ph3Xtoss,8,table10);
   settable(ph4Xtoss,8,table11);
   settable(phRec,8,table12);
   setreceiver(phRec);

// Add STATES

   if (phase1 == 2) tsadd(ph1X90,3,4); 

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
   decphase(phHdec);

// F1 Period with Synchronized SUPER

   _dseqon(mix);
   _mpseqon(super,phXsuper);
   delay(d2);
   _mpseqoff(super);
   _dseqoff(mix); 

// Gamma Integral for Clean Sideband-Suppression

   rgpulse(getval("pwX90"),ph1X90,0.0,0.0);
   obsunblank();
   getelem(t14,ct,v1);
   vdelay(NSEC,v1);  
   rgpulse(getval("pwX90"),ph2X90,0.0,0.0);
   obsunblank();

// TOSS4 Sideband Suppression

   _dseqon(dec);

   obspwrf(getval("aXtoss"));
   txphase(ph1Xtoss);
   delay(0.1226*tauR - getval("pwXtoss")/2.0);
   rgpulse(getval("pwXtoss"),ph1Xtoss,0.0,0.0);
   obsunblank();
   txphase(ph2Xtoss);
   delay((0.0773*tauR) - getval("pwXtoss"));                   
   rgpulse(getval("pwXtoss"),ph2Xtoss,0.0,0.0);
   obsunblank();
   txphase(ph3Xtoss);
   delay((0.2236*tauR) - getval("pwXtoss"));
   rgpulse(getval("pwXtoss"),ph3Xtoss,0.0,0.0);
   obsunblank();
   txphase(ph4Xtoss);
   delay((1.0433*tauR) - getval("pwXtoss"));
   rgpulse(getval("pwXtoss"),ph3Xtoss,0.0,0.0);
   obsunblank();
   delay((0.774*tauR) - getval("pwXtoss"));

// Begin Acquisition

   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

