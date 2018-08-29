/*idInept.c - A 2D XH HETCOR with preparation through HX CP, an X 
              d2 period, mixing through rotary resonance, Z-filter 
              with XH INEPT, and H detection. The XH INEPT allows 
              optional refocussing and homonulcear decoupling on H.
              The d2 period allows decoupling with an optional H 
              pi pulse or TPPM or SPINAL decoupling. H acquisition
              allows optional X TPPM or SPINAL decoupling. H-d2 
              decoupling is balanced with continued decoupling after 
              a flip to Z. 

              Update for VnmrJ3.2 2012/07/27
              Updated for VnmrJ4.2 V.Zorin 01/07/14                */ 

#include "standard.h"
#include "solidstandard.h"

// Define Values For Phasetables

static int tblH90[4]        = {0,0,0,0};          // phH90    
static int tblHhx[4]        = {1,1,1,1};          // phHhx    
static int tblXhx[4]        = {1,1,1,1};          // phXhx
static int tblXRE[4]        = {0,0,0,0};          // X90 restore
static int tblXxhsta[16]    = {0,0,0,0,0,0,0,0,
                               2,2,2,2,2,2,2,2};  // X90 start phase in INEPT
static int tbl1Xxhinept[4]  = {0,2,0,2};          // X180 phase in INEPT
static int tbl1Hxhinept[4]  = {0,2,0,2};          // H180 phase in INEPT  
static int tbl2Xxhinept[4]  = {1,1,3,3};          // X90 in INEPT 
static int tbl2Hxhinept[16] = {0,0,0,0,1,1,1,1,
                               2,2,2,2,3,3,3,3};  // H90 in INEPT
static int tbl3Xxhinept[4]  = {0,2,0,2};          // X180 phase in INEPT  
static int tbl3Hxhinept[8]  = {0,2,0,2,1,3,1,3};  // H180 last phase in INEPT
static int tblRec[16]       = {0,0,2,2,1,1,3,3,
                               0,0,2,2,1,1,3,3};  // receiver

#define phH90 t1
#define phHhx t2
#define phXhx t3
#define phXRE t4
#define phXxhsta t5
#define ph1Xxhinept t6
#define ph1Hxhinept t7
#define ph2Xxhinept t8
#define ph2Hxhinept t9
#define ph3Xxhinept t10
#define ph3Hxhinept t11
#define phRec t12

static double d2_init;

