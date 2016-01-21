/*ahXX.c - A sequence to provide XX homonuclear correlation with the option of using 
           DARR, PARIS, RFDR, SPC5, POSTC7 and POSTC6 mixing.

              2D hXX (hCC), with HX CP followed by X (d2) evolution, a choice of mixing
              sequences, X acquisition and a constant-time decoupling period. Choose
              2-angle SPINAL or TPPM decoupling during d2 evolution and acquisition.   

              Select one of 6 XX mixing choices, DARR, PARIS, RFDR, C7, SPC5, C6, 
              The RFDR, C6, C7 and SPC5 mixing are enclosed in a rotor-synchronized
              pair of Z-filter periods with a variable decoupler level. The C6, C7
              and SPC5 can be executed as a double quantum filter bysetting the 
              flag dqfX (i.e dqfXspc5 etc). The number of executed elements qX 
              doubles. The mixing time tX is returned as a message parameter with 
              the suffix "ret" (i.e tXspc5ret).

              The C6, C7, SPC5 (and dqf versions) have their own H 
              decoupling levels (identified by "mix", i.e. aHmixspc5). The RFDR uses 
              aHrfdr.  DARR and PARIS are H irradiation sequences using  
              "aHmix".

              HX CP is executed with a standard CP module, allowing constant, 
              linear and tangent CP.   

              The program calculates the value of the d2 evolution time as d2_.  
              This calculation uses the value of d2_init the value entered as "d2". 
              The value of d2_ is returned as a message parameters d2acqret. Also the 
              d2 dwell, 1.0/sw1, is returned as d2dwret.

              The d2 evolution is standard STATES TPPI with no 15N refocusing. The 
              TPPI index is derived from d2 rather than d2_index. That means that if 
              d2_init (the entered value of d2) is greater than 1 dwell TPPI phases 
              will be shifted. The value of d2_init is a static variable set on the 
              first increment to retain the value of the initial d2 for all increments. 

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
              1.0/srate),'c6' to choose C6 mixing, 'c7' to choose C7 mixing, 'spc5' 
              to choose SPC5 mixing, 'darr' to choose DARR mixing or 'paris' to choose 
              PARIS mixing. 

              Set the number of periods of C6 with qXc6, C7 with qXc7 or SPC5 with 
              qXspc5. Choose double-quantum filtering with dqfXc6 = 'y', dqfXc7 = 'y' or 
              dqfXspc5 = 'y'. Double quantum filtering doubles the number of elements
              and the mixing time for a given value of qX. The mixing time of C6 is 
              returned as a message parameter tXc6ret, C7 is tXc7ret and SPC5 is tXspc5ret. 

              The desired DARR or PARIS mxing time is set from tXmix and the actual 
              DARR or PARIS mixing time is returned as the message parammeter tXmixret. 
              The DARR amplitude is aHmix. The PARIS amplitude is aHmparis. 

              The SPINAL for both DEC and DEC2 is executed as 2-angle SPINAL using the phases
              alp and ph. alp = ph/2.0 reproduces 1-angle SPINAL and alp = 0 reproduces TPPM. 

          J. Rapp 09/17/09, CMR test 11/4/09, LJS & MT 8/18/10 */
                                   
#include "standard.h"
#include "solidstandard.h"

// Define Static Value to Hold d2 of First Increment

static double d2_init;

// Define Values for Phasetables

static int table1[8] = {1,1,3,3,1,1,3,3};           // phH90
static int table2[8] = {0,0,0,0,0,0,0,0};           // phHhx
static int table3[8] = {0,0,0,0,2,2,2,2};           // phXhx
static int table4[8] = {3,3,3,3,3,3,3,3};           // ph1Xmix
static int table5[8] = {1,2,1,2,1,2,1,2};           // ph2Xmix;
static int table6[8] = {0,1,2,3,2,3,0,1};           // phRec;
static int table7[8] = {0,0,0,0,0,0,0,0};	    // phXrcpl
static int table8[8] = {0,1,0,1,0,1,0,1};	    // phXrcplref
static int table9[8] = {3,2,3,2,3,2,3,2};           // phXmixdqf;

#define phH90 t1
#define phHhx t2
#define phXhx t3
#define ph1Xmix t4
#define ph2Xmix t5
#define phRec t6
#define phXrcpl t7
#define phXrcplref t8
#define phXmixdqf t9

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   CP hx = getcp("HX",0.0,0.0,0,1);
   strncpy(hx.fr,"dec",3);
   strncpy(hx.to,"obs",3);
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   DSEQ dec = getdseq2("H"); 
 
