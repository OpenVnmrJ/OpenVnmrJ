/*mashsqc2d.c - A sequence to obtain heteronuclear correlation through
                evolution of J-coupling (SQ) during 1H FSLG.

                The phase cycle for this program was obtained from
                the corresponding HSQC sequence in the ENS-Lyon Pulse
                Programming Library www.ens-lyon.fr/STIM/NMR/pp.html.
                All other code is specific to the Varian NMR System.

         Lesage, A., Emsley, L. J. Am. Chem. Soc. 2001, 148, 449-454.

                D. Rice 3/6/06                                       */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,3,1,3};           // phH90 - excitation pi/2
static int table2[4] = {0,0,0,0};           // phXhx - X contact period
static int table3[4] = {0,0,0,0};           // phHhx - H contact period
static int table4[4] = {0,0,0,0};           // ph1Hhxhsqc - All FSLG
static int table5[4] = {1,1,1,1};           // ph2Hhxhsqc - first H pi
static int table6[4] = {1,1,1,1};           // ph1Xhxhsqc - first X pi
static int table7[4] = {3,3,3,3};           // ph3Hhxhsqc - magic angle #1
static int table8[4] = {1,1,3,3};           // ph4Hhxhsqc - first H pi/2
static int table9[8] = {0,0,0,0,2,2,2,2};   // ph2Xhxhsqc - first X pi/2
static int table10[4] = {1,1,1,1};          // ph5Hhxhsqc - magic angle #2
static int table11[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2}; // ph3Xhxhsqc - X d2 refocus                        
static int table12[4] = {3,3,3,3};          // ph6Hhxhsqc - second H pi/2
static int table13[4] = {0,0,0,0};          // ph4Xhxhsqc - second X pi/2
static int table14[4] = {1,1,1,1};          // ph7Hhxhsqc - second H pi
static int table15[4] = {1,1,1,1};          // ph5Xhxhsqc - second X pi
static int table16[8] = {0,2,2,0,2,0,0,2};  // phREC - reciever

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define ph1Hhxhsqc t4
#define ph2Hhxhsqc t5
#define ph1Xhxhsqc t6
#define ph3Hhxhsqc t7
#define ph4Hhxhsqc t8
#define ph2Xhxhsqc t9
#define ph5Hhxhsqc t10
#define ph3Xhxhsqc t11
#define ph6Hhxhsqc t12
#define ph4Xhxhsqc t13
#define ph7Hhxhsqc t14
#define ph5Xhxhsqc t15
#define phRec t16

static double d2_init;

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values
    
   double tHXhsqcinit = getval("tHXhsqc");    //parameters for hsqcHX  are implemented
   double pw1Hhxhsqc = getval("pw1Hhxhsqc");  //directly in the pulse sequence
   double pw2Hhxhsqc = getval("pw2Hhxhsqc");
   double pmHhxhsqc = getval("pmHhxhsqc");
   double pw1Xhxhsqc = getval("pw1Xhxhsqc");
   double pw2Xhxhsqc = getval("pw2Xhxhsqc");
   double aXhxhsqc = getval("aXhxhsqc");
   double aHhxhsqc = getval("aHhxhsqc");
   double d2init = getval("d2");
   d2init = d2init - pw2Xhxhsqc;
   if (d2init < 0.0) d2init = 0.0;
   double d21 = d2init/2.0;
   double d22 = d2init/2.0;
   
   double tau1 = tHXhsqcinit;
   double tau2 = tHXhsqcinit;
   double tau3 = tHXhsqcinit;
   double tau4 = tHXhsqcinit;

// Adjust First Composite 90 Simpulse

   double del1 = 0.0;
   int rev1 = 0;
   if ((pw1Xhxhsqc - pw1Hhxhsqc - 2.0*pmHhxhsqc)/2.0 > 0.0) { 
      del1 = (pw1Xhxhsqc - pw1Hhxhsqc - 2.0*pmHhxhsqc)/2.0;     
      rev1 = 0;
   }
   else if ((pw1Xhxhsqc - pw1Hhxhsqc) > 0.0) {
      del1 = (pw1Xhxhsqc - pw1Hhxhsqc)/2.0;
      rev1 = 1;
   }
   else {
      del1 = (pw1Hhxhsqc - pw1Xhxhsqc)/2.0;
      rev1 = 2;
   }
   del1 = (double) ((int) (del1/0.0125e-6 + 0.5));
   del1 = del1*0.0125e-6;
   if (del1 < 0.05e-6) del1 = 0.0;

   if (rev1 == 0)  {
      tau2 = tau2 - del1;
      if (tau2 < 0.0) tau2= 0.0;
      if (tau2 == 0.0) del1 = 0.0;
      d21 = d21 - del1;
      if (d21 < 0.0) d21 = 0.0;
      if (d21 == 0.0) del1 = 0.0;
   }

