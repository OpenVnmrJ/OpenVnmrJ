/* jmasnca2d.c - A J-based NCA 2D experiment using CP to Y followed
                 by refocussed constant time INEPT transfer to X, 
                 with two-level SPINAL or TPPM decoupling during INEPT 
                 and acquisition. 

                 The simultaneous shcaX and shY shaped pulses 
                 selectively invert the 13C-alpha and 15N-amide 
                 regions before transfer and shcaX also 
                 refocuses the C-alpha couplings after transfer.  
                 The shcoX pulse selectively inverts the carbonyl 
                 region to suppress transfer. All shaped pulses 
                 sit with an integral number of rotor periods if 
                 srate is set correctly.
			
	         Use r-snob from Pbox for shca and shco pulses. The 
                 shY pulses can be "square" and hard. The values of 
                 taua and taub should be rotor synchronized. 


          JMAS Ref: Chen et al. 2007 MRC 45, S84-S92.
                  						       */ 
#include "standard.h"
#include "solidstandard.h"
#include "pboxpulse.h"

// Define Values for Phasetables

static int table1[4] = {1,1,1,1};  // phH90
static int table2[4] = {0,0,0,0};  // phHhy
static int table3[4] = {0,2,0,2};  // phYhy
static int table4[4] = {0,0,0,0};  // ph1Xshca
static int table5[4] = {0,0,0,0};  // ph1Ysh
static int table6[4] = {0,0,0,0};  // phXshco
static int table7[4] = {0,0,0,0};  // phX90
static int table8[4] = {0,0,0,0};  // phY90
static int table9[4] = {0,0,0,0};  // ph2Xshca
static int table10[4] = {0,0,0,0}; // ph2Ysh
static int table11[4] = {0,2,0,2}; // phRec

#define phH90 t1
#define phHhy t2
#define phYhy t3
#define ph1Xshca t4
#define ph1Ysh t5
#define phXshco t6
#define phX90 t7
#define phY90 t8
#define ph2Xshca t9
#define ph2Ysh t10
#define phRec t11

static double d2_init;

pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   CP hy = getcp("HY",0.0,0.0,0,1);
   strncpy(hy.fr,"dec",3);
   strncpy(hy.to,"dec2",4);
   putCmd("frHY='dec'\n");
   putCmd("toHY='dec2'\n");

   PBOXPULSE shca = getpboxpulse("shcaX",0,1);
   strncpy(shca.ch,"obs",3);
   putCmd("chXshca ='obs'\n");

   PBOXPULSE sh = getpboxpulse("shY",0,1);
   strncpy(sh.ch,"dec2",4);
   putCmd("chYsh ='dec2'\n");

   PBOXPULSE shco = getpboxpulse("shcoX",0,1);
   strncpy(shco.ch,"obs",3);
   putCmd("chXshco ='obs'\n"); 

   DSEQ dec = getdseq("H");   
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHtppm='dec'\n");
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHspinal='dec'\n");

   DSEQ mix = getdseq("Hmix");
   strncpy(mix.t.ch,"dec",3);
   putCmd("chHmixtppm='dec'\n");
   strncpy(mix.s.ch,"dec",3);
   putCmd("chHmixspinal='dec'\n");

   double pwsim = getval("pwX90"); 
   if (getval("pwY90") > getval("pwX90")) pwsim = getval("pwY90"); 
   pwsim = pwsim/2.0;

   double shcalen = (shca.pw + 2.0*shca.t2)/2.0;
   double shlen = (sh.pw + 2.0*sh.t2)/2.0;
   double shsim = shcalen;
   double simpw = shca.pw;
   double simt1 = shca.t1;
   double simt2 = shca.t2;
   if (shlen > shcalen) { 
      shsim = shlen; 
      simpw = sh.pw;
      simt1 = sh.t1;
      simt2 = sh.t2;
   }
   double shcolen = (shco.pw + 2.0*shco.t2)/2.0;
   double d22 = d2/2.0;

// Set Constant-time Period for d2. 

   if (d2_index == 0) d2_init = getval("d2");
   double d2_ = (ni - 1)/sw1 + d2_init;
   putCmd("d2acqret = %f\n",roundoff(d2_,12.5e-9));
   putCmd("d2dwret = %f\n",roundoff(1.0/sw1,12.5e-9));

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");
  
// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90") + getval("tHY") + 2.0* simpw + shco.pw;
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = d2_ + getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = d2_ + getval("rd") + getval("ad") + at;
   d.c3 = d.c3 + (!strcmp(mix.seq,"tppm"));
   d.c3 = d.c3 + ((!strcmp(mix.seq,"tppm")) && (mix.t.a > 0.0));
   d.t3 = 2.0*getval("taua") + 2.0*getval("taub") - 2.0*shsim - shcolen +
          2.0*simt1 + 2.0*simt2 - pwsim - 2.0*shsim;
   d.c4 = d.c4 + (!strcmp(mix.seq,"spinal"));
   d.c4 = d.c4 + ((!strcmp(mix.seq,"spinal")) && (mix.s.a > 0.0));
   d.t4 = 2.0*getval("taua") + 2.0*getval("taub") - 2.0*shsim - shcolen +
          2.0*simt1 + 2.0*simt2 - pwsim - 2.0*shsim;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Up 2D
   
   int errval = (int) ((getval("taua") - shsim)*2.0*sw1);
   if ((getval("taua") - ni/(2.0*sw1) - shsim) < 0.0) { 
     text_error("Error:ni is too large. Make ni equal to %d or less.\n",errval); 
     psg_abort(1);
   }

// Set Phase Tables

   settable(phH90,4,table1);
   settable(phHhy,4,table2);
   settable(phYhy,4,table3);
   settable(ph1Xshca,4,table4);
   settable(ph1Ysh,4,table5);
   settable(phXshco,4,table6);
   settable(phX90,4,table7);
   settable(phY90,4,table8);
   settable(ph2Xshca,4,table9);
   settable(ph2Ysh,4,table10);
   settable(phRec,4,table11);
   setreceiver(phRec);

// States Acquisition

   if (phase1 == 2)       
      tsadd(phYhy,1,4);

// Begin Sequence

   txphase(ph1Xshca); decphase(phH90); dec2phase(phYhy);
   obspwrf(getval("aXshca")); decpwrf(getval("aH90")); dec2pwrf(getval("aYhy"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to Y Cross Polarization

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decphase(phHhy);
   _cp_(hy,phHhy,phYhy);
   decphase(zero);

// Begin F1 INEPT

   _dseqon(mix);
   dec2phase(ph1Ysh); 
   dec2pwrf(getval("aYsh"));
   delay(getval("taua") - d22 - shsim);
   _pboxsimpulse(shca,sh,ph1Xshca,ph1Ysh);
   obsblank();
   obspower(getval("tpwr")); dec2power(getval("dpwr2"));
   delay(3.0e-6);
   obsunblank();
   if (d22 < (shcolen + pwsim))  {
      txphase(phX90); dec2phase(phY90);   
      obspwrf(getval("aX90")); dec2pwrf(getval("aY90"));
      delay(getval("taua") + d22 - shsim - pwsim - 3.0e-6);
   } 
   else
   {
      txphase(phXshco);
      obspwrf(getval("aXshco"));
      delay(getval("taua") - shsim - shcolen - 3.0e-6); 
      _pboxpulse(shco,phXshco);
      obsblank();
      obspower(getval("tpwr"));
      delay(3.0e-6);
      obsunblank();
      txphase(phX90); dec2phase(phY90);
      obspwrf(getval("aX90")); dec2pwrf(getval("aY90"));
      delay(d22 - shcolen - pwsim - 3.0e-6);
   }
   sim3pulse(getval("pwX90"),0.0,getval("pwY90"),phX90,zero,phY90,0.0,0.0);
   obsunblank(); dec2unblank(); 
   txphase(ph2Xshca); dec2phase(ph2Ysh);
   delay(getval("taub") - pwsim - shsim);
   _pboxsimpulse(shca,sh,ph2Xshca,ph2Ysh);
   obsblank();
   obspower(getval("tpwr")); dec2power(getval("dpwr2"));
   delay(3.0e-6);
   obsunblank();
   delay(getval("taub") - shsim - 3.0e-6);
   _dseqoff(mix);

// Begin Acquisition

   _dseqon(dec);
   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

