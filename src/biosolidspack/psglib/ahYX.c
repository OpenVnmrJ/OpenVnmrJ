/* ahYX.c                  --Agilent BioSolids--                                    

              2D hYX (hNC), first with HY CP, a Y (d2) evolution time, then specific
              YX CP, a (DARR) Z-filter and acqusition and a constant-time decoupling
              period. Choose 2-angle SPINAL or TPPM decoupling during d2 evolution and
              acquisition.   

              Select standard or rotor-synchronized,constant-time Y (d2) evolution.
              For standard evolution a composite 180 is used on the opposite low-band 
              channel for hetero-J refocusing. Constant-time and uses a simultaneous 
              hard 180 on the OBS and DEC2 channels. 

              Select constant-time decoupling to comepensate d2, using a CW pulse on H
              following d2 evolution.

              The selection of DARR mixing (also used as a Z-filter) is fixed.

              HY and YX CP are executed with standard CP modules, allowing constant, 
              linear and tangent CP. Decoupling of amplitude aHyx is executed during
              YX CP. 

              FLAGS: 

              Set ctN = 'y' to choose constant-time Y (d2) evolution. Set standard 
              evolution with (ctN = 'n'). For constant-time the value t2max is designated
              as the constant time and t2max is synchronized with the rotor period 
              taur =  1.0/srate. If t2max will not fit the d2 evolution time
              (including the initial d2) its value will be increased to do so. 
              A putCmd statement is used to reset the parameter t2max.  

              For ctN = 'n' the Y d2 evolution is a simple delay d2 or d3 with a 
              centered composite 90-180-90 pulse on the lowband channel not acquired.
              The composite pulse is not executed if d2 is less than the composite 
              width. 

              The program calculates the value of the d2 evolution time as d2_.  
              This calculation uses the value of d2_init the value entered as "d2". 
              The value of d2_ is returned as a message parameters d2acqret. Also the 
              d2 dwell, 1.0/sw1, is returned as d2dwret.

              Set ctd = 'y' to choose constant-time decoupling over no constant-time 
              decoupling (ctd = 'n'). The parameter tRFmax inputs a proposed maximum 
              decoupling period tRF after acquisition. The value of d2 is subtracted  
              from this value. The sequence sets tRFmax to the d2 evolution time if  
              tRFmax is less than this value. The putCmd statement is used to reset 
              the parameter tRFmax. If ctd ='n' the tRF decoupling is not executed.
              The putCmd statement is used to set tRFmax=0.0.

              Set ddec2 = 'y' to choose Y decoupling during F2 with the choice of 
              SPINAL, TPPM, or WALTZ or set no Y decoupling during F2 with ctd='n'. 
              The Y channel is blanked if no decoupling is used and if ampmode for 
              the channel is 'p'. 

              Set mMix = 'darr' to choose, rotor-synchronized DARR mixing (based on 
              1.0/srate). The desired DARR mxing time is set from tXmix. The putCmd 
              statement is used to reset the parameter tXmix. The DARR amplitude is
              aHmix. 

              DARR mixing is functionally equivalent to a pair of Z-filter periods
              that are usually applied around recoupled mixing sequences such as C7.  

              This sequence was derived from hYXX (AJN 052511)
              provided by C. Rienstra, UIUC.                        */

#include "standard.h"
#include "solidstandard.h"  

// Define Static Value to Hold d2 of First Increment

static double d2_init;

// Define Values for Phasetables

static int table1[8]  = {0,0,0,0,0,0,0,0};   // phH90
static int table2[8]  = {3,3,3,3,3,3,3,3};   // phHhy
static int table4[8]  = {0,0,0,0,0,0,0,0};   // phYhy
static int table24[8] = {0,0,0,0,0,0,0,0};   // phY180
static int table5[8]  = {0,0,0,0,0,0,0,0};   // phHyx
static int table6[8]  = {0,2,0,2,0,2,0,2};   // phYyx
static int table7[8]  = {0,0,2,2,0,0,2,2};   // phXyx
static int table8[8]  = {3,3,1,1,3,3,1,1};   // ph1Xmix
static int table9[8]  = {0,0,1,1,2,2,3,3};   // ph2Xmix
static int table10[8] = {0,2,1,3,2,0,3,1};   // phRec
static int table31[4] = {0,0,0,0};           // ph1Xcomp
static int table32[4] = {1,1,1,1};           // ph2Xcomp

#define phH90 t1
#define phHhy t2
#define phYhy t4
#define phHyx t5
#define phYyx t6
#define phXyx t7
#define ph1Xmix t8
#define ph2Xmix t9
#define phY180 t24
#define ph1Xcomp t31
#define ph2Xcomp t32
#define phRec t10

