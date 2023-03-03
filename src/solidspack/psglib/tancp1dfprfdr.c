/*tancp1dfprfdr.c  Preparation of X with CP,  finite-pulse XY4 RFDR
                   mixing with SPINAL or TPPM decoupling

      Y. Ishii, J. Chem. Phys, 114,8473 (2007)

                   D. Rice 01/15/08                                  */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,3,1,3};           // phH90
static int table2[4] = {0,0,0,0};           // phHhx
static int table3[8] = {0,0,1,1,2,2,3,3};   // phXhx
static int table4[4] = {0,0,0,0};           // phXmix
static int table5[8] = {0,2,1,3,2,0,3,1};   // phRec

#define phH90 t1
#define phHhx t2
#define phXhx t3
#define phXmix t4
#define phRec t5

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   CP hx = getcp("HX",0.0,0.0,0,1);
   strcpy(hx.fr,"dec");
   strcpy(hx.to,"obs");
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   printf("hithere\n");
   MPSEQ fp = getfprfdr("fprfdrX",0,0.0,0.0,0,1);
   strcpy(fp.ch,"obs");
   putCmd("chXfprfdr='obs'\n");

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

// Round tXmix to the element length, 12*pwXfprfdr

   double tXmix = getval("tXmix");
   tXmix = roundoff(tXmix,12.0*getval("pwXfprfdr"));
   putCmd("tXmix = 12.0*pwXfprfdr");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection
   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90") + getval("tHX") + tXmix/3.0; 
   d.dutyoff = d1 + 4.0e-6; 
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d.c3 = d.c3 + (!strcmp(mix.seq,"tppm"));
   d.c3 = d.c3 + ((!strcmp(mix.seq,"tppm")) && (mix.t.a > 0.0));
   d.t3 = (2.0/3.0)*tXmix;
   d.c4 = d.c4 + (!strcmp(mix.seq,"spinal"));
   d.c4 = d.c4 + ((!strcmp(mix.seq,"spinal")) && (mix.s.a > 0.0));
   d.t4 = (2.0/3.0)*tXmix;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0); 

// Create Phasetables

   settable(phH90,4,table1);
   settable(phHhx,4,table2);
   settable(phXhx,8,table3);
   settable(phXmix,4,table4);
   settable(phRec,8,table5);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXhx); decphase(phH90);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to X Cross Polarization

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decphase(phHhx);
    _cp_(hx,phHhx,phXhx);

// Finite-Pulse RFDR Mixing

   _dseqon(mix);
   _mpseqon(fp,phXmix);
   delay(tXmix);
   _mpseqoff(fp);
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

