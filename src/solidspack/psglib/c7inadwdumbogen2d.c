/*c7inadwdumbogen2d.c - A 2Q-1Q 2D sequence using C7 with homonuclear decoupling during F1
                    and F2 using Z-rotation DUMBO and windowed DUMBO. DUMBO periods are built
		    from Fourier coefficients

                    Orignal version, See: Brown, S.P.. Lesage, S.P. Elena, B. and Emsley, L., J. Am. Chem. Soc.,
                         (2004) 126,13230-13231.  Uses I+ phase cycle.

                    This sequence is intended to be used with 1H observe and has no
                    decoupling.

		    V.Zorin 06/08/09                                                       */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,0,0,0};           // ph1Xc7
static int table2[4] = {0,0,0,0};           // phXdumbo
static int table3[4] = {0,1,2,3};           // ph2Xc7
static int table4[16] = {0,0,0,0,2,2,2,2,1,1,1,1,3,3,3,3};   // phX90
static int table5[4] = {0,0,0,0};           // phXwdumbo
static int table6[16] = {0,2,0,2,2,0,2,0,1,3,1,3,3,1,3,1};   // phRec
static int table7[4] = {1,1,1,1};           // ph1Xtilt
static int table8[4] = {3,3,3,3};           // ph2Xtilt

#define ph1Xc7 t1
#define phXdumbo t2
#define ph2Xc7 t3
#define phX90 t4
#define phXwdumbo t5
#define phRec t6
#define ph1Xtilt t7
#define ph2Xtilt t8

static double d2_init;

void pulsesequence() {

// Set the Maximum Dynamic Table and v-var Numbers

   settablenumber(20);
   setvvarnumber(30);

// Define Variables and Objects and Get Parameter Values

   MPSEQ dumbo = getdumbogen("dumboX","dcf1X",0,0.0,0.0,0,1);
   strcpy(dumbo.ch,"obs"); 
   putCmd("chXdumbo='obs'\n");

   MPSEQ c7 = getpostc7("c7X",0,0.0,0.0,0,1);  
   MPSEQ c7ref = getpostc7("c7X",c7.iSuper,c7.phAccum,c7.phInt,1,1);
   strcpy(c7.ch,"obs");
   putCmd("chXc7='obs'\n");

   WMPA wdumbo = getwdumbogen("wdumboX","dcfX");
   strcpy(wdumbo.ch,"obs");
   putCmd("chXwdumbo='obs'\n");

   double tXzfinit = getval("tXzf");            //Define the Z-filter delay in the sequence
   double tXzf = tXzfinit - 5.0e-6 - wdumbo.r1;

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
                   + wdumbo.q*wdumbo.cycles*wdumbo.pw;
   d.dutyoff = 4.0e-6 + d1 + tXzfinit + wdumbo.r2 + at - wdumbo.q*wdumbo.cycles*wdumbo.pw;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0); 

// Set Phase Tables

   settable(ph1Xc7,4,table1);
   settable(phXdumbo,4,table2);
   settable(ph2Xc7,4,table3);
   settable(phX90,16,table4);
   settable(phXwdumbo,4,table5);
   settable(phRec,16,table6);
   settable(ph1Xtilt,4,table7);
   settable(ph2Xtilt,4,table8);

// Set the Small-Angle Prep Phase

   double obsstep = 360.0/(PSD*8192);
   obsstepsize(obsstep);
   int phfX90 = initphase(0.0, obsstep);

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
   if (!getval("scXdcf1")){
   	obspwrf(getval("aX90"));
   	rgpulse(getval("pwXtilt"),ph1Xtilt,0.0,0.0);
   }
   obspwrf(getval("aXdumbo"));
   obsunblank();
   _mpseqon(dumbo,phXdumbo);
   delay(d2);
   _mpseqoff(dumbo);
   if (!getval("scXdcf1")){
   	obspwrf(getval("aX90"));
   	rgpulse(getval("pwXtilt"),ph2Xtilt,0.0,0.0);
   }
   obspwrf(getval("aX90"));
   obsunblank();

// C7 Transfer to 1Q Coherence

   xmtrphase(v2);
   _mpseq(c7ref, ph2Xc7);

// Z-filter Delay

   delay(tXzf);

// Detection Pulse

   txphase(phX90);
   obspwrf(getval("aX90"));
   startacq(5.0e-6);
   rcvroff();
   delay(wdumbo.r1);
   rgpulse(getval("pwX90"), phX90, 0.0, 0.0);
   obsunblank();
   xmtrphase(v3);
   delay(wdumbo.r2);

// Apply WPMLG Cycles During Acqusition

   decblank(); _blank34();
   _wdumbo(wdumbo,phXwdumbo);
   endacq();
   obsunblank(); decunblank(); _unblank34();  
}

