/*onepult1rho.c -  A sequence execute an X-channel spinlock with decoupling
                   after a single-pulse polarization.
                                         
	            D.Rice 07/16/08                                      */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,0,0,0};           // phHdec
static int table2[4] = {0,2,1,3};           // phX90
static int table3[4] = {1,3,2,0};           // phXlock
static int table4[4] = {0,2,1,3};           // phRec

#define phHdec t1
#define phX90 t2
#define phXlock t3
#define phRec t4

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   DSEQ dec = getdseq("H");
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHtppm='dec'\n"); 
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHspinal='dec'\n");

   DSEQ mix = getdseq("Hmix");
   strncpy(mix.t.ch,"dec",3);
   putCmd("chHmixtppm='dec'\n"); 
   strncpy(mix.s.ch,"dec",3);
   putCmd("chHmixspinal='dec'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection 

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwX90") + getval("tlock");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phHdec,4,table1);
   settable(phX90,4,table2);
   settable(phXlock,4,table3);
   settable(phRec,4,table4);
   setreceiver(phRec);

// Begin Sequence

   txphase(phX90); decphase(phHdec);
   obspwrf(getval("aX90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// X Direct Polarization

   rgpulse(getval("pwX90"),phX90,0.0,0.0);

// X Channel Spinlock

   _dseqon(mix);
   obspwrf(getval("aXlock"));
   decunblank();
   rgpulse(getval("tlock"),phXlock,0.0,0.0);
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

