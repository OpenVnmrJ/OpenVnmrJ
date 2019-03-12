/*hh2dhdec.c - A 2D HH correlation with direct polarization of H, 
               a d2 period with optional homonuclear decoupling, a
               Z-filter and H acquisition with optional X TPPM or 
               SPINAL decoupling. 

               Updated for VnmrJ3.2 D. Rice 08/01/12
               Updated for VnmrJ4.2 V.Zorin 01/07/14              */

#include "standard.h"
#include "solidstandard.h"

//================================
// Define Values for Phasetables
//================================

static int table1[8] = {0,2,0,2,0,2,0,2};           // ph1H90
static int table2[8] = {0,0,0,0,0,0,0,0};           // ph2H90   
static int table3[8] = {0,2,1,3,2,0,3,1};           // phRec 
static int table4[8] = {0,0,1,1,2,2,3,3};           // ph3H90

#define ph1H90  t1
#define ph2H90  t2
#define phRec   t3
#define ph3H90  t4

static double d2_init;

void pulsesequence() {

//=======================================================
// Define Variables and Objects and Get Parameter Values
//=======================================================

// --------------------------
// Acquisition Decoupling
// -------------------------

   char Xseq[MAXSTR];
   getstr("Xseq",Xseq);
   DSEQ dec = getdseq("X");
   strncpy(dec.t.ch,"dec",3);
   putCmd("chXtppm='dec'\n"); 
   strncpy(dec.s.ch,"dec",3);
   putCmd("chXspinal='dec'\n");

//---------------------------
// Homonuclear Decoupling
//---------------------------

   MPDEC homo1 = getmpdec("hdec1H",0,0.0,0.0,0,1);
   strncpy(homo1.mps.ch,"obs",3);
   putCmd("chHhdec1='obs'\n"); 

//---------------------------------------------
// Set d2 Period to Return Evolution and Dwell 
//---------------------------------------------

   if (d2_index == 0) d2_init = getval("d2");
   double d2_ = (ni - 1)/sw1 + d2_init;
   putCmd("d2acqret = %f\n",roundoff(d2_,12.5e-9));
   putCmd("d2dwret = %f\n",roundoff(1.0/sw1,12.5e-9));

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

//-------------------------
// Dutycycle Protection
//-------------------------

   DUTY d = init_dutycycle();
   d.dutyon = 3.0*getval("pwH90");
   d.dutyoff = d1 + 4.0e-6 + getval("tZF");
   if (!strcmp(homo1.dm,"y"))
       d.dutyon += d2_ + 2.0*getval("pwHshort1");
   else
       d.dutyoff += d2_;
   d.c1 = d.c1 + (!strcmp(Xseq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(Xseq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(Xseq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(Xseq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);
   
//----------------------
// Set Phase Tables
//----------------------
 
   settable(ph1H90,8,table1);
   settable(ph2H90,8,table2);
   settable(phRec,8,table3);
   settable(ph3H90,8,table4);
   setreceiver(phRec);
   
  if(phase1 == 2)
      tsadd(ph1H90,1,4);
  if ((d2_index%2)==1)
      tsadd(ph1H90,2,4);

//=================
// Begin Sequence
//=================

   txphase(ph1H90); decphase(zero);
   obspwrf(getval("aH90"));   
   obsunblank(); decunblank(); _unblank34(); 
   delay(d1);  
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

//-------------------------   
// H Direct Polarization
//-------------------------

   rgpulse(getval("pwH90"),ph1H90,0.0,0.0);
   obsunblank();

//---------------------------------------------------
// H d2 Evolution Period with Homonuclear Decoupling
//---------------------------------------------------

   if (!strcmp(homo1.dm,"y")) {
      if (getval("pwHshort1") > 0.0) {
         obspwrf(getval("aHhdec1"));
         rgpulse(getval("pwHshort1"),three,0.0,0.0);
         obsunblank();
      }
      _mpseqon(homo1.mps,zero); 
      delay(d2);    
      _mpseqoff(homo1.mps);

      if (getval("pwHshort1") > 0.0) {
         obspwrf(getval("aHhdec1"));
         rgpulse(getval("pwHshort1"),one,0.0,0.0); 
         obspwrf(getval("aH90"));
         obsunblank();
      }
   }
   else {
      txphase(ph2H90); 
      obspwrf(getval("aH90"));
      delay(d2);
   }

// --------------------------------
// Store X Magnetization Along Z
// --------------------------------
   
   rgpulse(getval("pwH90"),ph2H90,0.0,0.0);
   txphase(ph3H90);
   obsunblank();

// --------------------------------
// Z-filter Delay
// --------------------------------

   delay(getval("tZF"));
   rgpulse(getval("pwH90"),ph3H90,0.0,0.0);
   obsunblank();

//=====================
// Begin Acquisition 
//=====================
   
   _dseqon(dec);
   obsblank(); _blank34();
   delay(getval("rd"));  
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

