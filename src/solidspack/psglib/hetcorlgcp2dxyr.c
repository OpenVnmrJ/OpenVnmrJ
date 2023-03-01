/*hetcorlgcp2dxyr.c - A sequence to provide correlation between both the
               X and Y chemical-shifts and the H chemical shift obtained
	       with FSLG. Uses an offset (Lee Goldburg) CP for mixing
	       Allows offset FSLG and tilt pulses

               D.Rice 12/15/05
	       Add offset FSLG 12/31/07
	       Add XY receive D. Rice 12/31/07

	       Uses X obs, Y dec and H dec2. Use probeConnect to set
	       dec to the second observe and dec2 to highband

	       Uses rampY to provide the Y HH match
               Finish offset capability 05/05/08                        */
#include "solidstandard.h"
#include <standard.h>

// Define Values for Phasetables

static int table1[4] = {1,1,1,1};           // ph1Htilt
static int table2[4] = {0,0,0,0};           // phHfslg
static int table3[4] = {3,3,3,3};           // ph2Htilt
static int table4[4] = {1,1,1,1};           // phH90
static int table5[4] = {1,1,3,3};           // ph3Htilt
static int table6[4] = {0,1,0,1};           // phXhx
static int table7[4] = {0,1,0,1};           // phYramp
static int table8[4] = {0,0,2,2};           // phHhx
static int table9[4] = {0,1,2,3};           // receiver

#define ph1Htilt t1
#define phHfslg t2
#define ph2Htilt t3
#define phH90 t4
#define ph3Htilt t5
#define phXhx t6
#define phYramp t7
#define phHhx t8
#define phRec t9

static double d2_init;

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   extern int NUMch;

   SHAPE p1 = getpulse("tiltH",0.0,0.0,0,1);
   strcpy(p1.pars.ch,"dec");

   MPSEQ fh = getfslg("fslgH",0,0.0,0.0,0,1);
   strcpy(fh.ch,"dec");
   putCmd("chHfslg='dec'\n");
   double pwHfslg = getval("pwHfslg");
   fh.nelem = (int) (d2/(2.0*pwHfslg) + 0.1);
   fh.hasArray = 1;
   fh = update_mpseq(fh,0,p1.pars.phAccum,p1.pars.phInt,1);

   SHAPE p2 = getpulse("tiltH",0.0,0.0,0,1);
   strcpy(p2.pars.ch,"dec");
   putCmd("chHtilt='dec'\n");
   p2.pars.hasArray = 1;
   p2 = update_shape(p2,fh.phAccum,fh.phInt,2);

// Set d22 "d2/2.0 - pwX180/2.0" From fh.t

   double pwX180 = getval("pwX180");
   double d22 = fh.t/2.0 - pwX180/2.0;
   if (d22 < 0.0) d22 = 0.0;

// CP hx RAMP ry and DSEQ dec Return to the Reference Phase

   CP hx = getcp("HX",0.0,0.0,0,1);
   strcpy(hx.fr,"dec2");
   strcpy(hx.to,"obs");
   putCmd("frHX='dec2'\n");
   putCmd("toHX='obs'\n");

   RAMP ry = getramp("rampY",0.0,0.0,0,1);
   strcpy(ry.ch,"dec");
   putCmd("chYramp='dec'\n");
   strcpy(ry.pol,"du");
   putCmd("chYramp='dec'\n");
   ry.t = getval("tHX");
   putCmd("tYramp = tHX\n");

   DSEQ dec2 = getdseq("H");
   strcpy(dec2.t.ch,"dec2");
   putCmd("chHtppm='dec2'\n"); 
   strcpy(dec2.s.ch,"dec2");
   putCmd("chHspinal='dec2'\n");

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
   d.dutyon = p1.pars.t + d2_ + p2.pars.t + getval("pwH90") + getval("pwHtilt") + 
              getval("tHX");
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

   settable(ph1Htilt,4,table1);
   settable(phHfslg,4,table2);
   settable(ph2Htilt,4,table3);
   settable(phH90,4,table4);
   settable(ph3Htilt,4,table5);
   settable(phXhx,4,table6);
   settable(phYramp,4,table7);
   settable(phHhx,4,table8);
   settable(phRec,4,table9);

//Add STATES TPPI ("States with "FAD")

   tsadd(phRec,2*d2_index,4);
   if (phase1 == 2) {
      tsadd(ph3Htilt,2*d2_index+3,4);
      tsadd(phHhx,2*d2_index+3,4);
   }
   else {
      tsadd(ph3Htilt,2*d2_index,4);
      tsadd(phHhx,2*d2_index,4);
   }
   setreceiver(phRec);

//  Begin Sequence

   txphase(phXhx); decphase(phYramp); dec2phase(ph1Htilt);
   obspwrf(getval("aXhx")); decpwrf(getval("aYramp")); dec2pwrf(getval("aHtilt"));
   obsunblank(); decunblank(); dec2unblank(); if (NUMch > 3) dec3unblank();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H Preparation with a Tilt Pulse

   _shape(p1,ph1Htilt);

// FSLG spinlock on H and Reverse Tilt to Zed

   _mpseqon(fh,phHfslg);
   delay(d22);
   rgpulse(pwX180,zero,0.0,0.0);
   obspwrf(getval("aX90"));
   txphase(phXhx);
   delay(d22);
   _mpseqoff(fh);

//Reverse tilt to Zed

   _shape(p2,ph2Htilt);
   dec2pwrf(getval("aH90"));

// H 90 and H to X Cross Polarization with LG Offset

   dec2rgpulse(getval("pwH90"),phH90,0.0,0.0);
   dec2pwrf(getval("aHtilt"));
   dec2rgpulse(getval("pwHtilt"),ph3Htilt,0.0,0.0);
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

