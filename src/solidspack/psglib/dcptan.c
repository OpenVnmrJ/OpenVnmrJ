/*dcptan.c  tuneup sequence for transfer from  H to Y to X using two constant,
            linear or tangent ramped cross poalrizations.

            CEB 06/28/05 for VNMRJ2.1A-B for the NMR SYSTEM
            Edited  D. Rice 6/28/05
	    updated with new .h files D.Rice 10/12/05                       */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,0,2};           // phH90
static int table2[4] = {1,1,1,1};           // phHhy
static int table3[4] = {1,1,1,1};           // phY90
static int table4[4] = {1,1,1,1};           // phYhy
static int table5[4] = {1,1,1,1};           // phHyx
static int table6[4] = {1,1,1,1};           // phYyx
static int table7[8] = {0,0,1,1,2,2,3,3};   // phXyx
static int table8[8] = {0,2,1,3,2,0,3,1};   // phRec

#define phH90 t1
#define phHhy t2
#define phY90 t3
#define phYhy t4
#define phHyx t5
#define phYyx t6
#define phXyx t7
#define phRec t8

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values
     
   CP hy = getcp("HY",0.0,0.0,0,1);
   strcpy(hy.fr,"dec");
   strcpy(hy.to,"dec2");
   putCmd("frHY='dec'\n");
   putCmd("toHY='dec2'\n");
    
   CP yx = getcp("YX",0.0,0.0,0,1);
   strcpy(yx.fr,"dec2");
   strcpy(yx.to,"obs");
   putCmd("frYX='dec2'\n");
   putCmd("toYX='obs'\n");

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
   d.dutyon = getval("pwY90") + getval("pwH90") + getval("tHY") + getval("tYX")
              + 2.0*getval("pwX90");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Create Phasetables

   settable(phH90,4,table1);
   settable(phHhy,4,table2);
   settable(phY90,4,table3);
   settable(phYhy,4,table4);
   settable(phHyx,4,table5);
   settable(phYyx,4,table6);
   settable(phXyx,8,table7);
   settable(phRec,8,table8);

// Begin Sequence

   setreceiver(phRec);
   txphase(phXyx); decphase(phH90); dec2phase(phY90);
   obspwrf(getval("aXyx")); decpwrf(getval("aH90")); dec2pwrf(getval("aY90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to Y Cross Polarization with a Y Prepulse

   dec2rgpulse(getval("pwY90"),phY90,0.0,0.0);
   dec2phase(phYhy);
   dec2pwrf(getval("aYyx"));
   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decphase(phHhy);
   _cp_(hy,phHhy,phYhy);

// Y to X Cross Polarization

   decphase(phHyx); dec2phase(phYyx);
   decpwrf(getval("aHyx"));
   decunblank(); decon();
   _cp_(yx,phYyx,phXyx);
   decphase(phHhy);
   dec2blank();
   decoff();

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

