/*onepuldec2.c - A sequence to do direct polarization with TPPM and SPINAL
             decoupling on dec and dec2.

             D.Rice 07/18/05 VNMRJ2.1A-B for the NMR SYSTEM 
	     updated with new .h files D.Rice 10/12/05              
             Add dec2 decoupling 03/12/09                               */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,1,3};           // phX90
static int table2[4] = {0,2,1,3};           // phRec

#define phX90 t1
#define phRec t2

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   DSEQ dec = getdseq("H");
   strcpy(dec.t.ch,"dec");
   putCmd("chHtppm='dec'\n");
   strcpy(dec.s.ch,"dec");
   putCmd("chHspinal='dec'\n");

   DSEQ dec2 = getdseq("Y");
   strcpy(dec2.t.ch,"dec2");
   putCmd("chYtppm='dec2'\n");
   strcpy(dec2.s.ch,"dec2");
   putCmd("chYspinal='dec2'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwX90");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   if ((d.c1 == -1) && (d.c2 == -1)) { 
      d.c1 = ((!strcmp(dec2.seq,"tppm")) && (dec2.t.a == 0.0));
      d.t1 = getval("rd") + getval("ad") + at;
      d.c2 = ((!strcmp(dec2.seq,"spinal")) && (dec2.s.a == 0.0));
      d.t2 = getval("rd") + getval("ad") + at;
   } 
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phX90,4,table1);
   settable(phRec,4,table2);
   setreceiver(phRec);

// Begin Sequence

   txphase(phX90); decphase(zero); dec2phase(zero);
   obspwrf(getval("aX90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// X Direct Polarization

   rgpulse(getval("pwX90"),phX90,0.0,0.0);

// Begin Acquisition

   _dseqon(dec);   
   if (NUMch > 2) _dseqon(dec2);
   if (NUMch > 3) dec3blank();
   obsblank();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);   
   if (NUMch > 2) _dseqoff(dec2);
   if (NUMch > 3) dec3unblank();
   obsunblank(); decunblank();
}