// Adjust Composite 180 Simpulse 

   double del2 = 0.0;
   int rev2 = 0;
   if ((pw2Xhxhsqc - pw2Hhxhsqc)/2.0 > 0.0) {
      del2 = (pw2Xhxhsqc - pw2Hhxhsqc )/2.0;
      rev2 = 0;
   }
   else {
      del2 = (pw2Hhxhsqc - pw2Xhxhsqc)/2.0;
      rev2 = 1;
   }
   del2 = (double) ((int) (del2/0.0125e-6 + 0.5));
   del2 = del2*0.0125e-6;
   if (del2 < 0.05e-6) del2 = 0.0;

   if (rev2 == 0)  {
      tau1 = tau1 - del2;
      tau2 = tau2 - del2;
      tau3 = tau3 - del2;
      tau4 = tau4 - del2;
      if (tau1 < 0.0) tau1 = 0.0;
      if (tau2 < 0.0) tau2 = 0.0;
      if (tau3 < 0.0) tau3 = 0.0;
      if (tau4 < 0.0) tau4 = 0.0;
      if (tau1 == 0.0) del2 = 0.0;
      if (tau2 == 0.0) del2 = 0.0;
      if (tau3 == 0.0) del2 = 0.0;
      if (tau4 == 0.0) del2 = 0.0;
   }

// Adjust Second 90 Simpulse

   double del3 = 0.0;
   int rev3 = 0;
   if ((pw1Xhxhsqc - pw1Hhxhsqc)/2.0 > 0.0) {
      del3 = (pw1Xhxhsqc - pw1Hhxhsqc )/2.0;
      rev3 = 0;
   }
   else {
      del3 = (pw1Hhxhsqc - pw1Xhxhsqc)/2.0;
      rev3 = 1;
   }

   del3 = (double) ((int) (del3/0.0125e-6 + 0.5));
   del3 = del3*0.0125e-6;
   if (del3 < 0.05e-6) del3 = 0.0;

   if (rev3 == 0)  {
      tau3 = tau3 - del3;
      if (tau3 < 0.0) tau3 = 0.0;
      if (tau3 == 0.0) del3 = 0.0;
      d22 = d22 - del3;
      if (d22 < 0.0) d22 = 0.0;
      if (d22 == 0.0) del3 = 0.0;
   }

   MPSEQ fh = getfslg("fslgH",0,0.0,0.0,0,1);
   strncpy(fh.ch,"dec",3);
   putCmd("chHfslg='dec'\n");
           
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
   d.dutyon = getval("pwH90") + getval("tHX") + 4.0*tHXhsqcinit + d2_ +
              4.0*pmHhxhsqc + pw1Hhxhsqc + pw2Hhxhsqc;
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
   settable(ph1Hhxhsqc,4,table4);
   settable(ph2Hhxhsqc,4,table5);
   settable(ph1Xhxhsqc,4,table6);
   settable(ph3Hhxhsqc,4,table7);
   settable(ph4Hhxhsqc,4,table8);
   settable(ph2Xhxhsqc,8,table9);
   settable(ph5Hhxhsqc,4,table10);
   settable(ph3Xhxhsqc,16,table11);
   settable(ph6Hhxhsqc,4,table12);
   settable(ph4Xhxhsqc,4,table13);
   settable(ph7Hhxhsqc,4,table14);
   settable(ph5Xhxhsqc,4,table15);
   settable(phRec,8,table16);

