/*tancpxyr.c - A sequence to form a constant, linear or tangent ramped CP.
              from H to both X and Y for multiple receive experiments

              CEB 06/28/05 for VNMRJ2.1A-B for the NMR SYSTEM
              Edited  D. Rice 6/28/05
              Edited  D. Rice 10/12/05
	      XY Receive D.Rice 12/31/07

	      Uses X obs, Y dec and H dec2. Use ProbeConnect to set
	      dec to the second observe and dec2 to highband

	      Uses rampY to provide the Y HH match               */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,0,2};           // phH90
static int table2[4] = {0,0,1,1};           // phXhx
static int table3[4] = {0,0,1,1};           // phYramp
static int table4[4] = {1,1,1,1};           // phHhx
static int table5[4] = {0,2,1,3};           // phRec

#define phH90 t1
#define phXhx t2
#define phYramp t3
#define phHhx t4
#define phRec t5

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   extern int NUMch;

   CP hx = getcp("HX",0.0,0.0,0,1);
   strncpy(hx.fr,"dec2",4);
   strncpy(hx.to,"obs",3);
   putCmd("frHX='dec2'\n");
   putCmd("toHX='obs'\n");

   RAMP ry = getramp("rampY",0.0,0.0,0,1);
   strncpy(ry.ch,"dec",3);
   putCmd("chYramp='dec'\n");
   strncpy(ry.pol,"du",2);
   putCmd("chYramp='dec'\n");
   ry.t = getval("tHX");
   putCmd("tYramp = tHX\n");

   DSEQ dec2 = getdseq("H");
   strncpy(dec2.t.ch,"dec2",3);
   putCmd("chHtppm='dec2'\n"); 
   strncpy(dec2.s.ch,"dec2",3);
   putCmd("chHspinal='dec2'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90") + getval("tHX");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec2.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec2.seq,"tppm")) && (dec2.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec2.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec2.seq,"spinal")) && (dec2.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phH90,4,table1);
   settable(phXhx,4,table2);
   settable(phYramp,4,table3);
   settable(phHhx,4,table4);
   settable(phRec,4,table5);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXhx); decphase(phYramp); decphase(phH90);
   obspwrf(getval("aXhx")); decpwrf(getval("aYramp")); dec2pwrf(getval("aH90"));
   obsunblank(); decunblank(); dec2unblank(); if (NUMch > 3) dec3unblank();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to X Cross Polarization

   dec2rgpulse(getval("pwH90"),phH90,0.0,0.0);
   dec2phase(phHhx);
   _rampon(ry,phYramp);
   _cp_(hx,phHhx,phXhx);
   _rampoff(ry);

// Begin Acquisition

   _dseqon(dec2);
   obsblank(); decblank(); if (NUMch > 3) dec3blank();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec2);
   obsunblank(); decunblank(); dec2unblank(); if (NUMch > 3) dec3unblank();
}

