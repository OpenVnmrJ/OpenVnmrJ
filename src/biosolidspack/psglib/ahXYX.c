/* ahXYX.c                  --Agilent BioSolids--

              3D hXYX (hCNC),first with HX CP followed by X (d2) evolution, specific
              XY CP followed by Y (d3) evolution, a choice of mixing sequences, 
              acquisition and a constant-time decoupling period. Choose
              2-angle SPINAL or TPPM decoupling during d2, d3 and acquisition.

              Select standard or band-selective (d2) X detection. The Y (d3)
              detection is standard. For standard detection a composite 180 is 
              used on the opposite low-band channel for hetero-J refocusing. 
              Constant-time acquisition uses a simultaneous hard 180 on the 
              opposite channel.
 
              Select constant-time decoupling to compensate d2 and d3, using a CW 
              pulse on H following F3 acquisition.

              The selection of DARR mixing (also used as a Z-filter) is fixed. 

              HX, XY and YX CP are executed with standard CP modules, allowing constant, 
              linear and tangent CP. Decoupling of amplitude aHyx is executed during
              YX CP and aHxy decoupling during XY CP. . 

              FLAGS: 

              There is no constant-time Y (d3) option (ctN) in this sequence. 

              Set softpul = 'y' to choose band-selective X (d2). Set standard 
              evolution with (softpul = 'n'). For band-selective acquisition 
              an X shaped, refocusing pulse (stXshp1 = 1) pulse with width pwXshp1
              and offset ofXshp1 is calculated by Pbox. The pulse is enclosed in 
              two delays of taur. Each taur delay contains a 5 us delay to set the
              scaler to dbXshp1 and back to tpwr. A 180 simpulse on X and Y follows 
              the shaped pulse and both pulses are enclosed in delays d3/2.0. 

              For softpul = 'n' the X (d3) evolution and Y (d2) evolution is a simple
              delay d3 or d2 with a centered composite 90-180-90 pulse on the
              lowband channel not acquired.  The composite pulse is not executed 
              if d2 or d3 is less than the composite width. 

              The program calculates the value of the d2 or d3 evolution times as d2_
              or _d3.These calculations use the value of d2_init or d3_init, the values
              entered as "d2" or "d3". The values of d2_ and d3_ are returned as 
              message parameters d2acqret and d3acqret. Also the d2 dwell, 1.0/sw1 
              and the d3 dwell 1.0/sw2 are returned as d2dwret.

              Set ctd = 'y' to choose constant-time decoupling over no constant-time 
              decoupling (ctd = 'n'). The parameter tRFmax inputs a proposed maximum 
              decoupling period tRF after acquisition. The values of d2 and d3 are 
              subtracted from this value. The sequence sets tRFmax to the sum of the
              F1 and F2 acquisition times if tRFmax is less than this value. The putCmd 
              statement is used to reset the parameter tRFmax.If ctd ='n' the tRF 
              decoupling is not executed. The putcmd statement is used to set tRFmax=0.0.

              Set ddec2 = 'y' to choose Y decoupling during F3 or no Y decoupling
              during F3. The Y channel is blanked if no decoupling is used and if 
              ampmode for the channel is 'p'. 

              The value mMix = 'darr' is set by the program to force rotor-
              synchronized DARR mixing (based on 1.0/srate). The desired DARR mxing
              time is set from tXmix. The putCmd statement is used to reset the
              parameter tXmix. The DARR amplitude is aHmix. DARR mixing is 
              functionally equivalent to a pair of Z-filter periods that are 
              usually applied around recoupled mixing sequences such as C7. 

              This sequence was derived from hXYX (AJN 121509)
              provided by C. Rienstra, UIUC.                                       */

#include "standard.h"
#include "solidstandard.h"
#include "pboxpulse.h"  

// Define Static Values to Hold d2 and d3 First Increments

static double d2_init;
static double d3_init;

// Define Values for Phasetables

