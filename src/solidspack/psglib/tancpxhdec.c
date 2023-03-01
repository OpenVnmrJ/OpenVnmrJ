/*tancpxhdec -  A 2D HX HETCOR with an HX CP transfer. A d2 period with either X 
                decoupling only or homonuclear decoupling with an X pi pulse. 
                An X acquisition using optional H TPPM or SPINAL decoupling.
                X-d2 decoupling is balanced with X decoupling after acquisition.

                J.W. Wiench 02/20/09                                    
                Modified for VnmrJ3.2 08/01/12
                Updated for VnmrJ4.2 V.Zorin 01/07/14                               */

#include "standard.h"
#include "solidstandard.h"

// define values for phasetables

static int table1[4] = {0,2,0,2};           // phH90    
static int table2[4] = {0,0,1,1};           // phXhx    
static int table3[4] = {1,1,1,1};           // phHhx   
static int table4[4] = {0,2,1,3};           // phRec 

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define phRec t4

static double d2_init;

void pulsesequence() {

//======================================================
//Define Variables and Objects and Get Parameter Values
//====================================================== 

// --------------------------------
// H to X CP
// --------------------------------  
 
   CP hx = getcp("HX",0.0,0.0,0,1);
   strcpy(hx.fr,"dec");
   strcpy(hx.to,"obs");
   putCmd("frHX='dec'\n"); 
   putCmd("toHX='obs'\n");

// -------------------------------
// H Acquisition Decoupling
// -------------------------------

   char Hseq[MAXSTR];
   getstr("Hseq",Hseq);
   DSEQ dec = getdseq("H");
   strcpy(dec.t.ch,"dec");
   putCmd("chHtppm='dec'\n"); 
   strcpy(dec.s.ch,"dec");
   putCmd("chHspinal='dec'\n"); 

//-----------------
// X d2 Decoupling
//-----------------

   DSEQ obs;
   char Xseq[MAXSTR];
   getstr("Xseq",Xseq);
   if (strcmp(Xseq,"pipulse") != 0) {
      obs = getdseq("X");
      strcpy(obs.t.ch,"obs");
      putCmd("chXtppm='obs'\n"); 
      strcpy(obs.s.ch,"obs");
      putCmd("chXspinal='obs'\n"); 
   }

   double Xseqmin=0.0;   
   if (!strcmp(Hseq,"tppm")) Xseqmin = getval("pwXtppm");
   if (!strcmp(Hseq,"spinal")) Xseqmin = getval("pwXspinal");

//--------------------------------------------
// Homonuclear Decoupling during d2 Evolution
//--------------------------------------------
   
   MPDEC homo1 = getmpdec("hdec1H",0,0.0,0.0,0,1);
   strcpy(homo1.mps.ch,"dec");
   putCmd("chHhdec1='dec'\n"); 

//----------------------------------
// Set Constant-time Period for d2. 
//----------------------------------

   if (d2_index == 0) d2_init = getval("d2");
   double d2_ = (ni - 1)/sw1 + d2_init;
   putCmd("d2acqret = %f\n",roundoff(d2_,12.5e-9));
   putCmd("d2dwret = %f\n",roundoff(1.0/sw1,12.5e-9));

   double t1bal = 0.0;
   if (sw1 > 1000.0) t1bal = d2_ - d2;
   if (t1bal < 0.0) t1bal = 0.0;

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// -----------------------------
// Dutycycle Protection   
// -----------------------------


   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90") + getval("tHX");
   d.dutyoff = d1 + 4.0e-6;
   
   d.c1 = d.c1 + (!strcmp(Hseq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(Hseq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(Hseq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(Hseq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d.c3 = d.c3 + (!strcmp(Xseq,"pipulse")) && (strcmp(homo1.dm,"y") != 0);
   d.t3 = d.t3 +  getval("pwX180");
   d.c4 = d.c4 + (!strcmp(Xseq,"tppm"));
   d.c4 = d.c4 + ((!strcmp(Xseq,"tppm")) && (obs.t.a > 0.0));
   d.t4 = d2_;
   d.c5 = d.c5 + (!strcmp(Xseq,"spinal"));
   d.c5 = d.c5 + ((!strcmp(Xseq,"spinal")) && (obs.t.a > 0.0));
   d.t5 = d2_;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);
   
//-----------------------
// Set Phase Tables
//-----------------------
 
   settable(phH90,4,table1);
   settable(phXhx,4,table2);
   settable(phHhx,4,table3);
   settable(phRec,4,table4);
   setreceiver(phRec);

// -----------------------------
// States and TPPI (FAD)
// -----------------------------

   if(phase1 == 2)
      tsadd(phH90,1,4);
   if ((d2_index%2)==1)
      tsadd(phH90,2,4);

//==============================
// Begin Sequence
//==============================

   txphase(phXhx); decphase(phH90);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));  
   obsunblank(); decunblank(); _unblank34();
   delay(d1);   
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// --------------------------------
// H Direct Polarization
// --------------------------------

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decphase(phHhx);    

// -----------------------------
// H d2 Evolution Period 
// -----------------------------

   if (strcmp(Xseq,"pipulse") != 0) {
      decphase(phHhx);
      decpwrf(getval("aHhx")); 
      if (d2 >= Xseqmin) _dseqon(obs);
      delay(d2);
      if (d2 >= Xseqmin) _dseqoff(obs);       
   }
   else {                   
      if ((getval("pwHshort3") > 0.0) && (!strcmp(homo1.dm,"y"))) {
         decpwrf(getval("aHhdec3"));
         decrgpulse(getval("pwHshort3"),three,0.0,0.0);  
         decunblank();
      }
      if (d2 >= getval("pwX180")) {
         if (!strcmp(homo1.dm,"y")) _mpseqon(homo1.mps,zero);
         obspwrf(getval("aX180"));
         delay(d2/2.0 - getval("pwX180")/2.0);
         rgpulse(getval("pwX180"),zero,0.0,0.0);
         txphase(phXhx);
         obspwrf(getval("aXhx"));
         obsunblank();
         delay(d2/2.0 - getval("pwX180")/2.0);
         if(!strcmp(homo1.dm,"y")) _mpseqoff(homo1.mps);
      }
      else {
         txphase(phXhx);
         obspwrf(getval("aXhx"));
         delay(d2);
      }
      if ((getval("pwHshort3") > 0.0) && (!strcmp(homo1.dm,"y"))) {
         decpwrf(getval("aHhdec3"));
         decrgpulse(getval("pwHshort3"),one,0.0,0.0);  
         decunblank();
      }
   }   

// -----------------------------
// H to X Cross Polarization 
// -----------------------------

    _cp_(hx,phHhx,phXhx); 

// --------------------------------
// Begin Acquisition
// --------------------------------

   obsblank(); _blank34();
   delay(getval("rd"));  
   _dseqon(dec);             
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);         

// ---------------------------------------------
// Constant-time X Decoupling after Acquisition
// ---------------------------------------------

   if ((strcmp(Xseq,"pipulse") != 0) && (d2 >= Xseqmin)) _dseqon(obs); 
   delay(t1bal);   
   if ((strcmp(Xseq,"pipulse") != 0) && (d2 >= Xseqmin)) _dseqoff(obs);
   obsunblank(); decunblank(); _unblank34(); 
}   

