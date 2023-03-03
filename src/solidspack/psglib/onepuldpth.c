/*onepuldpth.c - A sequence to do direct polarization with TPPM and SPINAL
             decoupling followed by a DEPTH filter for background removal.

              D.Rice 08/25/06                                           */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[16] = {0,2,3,1,2,0,1,3,0,2,3,1,2,0,1,3};  // phX90 
static int table2[16] = {0,2,3,1,3,1,2,0,2,0,1,3,1,3,0,2};  // ph1X180 
static int table3[16] = {0,3,1,0,3,0,0,1,0,3,1,0,3,0,0,1};  // ph2X180 
static int table4[16] = {0,0,3,3,2,2,1,1,0,0,3,3,2,2,1,1};  // receiver

#define phX90 t1
#define ph1X180 t2
#define ph2X180 t3
#define phRec t4

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

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
   d.dutyon = getval("pwX90") + 2.0*getval("pwX180");
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

   settable(phX90,16,table1);
   settable(ph1X180,16,table2);
   settable(ph2X180,16,table3);
   settable(phRec,16,table4);
   setreceiver(phRec);

// Begin Sequence

   txphase(phX90); decphase(zero);
   obspwrf(getval("aX90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// X Cross Direct Polarization 

   rgpulse(getval("pwX90"),phX90,0.0,0.0);
   obspwrf(getval("aX180"));

// A DEPTH Filter

   if (getval("pwX180") > 0.0) {
      rgpulse(getval("pwX180"),ph1X180,0.0,0.0);
      rgpulse(getval("pwX180"),ph2X180,0.0,0.0);
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

