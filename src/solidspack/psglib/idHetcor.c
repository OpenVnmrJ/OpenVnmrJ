/* idHetcor.c - A 2D XH HETCOR with preparation through HX CP, an X 
                d2 period, mixing through rotary resonance, Z-filter 
                with XH CP, and H detection. The d2 period allows 
                decoupling with an optional H pi pulse or TPPM or 
                SPINAL decoupling. H acquisition allows optional X 
                TPPM or SPINAL decoupling. H-d2 decoupling is balanced
                with continued decoupling after a flip to Z. 

                J.W. Wiench 2009/02/26 
                updated for vnmrj3.2 08/01/12
                Updated for VnmrJ4.2 V.Zorin 01/07/14               */                                    

/* Changes: 
1. Move duty and Hseq definitions.
2. Add abort for srate=0.0.  
3. Round taur to 12.5 ns. 
4. Add initial "d2_init" value to constant-time period. 
5. Write dwell and evolution time to panels. 
6. Change "pidec" to "pipulse" and pidec suffix to H180. 
7. Standardize strcmp for "pipulse".  
8. Remove obsunblank statements, except after rgpulse. 
9. Replace dec2blank with _blank34 and dec2unblank with
   _unblank34 to allow operation with two-channel systems. 
10.Move first decphase and decpwrf to before evolution period.
11.Move second decphase and decpwrf to before RR period.
12.Move txphase and obspwrf to before the Z-filter delay.
13.Remove redundant decphase and decpwrf before second CP
   - CP sets that. 
14.Remove redundant txphase and obspwrf before first CP
   - CP sets that. 
15.Add txphase zero before pwH180 rgpulse.
16.Upgrade duty calculation to include all conditionals (this is new).
17.Add a retrun value for 2.0*tHrr 
18.Add a groupcopy to processed
*/

#include "standard.h"
#include "solidstandard.h"

//=================================
// Define Values for Phasetables
//=================================

static int tblRec[4]  = {0,2,1,3};           // receiver
static int tblH90[4]  = {1,1,1,1};           // phH90
static int tblXhx[8]  = {0,0,0,0};           // phXhx
static int tblHhx[4]  = {0,0,0,0};           // phHhx
static int tblXxh[4]  = {1,1,1,1};           // phXxh
static int tblHxh[4]  = {0,0,1,1};           // phHxh
static int tblXRE[4]  = {1,1,1,1};           // X90 restore
static int tblX90[4]  = {0,2,0,2};           // X90 pulse

#define phRec  t1
#define phH90  t2
#define phXhx  t3
#define phHhx  t4
#define phXRE  t5
#define phX90  t6
#define phXxh  t7
#define phHxh  t8

static double d2_init;

void pulsesequence() {


// =========================================================
// Define Variables and Objects and Get Parameter Values
// =========================================================

// --------------------------------
// H to X CP
// --------------------------------

   CP hx = getcp("HX",0.0,0.0,0,1);
   strcpy(hx.fr,"obs");
   strcpy(hx.to,"dec");
   putCmd("frHX='obs'\n"); 
   putCmd("toHX='dec'\n");
   
// --------------------------------
// X to H CP
// --------------------------------

   CP xh = getcp("XH",0.0,0.0,0,1);
   strcpy(xh.fr,"dec");
   strcpy(xh.to,"obs");
   putCmd("frXH='dec'\n"); 
   putCmd("toXH='obs'\n"); 

// --------------------------------
// Acquisition Decoupling
// -------------------------------

   char Xseq[MAXSTR];
   getstr("Xseq",Xseq);
   DSEQ dec = getdseq("X");
   strcpy(dec.t.ch,"dec");
   putCmd("chXtppm='dec'\n"); 
   strcpy(dec.s.ch,"dec");
   putCmd("chXspinal='dec'\n"); 

//--------------------
// d2 Decoupling
//--------------------

   DSEQ obs;
   char Hseq[MAXSTR];
   getstr("Hseq",Hseq);
   if (strcmp(Hseq,"pipulse") != 0) {
      obs = getdseq("H");
      strcpy(obs.t.ch,"obs");
      putCmd("chHtppm='obs'\n");
      strcpy(obs.s.ch,"obs");
      putCmd("chHspinal='obs'\n");
   }
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

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// ---------------------------------
// Dutycycle Protection   
// ---------------------------------
 
   DUTY d = init_dutycycle();
   d.dutyon = getval("tHX") + getval("tXH") + 3.0*getval("pwX90");
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

   d.c1 = d.c1 + (!strcmp(Xseq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(Xseq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(Xseq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(Xseq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);
   
// =========================================================
// Set Phase Tables
// =========================================================
 
   settable(phRec,4,tblRec);
   settable(phH90,4,tblH90);
   settable(phXhx,4,tblXhx);
   settable(phHhx,4,tblHhx);
   settable(phXxh,4,tblXxh);
   settable(phHxh,4,tblHxh);
   settable(phXRE,4,tblXRE);
   settable(phX90,4,tblX90);

// -----------------------------
// States and TPPI (FAD)
// -----------------------------

   if (phase1 == 2) tsadd(phXRE,3,4);
   if ((d2_index%2) == 1) tsadd(phXRE,2,4);
  
// --------------------------------
// Set Receiver Phase
// --------------------------------

   setreceiver(phRec);
    
// =========================================================
// Begin Sequence
// =========================================================

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

   decphase(phX90);
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

   txphase(phHxh); 
   obspwrf(getval("aHxh"));
   delay(getval("tZF"));

// --------------------------------
// X to H Cross Polarization
// --------------------------------

   decrgpulse(getval("pwX90"),phX90,0.0,0.0);
   _cp_(xh,phXxh,phHxh);

// --------------------------------
// Begin Acquisition 
// --------------------------------

   _dseqon(dec);                 
   obsblank(); _blank34();
   delay(getval("rd"));   
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34(); 
}
