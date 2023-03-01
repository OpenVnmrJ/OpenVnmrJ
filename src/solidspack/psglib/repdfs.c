/*repdfs.c -   A sequence to do direct polarization of quadruple nuclei
               with TPPM and SPINAL decoupling, using a double frequency
               sweep pulse.

                D.Rice 05/17/06 VNMRJ2.1A-B for the NMR SYSTEM
	        updated with new .h files D.Rice 10/12/05               */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] = {1,1,1,1};           // phXdfs
static int table2[4] = {0,2,1,3};           // phX90
static int table3[4] = {0,2,1,3};           // phRec

#define phXdfs t1
#define phX90 t2
#define phRec t3

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   SHAPE dfs = getdfspulse("dfsX",0.0,0.0,0,1);
   strcpy(dfs.pars.ch,"obs");
   putCmd("chXdfs='obs'\n");

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
   d.dutyon = getval("nf")*(getval("pwX90") + dfs.pars.t);
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = getval("nf")*(getval("pwXdfs") + 200.0e-6 + getval("rd") + 
          getval("ad") + at);
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = getval("nf")*(getval("pwXdfs") + 200.0e-6 + getval("rd") + 
          getval("ad") + at);
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phXdfs,4,table1);
   settable(phX90,4,table2);
   settable(phRec,4,table3);
   setreceiver(phRec);

// Get the Number of Repetitions

   initval(getval("nf"),v1);

// Begin Sequence

   txphase(phX90); decphase(zero);
   obspwrf(getval("aXdfs"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// Start Decoupling on H

   _dseqon(dec);

// Rep DFS Acquisition

   rcvroff();
   loop(v1,v2);
      obspwrf(getval("aXdfs"));
      obsunblank(); _unblank34();
      delay(200.0e-6);
      delay(rof1);
      _shape(dfs,phXdfs);
      delay(getval("pwXdfs"));
      obspwrf(getval("aX90"));
      delay(200.0e-6);
      delay(rof1);
      rgpulse(getval("pwX90"),phX90,0.0,0.0);
      obsblank(); _blank34();
      delay(getval("rd"));
      acquire(np, 1/sw);
   endloop(v2);
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

