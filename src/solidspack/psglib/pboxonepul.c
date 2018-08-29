/*pboxonepul.c - A test sequence to test simultaneous pulses from Pbox with
             spinal and tppm decoupling.

             D.Rice 04/04/07                                             */

#include "standard.h"
#include "solidstandard.h"
#include "pboxpulse.h"

// Define Values for Phasetables

static int table1[4] = {0,2,1,3};           // phX90
static int table2[4] = {0,0,0,0};           // phAsft1
static int table3[4] = {0,0,0,0};           // phAsft2
static int table4[4] = {0,0,0,0};           // phAsft3
static int table5[4] = {0,2,1,3};           // phRec

#define phX90 t1
#define phAsft1 t2
#define phAsft2 t3
#define phAsft3 t4
#define phRec t5

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   PBOXPULSE shp1  = getpboxpulse("sft1A",0,1);
   PBOXPULSE shp2  = getpboxpulse("sft2A",0,1);
   PBOXPULSE shp3  = getpboxpulse("sft3A",0,1);

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
   d.dutyon = getval("pwX90") + shp1.pw + shp2.pw + shp3.pw;
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

   settable(phX90,4,table1);
   settable(phAsft1,4,table2);
   settable(phAsft2,4,table3);
   settable(phAsft3,4,table4);
   settable(phRec,4,table5);
   setreceiver(phRec);
    
// Begin Sequence

   txphase(phX90); decphase(zero);
   obspwrf(getval("aX90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// X Direct Polarization

   rgpulse(getval("pwX90"),phX90,0.0,0.0);
   delay(20.0e-6);

// X Shaped Pulse

   _pboxpulse(shp1, phAsft1);
   delay(20.0e-6);

// X Simultaneous Shaped Pulse

   _pboxsimpulse(shp1,shp2,phAsft1,phAsft2);
   delay(20.0e-6);

// X 3-channel Simultaneous Shaped Pulse

   delay(20.0e-6);
   _pboxsim3pulse(shp1,shp2,shp3,phAsft1,phAsft2,phAsft3);

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

