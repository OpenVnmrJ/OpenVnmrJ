/*cpcpmg1d.c - A sequence to do constant, linear or tangent
               ramped cross polarization and an echo with
               CPMG detection and TPPM or SPINAL decoupling.

              Larsen et al, J. Phys Chem A Vol 101, 46,8597-8606, 1997

              Phase cycle provided by A. Lipton
              D.Rice 01/29/07  */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[64] = {0,2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,
                        1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,
                        2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,0,
                        3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1};// phH90
static int table2[64] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                        2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
                        3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
                        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};// phXhx
static int table3[64] = {2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,0,
                        3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,
                        0,2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,
                        1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3};// phHhx
static int table4[64] = {3,3,3,3,3,3,3,3,1,1,1,1,1,1,1,1,
                        0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,
                        1,1,1,1,1,1,1,1,3,3,3,3,3,3,3,3,
                        2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0};// phXecho
static int table5[64] = {0,0,1,1,2,2,3,3,0,0,1,1,2,2,3,3,
                        1,1,2,2,3,3,0,0,1,1,2,2,3,3,0,0,
                        2,2,3,3,0,0,1,1,2,2,3,3,0,0,1,1,
                        3,3,0,0,1,1,2,2,3,3,0,0,1,1,2,2};// phXcpmg
static int table6[64] = {1,3,3,1,1,3,3,1,1,3,3,1,1,3,3,1,
                        2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,
                        3,1,1,3,3,1,1,3,3,1,1,3,3,1,1,3,
                        0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0};// phRec

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define phXecho t4
#define phXcpmg t5
#define phRec t6

void pulsesequence() {

//
// Set the Maximum Dynamic Table Number
//

   settablenumber(10);
   setvvarnumber(30);

//Define Variables and Objects and Get Parameter Values

   CP hx = getcp("HX",0.0,0.0,0,1);
   strncpy(hx.fr,"dec",3);
   strncpy(hx.to,"obs",3);
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   WMPA cpmg = getcpmg("cpmgX");
   strncpy(cpmg.ch,"obs",3);
   putCmd("chXcpmg='obs'\n");

   double aXecho = getval("aXecho");  // define the echoX group in the sequence
   double t1Xechoinit = getval("t1Xecho");
   double pwXecho = getval("pwXecho");
   double t2Xechoinit = getval("t2Xecho");
   double t1Xecho  = t1Xechoinit - pwXecho/2.0 - getval("pwX90")/2.0;
   if (t1Xecho < 0.0) t1Xecho = 0.0;
   double t2Xecho  = t2Xechoinit - pwXecho/2.0 - cpmg.r1 - cpmg.t2 - getval("ad");
   if (t2Xecho < 0.0) t2Xecho = 0.0;

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
   d.dutyon = getval("pwH90") + getval("tHX") + pwXecho + (cpmg.cycles - 1)*cpmg.pw; 
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = t1Xecho + t2Xecho + getval("rd") + getval("ad") + 
          at - (cpmg.cycles - 1)*cpmg.pw;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = t1Xecho + t2Xecho + getval("rd") + getval("ad") + 
          at - (cpmg.cycles - 1)*cpmg.pw;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);


// Set Phase Tables

   settable(phH90,64,table1);
   settable(phXhx,64,table2);
   settable(phHhx,64,table3);
   settable(phXecho,64,table4);
   settable(phXcpmg,64,table5);
   settable(phRec,64,table6);
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

// H Decoupling On

   decphase(zero);
   _dseqon(dec);

// X Hahn Echo

   txphase(phXecho);
   obspwrf(aXecho);
   delay(t1Xecho);
   rgpulse(pwXecho,phXecho,0.0,0.0);
   delay(t2Xecho);

// Apply CPMG Cycles

   obsblank(); _blank34();
   delay(cpmg.r1);
   startacq(getval("ad"));
   _cpmg(cpmg,phXcpmg);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

