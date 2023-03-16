/*presto1cp.c - Cross polarization with homonuclear decoupling on H
                using R1852.

	        Zhao, X, Hoffbauer, W., Schmedt-A-D-Gunne, J,Levitt, M.
		Solid State Magn. Reson. 26, 2004, 57-64.

                David Rice 4/18/06                                     */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,0,2};           // ph1Hhxpto1
static int table2[4] = {1,1,1,1};           // ph2Hhxpto1
static int table3[4] = {1,1,2,2};           // phXhxpto1
static int table4[4] = {0,0,0,0};           // phHdec
static int table5[4] = {0,2,1,3};           // phRec

#define ph1Hhxpto1 t1
#define ph2Hhxpto1 t2
#define phXhxpto1 t3
#define phHdec t4
#define phRec t5

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   double aXhxpto1 = getval("aXhxpto1");
   double pwXhxpto1 = getval("pwXhxpto1");
   double t1HXpto1 = getval("t1HXpto1");
   double tau1 = t1HXpto1 - pwXhxpto1/2.0;
   double t2HXpto1 = getval("t2HXpto1");
   double tau2 = t2HXpto1 - pwXhxpto1/2.0;

   MPSEQ r18 = getr1825("r18H",0,0.0,0.0,0,1);
   MPSEQ r18ref = getr1825("r18H",r18.iSuper,r18.phAccum,r18.phInt,1,1);
   strcpy(r18.ch,"dec");
   strcpy(r18ref.ch,"dec");
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
   d.dutyon = tau1 + tau2 + pwXhxpto1;
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

   settable(ph1Hhxpto1,4,table1);
   settable(ph2Hhxpto1,4,table2);
   settable(phXhxpto1,4,table3);
   settable(phHdec,4,table4);
   settable(phRec,4,table5);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXhxpto1); decphase(ph1Hhxpto1);
   obspwrf(aXhxpto1); decpwrf(r18.a);
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to X Cross Polarization with PRESTO1

   _mpseqon(r18,ph1Hhxpto1);
   delay(tau1);
   rgpulse(pwXhxpto1/2.0,phXhxpto1,0.0,0.0);
   _mpseqoff(r18);
   _mpseqon(r18ref,ph2Hhxpto1);
   rgpulse(pwXhxpto1/2.0,phXhxpto1,0.0,0.0);
   delay(tau2);
   _mpseqoff(r18ref);
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