static int table1[8]  = {0,0,0,0,2,2,2,2};    // phH90
static int table21[8] = {0,0,0,0,0,0,0,0};    // phH90_soft
static int table2[8]  = {3,3,3,3,3,3,3,3};    // phHhx
static int table4[8]  = {0,0,0,0,0,0,0,0};    // phXhx
static int table6[8]  = {0,2,0,2,0,2,0,2};    // phXxy
static int table17[8] = {0,0,0,0,1,1,1,1};    // phXsoft 
static int table18[8] = {0,1,0,1,2,3,2,3};    // phXhard
static int table7[8]  = {0,0,0,0,0,0,0,0};    // phYxy
static int table9[8]  = {0,0,2,2,0,0,2,2};    // phYyx
static int table10[8] = {1,3,1,3,1,3,1,3};    // phXyx
static int table22[8] = {1,1,1,1,1,1,1,1};    // phXyx_soft
static int table11[8] = {0,2,0,2,0,2,0,2};    // ph1Xmix
static int table12[8] = {0,1,0,1,2,3,2,3};    // ph2Xmix
static int table19[8] = {0,0,0,0,0,0,0,0};    // phYhard
static int table8[8]  = {3,3,3,3,3,3,3,3};    // phHyx
static int table5[8]  = {1,1,1,1,1,1,1,1};    // phHxy
static int table20[8] = {3,2,1,0,3,2,1,0};    // phRec
static int table13[4] = {0,0,0,0};            // ph1Xcomp
static int table14[4] = {1,1,1,1};            // ph2Xcomp
static int table15[4] = {0,0,0,0};            // ph1Ycomp
static int table16[4] = {1,1,1,1};            // ph2Ycomp

#define phH90 t1
#define phHhx t2
#define phXhx t4
#define phHxy t5
#define phXxy t6
#define phYxy t7
#define phHyx t8
#define phYyx t9
#define phXyx t10
#define ph1Xmix t11
#define ph2Xmix t12
#define ph1Xcomp t13
#define ph2Xcomp t14
#define ph1Ycomp t15
#define ph2Ycomp t16
#define phXsoft t17
#define phXhard t18
#define phYhard t19
#define phH90_soft t21
#define phXyx_soft t22
#define phRec t20

