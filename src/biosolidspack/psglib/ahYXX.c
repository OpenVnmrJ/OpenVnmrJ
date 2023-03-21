/* ahYXX.c                  --Agilent BioSolids--

              3D hYXX (hNCC),first with HY CP followed by Y (d3) evolution, specific
              YX CP followed by X (d2) evolution, a choice of mixing sequences, 
              F3 acquisition and a constant-time decoupling period. Choose
              2-angle SPINAL or TPPM decoupling during d2, d3 and acquisition.

              Select standard or band-selective X (d2) detection. Select standard 
              or rotor-synchronized,constant-time Y (d3) detection. For standard
              detection a composite 180 is used on the opposite low-band channel for
              hetero-J refocusing. Constant-time and band-selective acquisition use a
              simultaneous hard 180 on the opposite channel. 

              The d2 evolution uses STATES TPPI. The TPPI index is derived from 
              d2 rather than d2_index.  

              Select constant-time decoupling to compensate d2 and d3, using a CW 
              pulse on H following F3 acquisition.

              Select one of 3 XX mixing periods, DARR, C7 and SPC5. The C7 and SPC5 
              mixing is enclosed in a pair of Z-filter periods with a variable 
              decoupler level. 

              HY and YX CP are executed with standard CP modules, allowing constant, 
              linear and tangent CP. Decoupling of amplitude aHyx is executed during
              YX CP. 

              FLAGS: 

              Set ctN = 'y' to choose constant-time Y (d3) evolution. Set standard 
              evolution with (ctN = 'n'). For constant-time the value t3max is designated
              as the constant time and t3max is synchronized with the rotor period 
              taur =  1.0/srate. If t3max will not fit the d3 evolution time
              (including the initial d3) its value will be increased to do so. 
              A putCmd statement is used to reset the parameter t3max.  

              For ctN = 'n' the Y d3 evolution is a simple delay d2 or d3 with a 
              centered composite 90-180-90 pulse on the lowband channel not acquired.
              The composite pulse is not executed if d2 is less than the composite 
              width. 

              Set softpul = 'y' to choose band-selective X (d2). Set standard 
              evolution with(softpul = 'n'). For band-selective acquisition 
              an X shaped, refocusing pulse (stXshp1 = 1) pulse with width pwXshp1
              and offset ofXshp1 is calculated by Pbox. The pulse is enclosed in 
              two delays of taur. Each taur dealy contains a 5 us delay to set the
              scaler to dbXshp1 and back to tpwr. A 180 simpulse on X and Y follows 
              the shaped pulse and both pulses are enclosed in delays d3/2.0. 

              For ctN = 'n' and/or softpul = 'n' the X (d2) or Y (d3) evolution 
              is a simple delay d2 or d3 with a centered composite 90-180-90 pulse
              on the lowband channel not acquired. The composite pulse is not 
              executed if d2 or d3 is less than the composite width. 

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
              statement is used to reset the parameter tRFmax. If ctd ='n' the tRF 
              decoupling is not executed. The putCmd statement is used to set tRFmax=0.0.            

              Set ddec2 = 'y' to choose Y decoupling during F3 or no Y decoupling
              during F3. The Y channel is blanked if no decoupling is used and if 
              ampmode for the channel is 'p'. 

              Set mMix = 'darr' to choose, rotor-synchronized DARR mixing (based on 
              1.0/srate), 'c7' to choose C7 mixing or 'spc5' to choose SPC5 mixing. 
              The desired DARR mxing time is set from tXmix. The putCmd statement 
              is used to reset the parameter tXmix. The DARR amplitude is aHmix. 

              Set the number of periods of C7 with qXc7 or SPC5 with qXspc5. Choose 
              double-quantum filtering with dqfXc7 = 'y' or dqfXspc5 = 'y'. Double 
              quantum filtering doubles the number of elements and the mixing time 
              for a given value of qX. The mixing time of C7 is returned as a message
              parameter tXc7ret and SPC5 is tXspc5ret. 

              This sequence was derived from hYXX (AJN 052511)
              provided by C. Rienstra, UIUC.                        
*/

#include "standard.h"
#include "solidstandard.h"
#include "pboxpulse.h"  

// Define Static Value to Hold d2 of First Increment

static double d2_init;
static double d3_init;

// Define Values for Phasetables

