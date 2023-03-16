/*exsy2d.c - A sequence for EXSY experiments in solids.

               V. Zorin 09/14/10                                      */ 

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[16] = {0,2,0,2,0,2,0,2,1,3,1,3,1,3,1,3};           				// phX90_1
static int table2[32] = {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3};   	// phX90_2
static int table3[16] = {0,0,1,1,2,2,3,3,1,1,2,2,3,3,0,0};   					// phX90_3
static int table4[32] = {2,0,3,1,0,2,1,3,3,1,0,2,1,3,2,0,0,2,1,3,2,0,3,1,1,3,2,0,3,1,0,2};      // phRec

#define phX90_1 t1
#define phX90_2 t2
#define phX90_3 t3
#define phRec t4

static double d2_init;

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   double tXmix=getval("tXmix");

   DSEQ dec = getdseq("H");
   strcpy(dec.t.ch,"dec");
   putCmd("chHtppm='dec'\n"); 
   strcpy(dec.s.ch,"dec");
   putCmd("chHspinal='dec'\n");

// Set Constant-time Period for d2. 

   if (d2_index == 0) d2_init = getval("d2");
   double d2_ = (ni - 1)/sw1 + d2_init;
   putCmd("d2acqret = %f\n",roundoff(d2_,12.5e-9));
   putCmd("d2dwret = %f\n",roundoff(1.0/sw1,12.5e-9));

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = 3.0*getval("pwX90");
   d.dutyoff = d1 + 4.0e-6 + tXmix;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = d2_ + getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = d2_ + getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phX90_1,16,table1);
   settable(phX90_2,32,table2);
   settable(phX90_3,16,table3);
   settable(phRec,32,table4);
   
   if (phase1 == 2) tsadd(phX90_1,1,4);
   setreceiver(phRec);

// Begin Sequence

   txphase(phX90_1); decphase(zero);
   obspwrf(getval("aX90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// X Direct Polarization

   rgpulse(getval("pwX90"),phX90_1,0.0,0.0);
   _dseqon(dec);

// X Evolution

   delay(d2);

   txphase(phX90_2);
   _dseqoff(dec);
   if (getval("aHmix")>0.0) {
     decpwrf(getval("aHmix"));
     decon();
   }
   rgpulse(getval("pwX90"),phX90_2,0.0,0.0);
   delay(tXmix);
   if (getval("aHmix")>0.0)
     decoff();
   _dseqon(dec);
   txphase(phX90_3);
   rgpulse(getval("pwX90"),phX90_3,0.0,0.0);

// Begin Acquisition

   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}
