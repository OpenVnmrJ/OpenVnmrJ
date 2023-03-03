/*mashmqc2d.c - A sequence to obtain heteronuclear correlation through
                evolution of J-coupling (MQ) during 1H FSLG.

             The phase cycle for this program was obtained from the
             corresponding HMQC sequence in the ENS-Lyon Pulse Programming
             Library www.ens-lyon.fr/STIM/NMR/pp.html.  All other code is
             specific to the Varian NMR System.

Lesage, A., Sakellariou, D., Steurernagle, S., Emsley, L. J. Am. Chem. Soc.
             1998, 120, 7095-7100.

                D. Rice 3/6/06                                           */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,3,1,3};           // phH90 - excitation pi/2 
static int table2[4] = {0,0,0,0};           // phXhx - X contact period
static int table3[4] = {0,0,0,0};           // phHhx - H contact period
static int table4[4] = {0,0,0,0};           // ph1Hhxhmqc - tau period #1
static int table5[4] = {3,3,3,3};           // ph2Hhxhmqc - magic angle #1
static int table6[4] = {0,0,2,2};           // ph3Hhxhmqc - first pi/2
static int table7[4] = {0,0,0,0};           // ph4Hhxhmqc - magic angle #2
static int table8[4] = {3,3,3,3};           // ph5Hhxhmqc - d2 period
static int table9[32] = {1,1,1,1,1,1,1,1,   // phXhxhmqc  - X refocus
                         2,2,2,2,2,2,2,2,
                         3,3,3,3,3,3,3,3,
                         0,0,0,0,0,0,0,0};
static int table10[4] = {2,2,2,2};           // ph6Hhxhmqc - magic angle #3
static int table11[8] = {2,2,2,2,0,0,0,0};   // ph7Hhxhmqc - second pi/2
static int table12[4] = {1,1,1,1};           // ph8Hhxhmqc - magic angle #4
static int table13[4] = {0,0,0,0};           // ph9Hhxhmqc - tau period #2
static int table14[16] = {0,2,2,0,2,0,0,2,
                         2,0,0,2,0,2,2,0};   // phREC - reciever

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define ph1Hhxhmqc t4
#define ph2Hhxhmqc t5
#define ph3Hhxhmqc t6
#define ph4Hhxhmqc t7
#define ph5Hhxhmqc t8
#define phXhxhmqc t9
#define ph6Hhxhmqc t10
#define ph7Hhxhmqc t11
#define ph8Hhxhmqc t12
#define ph9Hhxhmqc t13
#define phRec t14

static double d2_init;

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   MPSEQ fh = getfslg("fslgH",0,0.0,0.0,0,1);
   strcpy(fh.ch,"dec");
   putCmd("chHfslg='dec'\n");

   double tHXhmqc = getval("tHXhmqc");     //parameters for hmqcHX implemented
   double pwHhxhmqc = getval("pwHhxhmqc"); //directly in the pulse sequence
   double pmHhxhmqc = getval("pmHhxhmqc");
   double pwXhxhmqc = getval("pwXhxhmqc");
   double aXhxhmqc = getval("aXhxhmqc");
   double aHhxhmqc = getval("aHhxhmqc");
   double d2init = getval("d2");
   d2init = d2init - pwXhxhmqc;
   if (d2init < 0.0) d2init = 0.0;
   double d22 = d2init/2.0; 

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
   d.dutyon = getval("pwH90") + getval("tHX")+ 2.0*tHXhmqc + d2_ +
              4.0*pmHhxhmqc + 2.0*pwHhxhmqc;
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
   settable(ph1Hhxhmqc,4,table4);
   settable(ph2Hhxhmqc,4,table5);
   settable(ph3Hhxhmqc,4,table6);
   settable(ph4Hhxhmqc,4,table7);
   settable(ph5Hhxhmqc,4,table8);
   settable(phXhxhmqc,32,table9);
   settable(ph6Hhxhmqc,4,table10);
   settable(ph7Hhxhmqc,8,table11);
   settable(ph8Hhxhmqc,4,table12);
   settable(ph9Hhxhmqc,4,table13);
   settable(phRec,16,table14);

// Add STATES TPPI (States with FAD)

   tsadd(ph3Hhxhmqc,2*d2_index,4);
   tsadd(ph4Hhxhmqc,2*d2_index,4);
   tsadd(ph5Hhxhmqc,2*d2_index,4);
   tsadd(ph6Hhxhmqc,2*d2_index,4);
   tsadd(phRec,2*d2_index,4);

   if (phase1 == 2) {
      tsadd(ph3Hhxhmqc,1,4);
      tsadd(ph4Hhxhmqc,1,4);
      tsadd(ph5Hhxhmqc,1,4);
      tsadd(ph6Hhxhmqc,1,4);
   }

   setreceiver(phRec);

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

// Begin hmqcHX with fh (FSLG) Between Pulses

   obspwrf(aXhxhmqc);
   _mpseqon(fh,ph1Hhxhmqc);             // First "tau" period for J evolution
   delay(tHXhmqc);
   _mpseqoff(fh);
   decpwrf(aHhxhmqc);
   decrgpulse(pmHhxhmqc, ph2Hhxhmqc, 0.0, 0.0);// Create HX double-quantum coherence
   decrgpulse(pwHhxhmqc, ph3Hhxhmqc, 0.0, 0.0);
   decrgpulse(pmHhxhmqc, ph4Hhxhmqc, 0.0, 0.0);
   _mpseqon(fh,ph5Hhxhmqc);              // Begin F1 evolution with FSLG
   delay(d22);
   rgpulse(pwXhxhmqc, phXhxhmqc, 0.0,0.0);
   delay(d22);
   _mpseqoff(fh);                        // End F1 evolution with FSLG
   decpwrf(aHhxhmqc);
   decrgpulse(pmHhxhmqc, ph6Hhxhmqc, 0.0, 0.0);// Refocus HX double quantum coherence
   decrgpulse(pwHhxhmqc, ph7Hhxhmqc, 0.0, 0.0);
   decrgpulse(pmHhxhmqc, ph8Hhxhmqc, 0.0, 0.0);
   _mpseqon(fh,ph9Hhxhmqc);              // Second "tau" period for J evolution
   delay(tHXhmqc);
   _mpseqoff(fh);      

// Begin Acquisition

   decphase(phHhx);
   _dseqon(dec);
   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}
