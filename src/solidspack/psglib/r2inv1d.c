/*r2inv1d.c - A sequence to measure rotational resonance through
              selective inversion and mixing. Also used to measure
	      inversion pulse length wit mix = 0.0.
                             
              D. Rice 04/3/06                                    */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {0,2,0,2};           // phH90
static int table2[4] = {0,0,1,1};           // phXhx
static int table3[4] = {1,1,1,1};           // phHhx
static int table4[4] = {1,3,2,0};           // ph1Xr2inv
static int table5[4] = {1,3,2,0};           // phsXr2inv
static int table6[4] = {0,2,1,3};           // ph2Xr2inv
static int table7[4] = {0,2,1,3};           // phRec

#define phH90 t1
#define phXhx t2
#define phHhx t3
#define ph1Xr2inv t4
#define phsXr2inv t5
#define ph2Xr2inv t6
#define phRec t7

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   double tXr2inv = getval("tXr2inv");
   double dbXr2inv = getval("tpwr");
   double aXr2inv = getval("aXr2inv");
   double dbsXr2inv = getval("dbsXr2inv");
   double asXr2inv = getval("asXr2inv");
   double pwXr2inv = getval("pwXr2inv");
   double pwsXr2inv = getval("pwsXr2inv");
   double ofsXr2inv = getval("ofsXr2inv");

   double taur = 0.0;
   double srate = getval("srate");
   if (srate >= 500.0)    
      taur = roundoff((1.0/srate), 0.0125e-6);
   else {
      printf("ABORT: Spin Rate (srate) must be greater than 500\n");
      psg_abort(1);
   }

   double taus = pwsXr2inv;                 
   int n = (int) ((taus + pwXr2inv + 3.0e-6)/taur);
   if (n%2 != 0) n = n + 1;
   double tauL = 0.5*((double) (n*taur - taus - pwXr2inv - 6.05e-6));
   while (tauL < 0.0) {
      n = n + 2;
      tauL = 0.5*((double) (n*taur - taus - pwXr2inv - 6.05e-6));
   }
   double tauR = tauL + pwXr2inv/2.0;

   double mix = tXr2inv;
   mix = roundoff(mix,taur);
   if (mix < pwXr2inv) {
      mix = 0.0;
      tauR = tauR + mix - pwXr2inv/2.0;
      if (tauR < 0.0) tauR = 0.0;
   }
   else {
      mix = mix - pwXr2inv/2.0;
      if (mix < 0.0) mix = 0.0;
   }

   static shape shXr2inv;
   char cmd[MAXSTR*2];
   if (getval("arraydim") < 1.5 || (ix==1) || isarry("ofsXr2inv") ||
      isarry("pwsXr2inv")) {
      sprintf(shXr2inv.name, "%s_%d", "r2inv", ix);
      sprintf(cmd, "Pbox %s -w \"rsnob %.7f %.1f\" -0\n", shXr2inv.name, pwsXr2inv, ofsXr2inv);
      system(cmd);
      shXr2inv = getRsh(shXr2inv.name);
   }

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

   DSEQ hr = getdseq("Hmix");
   strcpy(hr.t.ch,"dec");
   putCmd("chHmixtppm='dec'\n");
   strcpy(hr.s.ch,"dec");
   putCmd("chHmixspinal='dec'\n");
//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90") + getval("tHX") + pwsXr2inv;
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d.c3 = d.c3 + (!strcmp(hr.seq,"tppm"));
   d.c3 = d.c3 + ((!strcmp(hr.seq,"tppm")) && (hr.t.a > 0.0));
   d.t3 = tauL + tauR + 6.0e-6;
   d.c4 = d.c4 + (!strcmp(hr.seq,"spinal"));
   d.c4 = d.c4 + ((!strcmp(hr.seq,"spinal")) && (hr.s.a > 0.0));
   d.t4 = tauL + tauR + 6.0e-6;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phH90,4,table1);
   settable(phXhx,4,table2);
   settable(phHhx,4,table3);
   settable(ph1Xr2inv,4,table4);
   settable(phsXr2inv,4,table5);
   settable(ph2Xr2inv,4,table6);
   settable(phRec,4,table7);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXhx); decphase(phH90);
   obspwrf(getval("aXhx")); decpwrf(getval("aH90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to X Cross Polarization

   decrgpulse(getval("pwH90"),phH90,0.0,0.0);
   decphase(phHhx);
    _cp_(hx,phHhx,phXhx);

// Decouple During Rotational Resonance

   _dseqon(hr);

// Selective Inversion with Hard 90 and Selective 180

   obspwrf(aXr2inv);
   rgpulse(pwXr2inv, ph1Xr2inv, 0.0, 0.0);
   txphase(phsXr2inv);
   obsblank(); 
   obspower(dbsXr2inv);
   delay(3.0e-6);
   obsunblank(); 
   obspwrf(asXr2inv);
   delay(tauL);
   shaped_pulse(shXr2inv.name,pwsXr2inv,phsXr2inv,0.0,0.0);
   obsblank();
   obspower(dbXr2inv);
   delay(3.0e-6);
   obsunblank();
   obspwrf(aXr2inv);
   delay(tauR);

// Rotational Resonance Mixing

   _dseqoff(hr);
   delay(mix);
   _dseqon(hr);

// Rotational Resonance Detection Pulse

   rgpulse(pwXr2inv, ph2Xr2inv, 0.0, 0.0);
   obsunblank();

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