// Choose DEC2 Decoupling

   char ddec2[MAXSTR];
   getstr("ddec2",ddec2);

   DSEQ dec2;
   if (!strcmp(ddec2,"y")) dec2 = getdseq2("Y");

 // Choose the Mixing Sequence 

   char mMix[MAXSTR];
   getstr("mMix",mMix);

// Define Optional SPC5, C6, C7, RFDR, PARIS and PAR Mixing Sequences

   MPSEQ spc5;
   MPSEQ spc5ref;
   char dqfXspc5[MAXSTR];
   double tXspc5ret = 0.0;

   if (!strcmp(mMix,"spc5")) {
      getstr("dqfXspc5",dqfXspc5);
      spc5 = getspc5("spc5X",0,0.0,0.0,0,1);
      tXspc5ret = spc5.t;
      strncpy(spc5.ch,"obs",3);
      if (!strcmp(dqfXspc5,"y")) {
         spc5ref = getspc5("spc5X",spc5.iSuper,spc5.phAccum,spc5.phInt,1,1); 
         strncpy(spc5ref.ch,"obs",3);
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
      strncpy(c7.ch,"obs",3);
      if (!strcmp(dqfXc7,"y")) {
         c7ref = getpostc7("c7X",c7.iSuper,c7.phAccum,c7.phInt,1,1);
         strncpy(c7ref.ch,"obs",3);
         tXc7ret = tXc7ret + c7.t;
      }
      putCmd("chXc7='obs'\n");
      putCmd("tXc7ret = %f\n",tXc7ret*1.0e6);
   }

   MPSEQ c6;
   MPSEQ c6ref;
   char dqfXc6[MAXSTR];
   double tXc6ret = 0.0;

   if (!strcmp(mMix,"c6")) {
      getstr("dqfXc6",dqfXc6);
      c6 = getpostc6("c6X",0,0.0,0.0,0,1);
      tXc6ret = c6.t;
      strncpy(c6.ch,"obs",3);
      if (!strcmp(dqfXc6,"y")) {
         c6ref = getpostc6("c6X",c6.iSuper,c6.phAccum,c6.phInt,1,1);
         strncpy(c6ref.ch,"obs",3);
         tXc6ret = tXc6ret + c6.t;
      }
      putCmd("chXc6='obs'\n");
      putCmd("tXc6ret = %f\n",tXc6ret*1.0e6);
   }

   MPSEQ rfdr;
   double tXrfdrret = 0.0;

   if (!strcmp(mMix,"rfdr")) {
      rfdr = getrfdrxy8("rfdrX",0,0.0,0.0,0,1);
      tXrfdrret = rfdr.t;
      strncpy(rfdr.ch,"obs",3);
      putCmd("chXrfdr='obs'\n");
      putCmd("tXrfdrret = %f\n",tXrfdrret*1.0e6);
   }

   PARIS paris;

   if (!strcmp(mMix,"paris")) {
      paris = getparis("Hm");
   }

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

 // Calculate the F1 Acquisition Time 

   if (d2_index == 0) d2_init = getval("d2");
   double d2_ = (ni - 1)/sw1 + d2_init;
   putCmd("d2acqret = %f\n",roundoff(d2_,12.5e-9));
   putCmd("d2dwret = %f\n",roundoff(1.0/sw1,12.5e-9));
      
// Calculate Constant-time Decoupling Period tRF

   char ctd[MAXSTR];
   getstr("ctd",ctd);

   double tRF = 0.0;
   double tRFmax = getval("tRFmax");
   if (!strcmp(ctd,"y")) {
      if (tRFmax <= d2_) tRFmax = d2_;
      tRF = tRFmax - d2; 
      putCmd("tRFmax = %f\n",tRFmax*1.0e6);  
   }

// Set tRFmax = 0.0 for No Constant-time Decoupling

   else { 
      putCmd("tRFmax = 0.0\n");   
   }

// Copy Current Parameters to Processed

   putCmd("groupcopy('current','processed','acquisition')");
       
// Dutycycle Protection

   double duty = 4.0e-6 + getval("pwH90") + getval("tHX") + 
                 getval("ad") + getval("rd") + at;

   if (!strcmp(ctd,"y")) duty = duty + tRFmax; 
   else duty = duty + d2;

   if (!strcmp(mMix,"darr")) duty = duty + 2.0*getval("pwX90") + tXmix;
   else if (!strcmp(mMix,"c7")) duty = duty + 2.0*getval("pwX90") + tXc7ret;
   else if (!strcmp(mMix,"spc5")) duty = duty + 2.0*getval("pwX90") + tXspc5ret;
   else if (!strcmp(mMix,"c6")) duty = duty + 2.0*getval("pwX90") + tXc6ret;
   else if (!strcmp(mMix,"rfdr")) duty = duty + 2.0*getval("pwX90") + tXrfdrret;
   else if (!strcmp(mMix,"paris")) duty = duty + 2.0*getval("pwX90") + tXmix;
   else {
      printf("ABORT: Mixing Sequence Not Found ");
      psg_abort(1);
   }
   duty = duty/(duty + d1 + 4.0e-6);
   if (duty > 0.1) {
      printf("Duty cycle %.1f%% >10%%. Abort!\n", duty*100.0);
      psg_abort(1);
   }

// Create Phasetables

   settable(phH90,8,table1);
   settable(phHhx,8,table2);
   settable(phXhx,8,table3);
   settable(ph1Xmix,8,table4);
   settable(ph2Xmix,8,table5);
   settable(phRec,8,table6);
   settable(phXrcpl,8,table7);
   settable(phXrcplref,8,table8);
   settable(phXmixdqf,8,table9);

   int id2_ = (int) (d2 * getval("sw1") + 0.1);
   if ((phase1 == 1) || (phase1 == 2)) {
      tsadd(phRec,2*id2_,4); 
      tsadd(ph1Xmix,2*id2_,4); 
   }  
   if (phase1 == 2) tsadd(ph1Xmix,3,4);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXhx); decphase(phH90); if (NUMch > 2) dec2phase(zero); 
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to X Cross Polarization

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
    _cp_(hx,phHhx,phXhx);

