/*c7inadwdumbot2d.c - A 2Q-1Q 2D sequence using C7 with homonuclear decoupling during F1
                    and F2 using DUMBO and windowed DUMBO with tilted acquisition.

                    See: Brown, S.P.. Lesage, S.P. Elena, B. and Emsley, L., J. Am. Chem. Soc.,
                         (2004) 126,13230-13231.  Uses I+ phase cycle.

                    This sequence is intended to be used with 1H observe and has no
                    decoupling.

                    D.Rice 05/18/06                                                       */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,0,0,0};           // ph1Xc7
static int table2[4] = {1,1,1,1};           // ph1Xtilt
static int table3[4] = {0,0,0,0};           // phXdumbo
static int table4[4] = {3,3,3,3};           // ph2Xtilt
static int table5[4] = {0,1,2,3};           // ph2Xc7
static int table6[8] = {0,0,0,0,2,2,2,2};   // phXprep
static int table7[4] = {1,1,1,1};           // phXwdumbot
static int table8[8] = {0,2,0,2,2,0,2,0};   // phRec

#define ph1Xc7 t1
#define ph1Xtilt t2
#define phXdumbo t3
#define ph2Xtilt t4
#define ph2Xc7 t5
#define phXprep t6
#define phXwdumbot t7
#define phRec t8

static double d2_init;

pulsesequence() {

// Set the Maximum Dynamic Table and v-var Numbers

   settablenumber(20);
   setvvarnumber(30);

// Define Variables and Objects and Get Parameter Values

   MPSEQ dumbo = getdumbo("dumboX",0,0.0,0.0,0,1);
   strncpy(dumbo.ch,"obs",3); 
   putCmd("chXdumbo='obs'\n");

   MPSEQ c7 = getpostc7("c7X",0,0.0,0.0,0,1);  
   MPSEQ c7ref = getpostc7("c7X",c7.iSuper,c7.phAccum,c7.phInt,1,1);
   strncpy(c7.ch,"obs",3);
   putCmd("chXc7='obs'\n");

   WMPA wdumbot = getwdumbot("wdumbotX");
   strncpy(wdumbot.ch,"obs",3);
   putCmd("chXwdumbot='obs'\n");

   double tXzfinit = getval("tXzf");            //Define the Z-filter delay in the sequence
   double tXzf = tXzfinit - 5.0e-6 - wdumbot.r1;

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
   d.dutyon = c7.t + getval("pwXtilt") + d2_ + getval("pwXtilt") + c7ref.t + getval("pwX90") +
                     wdumbot.cycles*(wdumbot.q*wdumbot.pw + 2.0*wdumbot.t1);
   d.dutyoff = 4.0e-6 + d1 + at - wdumbot.cycles*(wdumbot.q*wdumbot.pw + 2.0*wdumbot.t1);
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0); 

// Set Phase Tables

   settable(ph1Xc7,4,table1);
   settable(ph1Xtilt,4,table2);
   settable(phXdumbo,4,table3);
   settable(ph2Xtilt,4,table4);
   settable(ph2Xc7,4,table5);
   settable(phXprep,8,table6);
   settable(phXwdumbot,4,table7);
   settable(phRec,8,table8);

// Set the Small-Angle Prep Phase

   double obsstep = 360.0/(PSD*8192);
   obsstepsize(obsstep);
   int phfXprep = initphase(getval("phXprep"), obsstep);

//Add STATES Quadrature Phase

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

// F1 Evolution With DUMBO

   xmtrphase(v3);
   obspwrf(getval("aXprep"));
   rgpulse(getval("pwXtilt"),ph1Xtilt,0.0,0.0);
   obsunblank();
   _mpseqon(dumbo,phXdumbo);
   delay(d2);
   _mpseqoff(dumbo);
   obspwrf(getval("aXprep"));
   rgpulse(getval("pwXtilt"),ph2Xtilt,0.0,0.0);
   obsunblank();

// C7 Transfer to 1Q Coherence

   xmtrphase(v2);
   _mpseq(c7ref, ph2Xc7);

// Z-filter Delay

   delay(tXzf);

// Detection Pulse

   xmtrphase(phfXprep); txphase(phXprep);
   obspwrf(getval("aXprep"));
   startacq(5.0e-6);
   rcvroff();
   delay(wdumbot.r1);
   rgpulse(getval("pwXprep"), phXprep, 0.0, 0.0);
   obsunblank();
   xmtrphase(v3);
   delay(wdumbot.r2);

// Apply WPMLG Cycles During Acqusition

   decblank(); _blank34();
   _wdumbot(wdumbot,phXwdumbot);
   endacq();
   obsunblank(); decunblank(); _unblank34();  
}

