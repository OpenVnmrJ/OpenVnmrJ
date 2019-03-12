/* grad_profile.c - pulse sequence to measure the gradient profile 
                    using direct polarization and a spin echo, with 
                    TPPM or SPINAL decoupling throughout. 

                    J. Stringer 05/06/2010  

                    RF - pwX90__________pwX180______Acq_____
                    GZ - _____+GZ---__________+GZ-----------
                                                               */
#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,1,3};           // phX90
static int table2[4] = {1,3,2,0};           // phXecho
static int table3[4] = {0,2,1,3};           // phRec

#define phX90 t1
#define phXecho t2
#define phRec t3

void pulsesequence() {

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
   d.dutyon = getval("pwX90") + getval("pwXecho");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = 2.0*getval("gpwZgrad") + getval("t1Xecho") + getval("t2Xecho") +
          getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = 2.0*getval("gpwZgrad") + getval("t1Xecho") + getval("t2Xecho") + 
          getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Maximum Gradient Time On

   double duty = getval("ad") + getval("rd") + at + 2.0*getval("gpwZgrad");
   if (duty > 0.01) {
      abort_message("Gradient during acquisition longer than 10ms. Abort!\n");
   }

// Set Phase Tables

   settable(phX90,4,table1);
   settable(phXecho,4,table2);
   settable(phRec,4,table3);
   setreceiver(phRec);

// Begin Sequence

   txphase(phX90); decphase(zero);
   obspwrf(getval("aX90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H Decoupler on Before Echo

   _dseqon(dec);

// X Direct Polarization

   rgpulse(getval("pwX90"),phX90,0.0,0.0);
   txphase(phXecho); 
   obspwrf(getval("aXecho")); 
   obsunblank();

// Gradient pulse

   zgradpulse(getval("gaZgrad"),getval("gpwZgrad"));

// X Inversion

   delay(getval("t1Xecho"));
   rgpulse(getval("pwXecho"),phXecho,0.0,0.0);
   obsunblank();
   delay(getval("t2Xecho"));

// Begin Acquisition

   rgradient('z',getval("gaZgrad"));
   delay(getval("gpwZgrad"));
   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   rgradient('z',0.0);
   obsunblank(); decunblank(); _unblank34();
}

