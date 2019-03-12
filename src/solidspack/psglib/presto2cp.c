/*presto2cp.c - Cross polarization with homonuclear decoupling on H
                using R1852 with 1 refocus pulse.
		
		Zhao, X, Hoffbauer, W., Schmedt-A-D-Gunne, J,Levitt, M.
		Solid State Magn. Reson. 26, 2004, 57-64.

                David Rice 4/18/06                                     */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,0,2};           // ph1Hhxpto2
static int table2[4] = {1,1,1,1};           // ph2Hhxpto2
static int table3[4] = {1,1,2,2};           // ph1Xhxpto2
static int table4[4] = {0,0,0,0};           // ph2Xhxpto2
static int table5[4] = {0,0,0,0};           // phHdec
static int table6[4] = {0,2,1,3};           // phRec

#define ph1Hhxpto2 t1
#define ph2Hhxpto2 t2
#define ph1Xhxpto2 t3
#define ph2Xhxpto2 t4
#define phHdec t5
#define phRec t6

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   double aXhxpto2 = getval("aXhxpto2");
   double pw1Xhxpto2 = getval("pw1Xhxpto2");
   double pw2Xhxpto2 = getval("pw2Xhxpto2");
   double t1HXpto2init = getval("t1HXpto2");
   double tau1 = t1HXpto2init - pw1Xhxpto2/2.0;
   double t2HXpto2init = getval("t2HXpto2");
   double tau2 = t2HXpto2init - pw1Xhxpto2/2.0 - pw2Xhxpto2/2.0; 
   double t3HXpto2init = getval("t3HXpto2");
   double tau3 = t3HXpto2init - pw2Xhxpto2/2.0;

   MPSEQ r18 = getr1825("r18H",0,0.0,0.0,0,1);
   MPSEQ r18ref = getr1825("r18H",r18.iSuper,r18.phAccum,r18.phInt,1,1); 
   strncpy(r18.ch,"dec",3);
   strncpy(r18ref.ch,"dec",3);
   putCmd("chHr18='dec'\n");

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
   d.dutyon = tau1 + tau2 + pw1Xhxpto2 + tau3 + pw2Xhxpto2;
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

   settable(ph1Hhxpto2,4,table1);
   settable(ph2Hhxpto2,4,table2);
   settable(ph1Xhxpto2,4,table3);
   settable(ph2Xhxpto2,4,table4);
   settable(phHdec,4,table5);
   settable(phRec,4,table6);
   setreceiver(phRec);

// Begin Sequence

   txphase(ph1Xhxpto2); decphase(ph1Hhxpto2);
   obspwrf(aXhxpto2); decpwrf(r18.a);
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to X Cross Polarization with PRESTO1

   _mpseqon(r18,ph1Hhxpto2);
   delay(tau1);
   rgpulse(pw1Xhxpto2/2.0,ph1Xhxpto2,0.0,0.0);
   _mpseqoff(r18);
   _mpseqon(r18ref,ph2Hhxpto2);
   rgpulse(pw1Xhxpto2/2.0,ph1Xhxpto2,0.0,0.0);
   delay(tau2);
   _mpseqoff(r18ref);
   decphase(zero);
   _dseqon(dec);
   rgpulse(pw2Xhxpto2,ph2Xhxpto2,0.0,0.0);
   delay(tau3);
   decphase(zero);

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

