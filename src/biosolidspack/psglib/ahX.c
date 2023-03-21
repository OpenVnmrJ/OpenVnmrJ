/* ahYX.c                  --Agilent BioSolids--

              1D sequences for tuneup experiments:

              1. Cross Polarization with 2-angle SPINAL or TPPM Decoupling. 
              2. Direct Polarization with 2-angle SPINAL or TPPM Decoupling. 
              3. Cross Poarization with Flipback for X pwX90, 2-angle SPINAL and TPPM Decoupling.
              4. Cross Polarization with a Hard Refocusing Pulse, 2-angle SPINAL and TPPM Decoupling.
              5. Cross Polarization with a Soft Refocusing Pulse, 2-angle SPINAL and TPPM Decoupling.
              6. Direct Polarization with a Hard Refocusing Pulse, 2-angle SPINAL and TPPM Decoupling.
              7. Direct Polarization with a Soft Refocusing Pulse, 2-angle SPINAL and TPPM Decoupling.

              SELECT EXPERIMENTS 1-5

              Select  pseq = "one", "two", "three", "four", "five", "six", "seven" to select the 
              experiment above. The internal flags cp, echo and flip are set automatically. 

              NO AUTOMATIC ROTOR SYNCHRONIZATION

              There are no delays in this experiment that are automatically rotor-synchronized. 
              The user should choose correct values for delays if synchronization is desired. 
              A calculation of taur from srate is included in the code but not used. 

              ECHO EXPERIMENTS 4, 5, 6 and 7: 

              For experiments 4,5 the two echo delay times tECHOhalf are calculated from tECHO. For 
              'hard' refocusing pwXecho is subtracted, for 'soft'refocussing pwXshp1 is not 
              subtracted. When tECHO is synchronized to 2.0*n*taur, then tECHOhalf is synchronized 
              to n*taur. For arrays of tECHO in experiments 4 and 5 the maximum value tECHOmaxret 
              is calculated and returned as a message parameter. 

              For experiments 1-3 tECHO is not used and tECHOmaxret is set to 0.0. The sequence, 
              however, does not remove irrelavant arrays of tECHO. 

              For experiments 5 and 7 the refocusing pulse is surrounded a pair of rotor periods
              taur. The delay 5.0e-6 corrects for the power-change time in the refocusing 
              delays and is included in each taur. The power is set to dbXshp1 in the first delay
              and back to tpwr in the second.  Note that because pwXshp1 is not inluded in 
              tECHO the shaped pulse will be surrounded by an integral number of rotor periods
              but it will not be centered on a complete rotor cycle unless pwXshp1= 2.0*m*taur. 
              If synchronization matters both delays must be synchronized. 

              For experiments 4 and 6 the hard refocusing pulse is subtracted from tECHO so 
              that the center of the hard pulse is placed at a complete rotor cycle. In 
              this case the refocusing delays will not be synchonized unless pwXecho = 2.0*m*taur.
              Both values should be synchronized if it matters. 
              
              CONSTANT-TIME DECOUPLING
              
              For ctd = "y", for experiments 1-3, DEC and DEC2 decoupling is added for a period 
              tRF = tRFmax. For experiments 4, 5, 6 and 7, tRF = tRFmax - 2.0*tECHOhalf. 

              For arrays of tECHO with ctd='y' the system finds the maximum of tECHOhalf and sets 
              tRFmax = 2.0*tECHOhalf. For experiments 4, 5, 6 and 7, tRF is always greater than 
              or equal to 0.0.  The putCmd statement is used to reset the parameter tRFmax
              If ctd ='n' the tRF decoupling is not executed. The putcmd statement is used to
              set tRFmax=0.0.

              For experiments 1,2 and 3 tECHOhalf is calculated as 0.0 and tRF is fixed by tRFmax.            

              If ctd = 'n' the tRF delay is not executed. The values of tECHOmax and tRFmax 
              are ignored, but not altered. The message parameter tRFmaxret is set to 0.0. 

              DEC2 DECOUPLING

              For ddec2 = "y", decoupling is executed on DEC2 during acquisition for any of 
              the experiments, with  the choice of SPINAL, TPPM, or WALTZ. Note that CW 
              can be selected for SPINAL or TPPM, using phY = 0.0 (and alpY = 0.0). "No 
              decoupling" can be selected in arrays by setting aY = 0.0. If DEC2 decoupling 
              is not desired for the experiment, set ddec2 = "n". In this case all DEC2 
              decoupling parameters are ignored.

              2-ANGLE SPINAL

              The SPINAL for both DEC and DEC2 is executed as 2-angle SPINAL using the phases
              alp and ph. alp = ph/2.0 reproduces 1-angle SPINAL and alp = 0 reproduces TPPM. 

              This sequence was derived from hX (AJN and LJS 022610)
              provided by C. Rienstra, UIUC.   

                                                                                                */
