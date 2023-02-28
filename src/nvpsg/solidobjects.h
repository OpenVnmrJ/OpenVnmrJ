/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* solidobjects.h -- A collection of windowed multiple-pulse objects
   with interleaved acquisition to be used in pulse sequences. Also
   the GP structure allows the programming of pulse sequences modules
   without acquisition.

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

   D. Rice - 01/12/06                                                */

// Contents:
// General Pulse Sequence Functions
// 1.  gen_RFpwrf()
// 2.  gen_RFvpwrf()
// 3.  gen_RFpower()
// 4.  gen_RFunblank()
// 5.  gen_RFblank()
// 6.  gen_RFphase()
// 7.  gen_RFfphase()
// 8.  gen_RFon()
// 9.  gen_RFoff()
// 10. gen_RFpulse()
// 11. gen_RFsimpulse()
//
// Structures
// 1.  WMPA
// 2.  GP

// Functions 
// 1. void OBinitializer
//
// Implementation Functions
// 1. WMPA getbr24()
// 2. WMPA getmrev8()
// 3. WMPA getswwhh4()
// 4. WMPA getxx()
// 5. WMPA getxmx()
// 6. WMPA gettoss4()
// 7. WMPA gettoss5()
// 8. WMPA getidref()
// 9. WMPA getwpmlg()
// 10. GP   getineptyx()
//11. WMPA getwdumbo()
//12. WMPA getwdumbot()
//13. WMPA getcpmg()
//14. GP   gethmqc()
//15. WMPA gethssmall()
//16. WMPA getwsamn()
//17. WMPA getwpmlgxmx()     //use with _wpmlg
//17. WMPA getwdumboxmx()    //use with _wdumbo
//18. WMPA getwdumbogen()
//
// _Underscore Functions
// Implementation Functions
// 1. _br24()
// 2. _mrev8()
// 3. _swwhh4()
// 4. _xx()
// 5. _xmx()
// 6. _toss4()
// 7. _toss5()
// 8. _idref()
// 9. _wpmlg()
// 10. _wdumbo()
//11. _wdumbot()             // do not use with getwdumboxmx()
//12. _inept()
//13. _ineptref()
//14. _cpmg()
//15. _hmqc()
//16. _hssmall()
//17. _wsamn()               // VNMRS ONLY

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

