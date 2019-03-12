/*cpinad2d.c - A sequence to do INADEQUATE with CP preparation and
               refocussed multiplets.

            The phase cycle of this program is similar to the corresponding 
	    INADEQUATE sequence in the ENS-Lyon Pulse Programming Library
            www.ens-lyon.fr/STIM/NMR/pp.html. All other code is specific 
	    to the Varian NMR System.

     Lesage, A.,Auger, C.,Calderelli, S., Emsley, L., J. Am. Chem. Soc., 119,
            7867-7869 (1997).
     Lesage, A.,Bardet, M.,Emsley, L., J. Am. Chem. Soc. 121 10987-10093 (1999). 	       	                                       
             D. Rice 3/6/06                                                */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,3,1,3};           // phH90
static int table2[8] = {1,1,2,2,3,3,0,0};   // phXhx
static int table3[4] = {0,0,0,0};           // phHhx
static int table4[32] = {0,0,1,1,2,2,3,3,
                         1,1,2,2,3,3,0,0,
                         2,2,3,3,0,0,1,1,
                         3,3,0,0,1,1,2,2};  // ph1Xinad
static int table5[8] = {0,0,1,1,2,2,3,3};   // ph2Xinad
static int table6[4] = {0,0,0,0};           // ph3Xinad
static int table7[4] = {1,1,1,1};           // ph4Xinad
static int table8[16] = {0,2,2,0,0,2,2,0,
                         2,0,0,2,2,0,0,2};  // phrec

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define ph1Xinad t4
#define ph2Xinad t5
#define ph3Xinad t6
#define phfXinad v1
#define ph4Xinad t7
#define phRec t8

static double d2_init;

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values 

   CP hx = getcp("HX",0.0,0.0,0,1);
   strncpy(hx.fr,"dec",3);
   strncpy(hx.to,"obs",3);
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   double aXinad = getval("aXinad");       //parameters for inadX implemented are 
   double pw1Xinad = getval("pw1Xinad");   //implemented directly in the pulse sequence
   double pw2Xinad = getval("pw2Xinad");
   double t1Xinadinit = getval("t1Xinad");
   double t2Xinadinit = getval("t2Xinad");
   double t1Xinad  = t1Xinadinit - pw1Xinad/2.0;
   if (t1Xinad < 0.0) t1Xinad = 0.0;
   double t2Xinad  = t2Xinadinit - pw1Xinad/2.0 - pw2Xinad/2.0;
   if (t2Xinad < 0.0) t2Xinad = 0.0;
   double d2init = getval("d2");
   d2 = d2init - pw2Xinad;
   if (d2 < 0.0) d2 = 0.0; 

   DSEQ dec = getdseq("H");
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHtppm='dec'\n"); 
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHspinal='dec'\n");

   DSEQ hr = getdseq("Hmix");
   strncpy(hr.t.ch,"dec",3);
   putCmd("chHmixtppm='dec'\n"); 
   strncpy(hr.s.ch,"dec",3);
   putCmd("chHmixspinal='dec'\n");

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
   d.dutyon = getval("pwH90") + getval("tHX") + 2.0*pw1Xinad + 2.0*pw2Xinad;
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d.c3 = d.c3 + (!strcmp(hr.seq,"tppm"));
   d.c3 = d.c3 + ((!strcmp(hr.seq,"tppm")) && (hr.t.a > 0.0));
   d.t3 = d2_ + 2.0*t1Xinad + 2.0*t2Xinad;
   d.c4 = d.c4 + (!strcmp(hr.seq,"spinal"));
   d.c4 = d.c4 + ((!strcmp(hr.seq,"spinal")) && (hr.s.a > 0.0));
   d.t4 = d2_ + 2.0*t1Xinad + 2.0*t2Xinad;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phH90,4,table1);
   settable(phXhx,8,table2);
   settable(phHhx,4,table3);
   settable(ph1Xinad,32,table4);
   settable(ph2Xinad,8,table5);
   settable(ph3Xinad,4,table6);
   settable(ph4Xinad,4,table7);
   settable(phRec,16,table8);

// Add STATES TPPI (States with FAD)

   tsadd(phXhx,d2_index,4);
   tsadd(ph1Xinad,d2_index,4);
   tsadd(ph2Xinad,d2_index,4);
   tsadd(phRec,2*d2_index,4);

   double obsstep = 360.0/(PSD*8192);
   if (phase1 == 2)
      initval((315.0/obsstep),phfXinad);
   else
      initval(0.0/obsstep,phfXinad);

   obsstepsize(obsstep);
   setreceiver(phRec);

// Begin Sequence

   xmtrphase(phfXinad); txphase(phXhx); decphase(phH90);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));  
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to X Cross Polarization

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decphase(phHhx);
   _cp_(hx,phHhx,phXhx);

// Begin INADEQUATE with Refocussing

   _dseqon(hr);
   obspwrf(aXinad);
   txphase(ph1Xinad);
   delay(t1Xinad);
   rgpulse(getval("pw1Xinad"),ph1Xinad,0.0,0.0);
   txphase(ph2Xinad);
   delay(t2Xinad);
   rgpulse(getval("pw2Xinad"),ph2Xinad,0.0,0.0);
   xmtrphase(zero); txphase(ph3Xinad);
   delay(d2);
   rgpulse(getval("pw2Xinad"),ph3Xinad,0.0,0.0);
   delay(t2Xinad);
   rgpulse(getval("pw1Xinad"),ph4Xinad,0.0,0.0);
   delay(t1Xinad);
   _dseqoff(hr);

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