#include "standard.h"
#include "solidstandard.h"
#include "pboxpulse.h"

// Define Values for Phasetables

static int table1[4] = {2,2,2,2};           // phH90 with softpulse    
static int table2[4] = {0,3,0,3};           // phXhx with softpulse
static int table3[4] = {1,1,1,1};           // phHhx with softpulse
static int table5[8] = {0,0,1,1,0,0,1,1};   // phXshp1
static int table7[8] = {0,1,2,3};           // phRec softpulse
static int table8[4] = {0,2,0,2};           // phH90 just CP
static int table9[4] = {0,0,1,1};           // phXhx just CP
static int table10[4] = {3,3,3,3};          // phHhx just CP
static int table11[4] = {0,2,1,3};          // phRec just CP
static int table12[4] = {1,3,2,0};          // phX90
static int table13[8] = {0,2,1,3,2,0,3,1};  // phXecho - solid echo cycle 
static int table14[4] = {1,3,2,0};   	    // phX90flip

#define phH90sftp t1
#define phXhxsftp t2
#define phHhxsftp t3
#define phXshp1 t5
#define phRecsftp t7
#define phH90 t8
#define phXhx t9
#define phHhx t10
#define phRec t11
#define phX90 t12
#define phXecho t13
#define phX90flip t14

