/*presto3cp.c - Cross polarization with homonuclear decoupling on H
                using R1852 with 2 refocussing pulses.
		
		Zhao, X, Hoffbauer, W., Schmedt-A-D-Gunne, J,Levitt, M.
		Solid State Magn. Reson. 26, 2004, 57-64.

                David Rice 4/18/06                                       */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,0,2};           // ph1Hhxpto3
static int table2[4] = {2,0,2,0};           // ph2Hhxpto3
static int table3[4] = {1,1,1,1};           // ph3Hhxpto3
static int table4[4] = {3,3,3,3};           // ph4Hhxpto3
static int table5[4] = {1,1,3,3};           // ph1Xhxpto3
static int table6[4] = {0,0,0,0};           // ph2Xhxpto3
static int table7[4] = {0,0,0,0};           // phHdec
static int table8[4] = {0,2,2,0};           // phRec

#define ph1Hhxpto3 t1
#define ph2Hhxpto3 t2
#define ph3Hhxpto3 t3
#define ph4Hhxpto3 t4
#define ph1Xhxpto3 t5
#define ph2Xhxpto3 t6
#define phHdec t7
#define phRec t8

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   double aXhxpto3 = getval("aXhxpto3");
   double pw1Xhxpto3 = getval("pw1Xhxpto3");
   double pw2Xhxpto3 = getval("pw2Xhxpto3");
   double t1HXpto3 = getval("t1HXpto3");
   double tau1a = t1HXpto3/2.0 - pw2Xhxpto3/2.0;
   double tau1b = t1HXpto3/2.0 - pw2Xhxpto3/2.0 - pw1Xhxpto3/2.0;
   double t2HXpto3 = getval("t2HXpto3");  
   double tau2a = t2HXpto3/2.0 - pw2Xhxpto3/2.0 - pw1Xhxpto3/2.0;
   double tau2b = t2HXpto3/2.0 - pw2Xhxpto3/2.0;

   MPSEQ r18 = getr1825("r18H",0,0.0,0.0,0,1);
   MPSEQ r18ref = getr1825("r18H",r18.iSuper,r18.phAccum,r18.phInt,1,1); 
// THe following two lines are the originals but they use uninitialized variables.
// The corrections may not be correct.
// MPSEQ r18ref2 = getr1825("r18H",r18ref2.iSuper,r18ref2.phAccum,r18ref2.phInt,2,1);
// MPSEQ r18ref3 = getr1825("r18H",r18ref3.iSuper,r18ref3.phAccum,r18ref3.phInt,3,1);
   MPSEQ r18ref2 = getr1825("r18H",r18.iSuper,r18.phAccum,r18.phInt,2,1);
   MPSEQ r18ref3 = getr1825("r18H",r18.iSuper,r18.phAccum,r18.phInt,3,1);
   strcpy(r18.ch,"dec");
   strcpy(r18ref.ch,"dec");
   strcpy(r18ref2.ch,"dec");
   strcpy(r18ref3.ch,"dec");
   putCmd("chHr18='dec'\n");

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
   d.dutyon = tau1a + tau1b + 2.0* pw2Xhxpto3 + tau2a + tau2b + pw1Xhxpto3;
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

   settable(ph1Hhxpto3,4,table1);
   settable(ph2Hhxpto3,4,table2);
   settable(ph3Hhxpto3,4,table3);
   settable(ph4Hhxpto3,4,table4);
   settable(ph1Xhxpto3,4,table5);
   settable(ph2Xhxpto3,4,table6);
   settable(phHdec,4,table7);
   settable(phRec,4,table8);
   setreceiver(phRec);

// Begin Sequence

   txphase(ph1Xhxpto3); decphase(ph1Hhxpto3);
   obspwrf(aXhxpto3); decpwrf(r18.a);
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to X Cross Polarization with PRESTO3

   _mpseqon(r18,ph1Hhxpto3);
   delay(tau1a);
   rgpulse(pw2Xhxpto3/2.0,ph2Xhxpto3,0.0,0.0);
   _mpseqoff(r18);
   _mpseqon(r18ref,ph2Hhxpto3);
   rgpulse(pw2Xhxpto3/2.0,ph2Xhxpto3,0.0,0.0);
   delay(tau1b);
   rgpulse(pw1Xhxpto3/2.0,ph1Xhxpto3,0.0,0.0);
   _mpseqoff(r18ref);
   _mpseqon(r18ref2,ph3Hhxpto3);
   rgpulse(pw1Xhxpto3/2.0,ph1Xhxpto3,0.0,0.0);
   delay(tau2a);
   rgpulse(pw2Xhxpto3/2.0,ph2Xhxpto3,0.0,0.0);
   _mpseqoff(r18ref2);
   _mpseqon(r18ref3,ph4Hhxpto3); 
   rgpulse(pw2Xhxpto3/2.0,ph2Xhxpto3,0.0,0.0);
   delay(tau2b);
   _mpseqoff(r18ref3);
   decphase(phHdec);

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