// Add STATES TPPI (States with FAD)

   tsadd(ph6Hhxhsqc,2*d2_index,4);
   tsadd(phRec,2*d2_index,4);

   if (phase1 == 2) {
      tsadd(ph6Hhxhsqc,3,4);
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

// Begin hsqcHX with fh (FSLG) Between Pulses

   _mpseqon(fh,ph1Hhxhsqc);

// First "tau/2.0" Delay

   obspwrf(aXhxhsqc);
   txphase(ph1Xhxhsqc);
   delay(tau1);

// First Simultaneous HX 180 Pulse 
   
   if (rev2 == 0) {
      xmtron();
      if (del2 > 0.0) delay(del2);
      _mpseqoff(fh);
      decphase(ph2Hhxhsqc);
      decpwrf(aHhxhsqc);
      decon();
      delay(pw2Hhxhsqc);
      decoff();
      _mpseqon(fh,ph1Hhxhsqc);
      if (del2 > 0.0) delay(del2);
      xmtroff();
   }
   else {
      _mpseqoff(fh);
      decphase(ph2Hhxhsqc);
      decpwrf(aHhxhsqc);
      decon();
      if (del2 > 0.0) delay(del2);
      xmtron();
      delay(pw2Xhxhsqc);
      xmtroff();
      if (del2 > 0.0) delay(del2);
      decoff();
      _mpseqon(fh,ph1Hhxhsqc);
   }

// Second "tau/2" Delay

   txphase(ph2Xhxhsqc);
   delay(tau2);

// Simultaneous HX (Tilted 90) Composite Pulse

   if (rev1 == 0) {
      xmtron();
      if (del1 > 0.0) delay(del1);
      _mpseqoff(fh);
      decphase(ph3Hhxhsqc);
      decpwrf(aHhxhsqc);
      decon();
      delay(pmHhxhsqc);
      decphase(ph4Hhxhsqc);
      delay(pw1Hhxhsqc);
      decphase(ph5Hhxhsqc);
      delay(pmHhxhsqc);
      decoff();
      _mpseqon(fh,ph1Hhxhsqc);
      if (del1 > 0.0) delay(del1);
      xmtroff();
   }
   else if (rev1 == 1) {
      _mpseqoff(fh);
      decphase(ph3Hhxhsqc);
      decpwrf(aHhxhsqc);
      decon();
      delay(pmHhxhsqc - del1);
      xmtron();
      if (del1 > 0.0) delay(del1);
      decphase(ph4Hhxhsqc);
      delay(pw1Hhxhsqc);
      decphase(ph5Hhxhsqc);
      if (del1 > 0.0) delay(del1);
      xmtroff();
      delay(pmHhxhsqc - del1);
      decoff();
      _mpseqon(fh,ph1Hhxhsqc);
   }
   else {
      _mpseqoff(fh);
      decphase(ph3Hhxhsqc);
      decpwrf(aHhxhsqc);
      decon();
      delay(pmHhxhsqc);
      decphase(ph4Hhxhsqc);
      if (del1 > 0.0) delay(del1);
      xmtron();
      delay(pw1Xhxhsqc);
      xmtroff();
      if (del1 > 0.0) delay(del1);
      decphase(ph5Hhxhsqc);
      delay(pmHhxhsqc);
      decoff();
      _mpseqon(fh,ph1Hhxhsqc);
   }

//  F1 Delay with X Refocussing Pulse

     txphase(ph3Xhxhsqc);
     delay(d21);
     double flag = getval("flag");
     if (flag == 0) {
        rgpulse(pw2Xhxhsqc, ph3Xhxhsqc, 0.0,0.0);
     }
     else {
        delay(pw2Xhxhsqc);
     }

     obsunblank();
     txphase(ph4Xhxhsqc);
     delay(d22);

//  Second Simulaneous HX Pulse (90 Only)

   if (rev3 == 0) {
      xmtron();
      if (del3 > 0.0) delay(del3);
      _mpseqoff(fh);
      decphase(ph6Hhxhsqc);
      decpwrf(aHhxhsqc);
      decon();
      delay(pw1Hhxhsqc);
      decoff();
      _mpseqon(fh,ph1Hhxhsqc);
      if (del3 > 0.0) delay(del3);
      xmtroff();
   }
   else {
      _mpseqoff(fh);
      decphase(ph6Hhxhsqc);
      decpwrf(aHhxhsqc);
      decon();
      if (del3 > 0.0) delay(del3);
      xmtron();
      delay(pw1Xhxhsqc);
      xmtroff();
      if (del3 > 0.0) delay(del3);
      decoff();
      _mpseqon(fh,ph1Hhxhsqc);
   }

//  Third  "tau/2.0" Delay

   txphase(ph5Xhxhsqc);
   delay(tau3);

// Second Simultaneous HX 180 Pulse

   if (rev2 == 0) {
      xmtron();
      if (del2 > 0.0) delay(del2);
      _mpseqoff(fh);
      decphase(ph7Hhxhsqc);
      decpwrf(aHhxhsqc);
      decon();
      delay(pw2Hhxhsqc);
      decoff();
      _mpseqon(fh,ph1Hhxhsqc);
      if (del2 > 0.0) delay(del2);
      xmtroff();
   }
   else {
      _mpseqoff(fh);
      decphase(ph7Hhxhsqc);
      decpwrf(aHhxhsqc);
      decon();
      if (del2 > 0.0) delay(del2);
      xmtron();
      delay(pw2Xhxhsqc);
      xmtroff();
      if (del2 > 0.0) delay(del2);
      decoff();
      _mpseqon(fh,ph1Hhxhsqc);
   }

// Fourth "tau/2.0" Delay

   delay(tau4);
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