void pulsesequence() {

// Define Variables and Objects and Get Parameter Values

   CP hx = getcp("HX",0.0,0.0,0,1);
   OSTRCPY(hx.fr, sizeof(hx.fr), "dec");
   OSTRCPY(hx.to, sizeof(hx.to), "obs");
   putCmd("frHX='dec'\n");
   putCmd("toHX='obs'\n");

   DSEQ dec = getdseq2("H");

// Choose DEC2 Decoupling

   char ddec2[MAXSTR];
   getstr("ddec2",ddec2);

   DSEQ dec2;
   if (!strcmp(ddec2,"y")) dec2 = getdseq2("Y");

// Choose the Experiment

   char cp[10]; 
   char flip[10]; 
   char echo[10];
   char pseq[MAXSTR];
   getstr("pseq",pseq);

   if (!strcmp(pseq,"one")) {              // Cross Polarization
      OSTRCPY( cp,   sizeof(cp),   "y");
      OSTRCPY( echo, sizeof(echo), "n");
      OSTRCPY( flip, sizeof(flip), "n");
   }
   else if (!strcmp(pseq,"two")) {         // Direct Polarization
      OSTRCPY( cp,   sizeof(cp),   "n");
      OSTRCPY( echo, sizeof(echo), "n");
      OSTRCPY( flip, sizeof(flip), "n");
   }
   else if (!strcmp(pseq,"three")) {       // Cross Poarization with Flipback
      OSTRCPY( cp,   sizeof(cp),   "y");
      OSTRCPY( echo, sizeof(echo), "n");
      OSTRCPY( flip, sizeof(flip), "y");
   }
   else if (!strcmp(pseq,"four")) {        // Cross Polarization with Hard Echo
      OSTRCPY( cp,   sizeof(cp),   "y");
      OSTRCPY( echo, sizeof(echo), "hard");
      OSTRCPY( flip, sizeof(flip), "n");
   }
   else if (!strcmp(pseq,"five")) {        // Cross Polarization with Soft Echo
      OSTRCPY( cp,   sizeof(cp),   "y");
      OSTRCPY( echo, sizeof(echo), "soft");
      OSTRCPY( flip, sizeof(flip), "n");
   } 
   else if (!strcmp(pseq,"six")) {         // Direct Polarization with Hard Echo
      OSTRCPY( cp,   sizeof(cp),   "n");
      OSTRCPY( echo, sizeof(echo), "hard");
      OSTRCPY( flip, sizeof(flip), "n");
   }
   else if (!strcmp(pseq,"seven")) {       // Direct Polarization with Soft Echo
      OSTRCPY( cp,   sizeof(cp),   "n");
      OSTRCPY( echo, sizeof(echo), "soft");
      OSTRCPY( flip, sizeof(flip), "n");
   }
   else {
      printf("ABORT: Experiment Not Found ");
   }

// Create Soft Pulse if Needed

   PBOXPULSE shp1;

   if (!strcmp(echo,"soft")) {
      shp1 = getpboxpulse("shp1X",0,1); 
      shp1.st = 1.0;
      shp1.ph = 0.0;
      putCmd("stXshp1=0.5\n");
      putCmd("phXshp1=0.0\n");
   }

// Determine taur, One Rotor Cycle

   double srate = getval("srate");
   double taur = 0.0;
   if (srate >= 500.0)
      taur = roundoff((1.0/srate), 0.125e-6);
   else {
      printf("ABORT: Spin Rate (srate) must be greater than 500\n");
      psg_abort(1);
   }

// Set shp1.t1 and shp1.t2 of the Soft Pulse Explicitily

   shp1.t1 = taur - 5.0e-6;                                             
   shp1.t2 = taur - 5.0e-6;   

// Set tECHOhalf for Hard or Soft Echos

   double tECHOhalf = 0.0;
   double tECHO = getval("tECHO");
   if (!strcmp(echo,"hard")) {
      tECHOhalf = (tECHO - getval("pwXecho"))/2.0;                
      if (tECHOhalf < 0.0) tECHOhalf = 0.0; 
   }
   if (!strcmp(echo,"soft"))
      tECHOhalf = tECHO/2.0;    
   if (tECHOhalf < 0.0) tECHOhalf = 0.0;                     

// Set tECHOmax and return it to the Panel

   double echomax = 0.0;
   double echoarray[256];
   int echodim = 0;
   int i = 0;
   echodim = getarray("tECHO",echoarray);
   for (i = 0; i < echodim; i++) {
      if (!strcmp(echo,"hard"))
         echoarray[i] = echoarray[i] - getval("pwXecho");      
      else if (!strcmp(echo,"soft"))
         echoarray[i] = echoarray[i];
      else
         echoarray[i] = 0.0;
      if (echoarray[i] > echomax) echomax = echoarray[i];
   }
   putCmd("tECHOmaxret = %f\n",echomax*1.0e6); 

// Calculate Constant-time Decoupling Period

   char ctd[MAXSTR];
   getstr("ctd",ctd);

   double tRF = 0.0; 
   double tRFmax = getval("tRFmax");
   if (!strcmp(ctd,"y")) {
      if (tRFmax < echomax) tRFmax = echomax;
      putCmd("tRFmax = %f\n",tRFmax*1.0e6);    
      if (!strcmp(echo,"hard"))
         tRF = tRFmax - 2.0*tECHOhalf;
      else if (!strcmp(echo,"soft"))
         tRF = tRFmax - 2.0*tECHOhalf;
      else
         tRF = tRFmax;              
   }

// Set parameter tRFmaxret = 0.0 for No Constant-time Decoupling

   else { 
      putCmd("tRFmax = 0.0\n");
   }

// Copy Current Parameters to Processed

   putCmd("groupcopy('current','processed','acquisition')");

// Dutycycle Protection

   double duty = 4.0e-6 + getval("ad") + getval("rd") + at; 
   if (!strcmp(cp,"y")) {
      duty = duty + getval("pwH90") + getval("tHX");
      if (!strcmp(echo,"soft")) duty = duty + tECHO + getval("pwXshp1");
      if (!strcmp(echo,"hard")) duty = duty + tECHO + getval("pwXecho");
      if (!strcmp(flip,"y")) duty = duty + getval("pwX90flip");
   }
   else duty = duty + getval("pwX90");
   if (!strcmp(ctd,"y")) duty = duty + tRF - 2.0*tECHOhalf;
   duty = duty/(duty + d1 + 4.0e-6);
   if (duty > 0.1) {
      abort_message("Duty cycle %.1f%% >10%%. Abort!\n", duty*100.0);
   }

// Set Phase Tables

   settable(phH90sftp,4,table1);
   settable(phXhxsftp,4,table2);
   settable(phHhxsftp,4,table3);
   settable(phXshp1,8,table5);
   settable(phRecsftp,4,table7);
   settable(phH90,4,table8);
   settable(phXhx,4,table9);
   settable(phHhx,4,table10);
   settable(phRec,4,table11);
   settable(phX90,4,table12);
   settable(phXecho,8,table13);
   settable(phX90flip,4,table14);

   if (!strcmp(echo, "soft")) setreceiver(phRecsftp);
   else setreceiver(phRec);

// Begin Sequence
   
   if (NUMch > 2) dec2phase(zero);    
   if (!strcmp(cp,"y")) {
      if (!strcmp(echo,"soft")) {
         txphase(phXhxsftp); decphase(phH90sftp);
      }
      else {
         txphase(phXhx); decphase(phH90);
      } 
      obspwrf(getval("aXhx")); decpwrf(getval("aH90"));
   }
   else {
      txphase(phX90); decphase(zero);
      obspwrf(getval("aX90"));
   }
   obsunblank(); decunblank(); _unblank34(); 
   delay(d1);
   sp1on(); delay(2.0e-6); sp1off(); delay(2.0e-6);

// H to X Cross Polarization

   if (!strcmp(cp,"y")) {
      if (!strcmp(echo,"soft")) {
         decrgpulse(getval("pwH90"),phH90sftp,0.0,0.0);
         _cp_(hx,phHhxsftp,phXhxsftp);
      }
      else {
         decrgpulse(getval("pwH90"),phH90,0.0,0.0);
         _cp_(hx,phHhx,phXhx);
      }
   }
   else {
      rgpulse(getval("pwX90"),phX90,0.0,0.0);
      obsunblank();
   }

// Start DEC Decoupling

   _dseqon2(dec); 

// Start Optional DEC2 Decoupling 

   if (!strcmp(ddec2,"y")) {
      if (NUMch > 2) _dseqon2(dec2);
   }
    
// Optional X flip back

   if (!strcmp(flip,"y")) {
      txphase(phX90flip);
      obspwrf(getval("aX90"));
      rgpulse(getval("pwX90"),phX90flip,0.0,0.0);
      obsunblank();
   }

// Optional Shaped Refocusing Delay

   if (!strcmp(echo,"soft")) {
      txphase(phXshp1); 
      obspwrf(getval("aXshp1")); 
      delay(tECHOhalf);
      _pboxpulse(shp1,phXshp1);
      obsblank();
      obspower(tpwr);
      delay(3.0e-6);
      obsunblank();
      delay(2.0e-6);
      delay(tECHOhalf);
   }

// Optional Hard Refocussing Delay

   if (!strcmp(echo,"hard")) { 
      txphase(phXecho);
      obspwrf(getval("aXecho"));
      delay(tECHOhalf);
      rgpulse(getval("pwXecho"),phXecho,0.0,0.0);
      obsunblank(); 
      delay(tECHOhalf); 
   }   

// Blank DEC3 and Blank DEC2 for No DEC2 Decoupling

   if ( (!strcmp(ddec2,"y")) != 1) {
      if (NUMch > 2) dec2blank(); 
   }
   if (NUMch > 3) dec3blank();

// Begin Acquisition
   
   obsblank(); 
   delay(getval("rd"));
   startacq(getval("ad"));
   acquire(np, 1/sw);
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
   
// Make DEC and DEC2 Constant-time with RF Following Acquisition 

   decphase(zero);
   dec2phase(zero); 
   if (!strcmp(ctd,"y")) {
      decon(); 
      if (!strcmp(ddec2,"y")) {
         if (NUMch > 2) dec2on();
      }
      delay(tRF); 
      decoff(); 
      if (!strcmp(ddec2,"y")) {
         if (NUMch > 2) dec2off();
      }
   }
   obsunblank(); decunblank(); _unblank34();
}
