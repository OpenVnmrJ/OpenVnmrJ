/*ineptxyonepul.c - INEPT polarization transfer from Y to X.  One-pulse
                    preparation of Y polarization with INEPT transfer to X.
		    SPINAL or TPPM decoupling throughout with separate levels
		    during acquisition and INEPT transfer.

                    Phase cycle based on the infinity+ sequence "inept_dec.s"
                    provided by Marek Prueski.

                    D. Rice 08/02/06                                      */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2}; // phY90
static int table2[4] = {0,2,0,2};                          // ph1Yyxinept
static int table3[4] = {0,2,0,2};                          // ph1Xyxinept
static int table4[4] = {1,1,3,3};                          // ph2Yyxinept
static int table5[16] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3}; // ph2Xyxinept
static int table6[8] = {0,0,2,2,1,1,3,3};                  // phRec

#define phY90 t1
#define ph1Yyxinept t2
#define ph1Xyxinept t3
#define ph2Yyxinept t4
#define ph2Xyxinept t5
#define phRec t6

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   GP inept = getinept("ineptYX");
   strcpy(inept.ch1,"dec2");
   strcpy(inept.ch2,"obs");
   putCmd("ch1YXinept='dec2'\n");
   putCmd("ch2YXinept='obs'\n");

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
   d.dutyon = getval("pwY90") + simpw1 + simpw2;
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

   settable(phY90,16,table1);
   settable(ph1Yyxinept,4,table2);
   settable(ph1Xyxinept,4,table3);
   settable(ph2Yyxinept,4,table4);
   settable(ph2Xyxinept,16,table5);
   settable(phRec,8,table6);
   setreceiver(phRec);

// Begin Sequence

   txphase(ph1Xyxinept); decphase(zero); dec2phase(phY90);
   obspwrf(getval("aXyxinept")); dec2pwrf(getval("aY90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// Y Direct Polarization 

   _dseqon(mix);
   dec2rgpulse(getval("pwY90"),phY90,0.0,0.0);

// INEPT Transfer from Y to X

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

