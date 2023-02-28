/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* mpobjects.h -- A collection of windowed multiple-pulse objects
   with interleaved acquisition to be used in pulse sequences. 

   Caution:  These objects do NOT use waveforms and they do 
   not call a chopper function. They do NOT allow a phase continuous
   offset.

   These objects do not include the startacq() and endacq() statements
   and these statements must be supplied at the appropriate time before
   and after the _underscore function. 

   This file defines a group of generic pulse sequence statements that 
   can be assigned to 1-4 channels with the argument 'rfch'.  The 
   _underscore functions for each object use only generic statements
   and can be assigned to any channel based upon the .ch parameter
   of the structure.

   Include this file after solidmisc.h 

   D. Rice - 01/12/06                                         */

// Contents: 
// Utility Functions for Tables and V-variables.
// 1.  settablenumber()
// 2.  settablename()
// 3.  setvvarnumber()
// 4.  setvvarname()
//
// General Pulse Sequence Functions 
// 1.  gen_RFpwrf()
// 2.  gen_RFpower()
// 3.  gen_RFunblank()
// 4.  gen_RFblank()
// 5.  gen_RFphase()
// 6.  gen_RFfphase()
// 7.  gen_RFon()
// 8.  gen_RFoff()
// 9.  gen_RFpulse()
//
// Structures
// 1.  WMPA
//
// Implementation Functions
// 1. WMPA getbr24()
// 2. WMPA getmrev8()
// 3. WMPA getswwhh4()
// 4. WMPA getxx()
// 5. WMPA getxmx()
// 6. WMPA gettoss4()
// 7. WMPA getidref()
// 8. WMPA getwpmlg()
// 9. WMPA getwdumbo()
//10. WMPA getwdumbot()
//
// _Underscore Functions
// Implementation Functions
// 1. _br24()
// 2. _mrev8()
// 3. _swwhh4()
// 4. _xx()
// 5. _xmx()
// 6. _toss4()
// 7. _idref()
// 8. _wpmlg()
// 9. _wdumbo()
//10. _wdumbot()

//----------------------------------------------------------------
//  Dynamic Assignment of V Variables and Table Numbers.
//-----------------------------------------------------------------
// 
//  settablenumber(55); phH90 = settablename(4,table); performs 
//  the same action as #define phH90 t55; settable(phH90,4,table); 
//  and also sets globatableindex = 54. The next settablename() 
//  instruction sets t54.  
//
//  setvvarnumber(90); phH90 = setvvarname(); performs the same 
//  action as #define phH90 v90; and sets globalvvarindex = 89. The 
//  next setvvarname() instruction sets v89. The variable v90 is 
//  intialized to codeint "zero". 
//
//  The table names count down (presumably from a large number) 
//  so that they will not interfere with statically assigned tables
//  and v-variables that should count up from 1. 
//  
 
static int globaltableindex;
static int globalvvarindex; 

//-----------------------------------------------------
// Initialize the first tablenumber (=<60)
//-----------------------------------------------------

void settablenumber(ltableindex) 
   int ltableindex;
{  
   globaltableindex = ltableindex + BASEINDEX;
}

//-----------------------------------------------------
// Initialize the first v-variable number (=<30)
//-----------------------------------------------------
void setvvarnumber(lvvarindex) 
   int lvvarindex;
{  
   globalvvarindex = lvvarindex;
}

//-----------------------------------------------------
// Assign the next tablenumber and do settable()
//-----------------------------------------------------
int settablename(lnumelements,ltablearray)
   int lnumelements,ltablearray[];
{   
   int tableindex = globaltableindex;  
   globaltableindex = globaltableindex - 1;
   settable(tableindex,lnumelements,ltablearray);
   return tableindex; 
} 

//-----------------------------------------------------
// Assign the next v-variable number and assign "zero"
//-----------------------------------------------------

int setvvarname() 
{  
   extern int dps_flag; 
   int vvarindex = globalvvarindex;
   if (dps_flag != 1) vvarindex = vvarindex + 60;
   globalvvarindex = globalvvarindex - 1;
   assign(vvarindex,zero);   
   return vvarindex; 
}

//-----------------------------------------------------------
// Initialize a Dynamic codeint with a (double) Phase Value,
// and Minimum Stepsize. 
//-----------------------------------------------------------

int initphase(double value, double phasestep)
{  
   int phasename = setvvarname();   
   while (value < 0.0 ) value = value + 360.0;
   while (value >= 360.0) value = value - 360.0;
   int value1 = (int) (value/phasestep + 0.1);
   value = (double) value1;   
   initval(value,phasename);
   return(phasename);
} 

//------------------------------------------------------------
// General Pulse Sequence Functions
//------------------------------------------------------------

//-----------------------------------------------------------
// Set the Amplitude
//-----------------------------------------------------------

