/*onepulHrr.c - A 1D sequence with preparation by direct polarization of X 
                followed by a rotoational resonance period, and acqisition 
                of X with optional H TPPM or SPINAL decoupling. 

 
	     J.W. Wiench 2007/08/28                                             
             SolidsPack Update for VnmrJ3.2 D. Rice 2012/08/01  
             Updated for VnmrJ4.2 V.Zorin 01/07/14                      */             

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int tblX90[4]   = {0,2,1,3};           // phX90   
static int tblHrr[4] = {3,3,0,0};             // phHrr
static int tblHrr2[4] = {0,0,1,1};            // ph1Hrr
static int tblRec[4]   = {2,0,3,1};           // receiver 

#define phX90   t1
#define phHrr   t2
#define ph1Hrr  t3
#define phRec   t4

void pulsesequence() {

// =========================================================
// Define Variables and Objects and Get Parameter Values
// =========================================================
   
// --------------------------------
// Acquisition Decoupling
// -------------------------------

   char Hseq[MAXSTR];
   getstr("Hseq",Hseq);
   DSEQ dec = getdseq("H");
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHtppm='dec'\n"); 
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHspinal='dec'\n"); 

// ---------------------------------------------
// Determine taur, One Rotor Cycle and Set tHrr.
// ---------------------------------------------

   double srate = getval("srate");
   double taur = 0.0;
   if (srate >= 500.0)
      taur = roundoff((1.0/srate), 0.125e-6);
   else {
      abort_message("ABORT: Spin Rate (srate) must be greater than 500\n");
   }
   double tHrr = getval("nHrr")*taur;
   double tHrrret= 2.0*tHrr;
   putCmd("tHrrret = %f\n",tHrrret*1.0e6);

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

//------------------------------
// Dutycycle Protection
//------------------------------
   
   DUTY d = init_dutycycle();
   d.dutyon = getval("pwX90");
   d.dutyoff = d1 + 4.0e-6;
   if (getval("aHrr") > 0.0)
     d.dutyon += 2.0*tHrr;
   d.c1 = d.c1 + (!strcmp(Hseq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(Hseq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(Hseq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(Hseq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);
   
//---------------------------
// Set Phase Tables
//---------------------------
 
   settable(phX90,4,tblX90);
   settable(phHrr,4,tblHrr);
   settable(ph1Hrr,4,tblHrr2);
   settable(phRec,4,tblRec);
   setreceiver(phRec);

//===========================    
// Begin Sequence
//===========================

   txphase(phX90); decphase(zero);
   obspwrf(getval("aX90"));   
   obsunblank(); decunblank(); _unblank34();
   delay(d1);  
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

//----------------------------   
// X Direct Polarization 
//----------------------------
   
   rgpulse(getval("pwX90"),phX90,0.0,0.0);
   obsunblank();

// -----------------------------
// Begin Rotary Resonance
// -----------------------------

   if(getval("aHrr") > 0.0) {
      obspwrf(getval("aHrr"));
      txphase(phHrr);
      xmtron();
      delay(tHrr);
      txphase(ph1Hrr);
      delay(tHrr);
      xmtroff();
   }

//===========================
// Begin Acquisition 
//===========================
   
   _dseqon(dec);
   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

