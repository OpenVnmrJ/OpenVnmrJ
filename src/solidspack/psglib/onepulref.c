/*onepulref.c - A sequence execute direct polarization, DP followed by a shaped
                refocusing pulse, with TPPM or SPINAL decoupling.

                D. Rice 05/05/11                                               */

#include "standard.h"
#include "solidstandard.h"
#include "pboxpulse.h"

// Define Values for Phasetables

static int table1[8] = {0,3,0,3,2,1,2,1};           // phX90    
static int table2[8] = {0,0,1,1,0,0,1,1};           // phXshp1
static int table3[8] = {0,1,2,3,2,3,0,1};           // phRec 
#define phX90 t1
#define phXshp1 t2
#define phRec t3

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   PBOXPULSE shp1 = getpboxpulse("shp1X",0,1); 

   DSEQ dec = getdseq("H"); 
   strcpy(dec.t.ch,"dec");
   putCmd("chHtppm='dec'\n"); 
   strcpy(dec.s.ch,"dec");
   putCmd("chHspinal='dec'\n");

   DSEQ mix = getdseq("Hmix");
   strcpy(mix.t.ch,"dec");
   putCmd("chHmixtppm='dec'\n"); 
   strcpy(mix.s.ch,"dec");
   putCmd("chHmixspinal='dec'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection
   DUTY d = init_dutycycle();
   d.dutyon = getval("pwX90") + shp1.pw;
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d.c3 = d.c3 + (!strcmp(mix.seq,"tppm"));
   d.c3 = d.c3 + ((!strcmp(mix.seq,"tppm")) && (mix.t.a > 0.0));
   d.t3 = getval("tau1") + getval("tau2");
   d.c4 = d.c4 + (!strcmp(mix.seq,"spinal"));
   d.c4 = d.c4 + ((!strcmp(mix.seq,"spinal")) && (mix.s.a > 0.0));
   d.t4 = getval("tau1") + getval("tau2");
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phX90,8,table1);
   settable(phXshp1,8,table2);
   settable(phRec,8,table3);
   setreceiver(phRec);

// Begin Sequence

   txphase(phX90); decphase(zero);
   obspower(tpwr);
   obspwrf(getval("aX90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// X Direct Polarization

   rgpulse(getval("pwX90"),phX90,0.0,0.0);

// Shaped Refocussing Pulse

   _dseqon(mix);
   delay(getval("tau1"));
   _pboxpulse(shp1,phXshp1);
   delay(getval("tau2"));
   _dseqoff(mix);

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