void gen_RFpwrf(double amp, int rfch)
{
if (rfch == 1) {
      obspwrf(amp);  
   }
   else if (rfch == 2) {
      decpwrf(amp);
   }
   else if (rfch == 3) { 
      dec2pwrf(amp);
   }
   else if (rfch == 4) {
      dec3pwrf(amp);
   }
   else { 
      printf("gen_RFpwrf Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
} 

//------------------------------------------------------------
// Set the Power
//------------------------------------------------------------

void gen_RFpower(double pwr, int rfch)
{
if (rfch == 1) {
      obspower(pwr);  
   }
   else if (rfch == 2) {
      decpower(pwr);
   }
   else if (rfch == 3) { 
      dec2power(pwr);
   }
   else if (rfch == 4) {
      dec3power(pwr);
   }
   else { 
      printf("gen_RFpower Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
} 

//-------------------------------------------------------------
// Unblank the Transmitter 
//-------------------------------------------------------------

void gen_RFunblank(int rfch)
{
if (rfch == 1) {
      obsunblank();  
   }
   else if (rfch == 2) {
      decunblank();
   }
   else if (rfch == 3) { 
      dec2unblank();
   }
   else if (rfch == 4) {
      dec3unblank();
   }
   else { 
      printf("gen_RFunblank Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
}

//----------------------------------------------------------
// Blank the Transmitter 
//----------------------------------------------------------

void gen_RFblank(int rfch)
{
if (rfch == 1) {
      obsblank();  
   }
   else if (rfch == 2) {
      decblank();
   }
   else if (rfch == 3) { 
      dec2blank();
   }
   else if (rfch == 4) {
      dec3blank();
   }
   else { 
      printf("gen_RFblank Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
} 

//----------------------------------------------------------
// Set the Quadrature Phase 
//----------------------------------------------------------

void gen_RFphase( int phase, int rfch)
{
if (rfch == 1) {
      txphase(phase);  
   }
   else if (rfch == 2) {
      decphase(phase);
   }
   else if (rfch == 3) { 
      dec2phase(phase);
   }
   else if (rfch == 4) {
      dec3phase(phase);
   }
   else { 
      printf("genRFphase Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
} 

//--------------------------------------------------------------
// Set the Fine Phase
//--------------------------------------------------------------

void gen_RFfphase( int phase, int rfch)
{
if (rfch == 1) {
      xmtrphase(phase);  
   }
   else if (rfch == 2) {
      dcplrphase(phase);
   }
   else if (rfch == 3) { 
      dcplr2phase(phase);
   }
   else if (rfch == 4) {
      dcplr3phase(phase);
   }
   else { 
      printf("gen_RFfphase Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
} 

//----------------------------------------------------
// Unblank and Turn the RF On. 
//----------------------------------------------------

void gen_RFon(int rfch)
{
if (rfch == 1) {     
      xmtron();  
   }
   else if (rfch == 2) {      
      decon();
   }
   else if (rfch == 3) {
     
      dec2on();
   }
   else if (rfch == 4) {   
      dec3on();
   }
   else { 
      printf("gen_RFon Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
}
 
//-----------------------------------------------------------  
// Turn the RF Off and Be Sure the Transmitter Stays Unblanked
//-----------------------------------------------------------

void gen_RFoff(int rfch)
{
if (rfch == 1) {
      xmtroff();       
   }
   else if (rfch == 2) {
      decoff();     
   }
   else if (rfch == 3) {
      dec2off();     
   }
   else if (rfch == 4) {
      dec3off();     
   }
   else { 
      printf("gen_RFoff Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
}

//-----------------------------------------------------------
// General RF Pulse Created From General Pulse Sequence functions
//-----------------------------------------------------------

void gen_RFPulse(double pw, int phase, double r1, double r2, int rfch)
{
   gen_RFunblank(rfch);
   gen_RFphase(phase, rfch);
   delay(r1);
   gen_RFon(rfch);
   delay(pw);
   gen_RFoff(rfch);
   delay(r2); 
}
                                         
//-------------------------------------------------------------
// Structures for Multiple-Pulse Objects
//-------------------------------------------------------------

//-------------------------------------------------------------
// The WMPA Structure
//-------------------------------------------------------------

typedef struct {
   char   seqName[NSUFFIX]; //channel-name + suffix, (c.f.) Xbr24.
   double   npa;            //200 ns steps during each acquire().
   double   q;              //cycles in the nested loop.
   double   a;              //amplitude. 
   double   r1;             //prepulse delay.
   double   pw;             //pulse delay. 
   double   r2;             //postpulse delay.
   double   r3;             //rof3 or 2.0us by default.
   double   tau;            //small-tau, (i.e.) {pw + delay}. 
   double   rtau;           //tau obtained from 1/srate.
   double   dtau;           //dummy acquisition delay.
   double   dtaua;          //acquisiton delay.
   double   t1;             //extra delay t1.
   double   t2;             //extra delay t2.
   char   ch[NCH];          //channel.
   double   cycles;         //number of MP cycles.
   int   vcycles;           //RT v-pointer for the number of cycles.
   int   vcount;            //RT v-pointer for the cycles loop counter. 
   int   vcycles1;          //RT v-pointer for cycles of a nested loop.
   int   vcount1;           //RT v-pointer for count of a nested loop.  
   int   vphase;            //RT v-pointer for the total phase.
   int   istep;             //RT v-pointer for index of MP phase cycle.
   int   vstep;             //RT v-pointer for the MP phase cycle.
   int   tstep;             //table-pointer of the MP phase cycle.
   int   va;                //RT v-pointer a.
   int   vb;                //RT v-pointer b. 
   int   vc;                //RT v-pointer c.
   int   vd;                //RT v-pointer d. 
   
} WMPA;

//----------------------------------------------------------
// Implementation Functions for Multiple-Pulse Objects
//----------------------------------------------------------

//----------------------------------------------------------
// Implement BR24
//----------------------------------------------------------

WMPA getbr24(char *seqName) 
{
   WMPA mp; 
   char *var; 
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("getbr24 Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,seqName);

// chXsuffix 

   Getstr(getname("ch",seqName,""),mp.ch,sizeof(mp.ch)); 
    
// aXsuffix 

   var = getname("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname("npa",mp.seqName,"");
   mp.npa = getval(var);  
    
// r1Xsuffix

   var = getname("r1",mp.seqName,""); 
   mp.r1 = getval(var); 

// mp.pwXsuffix

   var = getname("pw",mp.seqName,""); 
   mp.pw = getval(var); 

// r2Xsuffix

   var = getname("r2",mp.seqName,"");
   mp.r2 = getval(var); 

// r3Xsuffix

   var = getname("r3",mp.seqName,"");
   mp.r3 = getval(var); 

// mp.tauXsuffix

   var = getname("tau",mp.seqName,""); 
   mp.tau = getval(var); 

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtau = mp.tau - mp.pw - mp.r1 - mp.r2;
   mp.dtaua = mp.dtau - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getbr24 Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set the Number of Br24 Cycles and create V-variables for the Loop. 
  
   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) (int) (np/(36.0*mp.tau*2.0*sw)); 
   initval(cycles, mp.vcycles);
   mp.cycles = cycles; 

// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps. 

   mp.vphase = setvvarname();  
   mp.vstep = setvvarname();
   mp.istep = setvvarname();  
   initval(0.0, mp.istep); 
   int table1[24] = {0,1,3,2,2,1,3,0,1,0,2,3,3,0,1,0,2,3,3,0,2,1,2,1};
   mp.tstep = settablename(24,table1);
   
   return mp;
}

//----------------------------------------------------------------
// Implement MREV8
//----------------------------------------------------------------

WMPA getmrev8(char *seqName) 
{
   WMPA mp; 
   char *var; 
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("getmrev8 Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,seqName);

// chXsuffix 

   Getstr(getname("ch",seqName,""),mp.ch,sizeof(mp.ch)); 
    
// aXsuffix 

   var = getname("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname("npa",mp.seqName,"");
   mp.npa = getval(var);  
    
// r1Xsuffix

   var = getname("r1",mp.seqName,""); 
   mp.r1 = getval(var); 

// mp.pwXsuffix

   var = getname("pw",mp.seqName,""); 
   mp.pw = getval(var); 

// r2Xsuffix

   var = getname("r2",mp.seqName,"");
   mp.r2 = getval(var); 

// r3Xsuffix

   var = getname("r3",mp.seqName,"");
   mp.r3 = getval(var); 

// mp.tauXsuffix

   var = getname("tau",mp.seqName,""); 
   mp.tau = getval(var); 

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtau = mp.tau - mp.pw - mp.r1 - mp.r2;
   mp.dtaua = mp.dtau - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getmrev8 Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set the Number of MREV8 Cycles and create V-variables for the Loop. 
  
   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) (int) (np/(12.0*mp.tau*2.0*sw)); 
   initval(cycles, mp.vcycles);
   mp.cycles = cycles; 

// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps. 

   mp.vphase = setvvarname();  
   mp.vstep = setvvarname();
   mp.istep = setvvarname();  
   initval(0.0, mp.istep); 
   int table1[8] = {0,3,1,2,2,3,1,0};
   mp.tstep = settablename(8,table1);
   
   return mp;
}

//------------------------------------------------------
// Implement SWWHH4
//------------------------------------------------------

WMPA getswwhh4(char *seqName) 
{
   WMPA mp; 
   char *var; 
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("getswwhh4 Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,seqName);

// chXsuffix 

   Getstr(getname("ch",seqName,""),mp.ch,sizeof(mp.ch)); 
    
// aXsuffix 

   var = getname("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname("npa",mp.seqName,"");
   mp.npa = getval(var);  
    
// r1Xsuffix

   var = getname("r1",mp.seqName,""); 
   mp.r1 = getval(var); 

// mp.pwXsuffix

   var = getname("pw",mp.seqName,""); 
   mp.pw = getval(var); 

// r2Xsuffix

   var = getname("r2",mp.seqName,"");
   mp.r2 = getval(var); 

// r3Xsuffix

   var = getname("r3",mp.seqName,"");
   mp.r3 = getval(var); 

// tauXsuffix

   var = getname("tau",mp.seqName,""); 
   mp.tau = getval(var); 

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtau = mp.tau - mp.pw - mp.r1 - mp.r2;
   mp.dtaua = mp.dtau - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getswwhh4 Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set the Number of WHH4 Cycles and create V-variables for the Loop. 
  
   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) (int) (np/((4.0*mp.tau + 2.0*mp.pw)*2.0*sw)); 
   initval(cycles, mp.vcycles);
   mp.cycles = cycles; 

// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps. 

   mp.vphase = setvvarname();  
   mp.vstep = setvvarname();
   mp.istep = setvvarname();  
   initval(0.0, mp.istep); 
   int table1[4] = {0,1,3,2};
   mp.tstep = settablename(4,table1);
   
   return mp;
}

//------------------------------------------------------------
// Implement XX
//------------------------------------------------------------

WMPA getxx(char *seqName) 
{
   WMPA mp; 
   char *var; 
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("getxx Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }   
  
   sprintf(mp.seqName,seqName);

// chXsuffix 

   Getstr(getname("ch",seqName,""),mp.ch,sizeof(mp.ch)); 
   
// aXsuffix 

   var = getname("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname("npa",mp.seqName,"");
   mp.npa = getval(var);  
    
// r1Xsuffix

   var = getname("r1",mp.seqName,""); 
   mp.r1 = getval(var); 

// mp.pwXsuffix

   var = getname("pw",mp.seqName,""); 
   mp.pw = getval(var); 

// r2Xsuffix

   var = getname("r2",mp.seqName,"");
   mp.r2 = getval(var); 

// r3Xsuffix

   var = getname("r3",mp.seqName,"");
   mp.r3 = getval(var); 

// tauXsuffix

   var = getname("tau",mp.seqName,""); 
   mp.tau = getval(var); 

// Calculate mp.dtaua - Abort if ((mp.dtaua) < 0.0)

   mp.dtaua = mp.tau - mp.pw - mp.r1 - mp.r2 - mp.r3 - mp.npa*1.0e-7;

   if ((mp.dtaua) <= 0.0) {
      printf("getxx4 Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set the Number of XX Cycles and create V-variables for the Loop. 
  
   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) (int) (np/((2.0*mp.tau)*2.0*sw)); 
   initval(cycles, mp.vcycles);
   mp.cycles = cycles; 

// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps. 

   mp.vphase = setvvarname();  
   mp.vstep = setvvarname();
   mp.istep = setvvarname();  
   initval(0.0, mp.istep); 
   int table1[2] = {0,0};
   mp.tstep = settablename(2,table1);
   
   return mp;
}

//-----------------------------------------------------
// Implement XmX
//-----------------------------------------------------

WMPA getxmx(char *seqName) 
{
   WMPA mp; 
   char *var; 
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("getxmx Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }   
  
   sprintf(mp.seqName,seqName);

// chXsuffix 

   Getstr(getname("ch",seqName,""),mp.ch,sizeof(mp.ch)); 
   
// aXsuffix 

   var = getname("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname("npa",mp.seqName,"");
   mp.npa = getval(var);  
    
// r1Xsuffix

   var = getname("r1",mp.seqName,""); 
   mp.r1 = getval(var); 

// mp.pwXsuffix

   var = getname("pw",mp.seqName,""); 
   mp.pw = getval(var); 

// r2Xsuffix

   var = getname("r2",mp.seqName,"");
   mp.r2 = getval(var); 

// r3Xsuffix

   var = getname("r3",mp.seqName,"");
   mp.r3 = getval(var); 

// tauXsuffix

   var = getname("tau",mp.seqName,""); 
   mp.tau = getval(var); 

// Calculate mp.dtaua - Abort if ((mp.dtaua) < 0.0)

   mp.dtaua = mp.tau - mp.pw - mp.r1 - mp.r2 - mp.r3 - mp.npa*1.0e-7;

   if ((mp.dtaua) <= 0.0) {
      printf("getxmx4 Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set the Number of XX Cycles and create V-variables for the Loop. 
  
   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) (int) (np/((2.0*mp.tau)*2.0*sw)); 
   initval(cycles, mp.vcycles);
   mp.cycles = cycles; 

// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps. 

   mp.vphase = setvvarname();  
   mp.vstep = setvvarname();
   mp.istep = setvvarname();  
   initval(0.0, mp.istep); 
   int table1[2] = {0,2};
   mp.tstep = settablename(2,table1);
   
   return mp;
}

//---------------------------------------------------------
// Implement TOSS4
//---------------------------------------------------------

WMPA gettoss4(char *seqName) 
{
   WMPA mp; 
   char *var; 
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("gettoss4 Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,seqName);

// chXsuffix 

   Getstr(getname("ch",seqName,""),mp.ch,sizeof(mp.ch)); 
    
// aXsuffix 

   var = getname("a",mp.seqName,"");
   mp.a = getval(var);

// pwXsuffix

   var = getname("pw",mp.seqName,""); 
   mp.pw = getval(var); 

// mp.rtau

   mp.rtau = 1.0/getval("srate"); 
   if (mp.rtau > 0.002) {   
      printf("gettoss4 Error: srate < 500. Abort!\n");
      psg_abort(1);
   }
   
// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps. 

   mp.vphase = setvvarname();  
   mp.vstep = setvvarname();
   mp.istep = setvvarname();  
   initval(0.0, mp.istep); 
   int table1[4] = {0,1,0,1};
   mp.tstep = settablename(4,table1);
   
   return mp;
}

// Implement Interrupted Decoupling with Refocussing

WMPA getidref(char *seqName) 
{
   WMPA mp;
   char *var; 
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("getidref Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,seqName);

// chXsuffix

   Getstr(getname("ch",seqName,""),mp.ch,sizeof(mp.ch));
    
// aXsuffix

   var = getname("a",mp.seqName,"");
   mp.a = getval(var);

// pwXsuffix

   var = getname("pw",mp.seqName,""); 
   mp.pw = getval(var); 

// mp.rtau

   mp.rtau = 1.0/getval("srate"); 
   if (mp.rtau > 0.002) {   
      printf("getidref Error: srate < 500. Abort!\n");
      psg_abort(1);
   }

// t1Xsuffix

   var = getname("t1",mp.seqName,""); 
   mp.t1 = getval(var); 

   return mp;
}

//-------------------------------------------------------
// Implement Windowed PMLG 
//-------------------------------------------------------

WMPA getwpmlg(char *seqName) 
{
   WMPA mp; 
   char *var; 
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("getwpmlg Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,seqName);

// chXsuffix 

   Getstr(getname("ch",seqName,""),mp.ch,sizeof(mp.ch)); 
    
// aXsuffix 

   var = getname("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname("npa",mp.seqName,"");
   mp.npa = getval(var);  
    
// r1Xsuffix

   var = getname("r1",mp.seqName,""); 
   mp.r1 = getval(var); 

// pwXsuffix

   var = getname("pw",mp.seqName,""); 
   mp.pw = getval(var); 

// r2Xsuffix

   var = getname("r2",mp.seqName,"");
   mp.r2 = getval(var); 

// r3Xsuffix

   var = getname("r3",mp.seqName,"");
   mp.r3 = getval(var); 

// tauXsuffix

   var = getname("tau",mp.seqName,""); 
   mp.tau = getval(var); 
   
// qXsuffix

   var = getname("q",mp.seqName,""); 
   mp.q = getval(var); 

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtaua = mp.tau - 2.0*mp.pw - mp.r1 - mp.r2 - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getwpmlg Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set PMLG Phase Array   

   double sign = 1;
   if (mp.q < 0.0) { 
      mp.q = -mp.q;
      sign = - sign; 
   } 
   double obsstep = 360.0/8192;    
   double delta = 360.0/(sqrt(3)*mp.q); 
   double temp; 
   int i;
   int table1[512]; 
   for (i = 0; i < mp.q; i++) {
      temp = sign*(i*delta + delta/2.0);
      temp = roundphase(temp,obsstep);
      table1[i] = (int) roundoff(temp/obsstep,1);       
   }   
   for (i = mp.q; i < 2*mp.q; i++) {
      temp = 180.0 + sign*((2*mp.q - i)*delta - delta/2.0);
      temp = roundphase(temp,obsstep);
      table1[i] = (int) roundoff(temp/obsstep,1);  
   }  
   
// Set PMLG pulse  
  
   mp.pw  = roundoff(mp.pw/mp.q, 0.0125e-6); 
   
// Double mp.q for +/- phase steps and and create v-vars for the nested loop. 

   mp.vcycles1 = setvvarname();
   mp.vcount1 = setvvarname();
   initval(((double) 2.0*mp.q), mp.vcycles1);
   int q  = mp.q*2;
   
// Set the Number of WPMLG Cycles and create V-vars for the main loop. 
  
   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) (int) (np/(mp.tau*2.0*sw)); 
   initval(cycles, mp.vcycles);
   mp.cycles = cycles; 

// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps. 

   mp.vphase = setvvarname();  
   mp.vstep = setvvarname();
   mp.istep = setvvarname();  
   initval(0.0, mp.istep); 
   mp.tstep = settablename(q,table1);

// Set mp.va and initialize 2048 steps (90 degrees)

   mp.va = setvvarname();
   initval(2048.0, mp.va);
   
   return mp;
}

//---------------------------------------------------
// Implement Windowed DUMBO 
//---------------------------------------------------

WMPA getwdumbo(char *seqName) 
{
   WMPA mp; 
   char *var; 
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("getwdumbo Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,seqName);

// chXsuffix 

   Getstr(getname("ch",seqName,""),mp.ch,sizeof(mp.ch)); 
    
// aXsuffix 

   var = getname("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname("npa",mp.seqName,"");
   mp.npa = getval(var);  
    
// r1Xsuffix

   var = getname("r1",mp.seqName,""); 
   mp.r1 = getval(var); 

// pwXsuffix

   var = getname("pw",mp.seqName,""); 
   mp.pw = getval(var); 

// r2Xsuffix

   var = getname("r2",mp.seqName,"");
   mp.r2 = getval(var); 

// r3Xsuffix

   var = getname("r3",mp.seqName,"");
   mp.r3 = getval(var); 

// tauXsuffix

   var = getname("tau",mp.seqName,""); 
   mp.tau = getval(var); 

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtaua = mp.tau - mp.pw - mp.r1 - mp.r2 - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getwdumbo Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set DUMBO Phase Array 

   double obsstep = 360.0/8192;
   double dumbo[64] = {31.824000, 111.959215, 140.187768, 118.272283,
   86.096791,   73.451274,  71.082669,  53.716611,  21.276000,   2.170197,
   12.992428,   28.379246,   5.986118, 302.048261, 248.763031, 264.564610,
   348.552000,  74.378423,  94.391707,  45.146603, 343.815209, 323.373522, 
   341.072484, 353.743630, 333.756000, 296.836742, 274.284097, 271.756242,
   268.693882, 255.782366, 257.225816, 304.420775, 124.420775,  77.225816,
    75.782366,  88.693882,  91.756242,  94.284097, 116.836742, 153.756000,
   173.743630, 161.072484, 143.373522, 163.815209, 225.146603, 274.391707,
   254.378423, 168.552000,  84.564610,  68.763031, 122.048261, 185.986118,
   208.379246, 192.992428, 182.170197, 201.276000, 233.716611, 251.082669,
   253.451274, 266.096791, 298.272283, 320.187768, 291.959215, 211.824000}; 

   int i;
   double temp;
   int table1[64]; 
   for (i = 0; i < 64; i++) { 
      temp = roundphase((dumbo[i]),obsstep);
      table1[i] = (int) roundoff(temp/obsstep,1);  
   } 
  
// Set DUMBO pulse
  
   mp.pw  = roundoff(mp.pw/64.0, 0.0125e-6);
   
// Double mp.q for +/- phase steps and and create v-vars for the nested loop. 

   mp.vcycles1 = setvvarname();
   mp.vcount1 = setvvarname();
   initval(64.0, mp.vcycles1);
   mp.q = 64; 
   
// Set the Number of DUMBO Cycles and create V-vars for the main loop. 
  
   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) (int) (np/(mp.tau*2.0*sw)); 
   initval(cycles, mp.vcycles);
   mp.cycles = cycles; 

// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps. 

   mp.vphase = setvvarname();  
   mp.vstep = setvvarname();
   mp.istep = setvvarname();  
   initval(0.0, mp.istep); 
   mp.tstep = settablename(64,table1);

// Add Phase Offset to the Table

// phXsuffix

   var = getname("ph",mp.seqName,""); 
   double ph = getval(var);
   temp =  roundphase(ph,obsstep);
   int phint = (int) roundoff(temp/obsstep,1);
   tsadd(mp.tstep,phint,8192);

// Set mp.va and initialize 2048 steps (90 degrees)

   mp.va = setvvarname();
   initval(2048.0, mp.va);
     
   return mp;
}

//----------------------------------------------
// Implement Windowed DUMBO With Tilt Pulses
//----------------------------------------------

WMPA getwdumbot(char *seqName) 
{
   WMPA mp; 
   char *var; 
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("getwdumbot Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,seqName);

// chXsuffix 

   Getstr(getname("ch",seqName,""),mp.ch,sizeof(mp.ch)); 
    
// aXsuffix 

   var = getname("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname("npa",mp.seqName,"");
   mp.npa = getval(var);  
    
// r1Xsuffix

   var = getname("r1",mp.seqName,""); 
   mp.r1 = getval(var); 

// pw1Xsuffix

   var = getname("pw1",mp.seqName,""); 
   mp.pw = getval(var);                // note the prefix shift

// pw2Xsuffix

   var = getname("pw2",mp.seqName,""); 
   mp.t1 = getval(var);                 // note the prefix shift

// r2Xsuffix

   var = getname("r2",mp.seqName,"");
   mp.r2 = getval(var); 

// r3Xsuffix

   var = getname("r3",mp.seqName,"");
   mp.r3 = getval(var); 

// tauXsuffix

   var = getname("tau",mp.seqName,""); 
   mp.tau = getval(var); 

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtaua = mp.tau - mp.pw - mp.r1 - mp.r2 - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getwdumbot Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set DUMBO Phase Array 

   double obsstep = 360.0/8192;
   double dumbo[64] = {31.824000, 111.959215, 140.187768, 118.272283,
   86.096791,   73.451274,  71.082669,  53.716611,  21.276000,   2.170197,
   12.992428,   28.379246,   5.986118, 302.048261, 248.763031, 264.564610,
   348.552000,  74.378423,  94.391707,  45.146603, 343.815209, 323.373522, 
   341.072484, 353.743630, 333.756000, 296.836742, 274.284097, 271.756242,
   268.693882, 255.782366, 257.225816, 304.420775, 124.420775,  77.225816,
    75.782366,  88.693882,  91.756242,  94.284097, 116.836742, 153.756000,
   173.743630, 161.072484, 143.373522, 163.815209, 225.146603, 274.391707,
   254.378423, 168.552000,  84.564610,  68.763031, 122.048261, 185.986118,
   208.379246, 192.992428, 182.170197, 201.276000, 233.716611, 251.082669,
   253.451274, 266.096791, 298.272283, 320.187768, 291.959215, 211.824000}; 

   int i;
   double temp;
   int table1[64]; 
   for (i = 0; i < 64; i++) { 
      temp = roundphase((dumbo[i]),obsstep);
      table1[i] = (int) roundoff(temp/obsstep,1);  
   } 
  
// Set DUMBO pulse
  
   mp.pw  = roundoff(mp.pw/64.0, 0.0125e-6);
   
// Double mp.q for +/- phase steps and and create v-vars for the nested loop. 

   mp.vcycles1 = setvvarname();
   mp.vcount1 = setvvarname();
   initval(64.0, mp.vcycles1);
   mp.q = 64; 
   
// Set the Number of DUMBO Cycles and create V-vars for the main loop. 
  
   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) (int) (np/(mp.tau*2.0*sw)); 
   initval(cycles, mp.vcycles);
   mp.cycles = cycles; 

// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps. 

   mp.vphase = setvvarname();  
   mp.vstep = setvvarname();
   mp.istep = setvvarname();  
   initval(0.0, mp.istep); 
   mp.tstep = settablename(64,table1);

// Add Phase Offset to the Table

// phXsuffix

   var = getname("ph",mp.seqName,""); 
   double ph = getval(var);
   temp =  roundphase(ph,obsstep);
   int phint = (int) roundoff(temp/obsstep,1);
   tsadd(mp.tstep,phint,8192);

// Set mp.va to mp.vc and initialize mp.va at 2048 steps (90 degrees)

   mp.va = setvvarname();
   mp.vb = setvvarname();
   mp.vc = setvvarname();
   initval(2048.0, mp.va);
   
   return mp;
}
//----------------------------------------------------
// Underscore Functions for Multiple-Pulse Objects
//----------------------------------------------------

//----------------------------------------------------
// BR24
//----------------------------------------------------

void _br24(WMPA mp, int phase)
{
   int chnl = 0;
   if (!strcmp(mp.ch,"obs")) chnl = 1;
   else if (!strcmp(mp.ch,"dec")) chnl = 2;
   else if (!strcmp(mp.ch,"dec2")) chnl = 3;
   else if (!strcmp(mp.ch,"dec3")) chnl = 4;
   else {
      printf("_br24 Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
   getelem(phase,ct,mp.vphase);
   if (mp.cycles > 0) {
      gen_RFpwrf(mp.a,chnl); 
      gen_RFunblank(chnl);      
      loop(mp.vcycles,mp.vcount);
         rcvron();
         delay(mp.tau + mp.dtaua);
         acquire(mp.npa,2.0e-7);
         rcvroff();
         sub(mp.istep,mp.istep,mp.istep);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);    
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // x 
    
         delay(mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // y

         delay(mp.tau + mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);  
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // -y

         delay(mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);  
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // -x

         delay(mp.tau + mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);  
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // -x 
      
         delay(mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);     
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // y  
  
         delay(mp.tau + mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);     
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // -y  
   
         delay(mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);     
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // x  
   
         delay(mp.tau + mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);           
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // y  
   
         delay(mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);     
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // x 
    
         delay(mp.tau + mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);     
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // -x 
   
         delay(mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);     
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // -y  
   
         delay(mp.dtau + mp.dtau); 
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);         
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // -y  
  
         delay(mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);     
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // x  
  
         delay(mp.tau + mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);     
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // y 
  
         delay(mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);  
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // x   
  
         delay(mp.tau + mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);     
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // -x   
 
         delay(mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);     
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // -y  

         delay(mp.tau + mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);     
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // -y 

         delay(mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);     
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // x 
 
         delay(mp.tau + mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);     
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // -x 

         delay(mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);     
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // y  

         delay(mp.tau + mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);     
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // -x 
  
         delay(mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);     
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // y
         gen_RFblank(chnl);       
      endloop(mp.vcount); 
      gen_RFunblank(chnl);
   }
} 
//------------------------------------------------------
// MREV8
//------------------------------------------------------

void _mrev8(WMPA mp, int phase)
{
   int chnl = 0;
   if (!strcmp(mp.ch,"obs")) chnl = 1;
   else if (!strcmp(mp.ch,"dec")) chnl = 2;
   else if (!strcmp(mp.ch,"dec2")) chnl = 3;
   else if (!strcmp(mp.ch,"dec3")) chnl = 4;
   else {
      printf("_mrev8 Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
   getelem(phase,ct,mp.vphase);
   if (mp.cycles > 0) {
      gen_RFpwrf(mp.a,chnl); 
      loop(mp.vcycles,mp.vcount);
         gen_RFblank(chnl);
         rcvron();
         delay(mp.tau + mp.dtaua);
         acquire(mp.npa,2.0e-7);
         rcvroff();
         sub(mp.istep,mp.istep,mp.istep);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);    
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // x 
    
         delay(mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // -y

         delay(mp.tau + mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);  
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // y

         delay(mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);  
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // -x

         delay(mp.tau + mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);  
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // -x 
      
         delay(mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);     
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // -y  
  
         delay(mp.tau + mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);     
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // y  
   
         delay(mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);     
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // x  
      endloop(mp.vcount);
   }
} 
//--------------------------------------------------------
// SWWHH4
//--------------------------------------------------------
void _swwhh4(WMPA mp, int phase)
{
   int chnl = 0;
   if (!strcmp(mp.ch,"obs")) chnl = 1;
   else if (!strcmp(mp.ch,"dec")) chnl = 2;
   else if (!strcmp(mp.ch,"dec2")) chnl = 3;
   else if (!strcmp(mp.ch,"dec3")) chnl = 4;
   else {
      printf("_swwhh4 Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
   getelem(phase,ct,mp.vphase);
   if (mp.cycles > 0) {
      gen_RFpwrf(mp.a,chnl); 
      loop(mp.vcycles,mp.vcount);
         gen_RFblank(chnl);
         rcvron();
         delay(mp.tau + mp.dtaua);
         acquire(mp.npa,2.0e-7);
         rcvroff();
         gen_RFunblank(chnl);
         sub(mp.istep,mp.istep,mp.istep);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFphase(mp.vstep, chnl);    
         delay(mp.r1);
         gen_RFon(chnl);                             // x 
         delay(mp.pw);
         
         // no window
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);         
         gen_RFphase(mp.vstep, chnl);                // y
         delay(mp.pw);
         gen_RFoff(chnl);
         delay(mp.r2);

         delay(mp.tau + mp.dtau);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);  
         gen_RFphase(mp.vstep, chnl);    
         delay(mp.r1);
         gen_RFon(chnl);                            // -y
         delay(mp.pw);

         // no window
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFphase(mp.vstep, chnl);               // X
         delay(mp.pw);
         gen_RFoff(chnl);
         delay(mp.r2);                  
      endloop(mp.vcount);
   }
} 

//------------------------------------------------------
// XX
//------------------------------------------------------

void _xx(WMPA mp, int phase)
{
   int chnl = 0;
   if (!strcmp(mp.ch,"obs")) chnl = 1;
   else if (!strcmp(mp.ch,"dec")) chnl = 2;
   else if (!strcmp(mp.ch,"dec2")) chnl = 3;
   else if (!strcmp(mp.ch,"dec3")) chnl = 4;
   else {
      printf("_xx Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
   getelem(phase,ct,mp.vphase);
   if (mp.cycles > 0) {
      gen_RFpwrf(mp.a,chnl); 
      loop(mp.vcycles,mp.vcount);
         gen_RFblank(chnl);
	 delay(mp.r2); 
         rcvron();
         delay(mp.dtaua);
         acquire(mp.npa,2.0e-7);
         rcvroff();
         gen_RFunblank(chnl);
         sub(mp.istep,mp.istep,mp.istep);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFphase(mp.vstep, chnl);    
         delay(mp.r1);
         gen_RFon(chnl);                             // x 
         delay(mp.pw);
	 gen_RFoff(chnl);
	 
	 gen_RFblank(chnl);
	 delay(mp.r2);
         rcvron();
         delay(mp.dtaua);
         acquire(mp.npa,2.0e-7);
         rcvroff();
         gen_RFunblank(chnl);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFphase(mp.vstep, chnl);    
         delay(mp.r1);
         gen_RFon(chnl);                             // x 
         delay(mp.pw); 
	 gen_RFoff(chnl);                      
      endloop(mp.vcount);
   }
} 
//----------------------------------------------------
// XmX
//----------------------------------------------------

void _xmx(WMPA mp, int phase)
{
   int chnl = 0;
   if (!strcmp(mp.ch,"obs")) chnl = 1;
   else if (!strcmp(mp.ch,"dec")) chnl = 2;
   else if (!strcmp(mp.ch,"dec2")) chnl = 3;
   else if (!strcmp(mp.ch,"dec3")) chnl = 4;
   else {
      printf("_xx Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
   getelem(phase,ct,mp.vphase);
   if (mp.cycles > 0) {
      gen_RFpwrf(mp.a,chnl); 
      loop(mp.vcycles,mp.vcount);
         gen_RFblank(chnl);
	 delay(mp.r2); 
         rcvron();
         delay(mp.dtaua);
         acquire(mp.npa,2.0e-7);
         rcvroff();
         gen_RFunblank(chnl);
         sub(mp.istep,mp.istep,mp.istep);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFphase(mp.vstep, chnl);    
         delay(mp.r1);
         gen_RFon(chnl);                             // x 
         delay(mp.pw);
	 gen_RFoff(chnl);
	 
	 gen_RFblank(chnl);
	 delay(mp.r2);
         rcvron();
         delay(mp.dtaua);
         acquire(mp.npa,2.0e-7);
         rcvroff();
         gen_RFunblank(chnl);
         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFphase(mp.vstep, chnl);    
         delay(mp.r1);
         gen_RFon(chnl);                             // -x 
         delay(mp.pw);
	 gen_RFoff(chnl);                        
      endloop(mp.vcount);
   }
} 

//---------------------------------------------
// TOSS4
//---------------------------------------------

void _toss4(WMPA mp, int phase)
{
   int chnl = 0;
   if (!strcmp(mp.ch,"obs")) chnl = 1;
   else if (!strcmp(mp.ch,"dec")) chnl = 2;
   else if (!strcmp(mp.ch,"dec2")) chnl = 3;
   else if (!strcmp(mp.ch,"dec3")) chnl = 4;
   else {
      printf("_toss4 Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
   
   getelem(t2,ct,mp.vphase); 
   gen_RFpwrf(mp.a, chnl);
   gen_RFunblank(chnl);
   getelem(mp.tstep,mp.istep,mp.vstep);
   incr(mp.istep);
   add(mp.vphase,mp.vstep,mp.vstep); 
   gen_RFphase(mp.vstep, chnl);
   delay((0.1226*mp.rtau) - mp.pw/2.0);
   gen_RFon(chnl);                         //-x
   delay(mp.pw);
   gen_RFoff(chnl);  

   getelem(mp.tstep,mp.istep,mp.vstep);
   incr(mp.istep);
   add(mp.vphase,mp.vstep,mp.vstep); 
   gen_RFphase(mp.vstep, chnl);
   delay((0.0773*mp.rtau) - mp.pw);
   gen_RFon(chnl);                         //-x
   delay(mp.pw);
   gen_RFoff(chnl);

   getelem(mp.tstep,mp.istep,mp.vstep);
   incr(mp.istep);
   add(mp.vphase,mp.vstep,mp.vstep); 
   gen_RFphase(mp.vstep, chnl);
   delay((0.2236*mp.rtau) - mp.pw);
   gen_RFon(chnl);                         //-x
   delay(mp.pw);
   gen_RFoff(chnl);

   getelem(mp.tstep,mp.istep,mp.vstep);
   incr(mp.istep);
   add(mp.vphase,mp.vstep,mp.vstep); 
   gen_RFphase(mp.vstep, chnl);
   delay((1.0433*mp.rtau) - mp.pw);
   gen_RFon(chnl);                         //-x
   delay(mp.pw);
   gen_RFoff(chnl);

   delay((0.774*mp.rtau) - mp.pw/2.0);
} 

//-------------------------------------------
// Interrupted Decoupling with Refocussing
//-------------------------------------------

void _idref(WMPA mp, DSEQ d, int phase)
{
   int chnl = 0;
   if (!strcmp(mp.ch,"obs")) chnl = 1;
   else if (!strcmp(mp.ch,"dec")) chnl = 2;
   else if (!strcmp(mp.ch,"dec2")) chnl = 3;
   else if (!strcmp(mp.ch,"dec3")) chnl = 4;
   else {
      printf("_idref Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }

   char ch2[5];
   if (!strcmp(d.seq,"tppm")) {
      sprintf(ch2,"%s",d.t.ch); 
   }
   if (!strcmp(d.seq,"spinal")) {
      sprintf(ch2,"%s",d.s.ch);
   }

   int chnl2 = 0;
   if (!strcmp(ch2,"obs")) chnl2 = 1;
   else if (!strcmp(ch2,"dec")) chnl2 = 2;
   else if (!strcmp(ch2,"dec2")) chnl2 = 3;
   else if (!strcmp(ch2,"dec3")) chnl2 = 4;
   else {
      printf("_idref Error: Undefined Decoupler Channel. Abort!\n");
      psg_abort(1);
   }

   if (chnl == chnl2) { 
      printf("_idref Error: Refocussing and Decoupling on Same Channel. Abort!\n");
      psg_abort(1);
   }

   if ((mp.rtau - mp.pw/2.0 - mp.t1) <=0.0) {
      printf("_idref Error: Interrupt Time Too Large. Set t1 = 1.0/srate - pw/2.0");
      mp.t1 = mp.rtau - mp.pw/2.0;
   }

   gen_RFphase(phase, chnl);
   gen_RFpwrf(mp.a, chnl);
   gen_RFunblank(chnl); 
   delay(mp.rtau - mp.pw/2.0 - mp.t1);
   _dseqoff(d);
   delay(mp.t1);
   _dseqon(d); 
   gen_RFon(chnl);
   delay(mp.pw);
   gen_RFoff(chnl); 
   delay(mp.rtau - mp.pw/2.0);
}

//------------------------------------------------------
// Windowed PMLG
//------------------------------------------------------

void _wpmlg(WMPA mp, int phase)
{
   int chnl = 0;
   if (!strcmp(mp.ch,"obs")) chnl = 1;
   else if (!strcmp(mp.ch,"dec")) chnl = 2;
   else if (!strcmp(mp.ch,"dec2")) chnl = 3;
   else if (!strcmp(mp.ch,"dec3")) chnl = 4;
   else {
      printf("_wpmlg Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
   getelem(phase,ct,mp.vphase);
   mult(mp.vphase,mp.va,mp.vphase);      //PMLG Overall Phase
   gen_RFpwrf(mp.a,chnl);
   gen_RFunblank(chnl);
   loop(mp.vcycles,mp.vcount);
      gen_RFblank(chnl);    
      rcvron();
      delay(mp.dtaua);
      acquire(mp.npa,2.0e-7); 
      rcvroff(); 
      gen_RFunblank(chnl); 
      delay(mp.r1);
      sub(mp.istep,mp.istep,mp.istep);
      gen_RFon(chnl);
      loop(mp.vcycles1,mp.vcount1);      //PMLG with mp.cycles steps
         getelem(mp.tstep,mp.istep,mp.vstep);
	 incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFfphase(mp.vstep, chnl); 
         delay(mp.pw);
      endloop(mp.vcount1);
      gen_RFoff(chnl);
      delay(mp.r2);                                 
   endloop(mp.vcount);
   gen_RFunblank(chnl); 
} 

//------------------------------------------------------
// Windowed DUMBO
//------------------------------------------------------

void _wdumbo(WMPA mp, int phase)
{
   int chnl = 0;
   if (!strcmp(mp.ch,"obs")) chnl = 1;
   else if (!strcmp(mp.ch,"dec")) chnl = 2;
   else if (!strcmp(mp.ch,"dec2")) chnl = 3;
   else if (!strcmp(mp.ch,"dec3")) chnl = 4;
   else {
      printf("_wdumbo Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
   getelem(phase,ct,mp.vphase);
   mult(mp.vphase,mp.va,mp.vphase);      //DUMBO Overall Phase
   gen_RFpwrf(mp.a,chnl);
   gen_RFunblank(chnl);
   loop(mp.vcycles,mp.vcount);
      gen_RFblank(chnl);    
      rcvron();
      delay(mp.dtaua);
      acquire(mp.npa,2.0e-7); 
      rcvroff(); 
      gen_RFunblank(chnl); 
      delay(mp.r1);
      sub(mp.istep,mp.istep,mp.istep);
      gen_RFon(chnl);
      loop(mp.vcycles1,mp.vcount1);      //DUMBO with mp.cycles steps
         getelem(mp.tstep,mp.istep,mp.vstep);
	 incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFfphase(mp.vstep,chnl);
         delay(mp.pw);
      endloop(mp.vcount1);
      gen_RFoff(chnl);
      delay(mp.r2);
   endloop(mp.vcount);
   gen_RFunblank(chnl);
}           

//------------------------------------------------------
// Windowed DUMBO with tilt pulses
//------------------------------------------------------

void _wdumbot(WMPA mp, int phase)
{
   int chnl = 0;
   if (!strcmp(mp.ch,"obs")) chnl = 1;
   else if (!strcmp(mp.ch,"dec")) chnl = 2;
   else if (!strcmp(mp.ch,"dec2")) chnl = 3;
   else if (!strcmp(mp.ch,"dec3")) chnl = 4;
   else {
      printf("_wdumbo Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
   getelem(phase,ct,mp.vphase);
   add(mp.vphase,one,mp.vb);
   add(mp.vphase,three,mp.vc);
   mult(mp.vphase,mp.va,mp.vphase);      //DUMBO Overall Phase
   mult(mp.vb,mp.va,mp.vb);              //Tilt 1 Phase
   mult(mp.vc,mp.va,mp.vc);              //Tilt 2 Phase
   gen_RFpwrf(mp.a,chnl);
   gen_RFunblank(chnl);
   loop(mp.vcycles,mp.vcount);
      gen_RFfphase(mp.vb,chnl);
      gen_RFon(chnl);
      delay(mp.t1);                      //Tilt Pulse
      gen_RFoff(chnl);
      gen_RFblank(chnl);    
      rcvron();
      delay(mp.dtaua);
      acquire(mp.npa,2.0e-7); 
      rcvroff(); 
      gen_RFunblank(chnl); 
      delay(mp.r1);
      sub(mp.istep,mp.istep,mp.istep);
      gen_RFfphase(mp.vc,chnl);
      gen_RFon(chnl);
      delay(mp.t1);                      //Tilt Pulse
      loop(mp.vcycles1,mp.vcount1);      //DUMBO with mp.cycles steps
         getelem(mp.tstep,mp.istep,mp.vstep);
	 incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFfphase(mp.vstep,chnl);
         delay(mp.pw);
      endloop(mp.vcount1);
      gen_RFoff(chnl);
      delay(mp.r2);                                                       
   endloop(mp.vcount);
   gen_RFunblank(chnl); 
}
