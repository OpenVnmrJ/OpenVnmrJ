/*tancpxedit.c - A sequence to do solids cross-polarization/depolarization/
                 repolarization (CP/RP/CP) for spectral editing of CP/MAS spectra. 

                 Formerly xpoledit.c of SolidsLib  6/1/96                      
                 Provided by H. Bildsoe, Univ. Aarhus, Denmark, for VXR, 
                 6/90 G. Simon, Darmstadt, 12/1/94 - Unityplus and Inova D. Rice    
                 6/1/96  Solidslib 6.1b

                 R.Sangil, H. Bildsoe, H. J. Jacobsen, J. Magn. 
                 Reson. A 107, 67 (1994).

                 D. Rice 04/30/09                                               */ 

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,1,3,3};         // phH90
static int table2[4] = {3,3,3,3};         // phXhx
static int table3[4] = {0,0,0,0};         // phHhx
static int table4[8] = {3,3,3,3};         // phXhxedit
static int table5[8] = {2,2,2,2};         // ph1Hhxedit
static int table6[8] = {0,0,0,0,2,2,2,2}; // ph2Hhxedit
static int table7[8] = {0,0,2,2,0,0,2,2}; // phRec

#define phH90      t1
#define phXhx      t2
#define phHhx      t3
#define phXhxedit  t4
#define ph1Hhxedit t5
#define ph2Hhxedit t6
#define phRec      t7

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

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = getval("pwH90") + getval("tHX") + getval("t1HXedit") + 
              getval("t2HXedit");
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phH90,4,table1);
   settable(phXhx,4,table2);
   settable(phHhx,4,table3);
   settable(phXhxedit,4,table4);
   settable(ph1Hhxedit,4,table5);
   settable(ph2Hhxedit ,4,table6);
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

// HX Depolarization and Repolarization 
    
   xmtron(); decon();
   txphase(phXhxedit); decphase(ph1Hhxedit);
   obspwrf(getval("aXhxedit")); decpwrf(getval("aHhxedit"));
   delay(getval("t1HXedit"));
   decphase(ph2Hhxedit); 
   delay(getval("t2HXedit"));
   xmtroff(); decoff();  
 
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

