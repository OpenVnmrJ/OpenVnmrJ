/*hh2dhdec.c - A 2D HH correlation with direct polarization of H, 
               a d2 period with optional homonuclear decoupling, a
               Z-filter and H acquisition with optional X TPPM or 
               SPINAL decoupling. 

               Updated for VnmrJ3.2 D. Rice 08/01/12                  */


// Reset minimum shape stepsize to 50 nsec
#define VNMRSN90 4                  //  N90 for VNMRS (50 ns is 4)

#include "standard.h"
#include "solidstandard.h"
#include "amestandard.h"
//#include "CompSens.h"  

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

pulsesequence() {
/*
//==========================================================================
// Add NUS for this Sequence for SPARSE='y', (ni > 1) and (nimax > ni + 1). 
//==========================================================================

   char sparseval[MAXSTR];
   getstr("SPARSE",sparseval);
   if (sparseval[A] == 'y' && getval("ni") > 1.0 && getval("nimax") >= (getval("ni") + 1.0)) {
      (void) set_RS(0);
      ni = getval("nimax");
      if (ni < 1) ni = 1;
   }

//-----------------------
// Else Force SPARSE='n'.
//-----------------------

   else {                     
      strncpy(sparseval,"n",3);
      putCmd("SPARSE='n'\n");   
      printf("No F1 Array - Set SPARSE='n'\n");
   }

//-------------------------------------------------------
// Replace the SPARSE schedule.sch with a Custom Schedule 
//-------------------------------------------------------

   char nus[MAXSTR];
   getstr("nusfilename",nus);
   char stypeval[MAXSTR];
   getstr("stype",stypeval);

   int i = 1000;
   char systemstring[4096]="";
   if (sparseval[A]=='y') { 
      if (strcmp(stypeval,"a") && strcmp(nus,"")) {
         sprintf(systemstring,"cp %s/schedules/%s.sch %s/sampling.sch",userdir,nus,curexp);
         i = system(systemstring);
         printf("i = %d\n",i);
         if (i != 0) {
            printf("Error: NUS File Does Not Exist\n");
            psg_abort(1); 
         }
         printf("Use Custom Schedule\n");
      }
      else printf("Use Standard Schedule\n");
   }
   else printf("linear Sample Schedule\n");
*/
//=======================================================
// Define Variables and Objects and Get Parameter Values
//=======================================================

// --------------------------
// Acquisition Decoupling
// -------------------------

   char Xseq[MAXSTR];
   getstr("Xseq",Xseq);
   DSEQ dec = getdseq("X");

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

   double duty = 3.0*getval("pwH90") + d2_ + 2.0*getval("pwHshort1") + getval("ad") + getval("rd") + at;
   double dutyon = duty; 
   if (strcmp(homo1.dm,"y")!=0) dutyon = dutyon - d2_ - 2.0*getval("pwHshort1");
   if ((!strcmp(Xseq,"tppm")) && (getval("aXtppm") == 0.0)) dutyon = dutyon - 
                 getval("ad") - getval("rd") - at; 
   if ((!strcmp(Xseq,"spinal")) && (getval("aXspinal") == 0.0)) dutyon = dutyon -
                 getval("ad") - getval("rd") - at; 
   duty = dutyon/(duty + d1 + 4.0e-6 + getval("tZF"));  
   if (duty > 0.1) {
      printf("Duty cycle %.1f%% >10%%. Abort!\n", duty*100.0);
      psg_abort(1);
   }

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

