/*mqmas3qse2d.c - A sequence to generate a two-pulse MQMAS 2D spectrum
                  with a third selective echo pulse, using a 3Q phase cycle.

                  Uses whole echo and wft2d(1,0,0,1) if phase = 0
                  Uses hypercomplex and wft2da if phase = 1,2

                  Brown, S.P., Wimperis, S.; J Magn Reson 128, 42-61 (1997).

                  D. Massiot, B Touzo, D. Truman, J.P, Coutures, J. Virlet, P. Florian,
                  P.J. Grandinetti; Solid State NMR 6, 73-83 (1996).

                  D. Rice 06/14/06                                                   */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables (phase = 0 whole echo)

static int table1[12] = {0,30,60,90,120,150,180,210,240,270,300,330};
                                        // phf1Xmqmas
static int table2[6] = {0,0,0,0,0,0};   // ph2Xmqmas
static int table3[96] = {0,0,0,0,0,0,0,0,0,0,0,0,
                         45,45,45,45,45,45,45,45,45,45,45,45,
                         90,90,90,90,90,90,90,90,90,90,90,90,
                         135,135,135,135,135,135,135,135,135,135,135,135,
                         180,180,180,180,180,180,180,180,180,180,180,180,
                         215,215,215,215,215,215,215,215,215,215,215,215,
                         270,270,270,270,270,270,270,270,270,270,270,270,
                         315,315,315,315,315,315,315,315,315,315,315,315};
                                       // phfXsel

//static int table4[48] = {0,3,2,1,0,3,2,1,0,3,2,1,1,0,3,2,1,0,3,2,1,0,3,2,
//                         2,1,0,3,2,1,0,3,2,1,0,3,3,2,1,0,3,2,1,0,3,2,1,0};
                                       // phRec wft2d(1,0,0,1) daslp = -2*setdaslp

static int table4[48] = {0,1,2,3,0,1,2,3,0,1,2,3,1,2,3,0,1,2,3,0,1,2,3,0,
                         2,3,0,1,2,3,0,1,2,3,0,1,3,0,1,2,3,0,1,2,3,0,1,2};
                                       // phRec-orig wft2d(1,0,0,-1) daslp = +2*setdaslp 

// Define Values for Phasetables (phase <> 0 hypercomplex)

static int table5[6] = {0,60,120,180,240,300}; // phf1Xmqmas 
static int table6[6] = {0,0,0,0,0,0};          // ph2Xmqmas
static int table7[48] = {90,90,90,90,90,90,135,135,135,135,135,135,
                         180,180,180,180,180,180,225,225,225,225,225,225,
                         270,270,270,270,270,270,315,315,315,315,315,315,
                         0,0,0,0,0,0,45,45,45,45,45,45};
                                               // phfXechsel
static int table8[24] = {0,2,0,2,0,2,1,3,1,3,1,3,2,0,2,0,2,0,3,1,3,1,3,1};
                                               // phRec 


#define phf1Xmqmas t1
#define ph2Xmqmas t2
#define phfXechsel t3
#define phRec t4

static double d2_init;

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   double tXechselinit = getval("tXechsel");  // Adjust the selective echo delay for the
   double tXechsel = tXechselinit - 3.0e-6;   // attenuator switch time.
   if (tXechsel < 0.0) tXechsel = 0.0;

   DSEQ dec = getdseq("H");
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHtppm='dec'\n");
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHspinal='dec'\n");

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
   d.dutyon = getval("pw1Xmqmas") + d2 + getval("pw2Xmqmas") + getval("pwXechsel");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = d2_ + tXechselinit + getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = d2_ + tXechselinit + getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   if (phase1 == 0) {
      settable(phf1Xmqmas,12,table1);
      settable(ph2Xmqmas,6,table2);
      settable(phfXechsel,96,table3);
      settable(phRec,48,table4);
   }
   else {
      settable(phf1Xmqmas,6,table5);
      settable(ph2Xmqmas,6,table6);
      settable(phfXechsel,48,table7);
      settable(phRec,24,table8);
      if (phase1 == 2) {
         tsadd(phf1Xmqmas,30,360);
      }
   }
   setreceiver(phRec);
   obsstepsize(1.0);

// Begin Sequence

   xmtrphase(phf1Xmqmas); decphase(zero);
   obspower(getval("tpwr"));
   obspwrf(getval("aXmqmas"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H Decoupler on Before MQMAS

   _dseqon(dec);

// Two-Pulse MQMAS

   rgpulse(getval("pw1Xmqmas"),zero,0.0,0.0);
   xmtrphase(zero); txphase(ph2Xmqmas);
   delay(d2);
   rgpulse(getval("pw2Xmqmas"),ph2Xmqmas,0.0,0.0);

// Tau Delay and Selective Echo Pulse

   xmtrphase(phfXechsel);
   obsblank();
   obspower(getval("dbXechsel"));
   obspwrf(getval("aXechsel"));
   delay(3.0e-6);
   obsunblank();
   delay(tXechsel);
   rgpulse(getval("pwXechsel"),zero,0.0,0.0);

// Begin Acquisition

   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

