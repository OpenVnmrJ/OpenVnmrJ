/*shapedtwopul1d.c - A sequence to do direct polarization with a shaped 
                  pulse from PBox, allowing a second preinversion 
                  pulse and with TPPM and SPINAL decoupling.

                  D.Rice 11/20/08                                    */

#include "standard.h"
#include "solidstandard.h"
#include "pboxpulse.h"

// Define Values for Phasetables

static int table1[4] = {0,0,0,0};           // phXshp1
static int table2[4] = {0,2,1,3};           // phXshp2
static int table3[4] = {0,2,1,3};           // phRec

#define phXshp1 t1
#define phXshp2 t2
#define phRec t3

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   PBOXPULSE shp1 = getpboxpulse("shp1X",0,1);
   strncpy(shp1.ch,"obs",3);
   putCmd("chHshp1 ='obs'\n");
   shp1.t1 = 0.0;
   shp1.t2 = 0.0;    

   PBOXPULSE shp2 = getpboxpulse("shp2X",0,1);
   strncpy(shp2.ch,"obs",3);
   putCmd("chHshp2 ='obs'\n");
   shp2.t1 = 0.0;
   shp2.t2 = 0.0;  
  
   double d2init = getval("d2");
   d2 = d2init - 5.0e-6; 
   if (d2 < 0.0) d2 = 0.0; 

   DSEQ dec = getdseq("H");
   strncpy(dec.t.ch,"dec",3);
   putCmd("chHtppm='dec'\n"); 
   strncpy(dec.s.ch,"dec",3);
   putCmd("chHspinal='dec'\n");

//--------------------------------------
// Copy Current Parameters to Processed
//-------------------------------------

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   DUTY d = init_dutycycle();
   d.dutyon = shp1.pw + shp2.pw;
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

   settable(phXshp1,4,table1);
   settable(phXshp2,4,table2);
   settable(phRec,4,table3);
   setreceiver(phRec);

// Begin Sequence

   txphase(phXshp1); decphase(zero);
   obspwrf(getval("aXshp1"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// Start Decoupling on H

   _dseqon(dec);

// Shaped X Pulse One

   _pboxpulse(shp1,phXshp1);
   txphase(phXshp2);
   obspwrf(getval("aXshp2"));
   delay(d2);

// Shaped X Pulse Two

   _pboxpulse(shp2,phXshp2);

// Begin Acquisition

   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