void pulsesequence() {

// Define Variables and Modules and Get Parameter Values

   CP hy = getcp("HY",0.0,0.0,0,1);
   OSTRCPY( hy.fr, sizeof(hy.fr), "dec");
   OSTRCPY( hy.to, sizeof(hy.to), "dec2");
   putCmd("frHY='dec'\n");
   putCmd("toHY='dec2'\n");
    
   CP yx = getcp("YX",0.0,0.0,0,1);
   OSTRCPY( xy.fr, sizeof(xy.fr), "dec2");
   OSTRCPY( xy.to, sizeof(xy.to), "obs");
   putCmd("frYX='dec2'\n");
   putCmd("toYX='obs'\n");

   DSEQ dec = getdseq2("H");

// Choose DEC2 Decoupling

   char ddec2[MAXSTR];
   getstr("ddec2",ddec2);

   DSEQ dec2;
   if (!strcmp(ddec2,"y")) dec2 = getdseq2("Y");

// Set the Mixing Sequence as DARR.

   char mMix[MAXSTR];
   getstr("mMix",mMix);

   OSTRCPY( mMix, sizeof(mMix), "darr");
   putCmd("mMix='darr'\n");

// Determine taur, One Rotor Cycle

   double srate =  getval("srate");
   double taur = 0.0;
   if (srate >= 500.0)
      taur = roundoff((1.0/srate), 0.125e-6);
   else {
      printf("ABORT: Spin Rate (srate) must be greater than 500\n");
      psg_abort(1);
   }

// Get pwX90 and pwY90 to Adjust For Composite and Simpulses

   double pwX90 = getval("pwX90");
   double pwY90 = getval("pwY90");
   double pwsim = 2.0*pwY90;
   if (pwX90 > pwY90) pwsim = 2.0*pwX90;

// Set Mixing Period to N Rotor Cycles

   double tXmix = getval("tXmix");
   tXmix = roundoff(tXmix,taur);
   putCmd("tXmix = %f\n",tXmix*1.0e6);

// Calculate the F1 Acquisition Time 

   if (d2_index == 0) d2_init = getval("d2");
   double d2_ = (ni - 1)/sw1 + d2_init;
   putCmd("d2acqret = %f\n",roundoff(d2_,12.5e-9));
   putCmd("d2dwret = %f\n",roundoff(1.0/sw1,12.5e-9));

// N Constant-time Rotor Synchronized Calculation for F1 using taur. 

   char ctN[MAXSTR];
   getstr("ctN",ctN);
   double Ndelay1 = 0.0;
   double Ndelay2 = 0.0;
   double t2max = getval("t2max");  
   if (!strcmp(ctN,"y")) {
      if (t2max < (d2_ + pwsim)) t2max = d2_+ pwsim;
      t2max = (double) ((int) ((t2max)/(2.0*taur)) + 1)*2.0*taur;
      putCmd("t2max = %f\n",t2max*1.0e6);
      Ndelay1 = (t2max - pwsim + d2)/2.0;
      Ndelay2 = (t2max - pwsim - d2)/2.0;
   }

// Calculate Constant-time Decoupling Period tRF

   char ctd[MAXSTR];
   getstr("ctd",ctd);

   double tRF = 0.0;
   double tRFmax = getval("tRFmax");
   if (!strcmp(ctd,"y")) {
      if (!strcmp(ctN,"y")) {
         if (tRFmax <= t2max) tRFmax = t2max;
         tRF = tRFmax - t2max; 
      }
      else {
         if (tRFmax <= d2_) tRFmax = d2_;
         tRF = tRFmax - d2; 
      }   
      putCmd("tRFmax = %f\n",tRFmax*1.0e6);  
   }

// Set tRFmax = 0.0 for No Constant-time Decoupling

   else { 
      tRFmax = 0.0;
      putCmd("tRFmax = 0.0\n");
   }

// Copy Current Parameters to Processed

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   double duty = 4.0e-6 + getval("pwH90") + getval("tHY") + getval("tYX")
                 + 2.0*getval("pwX90") + getval("ad") + getval("rd") + at;

   if (!strcmp(ctd,"y")) duty = duty + tRFmax; 
   else {
      if (!strcmp(ctN,"y")) duty = duty + t2max; 
      else duty = duty + d2;
   }

   if (!strcmp(mMix,"darr")) duty = duty + 2.0*getval("pwX90") + tXmix;

   duty = duty/(duty + d1 + 4.0e-6);
   if (duty > 0.1) {
      printf("Duty cycle %.1f%% >10%%. Abort!\n", duty*100.0);
      psg_abort(1);
   }

// Create Phasetables

   settable(phH90,8,table1);
   settable(phHhy,8,table2);
   settable(phYhy,8,table4);
   settable(phHyx,8,table5);
   settable(phY180,8,table24);
   settable(ph1Xcomp,4,table31);
   settable(ph2Xcomp,4,table32);
   settable(phRec,8,table10);
   settable(phYyx,8,table6);
   settable(phXyx,8,table7);
   settable(ph1Xmix,8,table8);
   settable(ph2Xmix,8,table9);

// Hypercomplex F1

   int id2_ = (int) (d2*sw1 + 0.1);
   if ((phase1 == 1) || (phase1 == 2)) {
      tsadd(phYhy,2*id2_,4);
      tsadd(phRec,2*id2_,4);
      if (phase1 == 2) {
         tsadd(phYhy,1,4); 
      }
   }
   setreceiver(phRec); 

// Begin Sequence

   txphase(phXyx); decphase(phH90); dec2phase(phYhy);
   obspwrf(getval("aXyx")); decpwrf(getval("aH90")); dec2pwrf(getval("aYhy"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to Y Cross Polarization

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   _cp_(hy,phHhy,phYhy);

// Constant Time or Standard F1 Period For Y

   if (!strcmp(ctN,"y")) {
      txphase(ph1Xcomp); dec2phase(phY180);
      obspwrf(getval("aX90")); dec2pwrf(getval("aY90"));
      _dseqon2(dec);
      delay(Ndelay1);
      _dseqoff2(dec);
      decpwrf(getval("aH90"));
      sim3pulse(2.0*pwX90,0.0,2.0*pwY90,ph1Xcomp,zero,phY180,0.0,0.0);
      txphase(phXyx); dec2phase(phYyx);
      obspwrf(getval("aXyx")); dec2pwrf(getval("aYyx"));
      obsunblank();
      _dseqon2(dec);
      delay(Ndelay2);
      _dseqoff2(dec);
   }
   else {
      _dseqon2(dec);
      if (d2 > 4.0*pwX90) {
         txphase(ph1Xcomp); dec2phase(phYyx);
         obspwrf(getval("aX90")); dec2pwrf(getval("aYyx"));
         delay(d2/2.0 - 2.0*pwX90);
         rgpulse(pwX90,ph1Xcomp,0.0,0.0);
         rgpulse(2.0*pwX90,ph2Xcomp,0.0,0.0);
         rgpulse(pwX90,ph1Xcomp, 0.0,0.0);
         txphase(phXyx);
         obspwrf(getval("aXyx"));
         obsunblank();
         delay(d2/2.0 - 2.0*pwX90);
      }
      else { 
         dec2phase(phYyx);
         dec2pwrf(getval("aYyx"));
         delay(d2); 
      }
      _dseqoff2(dec);
   }

// Y to X Cross Polarization with H CW Decoupling

   decphase(phHyx);
   decpwrf(getval("aHyx"));
   decon();
   _cp_(yx,phYyx,phXyx);
   decphase(phHhy);
   decoff();

// Optional DARR Mixing

   if (!strcmp(mMix,"darr")) {
      obspwrf(getval("aX90")); decpwrf(getval("aH90"));
      decon();
      rgpulse(getval("pwX90"),ph1Xmix,0.0,0.0);
      txphase(ph2Xmix);
      decpwrf(getval("aHmix"));
      obsunblank();
      delay(tXmix);
      decpwrf(getval("aH90"));
      rgpulse(getval("pwX90"),ph2Xmix,0.0,0.0);
      decoff();
   }

// Blank DEC3, Start Optional DEC2 Decoupling or Blank DEC2

   if (!strcmp(ddec2,"y")) {
      if (NUMch > 2) _dseqon2(dec2);
   }
   else {
      if (NUMch > 2) dec2blank();
   }
   if (NUMch > 3) dec3blank();

// Begin Acquisition with DEC decoupling

   _dseqon2(dec);
   obsblank();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();

// Halt DEC Decoupling 

   _dseqoff2(dec);

// Halt Optional DEC2 Decoupling or Unblank DEC2, if Blanked

   if (!strcmp(ddec2,"y")) {
      if (NUMch > 2) _dseqoff2(dec2);
   }
   else {
      if (NUMch > 2) dec2unblank();
   }

// Make DEC Constant-time with RF Following Acquisition

   decphase(zero);
   if (!strcmp(ctd,"y")) {
      decon(); 
      delay(tRF);
      decoff();
   }
   obsunblank(); decunblank(); _unblank34();
}

