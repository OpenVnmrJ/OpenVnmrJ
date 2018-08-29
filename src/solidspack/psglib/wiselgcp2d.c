/*wisetancp2d.c - A sequence to provide correlation between the
                  X chemical-shift and 1H wideline spectrum using 
                  Lee_Goldburg CP and Decoupling on X.

                  D.Rice 12/15/05
                  Full NOESY cycle 06/14/07                          
                  Lee-Goldburg CP and decoupling  04/09/10        */

#include "solidstandard.h"
#include <standard.h>

// Define Values for Phasetables

static int table1[16] = {0,2,2,0,3,1,1,3,2,0,0,2,1,3,3,1}; // phH90
static int table2[16] = {0,0,0,0,3,3,3,3,2,2,2,2,1,1,1,1}; // phHmix1
static int table3[4] = {0,0,2,2};                          // phHmix2
static int table4[4] = {2,2,2,2};                           // phHtilt
static int table5[4] = {0,0,1,1};                          // phXhx
static int table6[4] = {1,1,1,1};                          // phHhx
static int table7[4] = {0,2,1,3};                          // phRec

#define phH90 t1
#define phHmix1 t2
#define phHmix2 t3
#define phHtilt t4
#define phXhx t5
#define phHhx t6
#define phRec t7

static double d2_init;

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   CP hx = getcp("HX",0.0,0.0,0,1);
   strncpy(hx.fr,"dec",3);
   strncpy(hx.to,"obs",3);
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   DSEQ dec = getdseq("H");
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHtppm='dec'\n"); 
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHspinal='dec'\n");

   DSEQ xdec = getdseq("X");
   strncpy(xdec.t.ch,"obs",3);    // These four statements assure 
   strncpy(xdec.s.ch,"obs",3);    // that the X decoupling will
   putCmd("chXtppm='obs'\n");     // be on X for either TPPM or 
   putCmd("chXspinal='obs'\n");   // SPINAL. 

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
   d.dutyon = 3.0*getval("pwH90") + getval("pwHtilt") + getval("tHX");
   d.dutyoff = d1 + 4.0e-6 + getval("tHmix"); 
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d.c3 = d.c3 + (!strcmp(xdec.seq,"tppm"));
   d.c3 = d.c3 + ((!strcmp(xdec.seq,"tppm")) && (xdec.t.a > 0.0));
   d.t3 = d2_;
   d.c4 = d.c4 + (!strcmp(xdec.seq,"spinal"));
   d.c4 = d.c4 + ((!strcmp(xdec.seq,"spinal")) && (xdec.s.a > 0.0));
   d.t4 = d2_;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0); 

// Set Phase Tables

   settable(phH90,16,table1);
   settable(phHmix1,16,table2);
   settable(phHmix2,4,table3);
   settable(phHtilt,4,table4);
   settable(phXhx,4,table5);
   settable(phHhx,4,table6);
   settable(phRec,4,table7);

   if (phase1 == 2) tsadd(phH90,1,4);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXhx); decphase(phH90);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H Preparation and tilt

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decunblank();

// Delay for 1H Wideline T2

   _dseqon(xdec);
   delay(d2);
   _dseqoff(xdec); 

// Mix period for Spin Diffison

   decrgpulse(getval("pwH90"),phHmix1,0.0,0.0);
   delay(getval("tHmix"));
   decrgpulse(getval("pwH90"),phHmix2,0.0,0.0);

// Tilt Pulse and Ramped H to X Cross Polarization with LG Offset

   decrgpulse(getval("pwHtilt"),phHtilt,0.0,0.0);
   decphase(phHhx);
   _cp_(hx,phHhx,phXhx);

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
