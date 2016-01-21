/*redor1onepul.c - XY REDOR with alternating pi pulses on the two channels
                  during REDOR evolution. Single pulse preparation.

                  D. Rice 03/24/06                                       */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,3,2,0};           // phX90
static int table2[8] = {0,1,0,1,1,0,1,0};   // phXYxy8
static int table3[4] = {0,0,0,0};           // phYxyxy8
static int table4[4] = {0,0,0,0};           // phXxyxy8
static int table5[4] = {0,2,1,3};           // phRec

static int table6[4] = {0,0,2,2};           // ph1Rec
static int table7[4] = {2,2,2,2};           // ph2Rec
static int table8[4] = {2,2,0,0};           // ph3Rec

#define phX90    t1
#define phXxyxy8 t2
#define phYxyxy8 t3
#define phXYxy8 t4
#define phRec    t5
#define ph1Rec   t6
#define ph2Rec   t7
#define ph3Rec   t8

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   double aXxyxy8 = getval("aXxyxy8");
   double aYxyxy8 = getval("aYxyxy8");
   double pwXxyxy8 = getval("pwXxyxy8");
   double pwYxyxy8 = getval("pwYxyxy8");
   double nXYxy8 = getval("nXYxy8");
   int counter = nXYxy8;
   initval(nXYxy8,v8);
   double fXYxy8 = getval("fXYxy8");
   double onYxyxy8 = getval("onYxyxy8");
   double srate = getval("srate");

   DSEQ dec = getdseq("H");
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHtppm='dec'\n"); 
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHspinal='dec'\n");

   DSEQ mix = getdseq("Hmix");
   strncpy(mix.t.ch,"dec",3);
   putCmd("chHmixtppm='dec'\n");
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHmixspinal='dec'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwX90") + nXYxy8*(pwXxyxy8 + pwYxyxy8);
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

   settable(phX90,4,table1);
   settable(phXYxy8,8,table2);
   settable(phXxyxy8,4,table3);
   settable(phYxyxy8,4,table4);
   settable(phRec,4,table5);
   settable(ph1Rec,4,table6);
   settable(ph2Rec,4,table7);
   settable(ph3Rec,4,table8);
   int tix = counter%8;

   if ((tix == 1) || (tix == 7)) ttadd(phRec,ph1Rec,4);
   if ((tix == 2) || (tix == 6)) ttadd(phRec,ph2Rec,4);
   if ((tix == 3) || (tix == 5)) ttadd(phRec,ph3Rec,4);
   setreceiver(phRec);

// Begin Sequence

   txphase(phX90); decphase(zero);
   obspwrf(getval("aX90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// X Direct Polarization

   rgpulse(getval("pwX90"),phX90,0.0,0.0);

// xy8XY Period

   if (counter > 0) {
      _dseqon(mix);
      delay(pwXxyxy8/2.0);
      obspwrf(aXxyxy8); dec2pwrf(aYxyxy8);
      sub(v1,v1,v1);
      if (counter >= 1) {
         loop(v8,v9);
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
	 endloop(v9);
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

