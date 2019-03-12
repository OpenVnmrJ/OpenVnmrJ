/*hahnechodfs1d.c - A sequence to generate a Hahn echo, preceded by 
                 a DFS pulse for signal enchancement.  This echo uses a
                 four-phase cycle relative to the first pulse and receiver
		 to generate in-phase and out-of-phase echos.

                 D. Rice 03/18/08                                      
                 Modified 05/17/10                                       */

#include "standard.h"
#include "solidstandard.h"

// Define Values for Phasetables

static int table1[4] =  {1,1,1,1};                          //phXdfs
static int table2[4] =  {1,2,3,0};                          //phX90
static int table3[16] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3};  //phXecho
static int table4[8] =  {1,0,3,2,3,2,1,0};                  //phRec 

#define phXdfs t1
#define phX90 t2
#define phXecho t3
#define phRec t4

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   SHAPE dfs = getdfspulse("dfsX",0.0,0.0,0,1);
   strncpy(dfs.pars.ch,"obs",3);
   putCmd("chXdfs='obs'\n");

//   dump_shape(dfs);

   double aXecho = getval("aXecho");  // define the echoX group in the sequence
   double t1Xechoinit = getval("t1Xecho");
   double pwXecho = getval("pwXecho"); 
   double t2Xechoinit = getval("t2Xecho");
   double t1Xecho  = t1Xechoinit - pwXecho/2.0;
   if (t1Xecho < 0.0) t1Xecho = 0.0; 
   double t2Xecho  = t2Xechoinit - pwXecho/2.0 - getval("rd");
   if (t2Xecho < 0.0) t2Xecho = 0.0;

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
   d.dutyon = getval("pwX90") + pwXecho + dfs.pars.t;
   d.dutyoff = d1 + 4.0e-6;
   d.c1 = d.c1 + (!strcmp(dec.seq,"tppm"));
   d.c1 = d.c1 + ((!strcmp(dec.seq,"tppm")) && (dec.t.a > 0.0));
   d.t1 = t1Xecho + t2Xecho + getval("rd") + getval("ad") + at;
   d.c2 = d.c2 + (!strcmp(dec.seq,"spinal"));
   d.c2 = d.c2 + ((!strcmp(dec.seq,"spinal")) && (dec.s.a > 0.0));
   d.t2 = t1Xecho + t2Xecho + getval("rd") + getval("ad") + at;
   d = update_dutycycle(d);
   abort_dutycycle(d,10.0);

// Set Phase Tables

   settable(phXdfs,4,table1);
   settable(phX90,4,table2);
   settable(phXecho,16,table3);
   settable(phRec,8,table4);
   setreceiver(phRec);
    
// Begin Sequence

   txphase(phX90); decphase(zero);
   obspwrf(getval("aX90"));
   obsunblank(); decunblank(); _unblank34();
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H Decoupler on Before Echo

   _dseqon(dec);

// X Double Frequency Sweep Pulse

   _shape(dfs,phXdfs);
   txphase(phX90);
   obspwrf(getval("aX90"));
   delay(200.0e-6);

// X Direct Polarization 

   rgpulse(getval("pwX90"),phX90,0.0,0.0);

// X Solid-State Echo

   txphase(phXecho);
   obspwrf(aXecho);
   delay(t1Xecho); 
   rgpulse(pwXecho,phXecho,0.0,0.0);
   delay(t2Xecho);

// Begin Acquisition

   obsblank(); _blank34();
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
   endacq();
   _dseqoff(dec);
   obsunblank(); decunblank(); _unblank34();
}

