/*masapt1d.c - A sequence to obtain an attached proton test through
               evolution of J-coupling during 1H FSLG.

               The phase cycle for this program was obtained from the
               corresponding APT sequence in the ENS-Lyon Pulse Programming
               Library www.ens-lyon.fr/STIM/NMR/pp.html.  All other code is
               specific to the Varian NMR System.

Lesage, A., Steurernagle, S., Emsley, L. J. Am. Chem. Soc. 1998, 120, 7095-7100.

               D. Rice 3/6/06                                               */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,3,1,3};           // phH90 - excitation pi/2
static int table2[4] = {0,0,0,0};           // phXhx - X contact period
static int table3[4] = {0,0,0,0};           // phHhx - H contact period
static int table4[4] = {0,0,0,0};           // ph1Hhxaptf - tau period #1
static int table5[4] = {1,1,1,1};           // ph2Hhxaptf - H pi pulse
static int table6[8] = {1,1,2,2,3,3,0,0};   // phXhxaptf  - X refocus
static int table7[4] = {0,0,0,0};           // ph3Hhxaptf - tau period #2
static int table8[4] = {0,2,2,0};           // phRec - reciever

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define ph1Hhxaptf t4
#define ph2Hhxaptf t5
#define phXhxaptf t6
#define ph3Hhxaptf t7
#define phRec t8

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   MPSEQ fh = getfslg("fslgH",0,0.0,0.0,0,1);
   strncpy(fh.ch,"dec",3);
   putCmd("chHfslg='dec'\n");

   double tHXaptfinit = getval("tHXaptf"); //parameters for aptHX implemented
   double tHXaptf = tHXaptfinit; 
   double pwHhxaptf = getval("pwHhxaptf"); //directly in the pulse sequence
   double pwXhxaptf = getval("pwXhxaptf");
   double aXhxaptf = getval("aXhxaptf");
   double aHhxaptf = getval("aHhxaptf");
   double del = (pwXhxaptf - pwHhxaptf)/2.0;
   double rev = 0.0;
   if (del < 0.0) {
      del = -del;
      rev = 1;
   }
   del = (double) ((int) (del/0.0125e-6 + 0.5));
   del = del*0.0125e-6;
   if (del < 0.05e-6) del = 0.0;

   if (rev == 0) {
      tHXaptf = tHXaptfinit - del;
      if (tHXaptf < 0.0) tHXaptf = 0.0;
      if (tHXaptf == 0.0) del = 0.0;
   }

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

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90") + getval("tHX") + 2.0*tHXaptf +
              pwHhxaptf;
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
   settable(ph1Hhxaptf,4,table4);
   settable(ph2Hhxaptf,4,table5);
   settable(phXhxaptf,8,table6);
   settable(ph3Hhxaptf,4,table7);
   settable(phRec,4,table8);
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

// Begin aptfHX with fh (FSLG) Between Pulses

   obspwrf(aXhxaptf);
   txphase(phXhxaptf);
   _mpseqon(fh,ph1Hhxaptf);  // First "tau" period for J evolution
   if (rev == 0) {
      delay(tHXaptf);
      xmtron();
      if (del > 0.0) delay(del);
      _mpseqoff(fh);
      decphase(ph2Hhxaptf);
      decpwrf(aHhxaptf);
      decon();
      delay(pwHhxaptf);
      decoff();
      _mpseqon(fh,ph3Hhxaptf);
      if (del > 0.0) delay(del);
      xmtroff();
      delay(tHXaptf);
   }
   else {
      delay(tHXaptf);
      _mpseqoff(fh);
      decphase(ph2Hhxaptf);
      decpwrf(aHhxaptf);
      decon();
      if (del > 0.0) delay(del);
      xmtron();
      delay(pwXhxaptf);
      xmtroff();
      if (del > 0.0) delay(del);
      decoff();
      _mpseqon(fh,ph3Hhxaptf);
      delay(tHXaptf);
   }
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