#ifdef NVPSG
// -----------------------------------------------------------
//  Set the Amplitude in a Real Time Loop - VNMRS Only 
// -----------------------------------------------------------

 void gen_RFvpwrf(int amp, int rfch)
 {
 if (rfch == 1) {
      vobspwrf(amp);
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
     printf("gen_vRFpwrf Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
 }
#endif

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

//-----------------------------------------------------------------
// General RF Pulse Created From General Pulse Sequence Functions
//-----------------------------------------------------------------

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

//------------------------------------------------------------------
// General RF Simpulse Created From General Pulse Sequence Functions
//------------------------------------------------------------------

void gen_RFsimPulse(double pwa, double pwb, int phasea, int phaseb, double r1, double r2, int rfcha, int rfchb)
{
   double dpw = 0.0;
   double pw = 0.0;
   int chnl1 = 0;
   int chnl2 = 0; 
   if (pwa > pwb) {
      dpw = (pwa - pwb)/2.0;
      pw = pwb;
      chnl1 = rfcha;
      chnl2 = rfchb;
   }
   else {
      dpw = (pwb - pwa)/2.0;
      pw = pwa;
      chnl1 = rfchb;
      chnl2 = rfcha;
   }

   gen_RFunblank(chnl1); gen_RFunblank(chnl2);
   gen_RFphase(phasea, chnl1); gen_RFphase(phaseb, chnl2);
   if (r1 > 0.0) delay(r1);
   gen_RFon(chnl1);
   if (dpw > 0.0) delay(dpw);
   gen_RFon(chnl2);
   delay(pw);
   gen_RFoff(chnl2);
   if (dpw > 0.0) delay(dpw);
   gen_RFoff(chnl1);
   if (r2 > 0.0) delay(r2);
}

//-------------------------------------------------------------
// Structures for Multiple-Pulse Objects
//-------------------------------------------------------------

//-------------------------------------------------------------
// The WMPA Structure
//-------------------------------------------------------------

typedef struct {
   char   seqName[NSUFFIX]; //channel-name + suffix, (c.f.) Xbr24
   double   npa;            //200 ns steps during each acquire()
   double   npa1;           //200 ns steps for extra acquire()
   double   npa2;           //200 ns steps for extra acquire()
   double   q;              //cycles in the nested loop
   double   a;              //amplitude
   double   r1;             //prepulse delay
   double   pw;             //pulse delay
   double   r2;             //postpulse delay
   double   r3;             //rof3 or 2.0us by default
   double   tau;            //small-tau, (i.e.) {pw + delay}
   double   rtau;           //tau obtained from 1/srate
   double   dtau;           //dummy acquisition delay
   double   dtaua;          //acquisiton delay
   double   t1;             //extra delay t1
   double   t2;             //extra delay t2
   char   ch[NCH];          //channel
   double   cycles;         //number of MP cycles in a main loop
   double   cycles1;        //number of MP cycles in a nested loop
   int   vcycles;           //RT v-pointer for the number of cycles
   int   vcount;            //RT v-pointer for the cycles loop counter
   int   vcycles1;          //RT v-pointer for cycles of a nested loop
   int   vcount1;           //RT v-pointer for count of a nested loop
   int   vphase;            //RT v-pointer for the total phase
   int   istep;             //RT v-pointer for index of MP phase cycle
   int   vstep;             //RT v-pointer for the MP phase cycle
   int   tstep;             //table-pointer of the MP phase cycle
   int   va;                //RT v-pointer a
   int   vb;                //RT v-pointer b
   int   vc;                //RT v-pointer c
   int   vd;                //RT v-pointer d   
   int   ta;                //RT table-pointer a
   int   tb;                //RT table-pointer b
   int   tc;                //RT table-pointer c
   int   td;                //RT table-pointer d
   double apdelay;    
   double strtdelay;   
   double offstdelay;
   int    nphBase;          //number of elements in the list
   int    npw;              //number of pulse widths
   int    nph;              //number of phases
   int    na;               //number of amplitudes
   int    ng;               //number of gate states
   int *pwBase;             //list of input pulse widths
   int *phBase;              //list of input phases
   int *aBase;              //list of input amplitudes
   int *gateBase;           //list of input gate states
} WMPA;

//-------------------------------------------------------------
// The GP Structure
//-------------------------------------------------------------

 typedef struct {
   char   seqName[NSUFFIX]; //channel-name + suffix, (c.f.) Xbr24
   double a1;               // Four amplitudes are available
   double a2;
   double a3;
   double a4;
   double pw1;              // Ten pulse widths are available
   double pw2;
   double pw3;
   double pw4;
   double pw5;
   double pw6;
   double pw7;
   double pw8;
   double pw9;
   double pw10;
   double t1;               //Ten delays are available
   double t2;
   double t3;
   double t4;
   double t5;
   double t6;
   double t7;
   double t8;
   double t9;
   double t10;
   char  ch1[NCH];            //Four channels are available
   char  ch2[NCH];
   char  ch3[NCH];
   char  ch4[NCH];
   double apdelay;         
   double strtdelay;        
   double offstdelay;
} GP;

//==================================================================
// OBinitializer - Setup Memory for Phase, Amplitude and Gate Lists
//==================================================================

void OBinitializer(WMPA *mp, int npw, int nph, int na, int ng, int nphBase)
{
   mp->pwBase = (int*) malloc(npw*sizeof(int));
   mp->phBase = (int*) malloc(nph*sizeof(int));
   mp->aBase = (int*) malloc(na*sizeof(int));
   mp->gateBase = (int*) malloc(ng*sizeof(int));
   mp->nphBase = nphBase;
   mp->npw = npw;
   mp->nph = nph;
   mp->na = na;
   mp->ng = ng;
}

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
      printf("getbr24() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,"%s",seqName);

// chXsuffix

   Getstr(getname0("ch",mp.seqName,""),mp.ch,sizeof(mp.ch));

// aXsuffix

   var = getname0("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname0("npa",mp.seqName,"");
   mp.npa = getval(var);

// r1Xsuffix

   var = getname0("r1",mp.seqName,"");
   mp.r1 = getval(var);

// mp.pwXsuffix

   var = getname0("pw",mp.seqName,"");
   mp.pw = getval(var);

// r2Xsuffix

   var = getname0("r2",mp.seqName,"");
   mp.r2 = getval(var);

// r3Xsuffix

   var = getname0("r3",mp.seqName,"");
   mp.r3 = getval(var);

// mp.tauXsuffix

   var = getname0("tau",mp.seqName,"");
   mp.tau = getval(var);

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtau = mp.tau - mp.pw - mp.r1 - mp.r2;
   mp.dtaua = mp.dtau - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getbr24() Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set the Number of Br24 Cycles and create V-variables for the Loop. 

   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) (int) (np/(36.0*mp.tau*2.0*sw));
   initval(cycles, mp.vcycles);
   mp.cycles = cycles;

// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps

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
      printf("getmrev8() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,"%s",seqName);

// chXsuffix

   Getstr(getname0("ch",seqName,""),mp.ch,sizeof(mp.ch));

// aXsuffix

   var = getname0("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname0("npa",mp.seqName,"");
   mp.npa = getval(var);

// r1Xsuffix

   var = getname0("r1",mp.seqName,"");
   mp.r1 = getval(var);

// mp.pwXsuffix

   var = getname0("pw",mp.seqName,"");
   mp.pw = getval(var);

// r2Xsuffix

   var = getname0("r2",mp.seqName,"");
   mp.r2 = getval(var);

// r3Xsuffix

   var = getname0("r3",mp.seqName,"");
   mp.r3 = getval(var);

// mp.tauXsuffix

   var = getname0("tau",mp.seqName,"");
   mp.tau = getval(var);

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtau = mp.tau - mp.pw - mp.r1 - mp.r2;
   mp.dtaua = mp.dtau - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getmrev8() Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set the Number of MREV8 Cycles and create V-variables for the Loop

   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) (int) (np/(12.0*mp.tau*2.0*sw));
   initval(cycles, mp.vcycles);
   mp.cycles = cycles;

// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps

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
      printf("getswwhh4() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,"%s",seqName);

// chXsuffix

   Getstr(getname0("ch",seqName,""),mp.ch,sizeof(mp.ch));

// aXsuffix

   var = getname0("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname0("npa",mp.seqName,"");
   mp.npa = getval(var);

// r1Xsuffix

   var = getname0("r1",mp.seqName,"");
   mp.r1 = getval(var); 

// mp.pwXsuffix

   var = getname0("pw",mp.seqName,"");
   mp.pw = getval(var);

// r2Xsuffix

   var = getname0("r2",mp.seqName,"");
   mp.r2 = getval(var);

// r3Xsuffix

   var = getname0("r3",mp.seqName,"");
   mp.r3 = getval(var);

// tauXsuffix

   var = getname0("tau",mp.seqName,"");
   mp.tau = getval(var);

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtau = mp.tau - mp.pw - mp.r1 - mp.r2;
   mp.dtaua = mp.dtau - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getswwhh4() Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set the Number of WHH4 Cycles and create V-variables for the Loop. 

   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) (int) (np/((4.0*mp.tau + 2.0*mp.pw)*2.0*sw));
   initval(cycles, mp.vcycles);
   mp.cycles = cycles; 

// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps

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
      printf("getxx() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }

   sprintf(mp.seqName,"%s",seqName);

// chXsuffix

   Getstr(getname0("ch",seqName,""),mp.ch,sizeof(mp.ch));

// aXsuffix

   var = getname0("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname0("npa",mp.seqName,"");
   mp.npa = getval(var);

// r1Xsuffix

   var = getname0("r1",mp.seqName,"");
   mp.r1 = getval(var);

// mp.pwXsuffix

   var = getname0("pw",mp.seqName,"");
   mp.pw = getval(var);

// r2Xsuffix

   var = getname0("r2",mp.seqName,"");
   mp.r2 = getval(var);

// r3Xsuffix

   var = getname0("r3",mp.seqName,"");
   mp.r3 = getval(var);

// tauXsuffix

   var = getname0("tau",mp.seqName,"");
   mp.tau = getval(var);

// Calculate mp.dtaua - Abort if ((mp.dtaua) < 0.0)

   mp.dtaua = mp.tau - mp.pw - mp.r1 - mp.r2 - mp.r3 - mp.npa*1.0e-7;

   if ((mp.dtaua) <= 0.0) {
      printf("getxx() Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set the Number of XX Cycles and create V-variables for the Loop

   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) (int) (np/((2.0*mp.tau)*2.0*sw));
   initval(cycles, mp.vcycles);
   mp.cycles = cycles;

// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps

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
      printf("getxmx() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }

   sprintf(mp.seqName,"%s",seqName);

// chXsuffix

   Getstr(getname0("ch",seqName,""),mp.ch,sizeof(mp.ch));

// aXsuffix

   var = getname0("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname0("npa",mp.seqName,"");
   mp.npa = getval(var);

// r1Xsuffix

   var = getname0("r1",mp.seqName,"");
   mp.r1 = getval(var); 

// mp.pwXsuffix

   var = getname0("pw",mp.seqName,"");
   mp.pw = getval(var);

// r2Xsuffix

   var = getname0("r2",mp.seqName,"");
   mp.r2 = getval(var);

// r3Xsuffix

   var = getname0("r3",mp.seqName,"");
   mp.r3 = getval(var);

// tauXsuffix

   var = getname0("tau",mp.seqName,"");
   mp.tau = getval(var);

// Calculate mp.dtaua - Abort if ((mp.dtaua) < 0.0)

   mp.dtaua = mp.tau - mp.pw - mp.r1 - mp.r2 - mp.r3 - mp.npa*1.0e-7;

   if ((mp.dtaua) <= 0.0) {
      printf("getxmx() Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set the Number of XX Cycles and create V-variables for the Loop. 

   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) (int) (np/((2.0*mp.tau)*2.0*sw));
   initval(cycles, mp.vcycles);
   mp.cycles = cycles;

// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps

   mp.vphase = setvvarname();
   mp.vstep = setvvarname();
   mp.istep = setvvarname();
   initval(0.0, mp.istep);
   int table1[2] = {0,2};
   mp.tstep = settablename(2,table1);

   return mp;
}

//-------------------------------------------------
// Implement TOSS4 with a single input phase cycle
//-------------------------------------------------

WMPA gettoss4(char *seqName)
{
   WMPA mp;
   char *var;
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("gettoss4() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,"%s",seqName);

// INOVA ap bus delay

   mp.apdelay = 0.5e-6;

// chXsuffix

   Getstr(getname0("ch",seqName,""),mp.ch,sizeof(mp.ch));

// aXsuffix

   var = getname0("a",mp.seqName,"");
   mp.a = getval(var);

// pwXsuffix

   var = getname0("pw",mp.seqName,"");
   mp.pw = getval(var);

// mp.rtau

   mp.rtau = 1.0/getval("srate");
   if (mp.rtau > 0.002) {
      printf("gettoss4() Error: srate < 500. Abort!\n");
      psg_abort(1);
   }

// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps

   mp.vphase = setvvarname();
   mp.vstep = setvvarname();
   mp.istep = setvvarname(); 
   initval(0.0, mp.istep);
   int ltable1[4] = {0,1,0,1};
   mp.tstep = settablename(4,ltable1);
   return mp;
}

//===================
// Implement TOSS5
//===================

WMPA gettoss5(char *seqName)
{
   WMPA mp;
   char *var;
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("gettoss5() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,"%s",seqName);

// chXsuffix

   Getstr(getname0("ch",seqName,""),mp.ch,sizeof(mp.ch));

// aXsuffix

   var = getname0("a",mp.seqName,"");
   mp.a = getval(var);

// pwXsuffix

   var = getname0("pw",mp.seqName,"");
   mp.pw = getval(var);

// mp.rtau

   mp.rtau = 1.0/getval("srate");
   if (mp.rtau > 0.002) {
      printf("gettoss5() Error: srate < 500. Abort!\n");
      psg_abort(1);
   }

// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps.

   mp.vphase = setvvarname();
   mp.vstep = setvvarname();
   mp.istep = setvvarname();
   initval(0.0, mp.istep);
   int ltable1[5] = {0,0,0,0,0};
   mp.tstep = settablename(5,ltable1);

// Set the Small-Angle Cogwheel Phase Tables.

   int i = 0;
   double temp;
   int ltable2[11], ltable3[11];
   while (i < 11) { 
      temp = roundcycle(-5.0*i*PSD*8192/11.0,1.0,PSD*8192.0);
      ltable2[i] = (int) temp;
      temp = roundcycle(-6.0*i*PSD*8192/11.0,1.0,PSD*8192.0);
      ltable3[i] = (int) temp;
      i++;
   }
   mp.ta = settablename(11,ltable2);
   mp.tb = settablename(11,ltable3);

   return mp;
}

//====================================================
// Implement Interrupted Decoupling with Refocussing
//====================================================

WMPA getidref(char *seqName)
{
   WMPA mp;
   char *var;
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("getidref() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,"%s",seqName);

// chXsuffix

   Getstr(getname0("ch",seqName,""),mp.ch,sizeof(mp.ch));

// aXsuffix

   var = getname0("a",mp.seqName,"");
   mp.a = getval(var);

// pwXsuffix

   var = getname0("pw",mp.seqName,"");
   mp.pw = getval(var);

// mp.rtau

   mp.rtau = 1.0/getval("srate");
   if (mp.rtau > 0.002) {
      printf("getidref() Error: srate < 500. Abort!\n");
      psg_abort(1);
   }

// t1Xsuffix

   var = getname0("t1",mp.seqName,"");
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
      printf("getwpmlg() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,"%s",seqName);

// chXsuffix

   Getstr(getname0("ch",seqName,""),mp.ch,sizeof(mp.ch));

// aXsuffix

   var = getname0("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname0("npa",mp.seqName,"");
   mp.npa = getval(var);

// r1Xsuffix

   var = getname0("r1",mp.seqName,"");
   mp.r1 = getval(var);

// pwXsuffix

   var = getname0("pw",mp.seqName,"");
   mp.pw = getval(var);

// r2Xsuffix

   var = getname0("r2",mp.seqName,"");
   mp.r2 = getval(var);

// r3Xsuffix

   var = getname0("r3",mp.seqName,"");
   mp.r3 = getval(var);

// tauXsuffix

   var = getname0("tau",mp.seqName,"");
   mp.tau = getval(var);

// qXsuffix

   var = getname0("q",mp.seqName,"");
   mp.q = getval(var);

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtaua = mp.tau - 2.0*mp.pw - mp.r1 - mp.r2 - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getwpmlg() Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set PMLG Phase Array

   double sign = 1;
   if (mp.q < 0.0) {
      mp.q = -mp.q;
      sign = - sign;
   }
   double obsstep = 360.0/(8192*PSD);
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

// Double mp.q for +/- phase steps and and create v-vars for the nested loop

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
// V-variables for the MP-phase steps

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

//----------------------------------------------
// Implement An INEPT Transfer
//----------------------------------------------

GP getinept(char *seqName)
{
   GP in;
   char *var;
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("getinept() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(in.seqName,"%s",seqName);

// ch1YXsuffix

   Getstr(getname1("ch1",seqName,0),in.ch1,sizeof(in.ch1));

// ch2YXsuffix

   Getstr(getname1("ch2",seqName,0),in.ch2,sizeof(in.ch2));

// aYyxsuffix

   var = getname1("a",in.seqName,1);
   in.a1 = getval(var);

// aXyxsuffix

   var = getname1("a",in.seqName,2);
   in.a2 = getval(var);

// pw1Yyxsuffix

   var = getname1("pw1",in.seqName,1); 
   in.pw1 = getval(var); 

// pw1Xyxsuffix

   var = getname1("pw1",in.seqName,2);
   in.pw2 = getval(var);

// pw2Yyxsuffix

   var = getname1("pw2",in.seqName,1);
   in.pw3 = getval(var);

// pw2Xyxsuffix

   var = getname1("pw2",in.seqName,2);
   in.pw4 = getval(var);

// t1YXsuffix

   var = getname1("t1",in.seqName,0);
   in.t1 = getval(var);

// t2YXsuffix

   var = getname1("t2",in.seqName,0);
   in.t3 = getval(var);

   double lsim = in.pw1/2.0;
   if (in.pw2 > in.pw1) lsim = in.pw2/2.0;

   double lsim1 = in.pw3/2.0;
   if (in.pw4 > in.pw3) lsim1 = in.pw4/2.0;

   in.t1 = in.t1 - lsim;
   in.t2 = in.t1 - lsim - lsim1;
   in.t3 = in.t3 - lsim - lsim1;
   in.t4 = in.t3 - lsim;

   return in;
}
//---------------------------------------------------
// Implement Windowed DUMBO
//---------------------------------------------------

WMPA getwdumbo(char *seqName)
{
   WMPA mp;
   char *var;
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("getwdumbo() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,"%s",seqName);

// chXsuffix

   Getstr(getname0("ch",seqName,""),mp.ch,sizeof(mp.ch));

// aXsuffix

   var = getname0("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname0("npa",mp.seqName,"");
   mp.npa = getval(var);

// r1Xsuffix

   var = getname0("r1",mp.seqName,"");
   mp.r1 = getval(var); 

// pwXsuffix

   var = getname0("pw",mp.seqName,"");
   mp.pw = getval(var);

// r2Xsuffix

   var = getname0("r2",mp.seqName,"");
   mp.r2 = getval(var);

// r3Xsuffix

   var = getname0("r3",mp.seqName,"");
   mp.r3 = getval(var);

// tauXsuffix

   var = getname0("tau",mp.seqName,"");
   mp.tau = getval(var);

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtaua = mp.tau - mp.pw - mp.r1 - mp.r2 - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getwdumbo Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set DUMBO Phase Array

   double obsstep = 360.0/(8192*PSD);
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

// Double mp.q for +/- phase steps and and create v-vars for the nested loop

   mp.vcycles1 = setvvarname();
   mp.vcount1 = setvvarname();
   initval(64.0, mp.vcycles1);
   mp.q = 64;

// Set the Number of DUMBO Cycles and create V-vars for the main loop

   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) (int) (np/(mp.tau*2.0*sw));
   initval(cycles, mp.vcycles);
   mp.cycles = cycles;

// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps

   mp.vphase = setvvarname();
   mp.vstep = setvvarname();
   mp.istep = setvvarname();
   initval(0.0, mp.istep);
   mp.tstep = settablename(64,table1);

// Add Phase Offset to the Table

// phXsuffix

   var = getname0("ph",mp.seqName,"");
   double ph = getval(var);
   temp =  roundphase(ph,obsstep);
   int phint = (int) roundoff(temp/obsstep,1);
   tsadd(mp.tstep,phint,8192*PSD);

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
      printf("getwdumbot() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,"%s",seqName);

// chXsuffix

   Getstr(getname0("ch",seqName,""),mp.ch,sizeof(mp.ch));

// aXsuffix

   var = getname0("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname0("npa",mp.seqName,"");
   mp.npa = getval(var);

// r1Xsuffix

   var = getname0("r1",mp.seqName,"");
   mp.r1 = getval(var);

// pw1Xsuffix

   var = getname0("pw1",mp.seqName,"");
   mp.pw = getval(var);                // note the prefix shift

// pw2Xsuffix

   var = getname0("pw2",mp.seqName,"");
   mp.t1 = getval(var);                 // note the prefix shift

// r2Xsuffix

   var = getname0("r2",mp.seqName,"");
   mp.r2 = getval(var);

// r3Xsuffix

   var = getname0("r3",mp.seqName,"");
   mp.r3 = getval(var);

// tauXsuffix

   var = getname0("tau",mp.seqName,"");
   mp.tau = getval(var);

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtaua = mp.tau - mp.pw - mp.r1 - mp.r2 - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getwdumbot() Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set DUMBO Phase Array

   double obsstep = 360.0/(8192*PSD);
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

// Double mp.q for +/- phase steps and and create v-vars for the nested loop

   mp.vcycles1 = setvvarname();
   mp.vcount1 = setvvarname();
   initval(64.0, mp.vcycles1);
   mp.q = 64;

// Set the Number of DUMBO Cycles and create V-vars for the main loop

   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) (int) (np/(mp.tau*2.0*sw));
   initval(cycles, mp.vcycles);
   mp.cycles = cycles;

// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps

   mp.vphase = setvvarname();
   mp.vstep = setvvarname();
   mp.istep = setvvarname();
   initval(0.0, mp.istep);
   mp.tstep = settablename(64,table1);

// Add Phase Offset to the Table

// phXsuffix

   var = getname0("ph",mp.seqName,"");
   double ph = getval(var);
   temp =  roundphase(ph,obsstep);
   int phint = (int) roundoff(temp/obsstep,1);
   tsadd(mp.tstep,phint,8192*PSD);

// Set mp.va to mp.vc and initialize mp.va at 2048 steps (90 degrees)

   mp.va = setvvarname();
   mp.vb = setvvarname();
   mp.vc = setvvarname();
   initval(2048.0, mp.va);

   return mp;
}

//---------------------------------------------------------------------
// Implementation of CPMG with Interleaved Acquisition
//--------------------------------------------------------------------

WMPA getcpmg(char *seqName)
{
   WMPA mp;
   char *var;
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("getcpmg() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }

   sprintf(mp.seqName,"%s",seqName);

// chXcpmg

   Getstr(getname0("ch",seqName,""),mp.ch,sizeof(mp.ch));

// aXcpmg

   var = getname0("a",mp.seqName,"");
   mp.a = getval(var);

// r1Xcpmg

   var = getname0("r1",mp.seqName,"");
   mp.r1 = getval(var);

// pwXcpmg

   var = getname0("pw",mp.seqName,"");
   mp.pw = getval(var);

// r2Xcpmg

   var = getname0("r2",mp.seqName,"");
   mp.r2 = getval(var);

// r3Xcpmg

   var = getname0("r3",mp.seqName,"");
   mp.r3 = getval(var);

// tauXcpmg

   var = getname0("tau",mp.seqName,"");
   mp.tau = getval(var);

// edXcpmg

   var = getname0("ed",mp.seqName,"");
   mp.t2 = getval(var);

/*Calculate the Acquisition Periods*/

   double ad = getval("ad");
   double frstacqtime = ((double) (int) ((1.0/2.0e-7)*(mp.tau/2.0 + mp.t2 - mp.r1 - mp.pw/2.0)))*2.0e-7;
   if (frstacqtime < 1.0e-6) {
      abort_message("ABORT! no time for acquisiton ,increase tau! \n");
   }
   mp.t1 = -frstacqtime + (mp.tau/2.0 + mp.t2 - mp.r1 - mp.pw/2.0);
   mp.npa1 = (double) ((int) (frstacqtime/2.0e-7 + 0.1))*2;

   double loopacqtime = ((double) (int) ((1.0/2.0e-7)*(mp.tau - mp.r1 - mp.r2 - ad - mp.r3 - mp.pw)))*2.0e-7;
     if (loopacqtime < 1.0e-6) {
      abort_message("ABORT! no time for acquisiton ,increase tau! \n");
   }
   mp.dtaua = -loopacqtime + (mp.tau - mp.r1 - mp.r2 - mp.r3 - mp.pw);
   mp.npa = (double) ((int) (loopacqtime/2.0e-7 + 0.1))*2;

// Set the Number of CPMG Cycles and create V-variables for the Loop

   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) (int) (np/(mp.tau*2.0*sw));
   initval(cycles - 1.0, mp.vcycles);
   mp.cycles = cycles;

// Create the Phase Table for the MP phase Cycle - Here all phases are the same

   mp.vphase = setvvarname();

   return mp;
}

//----------------------------------------------
// Implement HMQC Evolution
//----------------------------------------------

GP gethmqc(char *seqName)
{
   GP hmqc;
   char *var;
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("gethmqc() Error: The type name %s is invalid!\n",seqName);
      psg_abort(1);
   }
   sprintf(hmqc.seqName,"%s",seqName);

// ch1YXhmqc

   Getstr(getname1("ch1",seqName,0),hmqc.ch1,sizeof(hmqc.ch1));

// ch2YXhmqc

   Getstr(getname1("ch2",seqName,0),hmqc.ch2,sizeof(hmqc.ch2));

// aYyxhmqc

   var = getname1("a",hmqc.seqName,1);
   hmqc.a1 = getval(var);

// aXyxhmqc

   var = getname1("a",hmqc.seqName,2);
   hmqc.a2 = getval(var);

// pwYyxhmqc

   var = getname1("pw",hmqc.seqName,1);
   hmqc.pw1 = getval(var);

// pwXyxhmqc

   var = getname1("pw",hmqc.seqName,2);
   hmqc.pw2 = getval(var);

// tYXsuffix

   var = getname1("t",hmqc.seqName,0);
   hmqc.t1 = getval(var);
   hmqc.t1 = hmqc.t1 - hmqc.pw1/2.0;

// Get the evolution time d2

   d2 = getval("d2");
   hmqc.t2 = d2/2.0 - hmqc.pw1/2.0 - hmqc.pw2/2.0;
   if (hmqc.t2 < 0.0) hmqc.t2 = 0.0;

   return hmqc;
}

//================================================
// Implement HSSMALL
//================================================

WMPA gethssmall(char *seqName)
{
   WMPA mp;
   char *var;
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("gethssmall() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,"%s",seqName);

// chXhssmall

   Getstr(getname0("ch",seqName,""),mp.ch,sizeof(mp.ch));

// aXhssmall

   var = getname0("a",mp.seqName,"");
   mp.a = getval(var);

// npaXhssmall

   var = getname0("npa",mp.seqName,"");
   mp.npa = getval(var);

// r1Xhssmall

   var = getname0("r1",mp.seqName,"");
   mp.r1 = getval(var);

// pwXhssmall

   var = getname0("pw",mp.seqName,"");
   mp.pw = getval(var);

// r2Xhssmall

   var = getname0("r2",mp.seqName,"");
   mp.r2 = getval(var);

// r3Xhssmall

   var = getname0("r3",mp.seqName,"");
   mp.r3 = getval(var);

// tauXhssmall

   var = getname0("tau",mp.seqName,"");
   mp.tau = getval(var); 
   printf("mp.tau = %f\n",mp.tau*1e6);

// ph1Xhssmall

   var = getname0("ph1",mp.seqName,"");
   double ph1 = getval(var);
   //ph1 = roundphase(ph1,360.0/(PSD*8192));
   ph1 = ph1*(PSD*8192)/360.0;
//   printf("ph1: %f\n",ph1);
   mp.va = setvvarname();
   initval(ph1,mp.va);

// ph2Xhssmall

   var = getname0("ph2",mp.seqName,"");
   double ph2 = getval(var);
   //ph2 = roundphase(ph2,360.0/(PSD*8192));
   ph2=ph2*(PSD*8192)/360.0;
   mp.vb = setvvarname();
   initval((ph1 + ph2),mp.vb);

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtau = mp.tau - mp.pw - mp.r1 - mp.r2;
   mp.dtaua = mp.dtau - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("gethssmall Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set the Number of HSSMALL Cycles and create V-variables for the Loop.

   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) (int) (np/(8.0*mp.tau*2.0*sw));
   initval(cycles, mp.vcycles);
   mp.cycles = cycles;

// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps.

   mp.vphase = setvvarname();
   mp.vstep = setvvarname();
   mp.istep = setvvarname();
   initval(0.0, mp.istep);
   int table1[8] = {0,1,1,0,0,1,1,0};
   mp.tstep = settablename(8,table1);

   return mp;
}

//-------------------------------------------------------
// Implement Windowed PMLG
//-------------------------------------------------------

WMPA getxmxwpmlg(char *seqName)
{
   WMPA mp;
   char *var;
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("getwpmlg() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,"%s",seqName);

// chXsuffix

   Getstr(getname0("ch",seqName,""),mp.ch,sizeof(mp.ch));

// aXsuffix

   var = getname0("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname0("npa",mp.seqName,"");
   mp.npa = getval(var);

// r1Xsuffix

   var = getname0("r1",mp.seqName,"");
   mp.r1 = getval(var);

// pwXsuffix

   var = getname0("pw",mp.seqName,"");
   mp.pw = getval(var);

// r2Xsuffix

   var = getname0("r2",mp.seqName,"");
   mp.r2 = getval(var);

// r3Xsuffix

   var = getname0("r3",mp.seqName,"");
   mp.r3 = getval(var);

// tauXsuffix

   var = getname0("tau",mp.seqName,"");
   mp.tau = getval(var);

// qXsuffix

   var = getname0("q",mp.seqName,"");
   mp.q = getval(var);

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtaua = mp.tau - 2.0*mp.pw - mp.r1 - mp.r2 - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getwpmlg() Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
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
   for (i = 2*mp.q; i < 3*mp.q; i++) {
      temp = 180.0 + sign*(i*delta + delta/2.0);
      temp = roundphase(temp,obsstep);
      table1[i] = (int) roundoff(temp/obsstep,1);
   }
   for (i = 3*mp.q; i < 4*mp.q; i++) {
      temp = sign*((2*mp.q - i)*delta - delta/2.0);
      temp = roundphase(temp,obsstep);
      table1[i] = (int) roundoff(temp/obsstep,1);
   }

// Set PMLG pulse

   mp.pw  = roundoff(mp.pw/mp.q, 0.0125e-6);

// Double mp.q for +/- phase steps and and create v-vars for the nested loop

   mp.vcycles1 = setvvarname();
   mp.vcount1 = setvvarname();
   initval(((double) 2.0*mp.q), mp.vcycles1);
   int q  = mp.q*4;

// Set the Number of WPMLG Cycles and create V-vars for the main loop. 

   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) (int) (np/(mp.tau*2.0*sw));
   initval(cycles, mp.vcycles);
   mp.cycles = cycles;

// Create the Phase Table for the MP phase Cycle and create
// V-variables for the MP-phase steps

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

//============================
// Implement Windowed SAMn
//============================

WMPA getwsamn(char *seqName)
{
   WMPA mp;
   char *var;
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("getwsamn() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,"%s",seqName);

// chXsuffix

   Getstr(getname0("ch",seqName,""),mp.ch,sizeof(mp.ch));

// aXsuffix

   var = getname0("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname0("npa",mp.seqName,"");
   mp.npa = getval(var);

// r1Xsuffix

   var = getname0("r1",mp.seqName,"");
   mp.r1 = getval(var);

// r2Xsuffix

   var = getname0("r2",mp.seqName,"");
   mp.r2 = getval(var);

// r3Xsuffix

   var = getname0("r3",mp.seqName,"");
   mp.r3 = getval(var);

// tauXsuffix

   var = getname0("tau",mp.seqName,"");
   mp.tau = getval(var);

// qXsuffix

   var = getname0("q",mp.seqName,"");
   int rfcycles = getval(var);

// pwXsuffix

   var = getname0("pw",mp.seqName,"");
   double taur = getval(var);

   taur = roundoff(taur,rfcycles*300.0e-9);
   int totalsteps = (int) roundoff(taur/300.0e-9,1);
   mp.pw = 300.0e-9; 

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtaua = mp.tau - taur - mp.r1 - mp.r2 - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getwsamn() Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Allocate Arrays

   int nphBase = totalsteps;
   int npw = 1;
   int nph = totalsteps; 
   int na = totalsteps;
   int ng = 1;

   OBinitializer(&mp, npw, nph, na, ng, nphBase);

// Set the Amplitude and Phase Lists

   int i;  
   for (i = 0; i < totalsteps; i++) {
      double a = mp.a*cos(2.0*3.14159*(i + 0.5)*rfcycles/totalsteps);
      if (a < 0) {
         a = -a;
         mp.aBase[i] = (int) roundoff(a,1.0);
         mp.phBase[i] = 2;
      }
      else {
         mp.aBase[i] = (int) roundoff(a,1.0);
         mp.phBase[i] = 0;
      }
   }

// Set the Number of SAM Rotor Cycles and Create V-vars for the Main Loop. 

   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();
   double cycles = (double) (int) (np/(mp.tau*2.0*sw));
   initval(cycles, mp.vcycles);
   mp.cycles = cycles;

// Create Tables for the SAMn Phase and Amplitude Lists and Create
// V-variables for the SAMn Phase and Amplitude Steps

   mp.vphase = setvvarname();
   mp.vstep = setvvarname();
   mp.va = setvvarname();
   mp.istep = setvvarname();
   initval(0.0, mp.istep);
   mp.tstep = settablename(totalsteps,mp.phBase);
   mp.ta = settablename(totalsteps,mp.aBase);

// Set the Number of Steps in the Nested Phase-Amplitude SAMn Loop

   mp.vcycles1 = setvvarname();
   mp.vcount1 = setvvarname();
   double ltotalsteps = (double) totalsteps; 
   initval(ltotalsteps, mp.vcycles1);
   mp.cycles1 = ltotalsteps;

   return mp;
}

//-------------------------------------------------------
// Implement Windowed PMLG with XMX Supercycle
//-------------------------------------------------------

WMPA getwpmlgxmx(char *seqName)
{
   WMPA mp;
   char *var;
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("getwpmlgxmx() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,"%s",seqName);

// chXsuffix

   Getstr(getname0("ch",seqName,""),mp.ch,sizeof(mp.ch));

// aXsuffix

   var = getname0("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname0("npa",mp.seqName,"");
   mp.npa = getval(var);

// r1Xsuffix

   var = getname0("r1",mp.seqName,"");
   mp.r1 = getval(var);

// pwXsuffix

   var = getname0("pw",mp.seqName,"");
   mp.pw = getval(var);

// r2Xsuffix

   var = getname0("r2",mp.seqName,"");
   mp.r2 = getval(var);

// r3Xsuffix

   var = getname0("r3",mp.seqName,"");
   mp.r3 = getval(var);

// tauXsuffix

   var = getname0("tau",mp.seqName,"");
   mp.tau = getval(var);

// qXsuffix

   var = getname0("q",mp.seqName,"");
   mp.q = getval(var);

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtaua = mp.tau - 2.0*mp.pw - mp.r1 - mp.r2 - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getwpmlg() Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set the xmx PMLG Phase Array

   double sign = 1;
   if (mp.q < 0.0) {
      mp.q = -mp.q;
      sign = - sign;
   }
   double obsstep = 360.0/(8192*PSD);
   double delta = 360.0/(sqrt(3)*mp.q);
   double temp = 0.0, temp1 = 0.0;
   int i;
   int j = (int) mp.q;
   int table1[512];
   for (i = 0; i < j; i++) {
      temp = sign*(i*delta + delta/2.0);
      temp = roundphase(temp,obsstep);
      temp1 = roundphase(temp + 180.0,obsstep);
      table1[i] = (int) roundoff(temp/obsstep,1);
      table1[i + 2*j] = (int) roundoff(temp1/obsstep,1);
   }
   for (i = mp.q; i < 2*j; i++) {
      temp = 180.0 + sign*((2*mp.q - i)*delta - delta/2.0);
      temp = roundphase(temp,obsstep);
      temp1 = roundphase(temp + 180.0,obsstep);
      table1[i] = (int) roundoff(temp/obsstep,1);      
      table1[i + 2*j] = (int) roundoff(temp1/obsstep,1); 
   }

// Set PMLG pulse

   mp.pw  = roundoff(mp.pw/mp.q, 0.0125e-6);

// Double mp.q for +/- phase steps and and create v-vars for the nested loop

   mp.vcycles1 = setvvarname();
   mp.vcount1 = setvvarname();
   initval(((double) 2.0*mp.q), mp.vcycles1); 
   int q  = mp.q*4;

// Set an even number of WPMLG cycles and create V-vars for the main loop. 

   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) 2.0*((int) (np/(mp.tau*4.0*sw)));
   initval(cycles, mp.vcycles);
   mp.cycles = cycles;

// Create the phase table for the MP phase cycle and create
// V-variables for the MP-phase steps

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

WMPA getwdumboxmx(char *seqName)
{
   WMPA mp;
   char *var;
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("getwdumboxmx() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,"%s",seqName);

// chXsuffix

   Getstr(getname0("ch",seqName,""),mp.ch,sizeof(mp.ch));

// aXsuffix

   var = getname0("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname0("npa",mp.seqName,"");
   mp.npa = getval(var);

// r1Xsuffix

   var = getname0("r1",mp.seqName,"");
   mp.r1 = getval(var); 

// pwXsuffix

   var = getname0("pw",mp.seqName,"");
   mp.pw = getval(var);

// r2Xsuffix

   var = getname0("r2",mp.seqName,"");
   mp.r2 = getval(var);

// r3Xsuffix

   var = getname0("r3",mp.seqName,"");
   mp.r3 = getval(var);

// tauXsuffix

   var = getname0("tau",mp.seqName,"");
   mp.tau = getval(var);

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtaua = mp.tau - mp.pw - mp.r1 - mp.r2 - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getwdumboxmx Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Set DUMBO phase array and expand to an xmx array

   double obsstep = 360.0/(8192*PSD);
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
   double temp, temp1;
   int table1[128]; 
   for (i = 0; i < 64; i++) {
      temp = roundphase(dumbo[i],obsstep);
      temp1 = roundphase((180.0 + dumbo[i]),obsstep);
      table1[i] = (int) roundoff(temp/obsstep,1);
      table1[i + 64] = (int) roundoff(temp1/obsstep,1);
   }

// Set the DUMBO pulse

   mp.pw  = roundoff(mp.pw/64.0, 0.0125e-6);

// Double mp.q for +/- phase steps and and create v-vars for the nested loop

   mp.vcycles1 = setvvarname();
   mp.vcount1 = setvvarname();
   initval(64.0, mp.vcycles1);
   mp.q = 64;
   int q = 2*mp.q; 

// Set an even number of DUMBO cycles and create V-vars for the main loop

   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) 2.0*((int) (np/(mp.tau*4.0*sw)));
   initval(cycles, mp.vcycles);
   mp.cycles = cycles;

// Create the phase table for the MP phase cycle and create
// V-variables for the MP phase steps

   mp.vphase = setvvarname();
   mp.vstep = setvvarname();
   mp.istep = setvvarname();
   initval(0.0, mp.istep);
   mp.tstep = settablename(q,table1);

// Add Phase Offset to the Table

// phXsuffix

   var = getname0("ph",mp.seqName,"");
   double ph = getval(var);
   temp =  roundphase(ph,obsstep);
   int phint = (int) roundoff(temp/obsstep,1);
   tsadd(mp.tstep,phint,8192*PSD);

// Set mp.va and initialize 2048 steps (90 degrees)

   mp.va = setvvarname();
   initval(2048.0*PSD, mp.va);

   return mp;
}

//-------------------------------------------------------------
// Implement Windowed DUMBO with calculation from coefficients
//-------------------------------------------------------------

WMPA getwdumbogen(char *seqName, char *coeffName)

//  DUMBO-1 experimental description A.Lesage et al., JMR, 163 (2003) 105
//  eDumbo method of optimisation G. de Paepe, Chem.Phys.Lett., 376 (2003) 259
//  Fourier coefficients from http://www.ens-lyon.fr/CHEMIE/Fr/Groupes/NMR/Pages/library.html
//  Z-Rotation Supercycle: M.Leskes, Chem. Phys. Lett., 466 (2008) 95

{
   WMPA mp;
   char *var;
   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("getwdumbogen() Error: The type name %s is invalid !\n",seqName);
      psg_abort(1);
   }
   sprintf(mp.seqName,"%s",seqName);

// chXsuffix

   Getstr(getname0("ch",seqName,""),mp.ch,sizeof(mp.ch));

// aXsuffix

   var = getname0("a",mp.seqName,"");
   mp.a = getval(var);

// npaXsuffix

   var = getname0("npa",mp.seqName,"");
   mp.npa = getval(var);

// r1Xsuffix

   var = getname0("r1",mp.seqName,"");
   mp.r1 = getval(var); 

// pwXsuffix

   var = getname0("pw",mp.seqName,"");
   mp.pw = getval(var);

// r2Xsuffix

   var = getname0("r2",mp.seqName,"");
   mp.r2 = getval(var);

// r3Xsuffix

   var = getname0("r3",mp.seqName,"");
   mp.r3 = getval(var);

// tauXsuffix

   var = getname0("tau",mp.seqName,"");
   mp.tau = getval(var);
   
// qXdumbocf - number of steps

   var = getname0("q",coeffName,"");
   int steps = getval(var);
   if (steps > 512) {
   	printf("too many steps: %d\n",steps);
	psg_abort(1);
   }

// scXdumbocf - supercycle: 1; no supercycle: 0

   var = getname0("sc",coeffName,"");
   int supercyc = getval(var);

// Calculate mp.dtau and mp.dtaua - Abort if ((mp.tau + mp.dtaua) < 0.0)

   mp.dtaua = mp.tau - mp.pw - mp.r1 - mp.r2 - mp.r3 - mp.npa*1.0e-7;

   if ((mp.tau + mp.dtaua) <= 0.0) {
      printf("getwdumbogen Error: Acquisition delay (mp.dtaua <= 0.0). Abort!");
      psg_abort(1);
   }

// Get DUMBO Fourier coefficients: ca#Xdumbocf, cb#Xdumbocf # = 1 to 6

   char *coef[12] = {"ca1","ca2","ca3","ca4","ca5","ca6","cb1","cb2","cb3","cb4","cb5","cb6"};
   double c[12];
   int i;
   for (i = 0; i < 12; i++) {
      var = getname0(coef[i],coeffName,"");
      c[i] = (double) getval(var);
   }

// Set DUMBO phase array   

   double obsstep = 360.0/(PSD*8192);
   int phl = (supercyc)? steps*2 : steps;  
   double* dumbo = malloc(sizeof(double)*phl);
   
// First half of the array

   for (i = 0; i < steps/2; i++) {
      dumbo[i]=0.0;
      double dumboscale = 2*M_PI*i/(steps/2);
      int j = 0;
      for (j = 0; j < 6; j++) {
         dumbo[i] += 360.0*c[j+6]*sin((j+1)*dumboscale) + 360.0*c[j]*cos((j+1)*dumboscale);
      }
   }

// Invert the phase for the second half

   int k;
   for (i = steps/2, k = steps/2 - 1; i < steps; i++, k--) {
      dumbo[i] = dumbo[k] + 180.0;
   }

   if (supercyc == 1) {
      for (i = 0; i < steps; i++) {
         dumbo[i + steps ]= dumbo[i] + 180.0;
      }
   }
      
   int* table1 = malloc(sizeof(int)*phl);
   double temp;
   for (i = 0; i < phl; i++) {
      temp = roundphase((dumbo[i]),obsstep);
      table1[i] = (int) roundoff(temp/obsstep,1);
   }

// Set DUMBO pulse

   mp.pw  = roundoff(mp.pw/steps, 0.0125e-6);

// Double mp.q for +/- phase steps and and create v-vars for the nested loop

   mp.vcycles1 = setvvarname();
   mp.vcount1 = setvvarname();
   initval(steps, mp.vcycles1);
   mp.q = steps;
   
// Set the number of DUMBO Cycles and create V-vars for the main loop

   mp.vcycles = setvvarname();
   mp.vcount = setvvarname();

   double cycles = (double) 2.0*((int) (np/(mp.tau*4.0*sw)));
   initval(cycles, mp.vcycles);
   mp.cycles = cycles;

// Create the phase table for the MP phase cycle and create
// V-variables for the MP-phase steps

   mp.vphase = setvvarname();
   mp.vstep = setvvarname();
   mp.istep = setvvarname();
   initval(0.0, mp.istep);
   mp.tstep = settablename(phl,table1);

// Add phase offset to the table

// phXsuffix

   var = getname0("ph",mp.seqName,"");
   double ph = getval(var);
   temp =  roundphase(ph,obsstep);
   int phint = (int) roundoff(temp/obsstep,1);
   tsadd(mp.tstep,phint,PSD*8192);

// Set mp.va and initialize 2048 steps (90 degrees)

   mp.va = setvvarname();
   initval(2048.0*PSD, mp.va);

   free(dumbo);
   free(table1);
   
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
      printf("_br24() Error: Undefined Channel. Abort!\n");
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
      printf("_mrev8() Error: Undefined Channel. Abort!\n");
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
      printf("_swwhh4() Error: Undefined Channel. Abort!\n");
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
         gen_RFphase(mp.vstep, chnl);               // -X
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
      printf("_xx() Error: Undefined Channel. Abort!\n");
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
      printf("_xmx() Error: Undefined Channel. Abort!\n");
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
      printf("_toss4() Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
   
   getelem(phase,ct,mp.vphase);
   gen_RFpwrf(mp.a, chnl);
   gen_RFunblank(chnl);
   getelem(mp.tstep,mp.istep,mp.vstep); 
   incr(mp.istep); 
   add(mp.vphase,mp.vstep,mp.vstep);
   gen_RFphase(mp.vstep, chnl);
   delay((0.1226*mp.rtau) - mp.pw/2.0 - mp.apdelay);
   gen_RFon(chnl);                         //x
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
   gen_RFon(chnl);                         //x
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

//=======
// TOSS5
//=======

void _toss5(WMPA mp, int phase)
{
   int chnl = 0;
   if (!strcmp(mp.ch,"obs")) chnl = 1;
   else if (!strcmp(mp.ch,"dec")) chnl = 2;
   else if (!strcmp(mp.ch,"dec2")) chnl = 3;
   else if (!strcmp(mp.ch,"dec3")) chnl = 4;
   else {
      printf("_toss5() Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }

   getelem(phase,ct,mp.vphase); 
   gen_RFpwrf(mp.a, chnl);
   gen_RFunblank(chnl);
   getelem(mp.tstep,mp.istep,mp.vstep);
   incr(mp.istep);
   add(mp.vphase,mp.vstep,mp.vstep);
   gen_RFfphase(mp.ta, chnl);              //-6cog11
   gen_RFphase(mp.vstep, chnl);
   delay((0.2029*mp.rtau) - mp.pw/2.0);
   gen_RFon(chnl);                         //x
   delay(mp.pw);
   gen_RFoff(chnl);

   getelem(mp.tstep,mp.istep,mp.vstep);
   incr(mp.istep);
   add(mp.vphase,mp.vstep,mp.vstep);
   gen_RFfphase(mp.tb, chnl);              //-5cog11
   gen_RFphase(mp.vstep, chnl);
   delay((0.1229*mp.rtau) - mp.pw);
   gen_RFon(chnl);                         //x
   delay(mp.pw);
   gen_RFoff(chnl);

   getelem(mp.tstep,mp.istep,mp.vstep);
   incr(mp.istep);
   add(mp.vphase,mp.vstep,mp.vstep);
   gen_RFfphase(mp.ta, chnl);              //-6cog11
   gen_RFphase(mp.vstep, chnl);
   delay((0.1742*mp.rtau) - mp.pw);
   gen_RFon(chnl);                         //x
   delay(mp.pw);
   gen_RFoff(chnl);

   getelem(mp.tstep,mp.istep,mp.vstep);
   incr(mp.istep);
   add(mp.vphase,mp.vstep,mp.vstep);
   gen_RFfphase(mp.tb, chnl);              //-5cog11
   gen_RFphase(mp.vstep, chnl);
   delay((0.1742*mp.rtau) - mp.pw);
   gen_RFon(chnl);                         //x
   delay(mp.pw);
   gen_RFoff(chnl);

   getelem(mp.tstep,mp.istep,mp.vstep);
   incr(mp.istep);
   add(mp.vphase,mp.vstep,mp.vstep);
   gen_RFfphase(mp.ta, chnl);              //-6cog11
   gen_RFphase(mp.vstep, chnl);
   delay((0.1229*mp.rtau) - mp.pw);
   gen_RFon(chnl);                         //x
   delay(mp.pw);
   gen_RFoff(chnl);

   delay((0.2029*mp.rtau) - mp.pw/2.0);
}

//-------------------------------------------
// Interrupted Decoupling with Refocussing
//-------------------------------------------

void _idref(WMPA mp, DSEQ d, int phase)
{

// Adjust del and mp.t1 assumming _desqon with preset1 = 0
// preceeds _idref.  Use both presets of 1 for the internal
// DSEQ module. 

   double adj = PWRF_DELAY + WFG_START_DELAY;
   double del = mp.rtau - mp.pw/2.0 - mp.t1 - adj;
   d = adj_dseq(d,&(mp.t1),&(mp.t1),1,1);  
   int chnl = 0;

   if (!strcmp(mp.ch,"obs")) chnl = 1;
   else if (!strcmp(mp.ch,"dec")) chnl = 2;
   else if (!strcmp(mp.ch,"dec2")) chnl = 3;
   else if (!strcmp(mp.ch,"dec3")) chnl = 4;
   else {
      printf("_idref() Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }

   char ch2[NCH];
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

   if ((mp.rtau - mp.pw/2.0 - mp.t1 - adj) <=0.0) {
      printf("_idref Error: Interrupt Time Too Large. Set t1 = 1.0/srate - pw/2.0");
      mp.t1 = mp.rtau - mp.pw/2.0;
   }   

   gen_RFphase(phase, chnl);
   gen_RFpwrf(mp.a, chnl);
   gen_RFunblank(chnl);
   delay(del);
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
      printf("_wpmlg() Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
   getelem(phase,ct,mp.vphase);
   mult(mp.vphase,mp.va,mp.vphase);      //PMLG Overall Phase
   gen_RFpwrf(mp.a,chnl);
   gen_RFunblank(chnl);
   sub(mp.istep,mp.istep,mp.istep);
   loop(mp.vcycles,mp.vcount);
      gen_RFblank(chnl);
      rcvron();
      delay(mp.dtaua);
      acquire(mp.npa,2.0e-7);
      rcvroff();
      gen_RFunblank(chnl);
      delay(mp.r1);
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
      printf("_wdumbo() Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
   getelem(phase,ct,mp.vphase);
   mult(mp.vphase,mp.va,mp.vphase);      //DUMBO overall phase
   gen_RFpwrf(mp.a,chnl);
   gen_RFunblank(chnl);
   sub(mp.istep,mp.istep,mp.istep);
   loop(mp.vcycles,mp.vcount);
      gen_RFblank(chnl);
      rcvron();
      delay(mp.dtaua);
      acquire(mp.npa,2.0e-7);
      rcvroff();
      gen_RFunblank(chnl);
      delay(mp.r1);
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
      printf("_wdumbot() Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
   getelem(phase,ct,mp.vphase);
   add(mp.vphase,one,mp.vb);
   add(mp.vphase,three,mp.vc);
   mult(mp.vphase,mp.va,mp.vphase);      //DUMBO overall phase
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

// ----------------------------------------------------------
// INEPT Transfer Between Two Channels
// ----------------------------------------------------------

void _inept(GP in, int ph1, int ph2, int ph3, int ph4)
{
   int chnl = 0;
   if (!strcmp(in.ch1,"obs")) chnl = 1;
   else if (!strcmp(in.ch1,"dec")) chnl = 2;
   else if (!strcmp(in.ch1,"dec2")) chnl = 3;
   else if (!strcmp(in.ch1,"dec3")) chnl = 4;
   else {
      printf("_inept() Error: Undefined Source Channel. Abort!\n");
      psg_abort(1);
   }

   int chnl2 = 0;
   if (!strcmp(in.ch2,"obs")) chnl2 = 1;
   else if (!strcmp(in.ch2,"dec")) chnl2 = 2;
   else if (!strcmp(in.ch2,"dec2")) chnl2 = 3;
   else if (!strcmp(in.ch2,"dec3")) chnl2 = 4;
   else {
      printf("_inept() Error: Undefined Destination Channel. Abort!\n");
      psg_abort(1);
   }

   if (chnl == chnl2) {
      printf("_inept() Error: Source and Destination on Same Channel. Abort!\n");
      psg_abort(1);
   }

   double t1a = in.t1;
   double t1b = in.t2;

   gen_RFphase(ph1, chnl); gen_RFphase(ph2, chnl2);
   gen_RFpwrf(in.a1, chnl); gen_RFpwrf(in.a2, chnl2);
   gen_RFunblank(chnl); gen_RFunblank(chnl2);
   delay(t1a);
   gen_RFsimPulse(in.pw1, in.pw2, ph1, ph2, 0.0, 0.0, chnl, chnl2);
   gen_RFphase(ph3, chnl); gen_RFphase(ph4, chnl2);
   delay(t1b);
   gen_RFsimPulse(in.pw3, in.pw4, ph3, ph4, 0.0, 0.0, chnl, chnl2);
}

//---------------------------------------------------------------------
// INEPT Transfer Between Two Channels with Multiplet Refocussing
//--------------------------------------------------------------------

void _ineptref(GP in, int ph1, int ph2, int ph3, int ph4, int ph5, int ph6)
{
   int chnl = 0;
   if (!strcmp(in.ch1,"obs")) chnl = 1;
   else if (!strcmp(in.ch1,"dec")) chnl = 2;
   else if (!strcmp(in.ch1,"dec2")) chnl = 3;
   else if (!strcmp(in.ch1,"dec3")) chnl = 4;
   else {
      printf("_ineptref() Error: Undefined Source Channel. Abort!\n");
      psg_abort(1);
   }

   int chnl2 = 0;
   if (!strcmp(in.ch2,"obs")) chnl2 = 1;
   else if (!strcmp(in.ch2,"dec")) chnl2 = 2;
   else if (!strcmp(in.ch2,"dec2")) chnl2 = 3;
   else if (!strcmp(in.ch2,"dec3")) chnl2 = 4;
   else {
      printf("_ineptref() Error: Undefined Destination Channel. Abort!\n");
      psg_abort(1);
   }

   if (chnl == chnl2) {
      printf("_ineptref() Error: Source and Destination on Same Channel. Abort!\n");
      psg_abort(1);
   }

   double t1a = in.t1;
   double t1b = in.t2;
   double t2a = in.t3;
   double t2b = in.t4;

   gen_RFphase(ph1, chnl); gen_RFphase(ph2, chnl2);
   gen_RFpwrf(in.a1, chnl); gen_RFpwrf(in.a2, chnl2);
   gen_RFunblank(chnl); gen_RFunblank(chnl2);
   delay(t1a);
   gen_RFsimPulse(in.pw1, in.pw2, ph1, ph2, 0.0, 0.0, chnl, chnl2);
   gen_RFphase(ph3, chnl); gen_RFphase(ph4, chnl2);
   delay(t1b);
   gen_RFsimPulse(in.pw3, in.pw4, ph3, ph4, 0.0, 0.0, chnl, chnl2);
   gen_RFphase(ph5, chnl); gen_RFphase(ph6, chnl2);
   delay(t2a);
   gen_RFsimPulse(in.pw1, in.pw2, ph5, ph6, 0.0, 0.0, chnl, chnl2);
   delay(t2b);
}

//---------------------------------------------------------------------
// Acquisition with Interleaved CPMG
//---------------------------------------------------------------------

void _cpmg(WMPA mp, int phase)
{
   int chnl = 0;
   if (!strcmp(mp.ch,"obs")) chnl = 1;
   else if (!strcmp(mp.ch,"dec")) chnl = 2;
   else if (!strcmp(mp.ch,"dec2")) chnl = 3;
   else if (!strcmp(mp.ch,"dec3")) chnl = 4;
   else {
      printf("_cpmg() Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }

   getelem(phase,ct,mp.vphase);
   txphase(mp.vphase);
   gen_RFpwrf(mp.a,chnl);
   acquire(mp.npa1, 2.0e-7);
   delay(mp.t1);
   loop(mp.vcycles,mp.vcount);
      rcvroff();
      gen_RFunblank(chnl);
      gen_RFphase(mp.vphase, chnl);
      delay(mp.r1);
      gen_RFon(chnl);
      delay(mp.pw);
      gen_RFoff(chnl);
      delay(mp.r2);
      rcvron();
      delay(mp.dtaua);
      acquire(mp.npa, 2.0e-7);
   endloop(mp.vcount);
}

//------------------------
// HMQC Evolution for F1
//------------------------

void _hmqc(GP hmqc, int ph1, int ph2, int ph3)
{
   int chnl = 0;
   if (!strcmp(hmqc.ch1,"obs")) chnl = 1;
   else if (!strcmp(hmqc.ch1,"dec")) chnl = 2;
   else if (!strcmp(hmqc.ch1,"dec2")) chnl = 3;
   else if (!strcmp(hmqc.ch1,"dec3")) chnl = 4;
   else {
      printf("_hmqc Error: Undefined Source Channel. Abort!\n");
      psg_abort(1);
   }

   int chnl2 = 0;
   if (!strcmp(hmqc.ch2,"obs")) chnl2 = 1;
   else if (!strcmp(hmqc.ch2,"dec")) chnl2 = 2;
   else if (!strcmp(hmqc.ch2,"dec2")) chnl2 = 3;
   else if (!strcmp(hmqc.ch2,"dec3")) chnl2 = 4;
   else {
      printf("_hmqc Error: Undefined Destination Channel. Abort!\n");
      psg_abort(1);
   }

   if (chnl == chnl2) {
      printf("_hmqc Error: Source and Destination on Same Channel. Abort!\n");
      psg_abort(1);
   }

   double tau = hmqc.t1;
   double d22 = hmqc.t2;

   gen_RFphase(ph1,chnl); gen_RFphase(ph2,chnl2);
   gen_RFpwrf(hmqc.a1,chnl); gen_RFpwrf(hmqc.a2,chnl2);
   gen_RFunblank(chnl); gen_RFunblank(chnl2);
   delay(tau);
   gen_RFPulse(hmqc.pw1,ph1,0.0,0.0,chnl2);
   gen_RFphase(ph3,chnl2);
   delay(d22);
   gen_RFPulse(hmqc.pw2,ph2,0.0,0.0,chnl);
   delay(d22);
   gen_RFPulse(hmqc.pw1,ph3,0.0,0.0,chnl2);
   delay(tau);
}

//---------------------------------------------
// HSSMALL for phase shift measurement
//---------------------------------------------

void _hssmall(WMPA mp, int phase)
{
   int chnl = 0;
   if (!strcmp(mp.ch,"obs")) chnl = 1;
   else if (!strcmp(mp.ch,"dec")) chnl = 2;
   else if (!strcmp(mp.ch,"dec2")) chnl = 3;
   else if (!strcmp(mp.ch,"dec3")) chnl = 4;
   else {
      printf("_hssmall() Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }
   getelem(phase,ct,mp.vphase);
   gen_RFfphase(mp.va,chnl);
   gen_RFpwrf(mp.a,chnl);
   printf("mp.dtau = %f\n",mp.dtau*1e6);
   printf("mp.dtaua = %f\n",mp.dtaua*1e6);
   if (mp.cycles > 0) {
      loop(mp.vcycles,mp.vcount);
         sub(mp.istep,mp.istep,mp.istep);

         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // x
         gen_RFfphase(mp.vb,chnl);
         delay(mp.dtau);

         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // y
         delay(mp.dtau);

         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // y
         gen_RFfphase(mp.va,chnl);
         delay(mp.dtau);

         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // x
         delay(mp.dtau);

         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // x
         gen_RFfphase(mp.vb,chnl);
         delay(mp.dtau);

         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // y
         delay(mp.dtau);

         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // y
         gen_RFfphase(mp.va,chnl);
         delay(mp.dtau);

         getelem(mp.tstep,mp.istep,mp.vstep);
         incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFPulse(mp.pw, mp.vstep, mp.r1, mp.r2, chnl);  // x
         gen_RFblank(chnl);
         rcvron();
         delay(mp.dtaua);
         acquire(mp.npa,2.0e-7);
         rcvroff();
      endloop(mp.vcount);
   }
}

#ifdef NVPSG 
 //------------------------------------------------------
 // Windowed SAMn - VNMRS Only
 //------------------------------------------------------

 void _wsamn(WMPA mp, int phase)
 {
   int chnl = 0;
   if (!strcmp(mp.ch,"obs")) chnl = 1;
   else if (!strcmp(mp.ch,"dec")) chnl = 2;
   else if (!strcmp(mp.ch,"dec2")) chnl = 3;
   else if (!strcmp(mp.ch,"dec3")) chnl = 4;
   else {
      printf("_wsamn() Error: Undefined Channel. Abort!\n");
      psg_abort(1);
   }

   getelem(phase,ct,mp.vphase);
   gen_RFunblank(chnl);
   sub(mp.istep,mp.istep,mp.istep);
   loop(mp.vcycles,mp.vcount);
      gen_RFblank(chnl);
      rcvron();
      delay(mp.dtaua);
      acquire(mp.npa,2.0e-7);
      rcvroff();
      gen_RFunblank(chnl);
      delay(mp.r1);
      gen_RFon(chnl);
      loop(mp.vcycles1,mp.vcount1); //Loop Over SAM Amplitude and Phase Steps
         getelem(mp.tstep,mp.istep,mp.vstep); //Element from Phase Table
         getelem(mp.ta,mp.istep,mp.va);       //Element from Amplitude Table
	 incr(mp.istep);
         add(mp.vphase,mp.vstep,mp.vstep);
         gen_RFfphase(mp.vstep, chnl);        //Set the Phase
         gen_RFvpwrf(mp.va, chnl);            //Set the Amplitude
         delay(mp.pw);
      endloop(mp.vcount1);
      gen_RFoff(chnl);
      delay(mp.r2);
   endloop(mp.vcount);
   gen_RFunblank(chnl);
 }
#endif 

