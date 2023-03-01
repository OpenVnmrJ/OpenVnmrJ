/*onepultoss.c - A sequence to do direct polarization with TPPM and SPINAL
             decoupling with TOSS4 for sideband suppression.

             D.Rice 07/18/05 VNMRJ2.1A-B for the NMR SYSTEM
	     updated with new .h files D.Rice 10/12/05 onepul.c
             D. Rice 01/30/06                                          */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,0,0,0};           // phHdec
static int table2[4] = {0,2,1,3};           // phX90
static int table3[4] = {1,3,2,0};           // phXtoss
static int table4[4] = {0,2,1,3};           // phRec

#define phHdec t1
#define phX90 t2
#define phXtoss t3
#define phRec t4

void pulsesequence() {

// Set the Maximum Dynamic Table Number

   settablenumber(10);
   setvvarnumber(30);

// Define Variables and Objects and Get Parameter Values

   WMPA toss = gettoss4("tossX");
   strcpy(toss.ch,"obs");
   putCmd("chXtoss='obs'\n");

   DSEQ dec = getdseq("H");
   strcpy(dec.t.ch,"dec");
   putCmd("chHtppm='dec'\n"); 
   strcpy(dec.s.ch,"dec");
   putCmd("chHspinal='dec'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwX90") + 4.0*getval("pwXtoss");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = 2.142*toss.rtau - 4.0*getval("pwXtoss") + getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = 2.142*toss.rtau - 4.0*getval("pwXtoss") + getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phHdec,4,table1);
   settable(phX90,4,table2);
   settable(phXtoss,4,table3);
   settable(phRec,4,table4);
   setreceiver(phRec);

// Begin Sequence

   txphase(phX90); decphase(phHdec);
   obspwrf(getval("aX90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);  
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// X Cross Direct Polarization

   rgpulse(getval("pwX90"),phX90,0.0,0.0);

// TOSS4 Sideband Suppression

   _dseqon(dec);
   _toss4(toss, phXtoss);

// Begin Acquisition

   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

