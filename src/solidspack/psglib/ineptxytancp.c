/*ineptxytancp.c - INEPT polarization transfer from Y to X.  Tancp preparation
                   of Y polarization with INEPT transfer to X.  SPINAL or
		   TPPM decoupling throughout with separate levels during
		   acquisition and INEPT transfer.

                   Phase cycle based on the infinity+ sequence "inept_dec.s"
                   provided by Marek Pruski.

                   D. Rice 08/02/06                                       */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[16] = {1,1,1,1,1,1,1,1,3,3,3,3,3,3,3,3}; // phH90
static int table2[4] = {0,0,0,0};                          // phHhy
static int table3[4] = {3,3,3,3};                          // phYhy
static int table4[4] = {0,2,0,2};                          // ph1Yyxinept
static int table5[4] = {0,2,0,2};                          // ph1Xyxinept
static int table6[4] = {1,1,3,3};                          // ph2Yyxinept
static int table7[16] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3}; // ph2Xyxinept
static int table8[8] = {0,0,2,2,1,1,3,3};                  // phRec

#define phH90 t1
#define phHhy t2
#define phYhy t3
#define ph1Yyxinept t4
#define ph1Xyxinept t5
#define ph2Yyxinept t6
#define ph2Xyxinept t7
#define phRec t8

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   CP hy = getcp("HY",0.0,0.0,0,1);
   strncpy(hy.fr,"dec",3);
   strncpy(hy.to,"dec2",4);
   putCmd("frHY='dec'\n");
   putCmd("toHY='dec2'\n");

   GP inept = getinept("ineptYX");
   strncpy(inept.ch1,"dec2",4);
   strncpy(inept.ch2,"obs",3);
   putCmd("ch1YXinept='dec2'\n");
   putCmd("ch2YXinept='obs'\n");
   
   DSEQ dec = getdseq("H");
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHtppm='dec'\n");
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHspinal='dec'\n");

   DSEQ mix = getdseq("Hmix");
   strncpy(mix.t.ch,"dec",3);
   putCmd("chHmixtppm='dec'\n"); 
   strncpy(mix.s.ch,"dec",3);
   putCmd("chHmixspinal='dec'\n"); 

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   double simpw1 = inept.pw1;
   if (inept.pw2 > inept.pw1) simpw1 = inept.pw2;

   double simpw2 = inept.pw3;
   if (inept.pw4 > inept.pw3) simpw2 = inept.pw4;

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90") + getval("tHY") + simpw1 + simpw2;
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = inept.t1 + inept.t2 + getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = inept.t1 + inept.t2 + getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phH90,16,table1);
   settable(phHhy,4,table2);
   settable(phYhy,4,table3);
   settable(ph1Yyxinept,4,table4);
   settable(ph1Xyxinept,4,table5);
   settable(ph2Yyxinept,4,table6);
   settable(ph2Xyxinept,16,table7);
   settable(phRec,8,table8);
   setreceiver(phRec);

// Begin Sequence

   txphase(ph1Xyxinept); decphase(phH90); dec2phase(phYhy);
   obspwrf(getval("aXyxinept")); decpwrf(getval("aH90")); dec2pwrf(getval("aYhy"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to Y Cross Polarization

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decphase(phHhy);
   _cp_(hy,phHhy,phYhy);
   decphase(zero);

// INEPT Transfer from Y to X

   _dseqon(mix);
   _inept(inept,ph1Yyxinept,ph1Xyxinept,ph2Yyxinept,ph2Xyxinept);
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

