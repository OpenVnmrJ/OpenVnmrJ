/*trapdor1d.c - Trapdor on X with H modulation with SPINAL or 
                TPPM decoupling.

                Uses the standard channel identifiers X = obs,
                H = dec. Note, however, that this sequence would
                nevertheless most often be used for high-band
                observe. In this case aXxhtrap would be the high-band
                amplitude and aHxhtrap would be the low-band quadrupole
                nucleus.

             Edited  D. Rice 01/10/05                             */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,1,3};           // ph1Xxhtrap
static int table2[4] = {0,0,0,0};           // phHxhtrap
static int table3[4]=  {0,0,0,0};           // ph2Xxhtrap
static int table4[4] = {2,0,1,3};           // phRec

#define ph1Xxhtrap t1
#define phHxhtrap t2
#define ph2Xxhtrap t3
#define phRec t4

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

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
   d.dutyon = getval("pw1Xxhtrap") + getval("pw1Xxhtrap");
   d.dutyoff = d1 + 4.0e-6 + getval("t1XHtrap") + getval("t2XHtrap");
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(ph1Xxhtrap,4,table1);
   settable(phHxhtrap,4,table2);
   settable(ph2Xxhtrap,4,table3);
   settable(phRec,4,table4);
   setreceiver(phRec);

// Begin Sequence

   txphase(ph1Xxhtrap); decphase(phHxhtrap);
   obspwrf(getval("aXxhtrap")); decpwrf(getval("aHxhtrap"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// TRAPDOR on H with X Modulation

   rgpulse(getval("pw1Xxhtrap"),ph1Xxhtrap,0.0,0.0);
   txphase(ph2Xxhtrap);
   obsunblank();
   decon();
   delay(getval("t1XHtrap"));
   decoff();
   rgpulse(getval("pw2Xxhtrap"),ph2Xxhtrap,0.0,0.0);
   obsunblank();
   delay(getval("t2XHtrap"));

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
