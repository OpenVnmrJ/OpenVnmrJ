/*seac71d.c - A sequence to provide X frequency selective 13C longitudinal
              homonuclear recoupling using shifted evolution enhanced C7, SEAC7
              with SPINAL and TPPM decoupling.


               D. Rice 01/09/08                                            */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,0,0,0};           // phH90
static int table2[4] = {0,0,0,0};           // phXhx
static int table3[4] = {1,1,1,1};           // phHhx
static int table4[4] = {1,1,1,1};           // ph1Xmix
static int table5[4] = {0,0,0,0};           // phXgauss
static int table6[4] = {0,0,0,0};           // phXseac7
static int table7[4] = {0,0,0,0};           // ph2Xmix
static int table8[4] = {0,2,0,2};           // phRec

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define ph1Xmix t4
#define phXgauss t5
#define phXseac7 t6
#define ph2Xmix t7
#define phRec t8

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   int n;

   CP hx = getcp("HX",0.0,0.0,0,1);
   strncpy(hx.fr,"dec",3);
   strncpy(hx.to,"obs",3);
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   MPSEQ seac7 = getseac7("seac7X",0,0.0,0.0,0,1);
   strncpy(seac7.ch,"obs",3);
   putCmd("chXseac7='obs'\n");

   DSEQ dec = getdseq("H");
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHtppm='dec'\n"); 
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHspinal='dec'\n");

// Set the Gaussian Pulse in n Rotor Periods

   double srate = getval("srate");
   double taur = 2.0e-3;
   if (srate > 500) taur = 1.0/srate;
   double pwXgauss = getval("pwXgauss");
   n = ((int) (pwXgauss/taur)) + 1;
   double tau = 0.5*((double) (n*taur - pwXgauss));

// Calculate the Selective Gaussian Pulse with PBox

   double ofXgauss = getval("ofXgauss");
   char   cmd[MAXSTR];
   static shape shXgauss;

   if (getval("arraydim") < 1.5||(ix==1)||isarry("ofXgauss")||isarry("pwXgauss")) {
      sprintf(shXgauss.name, "%s_%d", "arr", ix);
      sprintf(cmd, "Pbox %s -w \"gaus180 %.7f %.1f\" -0\n",shXgauss.name,pwXgauss,ofXgauss);
      system(cmd);
      shXgauss = getRsh(shXgauss.name);
   }

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90") + getval("tHX") + getval("pwX90") + 2.0*tau + 
   pwXgauss + seac7.t + getval("tZF");
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
   settable(ph1Xmix,4,table4);
   settable(phXgauss,4,table5);
   settable(phXseac7,4,table6);
   settable(ph2Xmix,4,table7);
   settable(phRec,4,table8);
   setreceiver(phRec); 

// Set Gaussian Counter

   mod2(ct,v1);  //{0,1,0,1)

// Begin Sequence

   txphase(phXhx); decphase(phH90);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to X Cross Polarization with Zed Prep Pulse

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decphase(phHhx);
   _cp_(hx,phHhx,phXhx);
   obspwrf(getval("aX90"));
   rgpulse(getval("pwX90"),ph1Xmix,0.0,0.0);
   decpwrf(getval("aHmix"));
   obsunblank();
   delay(getval("tZF"));

// Selective Gaussian Pulse Applied on Alternate Scans

   decon();
   txphase(phXgauss);
   obspower(getval("dbXgauss"));
   obspwrf(getval("aXgauss"));
   delay(tau);
   ifzero(v1);
      shaped_pulse(shXgauss.name,pwXgauss,phXgauss,0.0,0.0);
   elsenz(v1);
      delay(pwXgauss);
   endif(v1);
   obspower(tpwr);
   obsunblank();
   txphase(phXseac7);
   obspwrf(getval("aXseac7"));
   delay(tau);

// N Cycles of Shift Evolution Assisted POSTC7, SEAC7

   _mpseq(seac7, phXseac7);

// Reconversion Pulse

   txphase(ph2Xmix);
   obspwrf(getval("aX90"));
   delay(getval("tZF"));
   rgpulse(getval("pwX90"),ph2Xmix,0.0,0.0);
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

