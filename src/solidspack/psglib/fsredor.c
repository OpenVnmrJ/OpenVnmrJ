/*fsredor.c -A sequence to obtain frequency selective REDOOR using selective
             inversion pulses on X and Y with Y-channel XY8 at the half
             rotor cycles. Prep with a constant, linear or tangent ramped CP.

             TPPM spans the evolution period.

             CEB 06/28/05 for VNMRJ2.1A-B for the NMR SYSTEM
             Edited  D. Rice 6/28/05
	     updated with new .h files D.Rice 10/12/05                      */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,1,1,1};           // phH90
static int table2[4] = {3,3,3,3};           // phXhx
static int table3[4] = {0,0,0,0};           // phHhx
static int table4[4] = {0,1,2,3};           // phXg
static int table5[4] = {0,0,0,0};           // phYg
static int table6[4] = {0,2,0,2};           // phRec
static int table7[16] = {0,1,0,1,1,0,1,0};  //phYxy8

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define phXg  t4
#define phYg  t5
#define phRec t6
#define phYxy8 t7

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   CP hx = getcp("HX",0.0,0.0,0,1);
   strcpy(hx.fr,"dec");
   strcpy(hx.to,"obs");
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   DSEQ dec = getdseq("H");   
   strcpy(dec.t.ch,"dec");
   putCmd("chHtppm='dec'\n"); 
   strcpy(dec.s.ch,"dec");
   putCmd("chHspinal='dec'\n");

   DSEQ hREDOR = setdseq("R","tppm");
   strcpy(hREDOR.t.ch,"dec");
   putCmd("chRtppm='dec'\n"); 
   strcpy(hREDOR.s.ch,"dec");
   putCmd("chRspinal='dec'\n");

// Set the Shaped Pulses in n Rotor Periods

   double srate = getval("srate");
   double pwXg = getval("pwXg");
   double pwYg = getval("pwYg");
   double pwY180 = getval("pwY180");
   double taur = 1.0/srate;
   double tauX0 = (taur - pwY180)/2.0;
   double tauX = tauX0 -  pwY180/2.0;

   double taug = pwXg;
   if (pwYg >= pwXg) taug = pwYg;

   int n = (int) ((taug - pwY180/2.0)/taur);
   if (n%2 != 0) n = n + 1;
   double tauL = 0.5*((double) (n*taur - taug - pwY180));
   if (tauL < 0) {
      n = n + 2;
      tauL = 0.5*((double)n*taur - taug - pwY180);
   }
   double tauR = tauL + pwY180/2.0;

   int nRotor = (int) (getval("nRotor") + 0.5);
   nRotor = nRotor/2;
   initval(2.0*nRotor - 1.0 ,v8);

   char mode[MAXSTR];
   getstr("Rmode",mode);

// Calculate the Selective Gaussian Pulses with PBox

   double ofXg = getval("ofXg");
   double ofYg = getval("ofYg");
   char   cmd[MAXSTR*2];
   static shape  shXg, shYg;

   if (getval("arraydim") < 1.5 || (ix==1) || isarry("ofXg") || isarry("pwXg")) {
      sprintf(shXg.name, "%s_%d", "arr", ix);
      sprintf(cmd, "Pbox %s -w \"sinc180r %.7f %.1f\" -0\n", shXg.name, pwXg, ofXg);
      system(cmd);
      shXg = getRsh(shXg.name);
   }

   if (getval("arraydim") < 1.5 || (ix==1) || isarry("ofYg") || isarry("pwYg")) {
      sprintf(shYg.name, "%s_%d", "crr", ix);
      sprintf(cmd, "Pbox %s -w \"sinc180r %.7f %.1f\" -0\n", shYg.name, pwYg, ofYg);
      system(cmd);
      shYg = getRsh(shYg.name);
   }

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   double pwXecho = pwXg;
   if ((mode[0] == 'y') && (pwYg > pwXg)) pwXecho = pwYg;
   
   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90") + getval("tHX") + 2.0*nRotor*pwY180 + pwXecho;
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d.c3 = d.c3 + (!strcmp(hREDOR.seq,"tppm"));
   d.c3 = d.c3 + ((!strcmp(hREDOR.seq,"tppm")) && (hREDOR.t.a > 0.0));
   d.t3 = nRotor*(1.0/srate - pwY180) + 1.0/srate - pwXecho;
   d.c4 = d.c4 + (!strcmp(hREDOR.seq,"spinal"));
   d.c4 = d.c4 + ((!strcmp(hREDOR.seq,"spinal")) && (hREDOR.s.a > 0.0));
   d.t4 = nRotor*(1.0/srate - pwY180) + 1.0/srate - pwXecho;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phH90,4,table1);
   settable(phXhx,4,table2);
   settable(phHhx,4,table3);
   settable(phXg,4,table4);
   settable(phYg,4,table5);
   settable(phRec,4,table6);
   settable(phYxy8,8,table7);

// Begin Sequence

   setreceiver(phRec);
   txphase(phXhx);
   decphase(phH90);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(1e-5); sp1off(); delay(2.0e-6);
   rof1 = 0.0; rof2 = 0.0; //comp for sim3pulse bug

// H to X Cross Polarization

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decunblank();
   decphase(phHhx);
    _cp_(hx,phHhx,phXhx);

// REDOR Period One

   _dseqon(hREDOR);
   obspwrf(getval("aXg"));
   if (nRotor >= 1) {
      dec2pwrf(getval("aY180"));
      delay(taur);
      assign(zero,v11);
      getelem(phYxy8,v11,v12); dec2phase(v12); incr(v11);
      delay(tauX0);
      dec2rgpulse(pwY180,v12,0.0,0.0);
      loop(v8,v9);
         getelem(phYxy8,v11,v12); dec2phase(v12); incr(v11);
         delay(tauX);     
         dec2rgpulse(pwY180,v12,0.0,0.0);                      	   	            
      endloop(v9);
   }

// Selective Inversion Pulses

      txphase(phXg); dec2phase(phYg);
      obspower(getval("dbXg")); dec2power(getval("dbYg"));
      obspwrf(getval("aYg")); dec2pwrf(getval("aYg"));
      delay(tauL);
      if (mode[0] == 'y')
         sim3shaped_pulse(shXg.name,"",shYg.name,pwXg,0.0,pwYg,phXg,phHhx,phYg,0.0,0.0);
      else
         sim3shaped_pulse(shXg.name,"","",pwXg,0.0,0.0,phXg,phHhx,phYg,0.0,0.0);
      obspower(tpwr); dec2power(dpwr2);
      delay(tauR);

// REDOR Period Two

   if (nRotor >= 1) {
      dec2pwrf(getval("aY180"));
      getelem(phYxy8,v11,v12); dec2phase(v12); incr(v11);
      delay(tauX0);
      dec2rgpulse(pwY180,v12,0.0,0.0);
      loop(v8,v9);
         getelem(phYxy8,v11,v12); dec2phase(v12); incr(v11);
         delay(tauX);
         dec2rgpulse(pwY180,v12,0.0,0.0);                       
      endloop(v9);
      delay(taur);
   }
   _dseqoff(hREDOR);

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