// F2 Indirect Period for X

   obspwrf(getval("aX90"));
   _dseqon2(dec);
   delay(d2);
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

// Optional Mixing with C6 Recoupling

   if (!strcmp(mMix, "c6")) {
      obspwrf(getval("aX90")); decpwrf(getval("aH90"));
      decon();
      rgpulse(getval("pwX90"),ph1Xmix,0.0,0.0);
      xmtrphase(v1); txphase(phXrcpl);
      obspwrf(getval("aXc6")); decpwrf(getval("aHZF"));
      obsunblank();
      delay(getval("tZF"));
      decpwrf(getval("aHmixc6"));
      _mpseq(c6,phXrcpl);
      if (!strcmp(dqfXc6,"y")) {
        _mpseq(c6ref,phXrcplref);
      }
      xmtrphase(zero);
      if (!strcmp(dqfXc6,"y")) txphase(phXmixdqf);
      else txphase(ph2Xmix);
      obspwrf(getval("aX90")); decpwrf(getval("aHZF"));
      delay(getval("tZF"));
      decpwrf(getval("aH90"));
      if (!strcmp(dqfXc6,"y")) rgpulse(getval("pwX90"),phXmixdqf,0.0,0.0);
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

// Optional PARIS Mixing

   if (!strcmp(mMix, "paris")) {
      obspwrf(getval("aX90")); decpwrf(getval("aH90"));
      decon();
      rgpulse(getval("pwX90"),ph1Xmix,0.0,0.0);
      txphase(ph2Xmix);
      obsunblank();
      decoff();
      _paris(paris);
      delay(tXmix);
      decprgoff();
      decon();
      decpwrf(getval("aH90"));
      rgpulse(getval("pwX90"),ph2Xmix,0.0,0.0);
      decoff();
   }

// Optional RFDR Mixing 

   if (!strcmp(mMix, "rfdr")) {
      obspwrf(getval("aX90")); decpwrf(getval("aH90"));
      decon();
      rgpulse(getval("pwX90"),ph1Xmix,0.0,0.0);
      txphase(zero);
      decpwrf(getval("aHZF"));
      obsunblank();
      delay(getval("tZF"));
      decpwrf(getval("aHrfdr"));
      _mpseq(rfdr,zero);
      txphase(ph2Xmix); 
      obspwrf(getval("aX90")); decpwrf(getval("aHZF"));
      delay(getval("tZF"));
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

