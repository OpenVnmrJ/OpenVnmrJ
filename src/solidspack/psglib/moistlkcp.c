/*moistlkcp.c -  MOIST CP with a prelock pulse and 
                 SPINAL decoupling. 
                              
            D. Rice 04/28/09                       */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,3,1,3};           // phH90
static int table2[4] = {0,0,0,0};           // phHlock
static int table3[4] = {3,3,3,3};           // phHcomp
static int table4[4] = {0,0,0,0};           // ph1Xhx
static int table5[4] = {2,2,2,2};           // ph2Xhx
static int table6[4] = {0,0,0,0};           // ph1Hhx
static int table7[4] = {2,2,2,2};           // ph2Hhx
static int table8[4] = {2,2,2,2};           // phHdec
static int table9[4] = {0,2,0,2};           // phRec

#define phH90 t1
#define phHlock t2
#define phHcomp t3
#define ph1Xhx t4
#define ph2Xhx t5
#define ph1Hhx t6
#define ph2Hhx t7
#define phHdec t8
#define phRec t9

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   double tHX3 = (getval("tHX"))/3.0;   //Define MOIST CP in the Sequence
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
   d.dutyon = getval("pwH90") + getval("pwHlock") + getval("pwHcomp")
              + getval("tHX");
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

   settable(phH90,4,table1);
   settable(phHlock,4,table2);
   settable(phHcomp,4,table3);
   settable(ph1Xhx,4,table4);
   settable(ph2Xhx,4,table5);
   settable(ph1Hhx,4,table6);
   settable(ph2Hhx,4,table7);
   settable(phHdec,4,table8);
   settable(phRec,4,table9);
   setreceiver(phRec);

// Begin Sequence

   txphase(ph1Xhx); decphase(phH90);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H 90-degree Pulse 

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);

// Prelock with Compensation Pulse

   decunblank(); decon(); 
   if (getval("onHlock") > 0) {
      decphase(phHlock);  
      delay(getval("pwHlock"));
      decphase(phHcomp);
      decpwrf(getval("aHcomp"));
      delay(getval("pwHcomp"));
   }

 // H to X MOIST Cross Polarization  

   xmtron();
   decphase(ph1Hhx);
   decpwrf(getval("aHhx"));
   delay(tHX3); 
   txphase(ph1Xhx); decphase(ph1Hhx);
   delay(tHX3); 
   txphase(ph2Xhx); decphase(ph2Hhx);
   delay(tHX3);
   xmtroff(); decoff(); 
   decphase(phHdec); 

// Begin Acquisition

   _dseqon(dec);
   obsblank(); _blank34();
   _decdoffset(getval("rd"),getval("ofHdec"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   decoffset(dof);
   obsunblank(); decunblank(); _unblank34();
}