void pulsesequence() {

// Define Variables and Modules and Get Parameter Values

   CP hx = getcp("HX",0.0,0.0,0,1);
   strcpy(hx.fr,"dec");
   strcpy(hx.to,"obs");
   putCmd("frHX='dec'\n"); 
   putCmd("toHX='obs'\n");

   CP xy = getcp("XY",0.0,0.0,0,1);
   strcpy(xy.fr,"obs");
   strcpy(xy.to,"dec2");
   putCmd("frXY='obs'\n");
   putCmd("toXY='dec2'\n");

   CP yx = getcp("YX",0.0,0.0,0,1);
   strcpy(yx.fr,"dec2");
   strcpy(yx.to,"obs");
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

   strncpy(mMix,"darr",5);
   putCmd("mMix='darr'\n");

// Get pwX90 and pwY90 to Adjust For Composite and Simpulses

   double pwX90 = getval("pwX90");
   double pwY90 = getval("pwY90");
   double pwsim = 2.0*pwY90;
   if (pwX90 > pwY90) pwsim = 2.0*pwX90;

// Determine taur, One Rotor Cycle

   double srate = getval("srate");
   double taur = 0.0;
   if (srate >= 500.0)
      taur = roundoff((1.0/srate), 0.125e-6);
   else {
      printf("ABORT: Spin Rate (srate) must be greater than 500\n");
      psg_abort(1);
   }

// Set Mixing Period to N Rotor Cycles

   double tXmix = getval("tXmix");
   tXmix = roundoff(tXmix,taur);
   putCmd("tXmix = %f\n",tXmix*1.0e6);

// Create Soft Pulse if Needed

   char softpul[MAXSTR];
   getstr("softpul",softpul);

   PBOXPULSE shp1;

   if (!strcmp(softpul,"y")) {
      shp1 = getpboxpulse("shp1X",0,1); 
      shp1.st = 1.0;
      shp1.ph = 0.0;
      putCmd("stXshp1=0.5\n");
      putCmd("phXshp1=0.0\n");
   }

//Set shp1.t1 and shp1.t2 explicitily

   shp1.t1 = taur - 5.0e-6;
   shp1.t2 = taur - 5.0e-6;

// Calculate the F1 and F2 Acquisition Times 

   if (d2_index == 0) d2_init = getval("d2");
   if (d3_index == 0) d3_init = getval("d3");

   double d2_ = (ni - 1)/sw1 + d2_init;
   putCmd("d2acqret = %f\n",roundoff(d2_,12.5e-9));
   putCmd("d2dwret = %f\n",roundoff(1.0/sw1,12.5e-9));
   double d3_ = (getval("ni2") - 1)/sw2 + d3_init;
   putCmd("d3acqret = %f\n",roundoff(d3_,12.5e-9));
   putCmd("d3dwret = %f\n",roundoff(1.0/sw2,12.5e-9));
      
// Calculate Constant-time Decoupling Period tRF

   char ctd[MAXSTR];
   getstr("ctd",ctd);

   double tRF = 0.0;
   double tRFmax = getval("tRFmax");
   if (!strcmp(ctd,"y")) {
      if (tRFmax <= (d2_ + d3_)) tRFmax = d2_ + d3_;
      tRF = tRFmax - d2 - d3; 
      putCmd("tRFmax = %f\n",tRFmax*1.0e6); 
   } 

// Set tRFmax = 0.0 for No Constant-time Decoupling

   else { 
      putCmd("tRFmax = 0.0\n");
   }

// Copy Current Parameters to Processed

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   double duty = 4.0e-6 + getval("pwH90") + getval("tHX") + getval("tYX") + 
             getval("tXY") + 2.0*getval("pwX90") + getval("ad") +
             getval("rd") + at + tRFmax;
             
   if (!strcmp(ctd,"y")) duty = duty + tRFmax; 
   else duty = duty + d2 + d3;

   if (!strcmp(softpul,"y")) 
             duty = duty + pwsim + shp1.t1 + shp1.t2 + getval("pwXshp1");

   if (!strcmp(mMix,"darr")) duty = duty + 2.0*getval("pwX90") + tXmix;

   duty = duty/(duty + d1 + 4.0e-6);

   if (duty > 0.1) {
      printf("Duty cycle %.1f%% >10%%. Abort!\n", duty*100.0);
      psg_abort(1);
  }

// Create Phasetables 

   settable(phH90,8,table1);
   settable(phHhx,8,table2);
   settable(phXhx,8,table4);
   settable(phHxy,8,table5);
   settable(phYxy,8,table6);
   settable(phXxy,8,table7);
   settable(phHyx,8,table8);
   settable(phYyx,8,table9);
   settable(phXyx,8,table10);
   settable(ph1Xmix,8,table11);
   settable(ph2Xmix,8,table12);
   settable(ph1Xcomp,4,table13);
   settable(ph2Xcomp,4,table14);
   settable(ph1Ycomp,4,table15);
   settable(ph2Ycomp,4,table16);

   if (!strcmp(softpul, "y")) {
      settable(phXsoft,8,table17);
      settable(phXhard,8,table18);
      settable(phYhard,8,table19);
      settable(phH90_soft,8,table21);
      settable(phXyx_soft,8,table22);
   }
   settable(phRec,8,table20);

// Hypercomplex F1 and F2

   int id2_ = (int) (d2*sw1 + 0.1);
   if ((phase1 == 1) || (phase1 == 2)) {
      tsadd(phYxy,2*id2_,4);
      tsadd(phRec,2*id2_,4);
      if (phase1 == 2) tsadd(phYxy,1,4);
   }

   int id3_ = (int) (d3*sw2 + 0.1);
   if ((phase2 == 1) || (phase2 == 2)) {
      tsadd(phXhx,2*id3_,4);
      tsadd(phRec,2*id3_,4);
      if (phase2 == 2) tsadd(phXhx,1,4);
   }
   setreceiver(phRec);

// Begin Sequence

   txphase(phXhx); dec2phase(phYxy);
   if (!strcmp(softpul,"y")) decphase(phH90_soft);
   if (!strcmp(softpul,"n")) decphase(phH90);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90")); dec2pwrf(getval("aYxy"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to X Cross Polarization 

   if (!strcmp(softpul,"y")) decrgpulse(getval("pwH90"),phH90_soft,0.0,0.0);
   if (!strcmp(softpul,"n")) decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   _cp_(hx,phHhx,phXhx);

// F2 Indirect Period For X

   _dseqon2(dec);
   if (!strcmp(softpul,"y")) {
      txphase(phXsoft); dec2phase(phYhard);
      obspwrf(getval("aXshp1")); dec2pwrf(getval("aY90"));
      delay(d3/2.0);
      _pboxpulse(shp1,phXsoft);
      txphase(phXhard);
      obspwrf(getval("aX90"));
      obsblank();
      obspower(tpwr);
      delay(3.0e-6);
      obsunblank();
      delay(2.0e-6);
      sim3pulse(2.0*pwX90,0.0,2.0*pwY90,phXhard,zero,phYhard,0.0,0.0);
      txphase(phXxy); dec2phase(phYxy); 
      obspwrf(getval("aXxy")); dec2pwrf(getval("aYxy"));
      delay(d3/2.0);
   }
   else {
      if (d3 > 4.0*pwY90) {
         txphase(phXxy); dec2phase(ph1Ycomp); 
         obspwrf(getval("aXxy")); dec2pwrf(getval("aY90"));
         delay(d3/2.0 - 2.0*pwY90);
         dec2rgpulse(pwY90,ph1Ycomp,0.0,0.0);
         dec2rgpulse(2.0*pwY90,ph2Ycomp,0.0,0.0);
         dec2rgpulse(pwY90,ph1Ycomp,0.0,0.0);
         dec2phase(phYxy);
         dec2pwrf(getval("aYxy"));
         delay(d3/2.0 - 2.0*pwY90);
      }
      else {
         txphase(phXxy); dec2phase(phYxy);
         obspwrf(getval("aXxy")); dec2pwrf(getval("aYxy"));
         delay(d3);
      }
   }
   _dseqoff2(dec);

// X to Y Cross Polarization

   printf("xy.fr=%s\n",xy.fr);
   printf("xy.fr=%s\n",xy.to);

   decphase(phHxy);
   decpwrf(getval("aHxy"));
   decon();
   _cp_(xy,phXxy,phYxy);
   decphase(phHhx);
   decoff();

// F1 Indirect Period for Y (no constant time)

   _dseqon2(dec);
   if (d2 > 4.0*pwX90) {
      txphase(ph1Xcomp); dec2phase(phYyx);
      obspwrf(getval("aX90")); dec2pwrf(getval("aYyx"));
      delay(d2/2.0 - 2.0*pwX90);
      rgpulse(pwX90,ph1Xcomp,0.0,0.0);
      rgpulse(2.0*pwX90,ph2Xcomp,0.0,0.0);
      rgpulse(pwX90,ph1Xcomp, 0.0,0.0);
      if(!strcmp(softpul,"y")) txphase(phXyx_soft);
      if(!strcmp(softpul,"n")) txphase(phXyx);
      obspwrf(getval("aXyx"));
      delay(d2/2.0 - 2.0*pwX90);
   }
   else {     
      if(!strcmp(softpul,"y")) txphase(phXyx_soft);
      if(!strcmp(softpul,"n")) txphase(phXyx);
      dec2phase(phYyx);
      obspwrf(getval("aXyx")); dec2pwrf(getval("aYyx"));
      delay(d2);
   }
   _dseqoff2(dec);

// Y to X Cross Polarization

   decphase(phHyx);
   decpwrf(getval("aHyx"));
   decon();
   if(!strcmp(softpul,"y")) _cp_(yx,phYyx,phXyx_soft);
   if(!strcmp(softpul,"n")) _cp_(yx,phYyx,phXyx);
   decphase(phHhx);
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
   acquire(np,1.0/sw);
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
