/*ssecho1dhdec.c - A sequence to provide a highband H solid echo 
                    with optional homonuclear decoupling during 
                    the echo delay and X TPPM or SPINAL decoupling 
                    during acquisition. 

               SAM - K. Mao, J.W. Wiench, M. Pruski 05/28/08
               updated for vnmrj3.2 08/01/12 
               Updated for VnmrJ4.2 V.Zorin 01/07/14                      */ 

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4]  = {0,2,1,3};           // phH90
static int table2[8]  = {1,1,2,2,3,3,0,0};   // phHecho
static int table3[4]  = {0,2,1,3};           // phRec      

#define phH90 t1
#define phHecho t2
#define phRec t3
 
pulsesequence() {

//======================================================
// Define Variables and Objects and Get Parameter Values
//======================================================

// --------------------------------
// Acquisition Decoupling
// -------------------------------

   char Xseq[MAXSTR];
   getstr("Xseq",Xseq);
   DSEQ dec = getdseq("X");
   strncpy(dec.t.ch,"dec",3);
   putCmd("chXtppm='dec'\n");
   strncpy(dec.s.ch,"dec",3);
   putCmd("chXspinal='dec'\n");

//-------------------------------------
// Homonuclear Decoupling During Echo
//-------------------------------------

   MPDEC homo1 = getmpdec("hdec1H",0,0.0,0.0,0,1);
   strncpy(homo1.mps.ch,"obs",3);
   putCmd("chHhdec1='obs'\n"); 

// --------------------
// H echo calculation
// --------------------

   double t1Hecho = getval("t1Hecho") - getval("pwHecho")/2.0 - 
                    ((!strcmp(homo1.dm,"y"))?getval("pwHshort1")*2.:0.0);
   if (t1Hecho < 0.0) t1Hecho = 0.0;
   double t2Hecho = getval("t2Hecho") - getval("pwHecho")/2.0 - 
                    ((!strcmp(homo1.dm,"y"))?getval("pwHshort1")*2.:0.0) - 
                    getval("rd")- getval("ad");
   if (t2Hecho < 0.0) t2Hecho = 0.0;
 

   double t1H_echo = 0.0; 
   double t2H_echo = 0.0;
   double t1H_left = 0.0; 
   double t2H_left = 0.0;
   if (!strcmp(homo1.dm,"y")) {
      t2H_echo = homo1.mps.t*((int)(t2Hecho/homo1.mps.t));
      t2H_left = t2Hecho - t2H_echo;
      t1H_echo = t2H_echo;
      t1H_left = t1Hecho - t1H_echo;
   }

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

//----------------------
// Dutycycle Protection
//----------------------

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90");
   d.dutyoff = d1 + 4.0e-6;
   if (!strcmp(homo1.dm,"y"))
     d.dutyon += t1H_echo + t2H_echo;
   else
     d.dutyoff += t1H_echo + t2H_echo;
   d.c1 = d.c1 + (!strcmp(Xseq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(Xseq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(Xseq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(Xseq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

//------------------------
// Set Phase Tables
//-----------------------

   settable(phH90,4,table1);    
   settable(phHecho,8,table2);
   settable(phRec,4,table3);
   setreceiver(phRec);

//=======================    
// Begin Sequence
//=======================

   txphase(phH90); decphase(zero);
   obspwrf(getval("aH90")); 
   obsunblank(); decunblank(); _unblank34();
   delay(d1);  
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

//------------------------  
// H Direct Polarization 
//------------------------
  
   rgpulse(getval("pwH90"),phH90,0.0,0.0);
   obsunblank(); decunblank(); _unblank34();

// -----------------------------
// H Hahn Echo
// -----------------------------

   if (!strcmp(homo1.dm,"y")) {
      delay (t1H_left);
      if (getval("pwHshort1") > 0.0 ) {
         obspwrf(getval("aHhdec1"));
         rgpulse(getval("pwHshort1"),three,0.0,0.0);  
         obsunblank();
      }
      if (!strcmp(homo1.dm,"y")) _mpseqon(homo1.mps,zero);
      delay(t1H_echo);
      if (!strcmp(homo1.dm,"y")) _mpseqoff(homo1.mps);

      if (getval("pwHshort1") > 0.0 ) {
         obspwrf(getval("aHhdec1")); txphase(one);
         rgpulse(getval("pwHshort1"),one,0.0,0.0);  
         obsunblank();
      }
   }
   else delay(t1Hecho);
   txphase(phHecho);
   obspwrf(getval("aHecho"));
   rgpulse(getval("pwHecho"),phHecho,0.0,0.0);
   obsunblank();

   if (!strcmp(homo1.dm,"y")) {
      if (getval("pwHshort1") > 0.0 ) {
         obspwrf(getval("aHhdec1"));
         rgpulse(getval("pwHshort1"),three,0.0,0.0);  
         obsunblank();
      }
      if (!strcmp(homo1.dm,"y")) _mpseqon(homo1.mps,zero);
      delay(t2H_echo);
      if (!strcmp(homo1.dm,"y")) _mpseqoff(homo1.mps);

      if(getval("pwHshort1")>0 )  {
         obspwrf(getval("aHhdec1"));
         rgpulse(getval("pwHshort1"),one,0.0,0.0);  
         obsunblank();
      }
      delay(t2H_left);
   }
   else delay(t2Hecho);


//====================
// Begin Acquisition 
//====================

   _dseqon(dec);    
   obsblank(); decblank(); _blank34();
   delay(getval("rd"));  
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec); 
   obsunblank(); decunblank(); _unblank34();
}

