/*c7inadwpmlg2d.c - A 2Q-1Q 2D sequence using C7 with homonuclear decoupling
                    during F1 and F2 using PMLG and windowed PMLG. 

  See: Brown, S.P.. Lesage, S.P. Elena, B. and Emsley, L., J. Am. Chem. Soc.,
                   (2004) 126,13230-13231. 

                    Relative to the reference this sequence replaces DUMBO
		    with PMLG and uses quadrature detection in F2 rather than
		    flip-back pulses to remove quadrature artifacts.

                    This sequence is intended to be used with 1H observe and
		    has no decoupling. 

                    D.Rice 05/18/06                                        */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,0,0,0};           // ph1Xc7
static int table2[4] = {1,1,1,1};           // ph1Xtilt
static int table3[4] = {0,0,0,0};           // phXpmlg
static int table4[4] = {3,3,3,3};           // ph2Xtilt
static int table5[4] = {0,2,1,3};           // ph2Xc7
static int table6[4] = {0,2,1,3};           // ph1Xprep1
static int table7[4] = {1,1,1,1};           // ph2Xprep1
static int table8[4] = {0,0,0,0};           // phXwpmlg
static int table9[4] = {0,2,1,3};           // phRec

#define ph1Xc7 t1
#define ph1Xtilt t2
#define phXpmlg t3
#define ph2Xtilt t4
#define ph2Xc7 t5
#define ph1Xprep1 t6
#define ph2Xprep1 t7
#define phXwpmlg t8
#define phRec t9

static double d2_init;

pulsesequence() {

// Set the Maximum Dynamic Table and v-var Numbers

   settablenumber(20);
   setvvarnumber(30);

// Define Variables and Objects and Get Parameter Values

   MPSEQ pmlg = getpmlg("pmlgX",0,0.0,0.0,0,1);
   strncpy(pmlg.ch,"obs",3);
   putCmd("chXpmlg='obs'\n");

   MPSEQ c7 = getpostc7("c7X",0,0.0,0.0,0,1);
   MPSEQ c7ref = getpostc7("c7X",c7.iSuper,c7.phAccum,c7.phInt,1,1);
   strncpy(c7.ch,"obs",3);
   putCmd("chXc7='obs'\n");

   WMPA wpmlg = getwpmlg("wpmlgX");
   strncpy(wpmlg.ch,"obs",3);
   putCmd("chXwpmlg='obs'\n");

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
   d.dutyon = c7.t + getval("pwXtilt") + d2_ + getval("pwXtilt") + c7ref.t + 
                   getval("pw1Xprep1") + getval("pw2Xprep1") + 
                   2.0*wpmlg.q*wpmlg.cycles*wpmlg.pw;

   d.dutyoff = 4.0e-6 + d1 + at - 2.0*wpmlg.q*wpmlg.cycles*wpmlg.pw;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0); 

// Set Phase Tables

   settable(ph1Xc7,4,table1);
   settable(ph1Xtilt,4,table2);
   settable(phXpmlg,4,table3);
   settable(ph2Xtilt,4,table4);
   settable(ph2Xc7,4,table5);
   settable(ph1Xprep1,4,table6);
   settable(ph2Xprep1,4,table7);
   settable(phXwpmlg,4,table8);
   settable(phRec,4,table9);

//Add STATES TPPI ("States with "FAD")

   double obsstep = 360.0/(PSD*8192);
   if (phase1 == 2)
      initval((45.0/obsstep),v1);
   else
      initval(0.0,v1);

   initval((d2*c7.of[0]*360.0/obsstep),v2);
   initval(0.0,v3);
   obsstepsize(obsstep);
   setreceiver(phRec);

// Begin Sequence

   xmtrphase(v1); txphase(ph1Xc7);
   obspwrf(getval("aXc7"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// C7 Recoupling of 2Q coherence 

   _mpseq(c7, ph1Xc7);

// F1 Evolution With PMLG

   obspwrf(getval("aXprep1"));
   rgpulse(getval("pwXtilt"),ph1Xtilt,0.0,0.0);
   _mpseqon(pmlg,phXpmlg);
   delay(d2);
   _mpseqoff(pmlg);
   obspwrf(getval("aXprep1"));
   rgpulse(getval("pwXtilt"),ph2Xtilt,0.0,0.0);

// C7 Transfer to 1Q coherence

   xmtrphase(v2);
   _mpseq(c7ref, ph2Xc7);

// Tilted Detection Pulse Using prep1X

   xmtrphase(v3);
   obspwrf(getval("aXprep1"));
   startacq(5.0e-6);
   rcvroff();
   delay(wpmlg.r1);
   rgpulse(getval("pw1Xprep1"), ph1Xprep1, 0.0, 0.0);
   rgpulse(getval("pw2Xprep1"), ph2Xprep1, 0.0, 0.0);
   delay(wpmlg.r2);             

// Apply WPMLG Cycles During Acqusition

   decblank(); _blank34();
   _wpmlg(wpmlg, phXwpmlg);
   endacq();
   obsunblank(); decunblank(); _unblank34();
}