static int table1[8]  = {0,0,0,0,0,0,0,0};            // phH90
static int table2[8]  = {3,3,3,3,3,3,3,3};            // phHhy
static int table4[8]  = {0,0,0,0,0,0,0,0};            // phYhy
static int table5[8]  = {0,0,0,0,0,0,0,0};            // phHyx
static int table6[8]  = {0,2,0,2,0,2,0,2};            // phYyx
static int table7[8]  = {0,0,2,2,0,0,2,2};            // phXyx
static int table8[8]  = {3,3,1,1,3,3,1,1};            // ph1Xmix
static int table9[8]  = {0,0,1,1,2,2,3,3};            // ph2Xmix
static int table10[8] = {0,2,1,3,2,0,3,1};            // phRec
static int table11[8] = {0,0,0,0,2,2,2,2};  	      // phYyx_soft
static int table12[8] = {0,0,0,0,1,1,1,1};            // phXyx_soft
static int table15[8] = {0,1,0,1,3,2,3,2};            // phXsoft 
static int table16[8] = {0,0,1,1,3,3,2,2};            // phXhard 
static int table17[8] = {0,0,0,0,0,0,0,0};            // phYhard 
static int table18[8] = {1,1,1,1,0,0,0,0};            // ph1Xmix_soft
static int table19[8] = {0,0,2,2,1,1,3,3};            // ph2Xmix_soft
static int table20[8] = {2,0,2,0,3,1,3,1};            // phRec_soft 
static int table21[8] = {0,0,0,0,2,2,2,2};	      // phXrcpl
static int table22[8] = {0,0,1,1,2,2,3,3};	      // phXrcplref
static int table23[8] = {2,2,1,1,0,0,3,3};            // phXmixdqf;
static int table24[8] = {0,0,0,0,0,0,0,0};            // phY180
static int table31[4] = {0,0,0,0};                    // ph1Xcomp
static int table32[4] = {1,1,1,1};                    // ph1Xcomp
static int table33[4] = {0,0,0,0};                    // ph1Ycomp
static int table34[4] = {1,1,1,1};                    // ph2Ycomp

#define phH90 t1
#define phHhy t2
#define phYhy t4
#define phHyx t5
#define phYyx t6
#define phXyx t7
#define ph1Xmix t8
#define ph2Xmix t9
#define phRec t10
#define phYyx_soft t11
#define phXyx_soft t12
#define phXsoft t15
#define phXhard t16
#define phYhard t17
#define ph1Xmix_soft t18
#define ph2Xmix_soft t19
#define phRec_soft t20
#define phXrcpl t21
#define phXrcplref t22
#define phXmixdqf t23
#define phY180 t24
#define ph1Xcomp t31
#define ph2Xcomp t32
#define ph1Ycomp t33
#define ph2Ycomp t34

