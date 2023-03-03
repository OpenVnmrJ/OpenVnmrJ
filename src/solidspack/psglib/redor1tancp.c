/*redor1tancp.c - XY REDOR with alternating pi pulses on the two channels
                  during REDOR evolution.

                  D. Rice 03/24/06                                     */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,3,1,3};           // phH90
static int table2[8] = {0,0,1,1,2,2,3,3};   // phXhx
static int table3[4] = {0,0,0,0};           // phHhx
static int table4[8] = {0,1,0,1,1,0,1,0};   // phXYxy8
static int table5[4] = {0,0,0,0};           // phYxyxy8
static int table6[4] = {0,0,0,0};           // phXxyxy8
static int table7[8] = {0,2,1,3,2,0,3,1};   // phRec

static int table8[8] = {0,0,2,2,0,0,2,2};   // ph1Rec
static int table9[8] = {2,2,2,2,2,2,2,2};   // ph2Rec
static int table10[8] = {2,2,0,0,2,2,0,0};  // ph3Rec

#define phH90    t1
#define phXhx    t2
#define phHhx    t3
#define phXxyxy8 t4
#define phYxyxy8 t5
#define phXYxy8 t6
#define phRec    t7
#define ph1Rec   t8
#define ph2Rec   t9
#define ph3Rec   t10

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   double aXxyxy8 = getval("aXxyxy8");
   double aYxyxy8 = getval("aYxyxy8");
   double pwXxyxy8 = getval("pwXxyxy8");
   double pwYxyxy8 = getval("pwYxyxy8");
   double nXYxy8 = getval("nXYxy8");
   int counter = (int) nXYxy8;
   initval(nXYxy8,v8);
   double fXYxy8 = getval("fXYxy8");
   double onYxyxy8 = getval("onYxyxy8");
   double srate = getval("srate");

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
   d.dutyon = getval("pwH90") + getval("tHX") + nXYxy8*(pwXxyxy8 + pwYxyxy8);
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d.c3 = d.c3 + (!strcmp(mix.seq,"tppm"));
   d.c3 = d.c3 + ((!strcmp(mix.seq,"tppm")) && (mix.t.a > 0.0));
   d.t3 = nXYxy8*(1.0/srate - pwXxyxy8 - pwYxyxy8);
   d.c4 = d.c4 + (!strcmp(mix.seq,"spinal"));
   d.c4 = d.c4 + ((!strcmp(mix.seq,"spinal")) && (mix.s.a > 0.0));
   d.t4 = nXYxy8*(1.0/srate - pwXxyxy8 - pwYxyxy8);
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables
 
   settable(phH90,4,table1);
   settable(phXhx,8,table2);
   settable(phHhx,4,table3);
   settable(phXYxy8,8,table4);
   settable(phXxyxy8,4,table5);
   settable(phYxyxy8,4,table6);
   settable(phRec,8,table7);
   settable(ph1Rec,8,table8);
   settable(ph2Rec,8,table9);
   settable(ph3Rec,8,table10);
   int tix = counter%8;

   if ((tix == 1) || (tix == 7)) ttadd(phRec,ph1Rec,4);
   if ((tix == 2) || (tix == 6)) ttadd(phRec,ph2Rec,4);
   if ((tix == 3) || (tix == 5)) ttadd(phRec,ph3Rec,4);     
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

// XY8 Period

   if (counter > 0) {
      _dseqon(mix);
      delay(pwXxyxy8/2.0);
      obspwrf(aXxyxy8); dec2pwrf(aYxyxy8);
      assign(zero,v1);
      if (counter >= 1) {
         if (counter > 1) loop(v8,v9);
	    getelem(phXYxy8,v1,v4);
	    incr(v1);
	    getelem(phXxyxy8,ct,v2);
	    getelem(phYxyxy8,ct,v3);
	    add(v4,v2,v2);
	    add(v4,v3,v3);
	    txphase(v2); dec2phase(v3);
	    delay((1.0 - fXYxy8)/srate - pwYxyxy8/2.0 - pwXxyxy8/2.0);
	    if (onYxyxy8 == 2)
               dec2rgpulse(pwYxyxy8,v3,0.0,0.0);
            else
               delay(pwYxyxy8);
	    delay(fXYxy8/srate - pwYxyxy8/2.0 - pwXxyxy8/2.0);
	    rgpulse(pwXxyxy8,v2,0.0,0.0);
	 if (counter > 1) endloop(v9);
	 delay(1.0/srate - pwXxyxy8/2.0);
      }
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

