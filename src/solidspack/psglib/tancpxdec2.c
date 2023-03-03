/*tancpxdec2.c - A sequence to form a constant, linear or tangent ramped CP
               with simultaneous dec and dec2 SPINAL or TPPM decoupling. 

             CEB 06/28/05 for VNMRJ2.1A-B for the NMR SYSTEM
             Edited  D. Rice 6/28/05
             Edited  D. Rice 10/12/05                                  
             INOVA Optimized D.Rice 10/28/08                          
             Add dec2 decoupling 03/12/09                              */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,0,2};           // phH90
static int table2[4] = {0,0,1,1};           // phXhx
static int table3[4] = {1,1,1,1};           // phHhx
static int table4[4] = {0,2,1,3};           // phRec

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define phRec t4

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values
   
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

   DSEQ dec2 = getdseq("Y"); 
   strcpy(dec2.t.ch,"dec2");
   putCmd("chYtppm='dec2'\n"); 
   strcpy(dec2.s.ch,"dec2");
   putCmd("chYspinal='dec2'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90") + getval("tHX");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   if ((d.c1 == -1) && (d.c2 == -1)) {
      d.c3 = d.c3 + (!strcmp(dec2.seq,"tppm"));
      d.c3 = d.c3 + ((!strcmp(dec2.seq,"tppm")) && (dec2.t.a > 0.0));
      d.t3 = getval("rd") + getval("ad") + at;
      d.c3 = d.c3 + (!strcmp(dec2.seq,"spinal"));
      d.c3 = d.c3 + ((!strcmp(dec2.seq,"spinal")) && (dec2.s.a > 0.0));
      d.t4 = getval("rd") + getval("ad") + at;
   }
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phH90,4,table1);
   settable(phXhx,4,table2);
   settable(phHhx,4,table3);
   settable(phRec,4,table4);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXhx); decphase(phH90); dec2phase(zero);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to X Cross Polarization

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decphase(phHhx);
    _cp_(hx,phHhx,phXhx);

// Begin Acquisition

   _dseqon(dec);
   if (NUMch > 2) _dseqon(dec2);
   if (NUMch > 3) dec3blank();
   obsblank();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   if (NUMch > 2) _dseqoff(dec2);
   if (NUMch > 3) dec3unblank();
   obsunblank(); decunblank();
}