void pulsesequence() {
  
// Define Variables and Modules and Get Parameter Values
     
   CP hy = getcp("HY",0.0,0.0,0,1);
   OSTRCPY( hy.fr, sizeof(hy.fr), "dec");
   OSTRCPY( hy.to, sizeof(hy.to), "dec2");
   putCmd("frHY='dec'\n");
   putCmd("toHY='dec2'\n");
   
   CP yx = getcp("YX",0.0,0.0,0,1);
   OSTRCPY( yx.fr, sizeof(yx.fr), "dec2");
   OSTRCPY( yx.to, sizeof(yx.to), "obs");
   putCmd("frYX='dec2'\n");
   putCmd("toYX='obs'\n");

   DSEQ dec = getdseq2("H");

// Choose DEC2 Decoupling

   char ddec2[MAXSTR];
   getstr("ddec2",ddec2);

   DSEQ dec2;
   if (!strcmp(ddec2,"y")) dec2 = getdseq2("Y");

// Choose the Mixing Sequence 

   char mMix[MAXSTR];
   getstr("mMix",mMix);

// Define Optional SPC5 and C7 Mixing Sequences

   MPSEQ spc5;
   MPSEQ spc5ref;
   char dqfXspc5[MAXSTR];
   double tXspc5ret = 0.0;

   if (!strcmp(mMix,"spc5")) {
      getstr("dqfXspc5",dqfXspc5);
      spc5 = getspc5("spc5X",0,0.0,0.0,0,1);
      tXspc5ret = spc5.t;
      OSTRCPY( spc5.ch, sizeof(spc5.ch), "obs");
      if (!strcmp(dqfXspc5,"y")) {
         spc5ref = getspc5("spc5X",spc5.iSuper,spc5.phAccum,spc5.phInt,1,1); 
         OSTRCPY( spc5ref.ch, sizeof(spc5ref.ch), "obs");
         tXspc5ret = tXspc5ret + spc5.t;
      }
      putCmd("chXspc5='obs'\n");
      putCmd("tXspc5ret = %f\n",tXspc5ret*1.0e6);
   }

   MPSEQ c7;
   MPSEQ c7ref;
   char dqfXc7[MAXSTR];
   double tXc7ret = 0.0;

   if (!strcmp(mMix,"c7")) {
      getstr("dqfXc7",dqfXc7);
      c7 = getpostc7("c7X",0,0.0,0.0,0,1);
      tXc7ret = c7.t;
      OSTRCPY( c7.ch, sizeof(c7.ch), "obs");
      if (!strcmp(dqfXc7,"y")) {
         c7ref = getpostc7("c7X",c7.iSuper,c7.phAccum,c7.phInt,1,1);
         OSTRCPY( c7ref.ch, sizeof(c7ref.ch), "obs");
         tXc7ret = tXc7ret + c7.t;
      }
      putCmd("chXc7='obs'\n");
      putCmd("tXc7ret = %f\n",tXc7ret*1.0e6);
   }

// Get pwX90 and pwY90 to Adjust For Composite and Simpulses

   double pwX90 = getval("pwX90");
   double pwY90 = getval("pwY90");
   double pwsim = 2.0*pwY90;
   if (pwX90 > pwY90) pwsim = 2.0*pwX90;

// Determine taur, One Rotor Cycle

   double srate =  getval("srate");
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

// N Constant-time Rotor Synchronized Calculation for F2 using taur. 

   char ctN[MAXSTR];
   getstr("ctN",ctN);
   double Ndelay1 = 0.0;
   double Ndelay2 = 0.0;
   double t3max = getval("t3max");
   if (!strcmp(ctN,"y")) {
      if (t3max < (d3_ + pwsim)) t3max = d3_+ pwsim;
      t3max = (double) ((int) ((t3max)/(2.0*taur)) + 1)*2.0*taur;  
      putCmd("t3max = %f\n",t3max*1.0e6);
      Ndelay1 = (t3max - pwsim + d3)/2.0;
      Ndelay2 = (t3max - pwsim - d3)/2.0;
   }

// Set parameter t3max = 0.0 for No Rotor Synchronized Calculation

   else {
      putCmd("t3max = 0.0\n");
   }
      
// Calculate Constant-time Decoupling Period tRF

   char ctd[MAXSTR];
   getstr("ctd",ctd);

   double tRF = 0.0;
   double tRFmax = getval("tRFmax");
   if (!strcmp(ctd,"y")) {
      if (!strcmp(ctN,"y")) {        
         if (tRFmax <= (d2_ + t3max)) tRFmax = d2_ + t3max;
         tRF = tRFmax - d2 - t3max; 
      }
      else {
         if (tRFmax <= (d2_ + d3_)) tRFmax = d2_ + d3_;
         tRF = tRFmax - d2 - d3; 
      }
      putCmd("tRFmax = %f\n",tRFmax*1.0e6); 
   }

// Set tRFmax = 0.0 for No Constant-time Decoupling

   else { 
      putCmd("tRFmax = 0.0\n");
   }

// Copy Current Parameters to Processed

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   double duty = 4.0e-6 + getval("pwH90") + getval("tHY") + getval("tYX")
                 + getval("ad") + getval("rd") + at;

   if (!strcmp(ctd,"y")) duty = duty + tRFmax; 
   else {
      if (!strcmp(ctN,"y")) duty = duty + d2 + t3max; 
      else duty = duty + d2 + d3;
   }
   if (!strcmp(softpul,"y")) duty = duty + getval("phXshp1") + pwsim;

   if (!strcmp(mMix,"darr")) duty = duty + 2.0*getval("pwX90") + tXmix;
   else if (!strcmp(mMix,"c7")) duty = duty + 2.0*getval("pwX90") + tXc7ret;
   else if (!strcmp(mMix,"spc5")) duty = duty + 2.0*getval("pwX90") + tXspc5ret;
   else {
      printf("ABORT: Mixing Sequence Not Found ");
   }
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
   settable(phXrcpl,8,table21);
   settable(phXrcplref,8,table22);
   settable(phXmixdqf,8,table23);
   settable(phY180,8,table24);
   settable(ph1Xcomp,4,table31);
   settable(ph2Xcomp,4,table32);
   settable(ph1Ycomp,4,table33);
   settable(ph2Ycomp,4,table34); 
   
   if (!strcmp(softpul, "y")) {  
      settable(phYyx,8,table11);
      settable(phXyx,8,table12);
      settable(phXsoft,8,table15);
      settable(phXhard,8,table16);
      settable(phYhard,8,table17);
      settable(ph1Xmix,8,table18);
      settable(ph2Xmix,8,table19);
      settable(phRec_soft,8,table20);
   }
   else {
      settable(phYyx,8,table6);
      settable(phXyx,8,table7);
      settable(ph1Xmix,8,table8);
      settable(ph2Xmix,8,table9);
      settable(phRec,8,table10);
   }
 
  int id2_ = (int) (d2 * getval("sw1") + 0.1);

   if ((phase1 == 1) || (phase1 == 2)) { 
      if(!strcmp(softpul, "y")) {
         tsadd(phRec_soft,2*id2_,4);
         tsadd(phXyx,2*id2_,4);
         tsadd(phXsoft,2*id2_,4); 
         tsadd(phXhard,2*id2_,4); 
      }
      else {
         tsadd(phXyx,2*id2_,4);
         tsadd(phRec,2*id2_,4);
      }

      if (phase1 == 2) {
         if (!strcmp(softpul,"y")) {
            tsadd(phXyx,1,4);
            tsadd(phXsoft,1,4);
            tsadd(phXhard,1,4);
         }
         else tsadd(phXyx,1,4);
      }
   } 

   int id3_ = (int) (d3 * getval("sw2") + 0.1);   

   if ((phase2 == 1) || (phase2 == 2)) {
      tsadd(phYhy,2*id3_,4); 
      if (!strcmp(softpul, "y")) {
         tsadd(phRec_soft,2*id3_,4);
      }
      else tsadd(phRec,2*id3_,4);

      if (phase2 == 2) {
         tsadd(phYhy,1,4); 
      }
   }
   
   if (!strcmp(softpul,"y")) setreceiver(phRec_soft);
   else setreceiver(phRec);

// Begin Sequence

   txphase(phXyx); decphase(phH90); dec2phase(phYhy);
   obspwrf(getval("aXyx")); decpwrf(getval("aH90")); dec2pwrf(getval("aYhy"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to Y Cross Polarization

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   _cp_(hy,phHhy,phYhy);

// Constant-time or Standard F2 period for Y

   if (!strcmp(ctN, "y")) {
      txphase(ph1Xcomp); dec2phase(phY180);
      obspwrf(getval("aX90")); dec2pwrf(getval("aY90"));
      _dseqon2(dec);
      delay(Ndelay1);
      _dseqoff2(dec);
      decpwrf(getval("aH90"));
      sim3pulse(2.0*pwX90,2.0*pwY90,2.0*pwY90,ph1Xcomp,zero,phY180,0.0,0.0);
      txphase(phXyx); dec2phase(phYyx);
      obspwrf(getval("aXyx")); dec2pwrf(getval("aYyx"));
      _dseqon2(dec);
      delay(Ndelay2);
      _dseqoff2(dec);
   }
   else {
      _dseqon2(dec);
      if (d3 > 4.0*pwX90) {
         txphase(ph1Xcomp); dec2phase(phYyx);
         obspwrf(getval("aX90")); dec2pwrf(getval("aYyx"));
         delay(d3/2.0 - 2.0*pwX90);
         rgpulse(pwX90,ph1Xcomp,0.0,0.0);
         rgpulse(2.0*pwX90,ph2Xcomp,0.0,0.0);
         rgpulse(pwX90,ph1Xcomp, 0.0,0.0);
         txphase(phXyx);
         obspwrf(getval("aXyx"));
         obsunblank();
         delay(d3/2.0 - 2.0*pwX90);
      }
      else {
         delay(d3);
         dec2phase(phYyx);
         dec2pwrf(getval("aYyx"));
      }
      _dseqoff2(dec);
   }

// Y to X Cross Polarization with H CW Dcoupling

   decphase(phHyx);
   decpwrf(getval("aHyx"));
   decon();
   _cp_(yx,phYyx,phXyx);
   decphase(phHhy);
   decoff();

// F1 Indirect Period for X

   _dseqon2(dec); 
   if (!strcmp(softpul, "y")) {
      txphase(phXsoft); dec2phase(phYhard);
      obspwrf(getval("aXshp1")); dec2pwrf(getval("aY90")); 
      delay(d2/2.0);
      _pboxpulse(shp1,phXsoft);
      txphase(phXhard);
      obspwrf(getval("aX90"));
      obsblank();
      obspower(tpwr);
      delay(3.0e-6);
      obsunblank();
      delay(2.0e-6);
      obspwrf(getval("aX90"));
      dec2pwrf(getval("aY90"));
      sim3pulse(2.0*pwX90,0.0,2.0*pwY90,phXhard,zero,phYhard,0.0,0.0);
      txphase(ph1Xmix);
      obspwrf(getval("aX90"));
      delay(d2/2.0);
   }
   else {
     if (d2 > 4.0*pwY90) {
         txphase(ph1Xmix); dec2phase(ph1Ycomp);
         obspwrf(getval("aX90")); dec2pwrf(getval("aY90"));
         delay(d2/2.0 - 2.0*pwY90);
         dec2rgpulse(pwY90,ph1Ycomp,0.0,0.0);
         dec2rgpulse(2.0*pwY90,ph2Ycomp,0.0,0.0);
         dec2rgpulse(pwY90,ph1Ycomp,0.0,0.0);
         delay(d2/2.0 - 2.0*pwY90);
      }
      else {
         txphase(ph1Xmix);
         obspwrf(getval("aX90"));
         delay(d2);
      }
   }
   _dseqoff2(dec);

// Optional Mixing with SPC5 Recoupling

   if (!strcmp(mMix, "spc5")) {
      obspwrf(getval("aX90")); decpwrf(getval("aH90"));
      decon();
      rgpulse(getval("pwX90"),ph1Xmix,0.0,0.0);
      xmtrphase(v1); txphase(phXrcpl);
      obspwrf(getval("aXspc5")); decpwrf(getval("aHZF"));
      obsunblank();
      delay(getval("tZF"));
      decpwrf(getval("aHmixspc5"));
      _mpseq(spc5,phXrcpl);
      if (!strcmp(dqfXspc5,"y")) {
         xmtrphase(v2);
         _mpseq(spc5ref,phXrcplref);
      }
      xmtrphase(zero);
      if (!strcmp(dqfXspc5,"y")) txphase(phXmixdqf);
      else txphase(ph2Xmix);
      obspwrf(getval("aX90")); decpwrf(getval("aHZF"));
      delay(getval("tZF"));
      decpwrf(getval("aH90"));
      if (!strcmp(dqfXspc5,"y")) rgpulse(getval("pwX90"),phXmixdqf,0.0,0.0);
      else rgpulse(getval("pwX90"),ph2Xmix,0.0,0.0);
      decoff();
   }

// Optional Mixing with C7 Recoupling

   if (!strcmp(mMix,"c7")) {
      obspwrf(getval("aX90")); decpwrf(getval("aH90"));
      decon();
      rgpulse(getval("pwX90"),ph1Xmix,0.0,0.0);
      xmtrphase(v1); txphase(phXrcpl);
      obspwrf(getval("aXc7")); decpwrf(getval("aHZF"));
      obsunblank();
      delay(getval("tZF"));
      decpwrf(getval("aHmixc7"));
      _mpseq(c7,phXrcpl);
      if (!strcmp(dqfXc7,"y")) {
         _mpseq(c7ref,phXrcplref);
      }
      xmtrphase(zero);
      if (!strcmp(dqfXc7,"y")) txphase(phXmixdqf);
      else txphase(ph2Xmix);
      obspwrf(getval("aX90")); decpwrf(getval("aHZF"));
      delay(getval("tZF"));
      decpwrf(getval("aH90"));
      if (!strcmp(dqfXc7,"y")) rgpulse(getval("pwX90"),phXmixdqf,0.0,0.0);
      else rgpulse(getval("pwX90"),ph2Xmix,0.0,0.0);
      decoff();
   }

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

