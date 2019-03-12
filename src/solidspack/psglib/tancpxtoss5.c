/*tancpxtoss5.c - A sequence to form a constant, linear or tangent ramped CP
                 with TOSS5 sideband suppression.

             CEB 06/28/05 for VNMRJ2.1A-B for the NMR SYSTEM
             Edited  D. Rice 6/28/05
             Edited  D. Rice 10/12/05 tancpx.c
             Editied D. Rice 01/31/06

             Change to TOSS5 D. Rice 10/01/08                             */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[22] = {0,0,0,0,0,0,0,0,0,0,0,
                         2,2,2,2,2,2,2,2,2,2,2};    // phH90
static int table2[11] = {0,3724,7447,2979,6703,2234,
                         5958,1489,5213,745,4468};  // phfXhx
static int table3[44] = {0,0,0,0,0,0,0,0,0,0,0,
                         0,0,0,0,0,0,0,0,0,0,0,
                         1,1,1,1,1,1,1,1,1,1,1,
                         1,1,1,1,1,1,1,1,1,1,1};    // phXhx
static int table4[4] = {1,1,1,1};                   // phHhx
static int table5[4] = {1,1,1,1};                   // phHdec
static int table6[44] = {0,0,0,0,0,0,0,0,0,0,0,
                         0,0,0,0,0,0,0,0,0,0,0,
                         1,1,1,1,1,1,1,1,1,1,1,
                         1,1,1,1,1,1,1,1,1,1,1};    // phXtoss
static int table7[44] = {0,0,0,0,0,0,0,0,0,0,0,
                         2,2,2,2,2,2,2,2,2,2,2,
                         1,1,1,1,1,1,1,1,1,1,1,
                         3,3,3,3,3,3,3,3,3,3,3};    // phRec

#define phH90 t1
#define phfXhx t2
#define phXhx t3
#define phHhx t4
#define phHdec t5
#define phXtoss t6
#define phRec t7

void pulsesequence() {

// Set the Maximum Dynamic Table Number

   settablenumber(10);
   setvvarnumber(30);
  
// Define Variables and Objects and Get Parameter Values

   CP hx = getcp("HX",0.0,0.0,0,1);
   strncpy(hx.fr,"dec",3);
   strncpy(hx.to,"obs",3);
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   WMPA toss = gettoss5("tossX");
   strncpy(toss.ch,"obs",3);
   putCmd("chXtoss='obs'\n");

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
   d.dutyon = getval("pwH90") + getval("tHX") + 5.0*toss.pw;
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = toss.rtau - 5.0*toss.pw + getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = toss.rtau - 5.0*toss.pw + getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phH90,22,table1);
   settable(phfXhx,11,table2);
   settable(phXhx,44,table3);
   settable(phHhx,4,table4);
   settable(phHdec,4,table5);
   settable(phXtoss,44,table6);
   settable(phRec,44,table7);
   setreceiver(phRec);

   obsstepsize(360.0/(PSD*8192));

// Begin Sequence

   xmtrphase(phfXhx); txphase(phXhx); decphase(phH90);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to X Cross Polarization - Shifted by -6COG11

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decphase(phHhx);
    _cp_(hx,phHhx,phXhx);
   decphase(phHdec);

// TOSS5 Sideband Suppression with included
// (-6,-5,-6,-5,-6)COG11 Cycle

   _dseqon(dec);
   _toss5(toss, phXtoss);

// Begin Acquisition with Quadrature Phase

   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

