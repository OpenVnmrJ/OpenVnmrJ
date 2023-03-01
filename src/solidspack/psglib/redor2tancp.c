/*redor2tancp.c - A sequence to do REDOR with a single pi pulse on X and
                  XY8 on Y with twp pulses per rotor period.

                  D. Rice 03/25/06                                     */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,1,3,3};           // phH90
static int table2[4] = {3,0,3,0};           // phXhx
static int table3[4] = {0,0,0,0};           // phHhx
static int table4[8] = {0,1,0,1,1,0,1,0};   // ph1Yxy8
static int table5[4] = {0,0,0,0};           // ph2Yxy8
static int table6[4] = {0,1,0,1};           // phX180
static int table7[4] = {2,3,0,1};           // phRec

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define ph1Yxy8 t4
#define ph2Yxy8 t5
#define phX180 t6
#define phRec t7

void pulsesequence() {

//Define Variables and Objects and Get Parameter Values

   double aYxy8 = getval("aYxy8");  // Define the xy8XY object in the Sequence
   double pwYxy8 = getval("pwYxy8");
   double nYxy8 = getval("nYxy8");
   int cycles = (int) nYxy8/2.0;
   nYxy8 = 2.0*cycles;              // Define total cycles in pairs
   int counter = (int) (nYxy8 - 1.0);
   initval((nYxy8 - 1.0),v8);
   double onYxy8 = getval("onYxy8");
   double srate = getval("srate");

   CP hx = getcp("HX",0.0,0.0,0,1);
   strcpy(hx.fr,"dec ");
   strcpy(hx.to,"obs ");
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   DSEQ dec = getdseq("H");
   strcpy(dec.t.ch,"dec");
   putCmd("chHtppm='dec'\n"); 
   strcpy(dec.s.ch,"dec");
   putCmd("chHspinal='dec'\n");

   DSEQ mix = getdseq("Hmix");
   strcpy(mix.t.ch,"dec");
   putCmd("chHmixtppm='mix'\n"); 
   strcpy(mix.s.ch,"dec");
   putCmd("chHmixspinal='mix'\n");
//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90") + getval("tHX") + 4.0*nYxy8*pwYxy8 + getval("pwX180");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d.c3 = d.c3 + (!strcmp(mix.seq,"tppm"));
   d.c3 = d.c3 + ((!strcmp(mix.seq,"tppm")) && (mix.t.a > 0.0));
   d.t3 = 2.0*nYxy8*(1.0/srate - 2.0*pwYxy8) + 1.0/srate - getval("pwX180");
   d.c4 = d.c4 + (!strcmp(mix.seq,"spinal"));
   d.c4 = d.c4 + ((!strcmp(mix.seq,"spinal")) && (mix.s.a > 0.0));
   d.t4 = 2.0*nYxy8*(1.0/srate - 2.0*pwYxy8) + 1.0/srate - getval("pwX180");
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phH90,4,table1);
   settable(phXhx,4,table2);
   settable(phHhx,4,table3);
   settable(ph1Yxy8,8,table4);
   settable(ph2Yxy8,4,table5);
   settable(phX180,4,table6);
   settable(phRec,4,table7);

   if (counter < 0) tsadd(phRec,2,4);
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

// xy8Y Period One

  obspwrf(getval("aX180"));
  txphase(phX180);
  if (counter >= 0) {
      _dseqon(mix);
      delay(pwYxy8/2.0);
      dec2pwrf(aYxy8);
      sub(v1,v1,v1);                           
      if (counter >= 1) {
         if (counter > 1) loop(v8,v9);
	    getelem(ph1Yxy8,v1,v4);
	    incr(v1);
	    getelem(ph2Yxy8,ct,v2);
	    add(v4,v2,v2);
	    dec2phase(v2);
	    delay(0.5/srate - pwYxy8);
	    if (onYxy8 == 2)
               dec2rgpulse(pwYxy8,v2,0.0,0.0);
            else
               delay(pwYxy8);
	 if (counter > 1) endloop(v9);
      }

// X Refocussing Pulse

      delay(0.5/srate - pwYxy8/2.0 - getval("pwX180")/2.0);
      rgpulse(getval("pwX180"),phX180,0.0,0.0);
      delay(0.5/srate - pwYxy8/2.0 - getval("pwX180")/2.0);

// xy8Y Period Two

      dec2pwrf(aYxy8);
      if (counter >= 1) {
         if (counter > 1) loop(v8,v9);

	    if (onYxy8 == 2) 
               dec2rgpulse(pwYxy8,v2,0.0,0.0);
            else
               delay(pwYxy8);
            getelem(ph1Yxy8,v1,v4);
	    incr(v1);
	    getelem(ph2Yxy8,ct,v2);
	    add(v4,v2,v2);
	    dec2phase(v2);
	    delay(0.5/srate - pwYxy8);
	 if (counter > 1) endloop(v9);
      }
      delay(pwYxy8/2.0);
      _dseqoff(mix);
   }

// Begin Acquisition

   _dseqon(dec);
   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

