/*dipshftr12dfs.c - A sequence to measure 1H dipolar recoupling with quadrupole
                    nuclei using r1235 on the proton channel and DFS signal
		    enhancement, with TPPM and SPINAL decoupling.

       A. Brinkmann and A.P.M Kentgens J. Phys. Chem B 2006, 110,16089-16101.

                    D. Rice 11/9/06                                         */

#include "standard.h"
#include "solidstandard.h"
// Define Values for Phasetables

static int table1[4] = {1,1,1,1};           // phXdfs
static int table2[4] = {0,2,0,2};           // phX90
static int table3[4] = {0,0,0,0};           // phHr12
static int table4[4] = {0,0,0,0};           // phXecho
static int table5[4] = {0,2,0,2};           // phRec

#define phXdfs t1
#define phX90 t2
#define phHr12 t3
#define phXecho t4
#define phRec t5

static double d2_init;

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   double aXecho = getval("aXecho");// Spin echo defined in the pulse sequence
   double t1Xechoinit = getval("t1Xecho");
   double pwXecho = getval("pwXecho"); 
   double t2Xechoinit = getval("t2Xecho");
   double t1Xecho  = t1Xechoinit - pwXecho/2.0 - getval("d2");
   if (t1Xecho < 0.0) t1Xecho = 0.0; 
   double t2Xecho  = t2Xechoinit - pwXecho/2.0 - getval("rd");
   if (t2Xecho < 0.0) t2Xecho = 0.0;

   MPSEQ r12 = getr1235("r12H",0,0.0,0.0,0,1);
   strncpy(r12.ch,"dec",3);
   putCmd("chHr12='dec'\n");

   SHAPE dfs = getdfspulse("dfsX",0.0,0.0,0,1);
   strncpy(dfs.pars.ch,"obs",3);
   putCmd("chXdfs='obs'\n");

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

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = dfs.pars.t + getval("pwX90") + d2_ + getval("pwXecho");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = t1Xecho + t2Xecho + getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phXdfs,4,table1);
   settable(phX90,4,table2);
   settable(phHr12,4,table3);
   settable(phXecho,4,table4);
   settable(phRec,4,table5);
   setreceiver(phRec);

// Begin Sequence

   txphase(phX90); decphase(phHr12);
   obspwrf(getval("aX90")); decpwrf(getval("aHr12"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// X Double Frequency Sweep Pulse

   _shape(dfs,phXdfs);
   txphase(phX90);
   obspwrf(getval("aX90"));
   delay(200.0e-6);

// X Direct Polarization

   rgpulse(getval("pwX90"),phX90,0.0,0.0);

// F1 with 1H r1253

   txphase(phXecho);
   obspwrf(aXecho);
   _mpseqon(r12,phHr12);
   delay(d2);
   _mpseqoff(r12);
   _dseqon(dec);

// X Hahn Echo

   delay(t1Xecho); 
   rgpulse(pwXecho,phXecho,0.0,0.0);
   delay(t2Xecho);

// Begin Acquisition

   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

