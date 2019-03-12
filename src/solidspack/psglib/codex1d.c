/*codex1d.c - A sequence to form a constant, linear or tangent ramped CP
             with SPINAL or TPPM decoupling. 

             This sequence uses a phase cycle derived from information 
             on the web site of K. Schmidt-Rohr, Iowa State University. 
             All other code is specific to the Varian NMR System.

             D. Rice 11/09/09                                           */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] =   {0,2,0,2};           // phH90
static int table2[8] =   {0,0,1,1,2,2,3,3};   // phXhx
static int table3[4] =   {1,1,1,1};           // phHhx
static int table4[16] =  {1,1,2,2,3,3,0,0,
                          3,3,0,0,1,1,2,2};   // ph1X90
static int table5[8] =   {3,3,0,0,1,1,2,2};   // ph2X90
static int table6[8] =   {0,1,0,1,1,0,1,0};   // ph1Xcdx
static int table7[64] =  {3,3,0,0,1,1,2,2,
                          3,3,0,0,1,1,2,2,
                          2,2,3,3,0,0,1,1,
                          2,2,3,3,0,0,1,1,
                          1,1,2,2,3,3,0,0,
                          1,1,2,2,3,3,0,0,
                          0,0,1,1,2,2,1,1,
                          0,0,1,1,2,2,1,1};   // ph3X90
static int table8[8] =   {0,1,0,1,1,0,1,0};   // ph2Xcdx
static int table9[32] =  {1,1,2,2,3,3,0,0,
                          3,3,0,0,1,1,2,2,
                          0,0,1,1,2,2,3,3,
                          2,2,3,3,0,0,1,1};   // ph4X90
static int table10[8] =  {3,3,0,0,1,1,2,2};   // ph5X90
static int table11[8] =  {1,1,2,2,3,3,0,0};   // ph6X90
static int table12[8] =  {0,0,1,1,2,2,3,3};   // phXecho
static int table13[64] = {0,2,1,3,2,0,3,1,
                          0,2,1,3,2,0,3,1,
                          0,2,1,3,2,0,3,1,
                          0,2,1,3,2,0,3,1,
                          2,0,3,1,0,2,1,3,
                          2,0,3,1,0,2,1,3,
                          2,0,3,1,0,2,1,3,
                          2,0,3,1,0,2,1,3};   // phRec

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define ph1X90 t4
#define ph2X90 t5
#define ph1Xcdx t6
#define ph3X90 t7
#define ph2Xcdx t8
#define ph4X90 t9
#define ph5X90 t10
#define ph6X90 t11
#define phXecho t12
#define phRec t13

#define tXinczf t14

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values
   
   CP hx = getcp("HX",0.0,0.0,0,1);
   strncpy(hx.fr,"dec",3);
   strncpy(hx.to,"obs",3);
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   DSEQ dec = getdseq("H"); 
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHtppm='dec'\n"); 
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHspinal='dec'\n");

   DSEQ mix = getdseq("Hmix"); 
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHmixtppm='dec'\n"); 
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHmixspinal='dec'\n");
    
   double srate = getval("srate");
   if(srate < 500.0) {
      printf("Error: Spin Rate less than 500 Hz, Abort"); 
      psg_abort(1); 
   }
   double taur = 1.0/srate;                   // Define CODEX Timing
   double t1Xcdx = taur/4.0 - getval("pwX90");
   double t2Xcdx = taur/4.0;
   double t3Xcdx = taur/4.0 - getval("pwXcdx");
   double t4Xcdx = taur/4.0 - 1.5*getval("pwX90");  

   double nXcdx = ((double) ((int) (getval("nXcdx") + 0.5)));
   initval(2.0*(nXcdx - 1.0),v1); 

   double tXmix = ((double) taur*((int) (getval("tXmix")/taur + 0.5)));
   tXmix = tXmix - 0.25*taur;    // Shorten tXmix so xgate() will 
   if (tXmix < 0.0) tXmix = 0.0; // catch the end of the period.

// Variable Delay Table 

   int delaytable[4];
   int i;
   for (i = 0; i < 4; i++) {
      double temp = taur*(i+1)/(4.0*12.5e-9);
      delaytable[i] = (int) roundoff(temp,1.0); 
   }
   settable(tXinczf,4,delaytable);

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");
   
// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90") + getval("tHX") + getval("pwX90") + 4.0*getval("pwX90") + 
              4.0*nXcdx*getval("pwXcdx") + getval("pwX90") + getval("pwXecho");
   d.dutyoff = d1 + 4.0e-6 + getval("t1ZF") + tXmix + getval("t2ZF");
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("t1Xecho") + getval("t2Xecho") + getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("t1Xecho") + getval("t2Xecho") + getval("rd") + getval("ad") + at;
   d.c3 = d.c3 + (!strcmp(mix.seq,"tppm"));
   d.c3 = d.c3 + ((!strcmp(mix.seq,"tppm")) && (mix.t.a > 0.0));
   d.t3 = 2.0*t1Xcdx + 4.0*t2Xcdx + 4.0*nXcdx*(t2Xcdx + t3Xcdx) + t4Xcdx;
   d.c4 = d.c4 + (!strcmp(mix.seq,"spinal"));
   d.c4 = d.c4 + ((!strcmp(mix.seq,"spinal")) && (mix.s.a > 0.0));
   d.t4 = 2.0*t1Xcdx + 4.0*t2Xcdx + 4.0*nXcdx*(t2Xcdx + t3Xcdx) + t4Xcdx;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phH90,4,table1);
   settable(phXhx,8,table2);
   settable(phHhx,4,table3);
   settable(ph1X90,16,table4);
   settable(ph2X90,8,table5);
   settable(ph1Xcdx,8,table6);
   settable(ph3X90,64,table7);
   settable(ph2Xcdx,8,table8);
   settable(ph4X90,32,table9);
   settable(ph5X90,8,table10);
   settable(ph6X90,8,table11);
   settable(phXecho,8,table12);
   settable(phRec,64,table13);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXhx); decphase(phH90);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   xgate(1.0);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to X Cross Polarization

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decphase(phHhx);
    _cp_(hx,phHhx,phXhx);

// F1 Period

   _dseqon(mix);
   txphase(ph1X90); 
   obspwrf(getval("aX90")); 
   delay(d2);
   rgpulse(getval("pwX90"),ph1X90,0.0,0.0);
   obsunblank(); 

// Z-filter Period

   _dseqoff(mix); 
   txphase(ph2X90); 
   delay(getval("t1ZF"));

// CODEX Excitation Period 
   
   xgate(1.0); 
   _dseqon(mix); 
   rgpulse(getval("pwX90"),ph2X90,0.0,0.0);
   obsunblank();
   assign(zero,v3);
   getelem(ph1Xcdx,v3,v5);
   incr(v3);
   txphase(v5);
   obspwrf(getval("aXcdx")); 
   delay(t1Xcdx);
   delay(t2Xcdx);
   rgpulse(getval("pwXcdx"),v5,0.0,0.0);
   obsunblank();
   delay(t2Xcdx);
   loop(v1,v2);
      getelem(ph1Xcdx,v3,v5);
      incr(v3);
      txphase(v5);  
      delay(t3Xcdx);
      rgpulse(getval("pwXcdx"),v5,0.0,0.0);
      obsunblank();
      delay(t2Xcdx);
   endloop(v2);
   txphase(ph3X90); 
   obspwrf(getval("aX90")); 
   delay(t4Xcdx);
   rgpulse(getval("pwX90"),ph3X90,0.0,0.0);
   obsunblank(); 

// Mixing Period for Exchange

   _dseqoff(mix);
   txphase(ph4X90);
   delay(tXmix);

// CODEX Reconversion Period

//   if (tXmix > 0.0) xgate(1.0);
   _dseqon(mix);
   rgpulse(getval("pwX90"),ph4X90,0.0,0.0);
   obsunblank();
   obspwrf(getval("aXcdx")); 
   delay(t4Xcdx);
   assign(zero,v4); 
   loop(v1,v2);
      getelem(ph2Xcdx,v4,v5);
      incr(v4); 
      txphase(v5);
      delay(t2Xcdx);
      rgpulse(getval("pwXcdx"),v5,0.0,0.0);
      obsunblank();
      delay(t3Xcdx);
   endloop(v2);
   getelem(ph2Xcdx,v4,v5);
   incr(v4); 
   txphase(v5);
   delay(t2Xcdx);
   rgpulse(getval("pwXcdx"),v5,0.0,0.0);
   obsunblank();
   txphase(ph5X90);
   obspwrf(getval("aX90"));
   delay(t2Xcdx);
   delay(t1Xcdx);
   rgpulse(getval("pwX90"),ph5X90,0.0,0.0);
   obsunblank();

// Variable Z-filter Period to provide a Gamma Integral

   _dseqoff(mix);
   txphase(ph6X90);
   getelem(tXinczf,ct,v14);
   delay(getval("t2ZF"));
   vdelay(NSEC,v14);  

// Hahn Echo for Detection

   _dseqon(dec);
   rgpulse(getval("pwX90"),ph6X90,0.0,0.0);
   obsunblank(); 
   txphase(phXecho);
   obspwrf(getval("aXecho"));
   delay(getval("t1Xecho"));
   rgpulse(getval("pwXecho"),phXecho,0.0,0.0);
   obsunblank();
   delay(getval("t2Xecho"));

// Begin Acquisition

   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