pulsesequence() {

// =========================================================
// Define Variables and Objects and Get Parameter Values
// =========================================================

// --------------------------------
// H to X CP
// --------------------------------

   CP hx = getcp("HX",0.0,0.0,0,1);
   strncpy(hx.fr,"obs",3);
   strncpy(hx.to,"dec",3);
   putCmd("frHX='obs'\n"); 
   putCmd("toHX='dec'\n");

// --------------------------------
// X to H INEPT
// --------------------------------

   GP inept = getinept("ineptXH");
   strncpy(inept.ch1,"dec",3);
   strncpy(inept.ch2,"obs",3);
   putCmd("ch1XHinept='dec'\n");
   putCmd("ch2XHinept='obs'\n");

   char refXHinept[MAXSTR];
   getstr("refXHinept",refXHinept);

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

//--------------------
// d2 Decoupling
//--------------------

   DSEQ obs;
   char Hseq[MAXSTR];
   getstr("Hseq",Hseq);
   if (strcmp(Hseq,"pipulse") != 0) {
      obs = getdseq("H");
      strncpy(obs.t.ch,"obs",3);
      putCmd("chHtppm='obs'\n");
      strncpy(obs.s.ch,"obs",3);
      putCmd("chHspinal='obs'\n");
   }
//-------------------------------------
// Homonuclear Decoupling during INEPT
//-------------------------------------

   MPDEC homo1 = getmpdec("hdec1H",0,0.0,0.0,0,1);
   strncpy(homo1.mps.ch,"obs",3);
   putCmd("chHhdec1='obs'\n"); 
   MPDEC homo2 = getmpdec("hdec2H",0,0.0,0.0,0,1);
   strncpy(homo2.mps.ch,"obs",3);
   putCmd("chHhdec2='obs'\n"); 

// ---------------------------------------------
// Determine taur, One Rotor Cycle and Set tHrr.
// ---------------------------------------------

   double srate = getval("srate");
   double taur = 0.0;  
   if (srate >= 500.0)
      taur = roundoff((1.0/srate), 0.125e-6);
   else {
      printf("ABORT: Spin Rate (srate) must be greater than 500\n");
      psg_abort(1);
   }
   double tHrr = getval("nHrr")*taur;
   double tHrrret= 2.0*tHrr;
   putCmd("tHrrret = %f\n",tHrrret*1.0e6);

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

// -------------------------------------
// Adjust Delay Times for INEPT module. 
// -------------------------------------

   if (!strcmp(refXHinept,"y")) {
      inept.t1 = inept.t1 - getval("pwX90")/2.0 - getval("pwHshort1")*2.0;
      inept.t2 = inept.t2 - getval("pwHshort1")*2.0;
      inept.t3 = inept.t3 - getval("pwHshort2")*2.0;
      inept.t4 = inept.t4 - getval("rd") - getval("pwHshort2")*2.0;
   }
   else {
      inept.t1 = inept.t1 - getval("pwX90")/2.0 - getval("pwHshort1")*2.0;
      inept.t2 = inept.t2 - getval("rd") - getval("pwHshort1")*2.0;
   }

//-------------------------------------------------------------------
// Set Homonuclear Decoupling Cycles to Fit t1HXinept and t2XHinept 
//-------------------------------------------------------------------

   if (!strcmp(homo1.dm,"y")) {
      homo1.t = homo1.mps.t*((int) (((inept.t1>inept.t2)?inept.t2:inept.t1)/homo1.mps.t));
   }

   if (!strcmp(homo2.dm,"y")) {
      homo2.t = homo2.mps.t*((int) (((inept.t3>inept.t4)?inept.t4:inept.t3)/homo2.mps.t));
   }

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// --------------------------------
// Dutycycle Protection   
// --------------------------------

   DUTY d = init_dutycycle();
   d.dutyon = getval("tHX") + 3.0*getval("pwX90");
   d.dutyoff = d1 + 4.0e-6 + getval("tZF");
   
   if (!strcmp(Hseq,"pipulse")) {
     d.dutyon += getval("pwH180");
     d.dutyoff += d2_;
   }
   else
     d.dutyon += d2_;
   
   if (getval("aHrr") == 0.0)
     d.dutyoff += 2.0*tHrr;
   else
     d.dutyon += 2.0*tHrr;
   
   if (!strcmp(homo1.dm,"y"))
     d.dutyon += inept.t1 + inept.t2;
   else
     d.dutyoff += inept.t1 + inept.t2;
   
   if (!strcmp(refXHinept,"y")) {
     if (!strcmp(homo2.dm,"y"))
       d.dutyon += inept.t3 + inept.t4;
     else
       d.dutyoff += inept.t3 + inept.t4;
   }
   
   d.c1 = d.c1 + (!strcmp(Xseq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(Xseq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(Xseq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(Xseq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// =====================
// Set Phase Tables
// =====================
 
   settable(phH90,4,tblH90);
   settable(phHhx,4,tblHhx);
   settable(phXhx,4,tblXhx);
   settable(phXRE,4,tblXRE);
   settable(phXxhsta,16,tblXxhsta);
   settable(ph1Xxhinept,4,tbl1Xxhinept);
   settable(ph1Hxhinept,4,tbl1Hxhinept);
   settable(ph2Xxhinept,4,tbl2Xxhinept);
   settable(ph2Hxhinept,16,tbl2Hxhinept);
   settable(ph3Xxhinept,4,tbl3Xxhinept);
   settable(ph3Hxhinept,8,tbl3Hxhinept);
   settable(phRec,16,tblRec);

// -----------------------------
// States and TPPI (FAD)
// -----------------------------

   if (phase1 == 2)
      tsadd(phXRE,3,4);
   if ((d2_index%2)==1)
      tsadd(phXRE,2,4);
  
// --------------------------------
// Set Receiver Phase
// --------------------------------

   setreceiver(phRec);
    
// ==================
// Begin Sequence
// ==================

   txphase(phH90); decphase(phXhx);
   obspwrf(getval("aH90")); decpwrf(getval("aXhx"));
   obsunblank(); decunblank(); _unblank34(); 
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);
 
// --------------------------------
// H to X Cross Polarization
// --------------------------------

   rgpulse(getval("pwH90"),phH90,0.0,0.0);
   _cp_(hx,phHhx,phXhx);

// --------------------------------
// X d2 Evolution Period 
// --------------------------------

   decphase(phXRE);
   decpwrf(getval("aX90"));
   if (strcmp(Hseq,"pipulse") != 0) {
      _dseqon(obs);
      delay(d2);
   }
   else {
      if (getval("pwH180") < d2) {
         txphase(zero); 
         obspwrf(getval("aH180"));
         delay((d2 - getval("pwH180"))/2.0);
         rgpulse(getval("pwH180"),zero,0.0,0.0);
         obsunblank();
         delay((d2 - getval("pwH180"))/2.0);
      }
      else {
         delay(d2);
      }
   }

// --------------------------------
// Store X Magnetization Along Z
// --------------------------------

   decrgpulse(getval("pwX90"),phXRE,0.0,0.0);
   delay(t1bal);             
   if (strcmp(Hseq,"pipulse")!=0) _dseqoff(obs);  

 // --------------------------------
// Begin Rotary Resonance
// --------------------------------

   decphase(phXxhsta);
   decpwrf(getval("aX90"));
   if(getval("aHrr") > 0.0) {
      obspwrf(getval("aHrr"));
      txphase(zero);
      xmtron();
      delay(tHrr);
      txphase(one);
      delay(tHrr);
      xmtroff();
   }
   else delay(2.0*tHrr);

// --------------------------------
// Z-filter Delay
// --------------------------------

   txphase(ph1Hxhinept);
   obspwrf(getval("aHxhinept"));
   delay(getval("tZF"));

// --------------------------------
// X to H INEPT Transfer
// --------------------------------

   decrgpulse(getval("pwX90"),phXxhsta,0.0,0.0);
   decpwrf(getval("aXxhinept"));
   
   if (!strcmp(refXHinept,"y")) {
      _ineptrefdec2(inept,ph1Xxhinept,ph1Hxhinept,ph2Xxhinept,ph2Hxhinept,
        ph3Xxhinept,ph3Hxhinept,homo1,zero,homo2,zero,getval("pwHshort1"),
                                                     getval("pwHshort2"));
   } 
   else {
      _ineptdec2(inept,ph1Xxhinept,ph1Hxhinept,ph2Xxhinept,ph2Hxhinept,
                                       homo1,zero,getval("pwHshort1"));
   }

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
   obsunblank(); decunblank(); _unblank34(); 
}
