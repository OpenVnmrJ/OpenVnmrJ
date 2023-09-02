/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <math.h>
#include <string.h>
#include "RFController.h"
#include "FFKEYS.h"
#include "ACode32.h"
#include "Console.h"
#include "PSGFileHeader.h"
#include "cpsg.h"
#include "aptable.h"
#include "WaveformUtility.h"
#include "Bridge.h"

#include <arpa/inet.h>

#define WriteWord( x )  pAcodeBuf->putCode( x )
#define putPattern( x ) pWaveformBuf->putCode( x )
#define OFFSETPULSE_TIMESLICE (0.00000020L)

extern int bgflag;
extern int dps_flag;
extern int rcvroff_flag;
extern int rcvr_is_on_now;
extern unsigned int ix;
extern int res_1_internal;
extern int res_2_internal;

static double mintpwr=-37.0; // if iCAT
static double maxtpwr=63.0;
static double tpwrstep=0.5;  // if iCAT

int debug_ampfit=0;
int debug_icat=0;

bool isICAT() {
	if(debug_icat)
		return true;
	if (P2TheConsole->getConsoleSubType() == 's')
		return true;
	else{
	    tpwrstep=1.0;
	    mintpwr=-16;
		return false;
	}
}

//
double RFController::xmtrHiLowFreq= 150.0;
double RFController::ampHiLowFreq=whatamphibandmin(0, 0.0);
double RFController::h1freq = 0.0;
double RFController::systemIF = 20.0; /* MHZ */

//
//
// RF Controller Constructor
//
//

RFController::RFController(char *name,int flags)
 : Controller(name,flags)
 , mySynthesizer(name,flags)
 , powerWatch(1.0,10.0,name)
 {
        patternDataStoreSize = 4000;
        patternDataStore = new int[patternDataStoreSize];
        patternDataStoreUsed = 0;

        progDecInterLock = 0;
        progDecMarker    = -1;
        auxHomoDecTicker = 0LL;
        p2Dec = NULL;
        kind = RF_TAG;
        if ( pAcodeBuf == NULL)
           abort_message("psg internal unable to allocate acode buffer for RF controller %s. abort!\n",Name);

       CW_mode=0; // off..
       observeFlag = 0;
       trs = 0L;
       defAmp = 4095.0;
       defAmpScale = 4095.0;
       defVAmpStep = 16.0;
       currentVAmpStep = defVAmpStep;
       defPhase = 0.0;
       defPhaseCycle = 0.0;
       currentPhaseStep = 0.0;  // for legacy small angle handling..
       baseFreq = 0.0;
       currentFreq = 0.0;
       baseOffset = 0.0;
       baseTpwr = 0.0;
       maxAllowedUserPower=63.0;
       // Logic is finished at initializeExp
       // gateDefault and RF_pulse_gates default to observe conditioning..
       gateDefault    = RFGATEON(RF_MIXER|R_LO_NOT) | RFGATEON(RF_TR_GATE) | RFGATEOFF(RF_OBS_PULSE_LIQUIDS);
       RF_pre_gates   = RF_OBS_PRE_LIQUIDS;
       RF_pulse_gates = RF_OBS_PULSE_LIQUIDS;
       RF_post_gates  = RF_OBS_POST_LIQUIDS;
       // blank is on and unblankCalled are complements? indicates quiescent blanking state..
       blank_is_on    = 1;
       unblankCalled  = 0;
       userGates=0;
       rfIsEnabled = TRUE;
       rfSharingEnabled = FALSE;
       // pattern alias list related
       patAliasListSize = 131072;
       patAliasList     = new int[patAliasListSize]; // (int *) malloc(patAliasListSize*sizeof(int));
       if (patAliasList == NULL)
          abort_message("psg internal unable to allocate memory for pattern alias list on %s. abort!\n",Name);
       patAliasListIndex= 0;

       strcpy(logicalName,""); // this initialization was missing
       strcpy(coarsePowerName,"NONE");
       strcpy(finePowerName,"NONE");
       strcpy(finePowerName2,"NONE");
       strcpy(tunePwrName,""); // this initialization was missing
       strcpy(freqName,"NONE");
       strcpy(freqOffsetName,"NONE");
       strcpy(dmName,"NONE");
       strcpy(dmmName,"NONE");
       strcpy(dmfName,"NONE");
       strcpy(dseqName,"NONE");
       strcpy(homoName,"NONE");
       strcpy(dresName,"NONE");
       strcpy(nucleusName,""); // this initialization was missing
       prevStatusDmIndex  = -1;
       prevStatusDmmIndex = -1;
       prevSyncType       = 1;
       prevAsyncScheme    = 0;


}

// other stuff freq.. base..
//
// use instead of realloc..
// malloc and realloc clash with new/delete in cpp files
//
int *RFController::increaseDataStore(int *origbuffer, int origsize, int finalsize)
{
  int *p1,*p2,*newpntr,i;
  p1 = origbuffer;
  p2 = newpntr = new int[finalsize];
  if (p2 == NULL)
    abort_message("psg internal error unable to allocate enough memory on %s\n",Name);
  for (i=0; i < origsize; i++)
    *p2++ = *p1++;
  delete [] origbuffer;
  return(newpntr);
}


void RFController::setMaxUserLevel(double l)
{
  if (l > 63.0) l = 63.0;
  if (l < 0.0)  l = 0.0;
  maxAllowedUserPower=l;
}


int RFController::getLogicalID()
{
  return logicalID;
}


void RFController::getNucleusName(char *nucName)
{
  if (getparm(nucleusName,"string",CURRENT,nucName,8) == 1)
     abort_message("parameter %s not defined. abort!\n",nucleusName);
  // fprintf(stdout,"%s is set to %s\n",nucleusName,nucName);
}


void RFController::setLogicalNames(int id, char rfNames[][8])
{
   //rfNames[][] = {"obs","tof","tpwr","tpwrf","xm","xmm","xmf","xseq","xhomo","sfrq","dres","xpwrt","tpwrm","tn"};

   logicalID = id;
   strcpy(logicalName,     rfNames[0]);
   strcpy(freqOffsetName,  rfNames[1]);
   strcpy(coarsePowerName, rfNames[2]);
   strcpy(finePowerName,   rfNames[3]);
   strcpy(dmName,          rfNames[4]);
   strcpy(dmmName,         rfNames[5]);
   strcpy(dmfName,         rfNames[6]);
   strcpy(dseqName,        rfNames[7]);
   strcpy(homoName,        rfNames[8]);
   strcpy(freqName,        rfNames[9]);
   strcpy(dresName,        rfNames[10]);
   strcpy(tunePwrName,     rfNames[11]);
   strcpy(finePowerName2,  rfNames[12]);
   strcpy(nucleusName,     rfNames[13]);

// sfrq, dfrq, etc, need to be know early on ( before initializeExpStates() )
// so let's look it up now.
  if (P_getreal(CURRENT, freqName,&baseFreq,1) < 0)
               baseFreq = 0.0;
  if (P_getreal(CURRENT ,freqOffsetName,&baseOffset,1) < 0)
               baseOffset  = 0.0;
  baseFreq -= (baseOffset/1000000.0L);

   if (strcmp(logicalName,"obs") == 0)
       observeFlag = 1;
   else
       observeFlag = 0;
}

int RFController::degrees2Binary(double xx)
{
   int tmp;
   while (xx < 0.0)
      xx += 360.0;
   tmp = (int) (fmod(xx,360.0)*65536.0/360.0 + 0.499999999999999999999999);
   tmp &= 0xffff;
   return(tmp);
}
// use only with RT Math...
int RFController::degrees2XBinary(double xx)
{
   double tmp;
   while (xx < 0.0)
      xx += 360.0;
   // 2^31 precision rt math down shifts by 15!
   tmp = (int) (fmod(xx,360.0)*32768.0*65536.0/360.0 + 0.499999999999999999999999);
   return((int) tmp);
}
// 4095.0 is the fine power standard
int RFController::amp2Binary(double xx)
{
   if (xx > 4095.0) xx = 4095.0; // clip for now..
   if (xx < 0)  xx = 0.0;
   xx *= 65535.0/4095.0;
   return((int) xx);
}

void RFController::setAmpScale(double xx)
{
   int tmp;
   powerWatch.eventFinePowerChange(xx/4095.0,getBigTicker());
   tmp = amp2Binary(xx) | RFAMPSCALEKEY;
   add2Stream(tmp);
}

// xx range 0.0 to 1.0
// 
void RFController::setPowerFraction(double xx)
{
     powerWatch.eventFineCalibrateUpdate(xx, getBigTicker());
}

void RFController::setAmp(double xx)
{
   int tmp;
   tmp = amp2Binary(xx) | RFAMPKEY;
   add2Stream(tmp);
}

//
void RFController::setVAmpScale(double scale, int rtvar)
{
   int buffer[4];
   // should set to full for safety..
   //powerWatch->eventFinePowerChange(xx,getBigTicker());
   buffer[0] = amp2Binary(scale*currentVAmpStep/defVAmpStep);
   buffer[1] = rtvar;     // the rtvar to use
   outputACode(VRFAMPS,2,buffer); // out they go....
}


void RFController::setVAmpStep(double step)
{
  if (P2TheConsole->getConsoleSubType() == 's')
  {
      if (step < 1.0)
      {
          step = 1.0;
          if (bgflag)
            printf(
                "advisory: real-time amplitude step size reset to 1 on %s controller\n",
                Name);
      }
      if (step > 16.0)
      {
          step = 16.0;
          if (bgflag)
            printf(
                "advisory: real-time amplitude step size reset to 16 on %s controller\n",
                Name);
      }

      currentVAmpStep = step;
  }
  else
  {
      if (bgflag)
        printf(
            "advisory: real-time amplitude step size not applicable on this console\n");
  }
}


void RFController::setPhaseCycle(double pcycle)
{
   int tmp;
   tmp = degrees2Binary(pcycle) | RFPHASECYCLEKEY;
   add2Stream(tmp);
}

void RFController::setPhase(double pcycle)
{
   int tmp;
   tmp = degrees2Binary(pcycle) | RFPHASEKEY;
   add2Stream(tmp);
}

//
//   setVQuadPhaseCycle(int rtvar)
//
//   extensive use for legacy C quadrature phase elements.
//   uses VRFPHASECQ acode.
//
void RFController::setVQuadPhase(int rtvar)
{
   int buffer[4];
   int trtvar;
   trtvar = rtvar;
   if (rtvar >= BASEINDEX)
   {
     // check if table already written, if not write it
     doWriteTable(rtvar);

     buffer[0] = Table[rtvar-BASEINDEX]->acodeloc;
     buffer[1] = Table[rtvar-BASEINDEX]->indexptr ;
     buffer[2] = res_1_internal;
     outputACode(TASSIGN,3,buffer); // out they go..
     trtvar = res_1_internal;
   }
   //
   buffer[0] = trtvar;    // the rtvar to uses
   outputACode(VPHASECQ,1,buffer); // out they go..
}
//
//   creates an rt calculation prior to a phase word load
//   phasec <= currentPhaseStep * rtvar
//
//   supports tables - although not defined in C
//   2^31 precision ONLY LEGAL use of degrees2XBinary .
//
//
void RFController::setVFullPhase(int rtvar)
{
   int buffer[4];
   int trtvar;
   trtvar = rtvar;
   if (rtvar >= BASEINDEX)
   {
     // check if table already written, if not write it
     doWriteTable(rtvar);

     buffer[0] = Table[rtvar-BASEINDEX]->acodeloc;
     buffer[1] = Table[rtvar-BASEINDEX]->indexptr ;
     buffer[2] = res_2_internal;
     outputACode(TASSIGN,3,buffer); // out they go..
     trtvar = res_2_internal;
   }
   buffer[0] = degrees2XBinary(currentPhaseStep);  // phase multiplier..
   buffer[1] = trtvar;    // the rtvar to uses
   outputACode(VPHASEC,2,buffer); // out they go..
}

void RFController::setPhaseStep(double value)
{
  currentPhaseStep = value;
}

//  6/6/04 .. gate fields to 12 bits + 12 bits mask
//  psg macros  RFGATEON(gate)  (gate << 12) | gate
//              RFGATEOFF(gate)  (gate << 12)
//  gate def's in FFKEYS.h
//
void RFController::setGates(int GatePattern)
{
   long long word;
   word = (GatePattern & RFGATEON(X_OUT)) == RFGATEON(X_OUT);
   powerWatch.eventGateChange(word,getBigTicker());
   add2Stream(GATEKEY | (GatePattern & 0xffffff));
}

//
// BDZ 03-15-2023 - made the code match the original developer's intent
//                  and made it easier to read.

void RFController::setUserGate(int gate, int turn_on)
{
  int buffer[2];
  int mask, action;

  if (gate < 1) {
    
    abort_message("RFController::setUserGate() input error: specified gate is < 1");
  }
  else if (gate <= 3) {
   
    // affect one gate: 1 or 2 or 3

    mask = (1 << (gate - 1)); 
  }
  else { // gate > 3

    // affect all three gates

    mask = 7;
  }  

  if (turn_on) {
 
    action = mask;
  }
  else {

    action = 0;
  }

  buffer[0] = (mask & 7) << 3 | (action & 7);
  
  outputACode(RFSPARELINE,1,buffer); // out they go..
}

//
//  does the common pulse pre amble statements.
//
//  notes on gates..
//  R_LO_NOT should be on for all but decouplers - else should follow rf_std_xmtr
//  CW_mode does not (yet) switch during an experiment..
//
void RFController::pulsePreAmble(double rof1, int Vvar4Phase)
{
	long long temp;
   temp = calcTicks(rof1);
   if (temp < 4L)
    temp = 0;
   pulsePreAmble(temp,Vvar4Phase);
}

void RFController::pulsePreAmble(long long rx1Ticks, int Vvar4Phase)
{
   char PDDacquireStr[MAXSTR];

   /* PDD control signal for switched transmit-receive imaging external pre-amp configuration */
   /* Global parameter PDDacquire determines whether switching is primarily about acquisition or RF pulses */
   getStringSetDefault(GLOBAL,"PDDacquire",PDDacquireStr,"u"); /* undefined if PDDacquire doesn't exist */
   if (isObserve() && (PDDacquireStr[0] == 'n')) splineon(3);

   setGates(RFGATEON(RF_pre_gates));
   rcvr_is_on_now = 0;
   setVQuadPhase(Vvar4Phase);
   setTickDelay(rx1Ticks);
   setGates(RFGATEON(RF_pulse_gates));
}

// unified signature
void RFController::pulsePostAmble(double rof2)
{
	long long temp;
	temp = calcTicks(rof2);
	if (temp < 4L)
	    temp = 0;
    pulsePostAmble(temp);
}

void RFController::pulsePostAmble(long long rx2Ticks)
{
   char PDDacquireStr[MAXSTR];

   setGates(RFGATEOFF(RF_pulse_gates));
   setTickDelay(rx2Ticks);
   setPhase(defPhase);
   setAmp(defAmp);
   if (blank_is_on)
     setGates(RFGATEOFF(RF_post_gates | RFAMP_UNBLANK));
   else
     setGates(RFGATEOFF(RF_post_gates));

   /* PDD control signal for switched transmit-receive configuration */
   /* Global parameter PDDacquire determines whether switching is primarily about acquisition or RF pulses */
   getStringSetDefault(GLOBAL,"PDDacquire",PDDacquireStr,"u"); /* undefined if PDDacquire doesn't exist */
   if (isObserve() && (PDDacquireStr[0] == 'n')) splineoff(3);  
}

void RFController::unblankAmplifier()
{
   if (isObserve())
     setGates(RFGATEON(RFAMP_UNBLANK | X_IF | R_LO_NOT));
   else
     setGates(RFGATEON(RFAMP_UNBLANK | X_IF));

   blank_is_on   = 0;
   unblankCalled = 1;
}


void RFController::blankAmplifier()
{
   if (isObserve())
     setGates(RFGATEOFF(RFAMP_UNBLANK | X_IF | X_LO | X_OUT | R_LO_NOT));
   else
     setGates(RFGATEOFF(RFAMP_UNBLANK | X_IF | X_LO | X_OUT));

   blank_is_on   = 1;
   unblankCalled = 0;
}

int RFController::getBlank_Is_On()
{
  return blank_is_on;
}


void RFController::setTxGateOnOff(int state)
{
  if (state)
    setGates( RFGATEON(RF_pulse_gates));
  else
    setGates(RFGATEOFF(RF_pulse_gates));

  if ( isObserve() )
  {
     if ( (! unblankCalled) && state && (ix==1) )
        text_message("transmitter on/off programmed without amplifier blank control for %s. rf pulse may have no power!\n", logicalName);
  }
  else
  {
     if ( (! unblankCalled) && state && (ix==1) )
        text_message("program explicit unblank/blank on channel %s for correct IF gate control for pulse output\n", logicalName);
  }
}


//
//  progDecOn turns on for all decooupler functions ....
//  it places the channel on the ASYNC status so that it accounts for time but does not
//  time along..  prog dec on initializes the structures and exits...
//  prog dec off resolved the patterns overall duration into N whole cycles with M state
//  left over and a few ticks in the end...

void RFController::progDecOn(char *name, double pw_90, double deg_res, int mode, const char *emsg)
{
  long long temp;
  int numreps, buffer[12];
  //
  // Check the interlock if already locked complain and die...
  //
  if ((progDecInterLock != 0) || (p2Dec != 0) )
  {
     abort_message("decoupler already in use %s",emsg);
  }

  if (bgflag) { cout << "progDecOn(): name=" << name<<"  1/dmf="<<pw_90<<"  dres="<< deg_res<<"  msg="<< emsg<< endl; }

  //  NOW LOCK IT DOWN...
  progDecInterLock = 1;
  P2TheConsole->setProgDecInterLock();

  clearSmallTicker();
  progDecMarker = getBigTicker();

  p2Dec = resolveDecPattern(name, (int)deg_res, pw_90, 1, mode, emsg);
  setAsync();  // track time do not copy delays

  // use a temporary long long...
  temp = calcTicks(pw_90*deg_res/90.0);
  if (temp > 0x3ffffffL)
     abort_message("programmable decoupling state duration too long on %s. abort!\n",logicalName);

  decTicks = (int) temp;
  // should done max and min decTick checks...
  // as well as variance warning..
  if (decTicks < (int)(P2TheConsole->getMinTimeEventTicks()) )
  {
     abort_message("effective dmf too high %s on controller %s",emsg,logicalName);
  }
  if (decTicks > 0x3ffffff)
  {
      abort_message("progDecOn time base violation %s",emsg);
  }


  buffer[0] = p2Dec->getReferenceID();

  union64 lscratch;
  lscratch.ll = temp * (p2Dec->getNumberStates());
  buffer[1] = lscratch.w2.word0;
  buffer[2] = lscratch.w2.word1;

  lscratch.ll = temp;
  buffer[3] = lscratch.w2.word0;
  buffer[4] = lscratch.w2.word1;

  buffer[5] = (int)(p2Dec->getNumberWords()/p2Dec->getNumberStates());
  buffer[6] = 0;

  numreps   = p2Dec->getNumReplications();
  lscratch.ll = temp*(p2Dec->getNumberStates())/((int)(getval("nt")) * numreps);
  buffer[7] = lscratch.w2.word0;
  buffer[8] = lscratch.w2.word1;

  buffer[9] = numreps;
  buffer[10] = 0;
  buffer[11] = 0;  // auto ramper off...

  outputACode(DECPROGON,12,buffer);
  powerWatch.eventGateChange(1,getBigTicker());
  setPowerFraction(p2Dec->getPowerFraction());  // set power calculation - reset on exit
  setChannelActive(1);

  if (bgflag)
  {
    cout << "DECPROGON   " << emsg << endl;
    cout << "ID                   " << buffer[0] << endl;
    cout << "Duration of a Cycle  " << temp * (p2Dec->getNumberStates())*12.5e-9 << endl;
    cout << "Power scale of pattern " << p2Dec->getPowerFraction() << endl;
    cout << "Duration of a State  " << temp*12.5e-9 << endl;
    cout << "Words per State      " << buffer[5] << endl;
  }
}
//
//  progDecOnOffset is used specially to frequency shift a decouplering pattern
//  it is called explicitly
//  it places the channel on the ASYNC status so that it accounts for time but does not
//  time along..  prog dec on initializes the structures and exits...
//  prog dec off resolved the patterns overall duration into N whole cycles with M state
//  left over and a few ticks in the end...

void RFController::progDecOnWithOffset(char *name, double pw_90, double deg_res, double foff,
                                       int mode, const char *emsg)
{
  int numreps, buffer[15];
  long long temp;
  //
  // Check the interlock if already locked complain and die...
  //
  if ((progDecInterLock != 0) || (p2Dec != 0) )
  {
     abort_message("decoupler already in use %s",emsg);
  }

  { cout << "progDecOn(): name=" << name<<"  1/dmf="<<pw_90<<"  dres="<< deg_res<<"  msg="<< emsg<< endl; }

  //  NOW LOCK IT DOWN...
  progDecInterLock = 1;
  P2TheConsole->setProgDecInterLock();

  clearSmallTicker();
  progDecMarker = getBigTicker();
  //  FUNCTION
  p2Dec = resolveDecOffsetPattern( name, (int)deg_res, pw_90, foff, 1, emsg);
  setAsync();  // track time do not copy delays

  decTicks = (int) (P2TheConsole->getMinTimeEventTicks());
  temp = (long long) decTicks;
  //fix on 4 fifo underflow issues.
  // temp = 4L;
  // dectTicks = (int) temp;

  buffer[0] = p2Dec->getReferenceID();

  union64 lscratch;
  lscratch.ll = temp * (p2Dec->getNumberStates());
  int eventcount = p2Dec->getNumberStates();
  //double deltaph = 360.0*foff*0.000000050; // full precision in degrees
  double deltaph = 360.0*foff*(12.5e-9*decTicks);
  double adj = fmod(deltaph*((double) eventcount),360.0);

  buffer[1] = lscratch.w2.word0;
  buffer[2] = lscratch.w2.word1;

  lscratch.ll = temp;
  buffer[3] = lscratch.w2.word0;
  buffer[4] = lscratch.w2.word1;

  buffer[5] = (int)(p2Dec->getNumberWords()/p2Dec->getNumberStates());
  buffer[6] = 0;

  numreps   = p2Dec->getNumReplications();
  lscratch.ll = temp*(p2Dec->getNumberStates())/((int)(getval("nt")) * numreps);
  buffer[7] = lscratch.w2.word0;
  buffer[8] = lscratch.w2.word1;

  buffer[9] = numreps;
  // these are auto ramper adds..
  buffer[10] = degrees2XBinary(adj);
  buffer[11] = 0x10000000;

  outputACode(DECPROGON,12,buffer);
  powerWatch.eventGateChange(1,getBigTicker());
  setPowerFraction(p2Dec->getPowerFraction());  // set power calculation - reset on exit
  setChannelActive(1);
  if (bgflag)
  {
    cout << "DECPROGON   " << emsg << endl;
    cout << "ID                   " << buffer[0] << endl;
    cout << "Duration of a Cycle  " << temp * (p2Dec->getNumberStates())*12.5e-9 << endl;
    cout << "Power scale of pattern " << p2Dec->getPowerFraction() << endl;
    cout << "Duration of a State  " << temp*12.5e-9 << endl;
    cout << "Words per State      " << buffer[5] << endl;
  }
}
//
// progDecOff resolves it all ...
// The time (the other channels are using) is broken into
// number of whole passes
// number of words
// ticks remaining
// the pattern runs for npasses*nstates+remstates
// the time remaining is just padded..
// 1 decTicks is the maximum error...
// I have left the acode to work out the details...
// VDELAY Will probaby bump the interlock and change this
// .. uses MULTIPATTERN ACODE...
//
// not checked against overflow of npasses...
// could programmably re-issue multi-loop...
//


void RFController::progDecOff(int decPolicy, const char *emsg, int syncType, int asyncScheme)
{
    long long ticks = 0L;
    int buffer[6];
    union64 lscratch;

    if (progDecInterLock != 1)
    {
       abort_message("invalid usage of obs/decprgoff on %s in pulse sequence. no obs/decprgon statement. abort!\n",logicalName);
    }

    if (progDecMarker >= 0)
    {
       ticks = getBigTicker() - progDecMarker;
    }
    else
       abort_message("invalid usage of obs/decprgoff on %s in pulse sequence. no obs/decprgon statement. abort!\n",logicalName);

    char homo[MAXSTR];
    getstr("homo",homo);
    if (strcmp(homo,"y") == 0)
    {
       ticks -= getAuxHomoDecTicker();
       setAuxHomoDecTicker(0LL);
    }

    if ( ticks <= 0 )
    {
       if (bgflag) text_message("decoupling duration is zero or negative on %s. check delays\n",logicalName);

       decPolicy = DECZEROTIME;
       lscratch.ll = 0LL;

       buffer[0] = 0;
       buffer[1] = decPolicy;
       buffer[2] = 0;
       buffer[3] = 0;
       buffer[4] = syncType;
       buffer[5] = asyncScheme;
    }
    else
    {
       setGates(RFGATEON(RF_pre_gates | RF_pulse_gates));

       add2Stream(DURATIONKEY | decTicks);

       buffer[0] = p2Dec->getReferenceID();
       buffer[1] = decPolicy;

       lscratch.ll = ticks;
       buffer[2] = lscratch.w2.word0;
       buffer[3] = lscratch.w2.word1;
       buffer[4] = syncType;
       buffer[5] = asyncScheme;

    }

    outputACode(DECPROGOFF, 6, buffer);
    powerWatch.eventGateChange(0,getBigTicker());
    setPowerFraction(1.0); // resets pattern bias
    if (bgflag)
    {
      if ( (ix == 1) && (syncType == 2) )
        text_message("advisory: decoupler is in asynchronous mode on %s\n",getName());

      cout << "DECPROGOFF   " << emsg << endl;
      cout << "ID                   " << buffer[0] << endl;
      cout << "Policy:              " << buffer[1] << endl;
      cout << "total dec duration = " << ticks*12.5e-9 << endl;
    }

    progDecMarker = -1;

    if ( (decPolicy == DECSTOPEND) || (decPolicy == DECZEROTIME) )
    {
      if (blank_is_on)
        setGates(RFGATEOFF(RF_pulse_gates | RF_post_gates | RFAMP_UNBLANK));
      else
        setGates(RFGATEOFF(RF_pulse_gates | RF_post_gates));

      setAmp(4095.0);  // reset amp reg back to default
      setPowerFraction(1.0);
    }

    p2Dec=NULL;        // pointer is still kept in cPattern list...
    progDecInterLock = 0;
    P2TheConsole->clearProgDecInterLock();

    setSync();         // now you have to follow synchronously....

}

//
//
//
void RFController::loopAction()
{
    int buffer[2];
    union64 lscratch;

    // get loop ticks and send it down in VLOOPTICKS acode
    if (progDecInterLock == 1)
    {
      if (P2TheConsole->getNestedLoopDepth() >= 2)
      {
        abort_message("obs/decprgon on %s followed by 2 or more nested loops not implemented. abort!\n",logicalName);
      }

      lscratch.ll = P2TheConsole->getTicker(LOOP_TICKER);
      buffer[0]   = lscratch.w2.word0;
      buffer[1]   = lscratch.w2.word1;
      outputACode(VLOOPTICKS,2,buffer);
    }

}


//  setStatus sorts out the status decoupling. With vdelays it will require
//  rt adjustments parse ahead to resole
//  normally tho, a call will finish any postponed resolution and open a new
//  phase..  status(A) ... event stream ... status(B) element tracks the
//  duration of the interval, controls started at A will not be resolved
//  at PSG time until B is parsed....
//
void RFController::setStatus(char ch)
{
    return;
}
//
//
//
void RFController::addToPatAliasList(int newpat, int oldpat)
{
   // first check if patAliasList needs to be made bigger?
   if ((patAliasListIndex+2) > patAliasListSize)
   {
      patAliasList   = increaseDataStore(patAliasList,patAliasListSize,patAliasListSize+65536);
      //patAliasList     = (int *) realloc(patAliasList,(patAliasListSize+65536)*sizeof(int));
      patAliasListSize = patAliasListSize + 65536;
      if (bgflag)
         text_message("advisory: memory re-allocation done for pattern alias list for %s controller\n",logicalName);
   }

   patAliasList[patAliasListIndex++] = newpat;
   patAliasList[patAliasListIndex++] = oldpat;
   if (bgflag)
      cout << Name << " controller pattern " << newpat << " aliased to "<< oldpat << endl;
}

//
//
//
int RFController::getPatternAlias(int newpatid)
{
  for(int i=0; i<patAliasListIndex; i=i+2)
  {
    if (patAliasList[i] == newpatid)
       return(patAliasList[i+1]);
  }
  return(0);
}

//
//
//
void RFController::writePatternAlias(void)
{
    if (patAliasListIndex > 0)
    {
       /* 0x7FFFFF is a special pattern id, reserved for the pattern alias list */
       /* bits 0-23 of COMBOHEADER are reserved for waveform numbering         */

       if (patternCount >= 0x7FFFFF)
           abort_message("too many waveforms on %s controller. abort!\n",logicalName);

       patternCount++ ;
       int patAliasID  = 0x7FFFFF ;   /* Pattern Alias 'pattern' has a special id 0x7FFFFF */
       pWaveformBuf->startSubSection(PATTERNHEADER+patAliasID);

       for(int i=0; i<patAliasListIndex; i++)
       {
          putPattern(patAliasList[i]);
       }
       char nm[32];
       strcpy(nm,"pattern_alias_list");

       cPatternEntry *tmp = new cPatternEntry(nm,1,patAliasID,patAliasListIndex,patAliasListIndex);
       addPattern(tmp);
       sendPattern(tmp);
       if (bgflag)
          cout << "write pattern alias file done for " << Name << " controller " << (patAliasListIndex-2) << " entries" << endl;
    }
}

//
//  this element is core to shaped pulses what in and out off the
//  object seems artificial boundary ...
//  finds data base or opens and parses files creates the patterns
//
//  SCALING patterns have a standard 1023 max and tpwrf is 4095
//  since these are merged some confusion results..
//
cPatternEntry *RFController::resolveRFPattern(char *nm, int flag, const char *emsg, int action)
{
    int i,l,value;
    double f1,f2,f3,f4,f5,scalef;
    double lastamp,lastphase;
    double pwrf;
    int latchphase;
    int wcount, eventcount, repeats;
    char tname[200];
    // do we already know the pattern??
    cPatternEntry *tmp = find(nm, flag);
    scalef = 1023.0;
    pwrf = 0.0;
    if (tmp != NULL)
      return(tmp);
    strcpy(tname,nm);
    strcat(tname,".RF");
    wcount = 0; eventcount = 0;
       i = findPatternFile(tname);
       if (i != 1)
	 {
           text_error("%s could not find %s", emsg, tname);
	   if (action > -1)
             psg_abort(1);
           else
	     return(NULL);
         }

       patternCount++;
       pWaveformBuf->startSubSection(PATTERNHEADER+patternCount);

       // initialize first level optimizer..
       latchphase = TRUE;
       lastamp = -10000.0;  // illegal value
       lastphase = -100012.3; // unlikely value
       // read in the file
       do
      {
         l = scanLine(&f1,&f2,&f3,&f4,&f5);
         if ((l > 2) && (f3 < 0.5))
         {
              scalef = f2;
              continue;
         }

         latchphase = TRUE;
         if ((l > 1) &&  ((lastamp != f2) && (lastphase == f1)))
            latchphase = FALSE;
         repeats = 1;
         if (l > 2) // 255 is low??
           repeats = ((int) f3) & 0xff;
         if ((l > 1) && (lastamp != f2))
         {
           // adjust this to the scalef
           //cout << "lastamp = " << lastamp << "   this amp " << f2 << endl;
           lastamp = f2;
           value = RFAMPKEY | amp2Binary(f2*4095.0/scalef) ;
           if (latchphase == FALSE)
             value |= LATCHKEY;
           putPattern( value );
           wcount++;
         }
        if (l > 0)
	  {
             pwrf += lastamp*lastamp*repeats/(scalef*scalef);
             if (latchphase == TRUE)
             {
             value = degrees2Binary(f1) | RFPHASEKEY | LATCHKEY ;
             lastphase = f1;
             putPattern( value );
             wcount++;
             }
             eventcount += repeats;
             repeats--;
             if (repeats > 0)
	       {
                  value = LATCHKEY | RFPHASEINCWORD(0, repeats, 1) ;
                  putPattern( value );
	          wcount++;
	       }
	  }
      }  while (l > -1);
     if (eventcount > 0)
     {
        pwrf = sqrt((pwrf)/((double) eventcount));
        tmp = new cPatternEntry(nm,flag,eventcount,wcount);
        tmp->setPowerFraction(pwrf);  // use this in calling code.
        addPattern(tmp);
        sendPattern(tmp);
     }
     else
        abort_message("file %s has no events",nm);
    return(tmp);
}
//
//  this element is core to decoupling pulses what in and out off the
//  object seems artificial boundary ...
//  finds data base or opens and parses files creates the patterns
//  - there is a better mechanism ..
//  flag toggles between phase resolution of 1 degree increments
//  probably needs improvements...
//
//  as of 10 Aug 06, decoupler can optimized patterns for synchronous dm=nns or progDecOn
//  not implemented for async modes. *yet*
//
cPatternEntry *RFController::resolveDecPattern(char *nm, int flag, double pw_90, int wfgdup, int mode, const char *emsg)
{
    int i,l,out;
    double f1,f2,f3,f4,f5,scalef,tipResolution;
    double lastgate,lastamp,pwrf;
    int wcount, eventcount, repeats;
    int bStore[8],bCntr;
    int phaseWord, ampWord, gateWord;
    int lastGateWord, lastAmpWord, lastPhaseWord;
    char tname[200],uname[200];
    double ampScale = 1023.0;
    strcpy(tname,nm);
    strcat(tname,".DEC");
    sprintf(uname,"%s_%d",nm,mode);
    // do we already know the pattern??
    // disambiguate synchronous/async mode 
    cPatternEntry *tmp = find(uname, flag);
    if (tmp != NULL)
      return(tmp);
    scalef = 4095.0/1023.0;
    lastamp=1.0;  // covers default case
    lastgate=1.0;
    pwrf = 0.0;
    if (flag < 1)   // safety..
      flag = 1;
    // better clipping ...
    tipResolution = (double) flag;
    wcount = 0;
    eventcount = 0;
    ampWord = gateWord = 0;
    // set to illegal or unusual values.
    lastGateWord = 21839;
    lastAmpWord = -21839;
    lastPhaseWord = -21839;
    bCntr=0;
    i = findPatternFile(tname);
    if (i != 1)
    {
           abort_message("%s could not find %s", emsg, tname);
    }

    patternCount++;
    pWaveformBuf->startSubSection(PATTERNHEADER+patternCount);

    int firstTime = 1;
    do
    {
       l = scanLine(&f1,&f2,&f3,&f4,&f5);

       if ( (l >= 3) && ((int)f1 == 0) )
       {
           ampScale = f3;
           continue;
       }

       switch( l )
       {
         case 4:
               if (((int)f4) & 0x1)
               {
                  gateWord = GATEKEY | (RFGATEON(X_OUT)  & 0xffffff);
                  lastgate = 1.0;
               }
               else
               {
                  gateWord = GATEKEY | (RFGATEOFF(X_OUT) & 0xffffff);
                  lastgate = 0.0;
               }
               if ((mode != 1) || (gateWord != lastGateWord))
                  bStore[bCntr++] = gateWord;

            // fall through
         case 3:
               ampWord = RFAMPKEY | amp2Binary(f3*scalef);
                if ((mode != 1) || (ampWord != lastAmpWord))
                  bStore[bCntr++] = ampWord;
               lastamp = scalef*f3/4095.0;
            //
            // fall through
         case 2:
               phaseWord = (degrees2Binary(f2) | RFPHASEKEY);
               if ((mode != 1) || (phaseWord != lastPhaseWord))
                  bStore[bCntr++] = phaseWord;

               if (bCntr == 0)
                    bStore[bCntr++] = phaseWord;  // gotta emit something...

               for (i=0; i<bCntr; i++)
               {
               	  out = bStore[i];
               	  if (i==(bCntr-1))  out |= LATCHKEY;
                  putPattern(out);
                  wcount++;
               }
               lastGateWord = gateWord;
               lastAmpWord = ampWord;
               lastPhaseWord = phaseWord;
               eventcount++;

               repeats   = ((int) ((f1/tipResolution)+0.5)) - 1;
               pwrf += lastgate*lastamp*lastamp*(repeats+1);

               if ( firstTime && (repeats < 0) )
               {
                 text_message("tip angle resolution (dres) on %s of %g may be too coarse for waveform %s\n",logicalName,tipResolution,tname);
                 firstTime = 0;
               }

	if (mode != 1)   { //non-optimized but ultra-robust.
         while(repeats >= 1)
         {
           for (i=0; i<bCntr; i++)
           {
       	     out = bStore[i];
       	     if (i==(bCntr-1))  out |= LATCHKEY;
             putPattern(out);
             wcount++;
           }
          repeats--;
          eventcount++;
         }
         bCntr = 0;
        }
        else
        {
	 int rclip,value;
         // an increment of zero is just a repeat..
         while (repeats > 0)
         {
	   if (repeats > 255)
	      rclip = 255;
	   else
	      rclip = repeats;
           value = LATCHKEY | RFPHASEINCWORD(0, rclip, 1) ;
           putPattern( value );
           wcount++;
           eventcount += rclip;
	   repeats -= rclip;
         }
         bCntr = 0;
        }
        break;
        default:
        break;
       }

    } while (l > -1);
     // add the waveform to Pattern List

     if (eventcount > 0)
     {
        // set a minimum of 200 words for efficient decoupling speeds
        // remove this line and set kk = 1 if problems
        int kk = 1; /* pWaveformBuf->duplicate(200,wcount); */
        pwrf = sqrt(pwrf/((double) eventcount));
        tmp = new cPatternEntry(uname,flag,kk*eventcount,kk*wcount);
        tmp->setPowerFraction(pwrf);
        addPattern(tmp);
        sendPattern(tmp);
      }
     else
       abort_message("file %s has no events",nm);
    return(tmp);
}
//  automatically creates a phase ramp atop the specified shape
//  full compression is used..
cPatternEntry *RFController::resolveDecOffsetPattern(char *nm, int flag, double pw_90, double offset,
                                    int mode, const char *emsg)
{
    int i,l,out;
    double f1,f2,f3,f4,f5,scalef,tipResolution;
    int wcount, eventcount, repeats;
    int bStore[400],bCntr,rclip,value;
    int phaseWord, ampWord, gateWord;
    int lastGateWord, lastAmpWord, lastPhaseWord;
    double deltaph, dres50d, dphaseAcc,dphaseStep;
    double pwrf,lastamp,lastgate;
    int dres50i;
    char tname[200],bname[200];
    double ampScale = 1023.0;
    //if (pw_90 > 0.002) text_error("driver needs an added memory");
    sprintf(tname,"%s_%6.2f_%d",nm,offset,flag); //generate a unique name
    // do we already know the pattern??
    cPatternEntry *tmp = find(tname, flag);
    if (tmp != NULL)
      return(tmp);
    strcat(tname,".DEC");
    sprintf(bname,"%s.DEC",nm);
    scalef = 4095.0/1023.0;
    if (flag < 1)   // safety..
      flag = 1;
    // better clipping ...
    // offset logic  50/25 ns multiple for pw90/90*dres...
    tipResolution = (double) flag;
    double timeStep = 12.5e-9 * ((int)(P2TheConsole->getMinTimeEventTicks()));
    deltaph = 360.0*offset*timeStep; // full precision in degrees
    dres50d = ((tipResolution * pw_90)/(90.0*timeStep))+0.005;
    dres50i = (int) dres50d;
    dphaseAcc = 0.0;
    dphaseStep = deltaph*dres50i;
    pwrf = 0.0;
    wcount = 0;
    eventcount = 0;
    lastamp  = 1.0; // set for default case
    lastgate = 1.0; // set for default case
    ampWord = gateWord = 0;
    // set to illegal or unusual values.
    lastGateWord = 21839;
    lastAmpWord = -21839;
    lastPhaseWord = -21839;
    bCntr=0;
    i = findPatternFile(bname);
    if (i != 1)
    {
		  abort_message("%s could not find %s", emsg, bname);
    }

    patternCount++;
    pWaveformBuf->startSubSection(PATTERNHEADER+patternCount);

    do
    {
	  l = scanLine(&f1,&f2,&f3,&f4,&f5);

	  if ( (l >= 3) && ((int)f1 == 0) )
	  {
		  ampScale = f3;
		  continue;
	  }

	  switch( l )
	  {
		case 4:   
			  if (((int)f4) & 0x1)
                          {
				 gateWord = GATEKEY | (RFGATEON(X_OUT)  & 0xffffff);
                                 lastgate = 1.0;
                          }
			  else
                          {
				 gateWord = GATEKEY | (RFGATEOFF(X_OUT) & 0xffffff);
                                 lastgate = 0.0;
                          }
			  if (gateWord != lastGateWord)
				 bStore[bCntr++] = gateWord;

		   // fall through
		case 3:
			  ampWord = RFAMPKEY | amp2Binary(f3*scalef);
                          lastamp = f3/(scalef*256.0);
			   if (ampWord != lastAmpWord)
				 bStore[bCntr++] = ampWord;

		   // fall through
		case 2:
		      repeats   = ((int) ((f1/tipResolution)+0.5))*dres50i;
			  phaseWord = (degrees2Binary(f2+dphaseAcc) | RFPHASEKEY);
                          dphaseAcc +=  deltaph * repeats;
		          bStore[bCntr++] = phaseWord;

                          pwrf += lastgate*lastamp*lastamp*(repeats+1);
		      while (repeats > 0)
		      {
		    	  if (repeats > 255) rclip = 255;
		    	  else rclip = repeats;
		    	  value = LATCHKEY | RFPHASEINCWORD(degrees2Binary(deltaph), rclip, 1) ;
			      bStore[bCntr++] = value;
			      repeats -= rclip;
			      eventcount += rclip;
              }
		      //////////////////////////////
			  for (i=0; i<bCntr; i++)
			  {
				 out = bStore[i];
				 putPattern(out);
				 wcount++;
			  }
			  lastGateWord = gateWord;
			  lastAmpWord = ampWord;
              bCntr = 0;
           break;
           default:
           break;
          }

       } while (l > -1);
        // add the waveform to Pattern List
        // cout << "resolve " << eventcount << "   "  << wcount << endl;
        if (2*wcount >= eventcount) text_message("fifo problems");
        if (eventcount > 0)
        {
           pwrf = sqrt(pwrf/((double) eventcount));
           tmp = new cPatternEntry(tname,flag,eventcount,wcount);
           tmp->setPowerFraction(pwrf);
           addPattern(tmp);
           sendPattern(tmp);
         }
        else
          abort_message("pattern %s has no events",nm);
       return(tmp);
   }
// list number lnum
// offsetArray is array of doubles
// nfreqs is the number of frequencies
#ifndef SWIFT
cPatternEntry *RFController::resolveFreqList(int lnum, double *offsetArray, int nfreqs)
{
	char nbuff[256];
	int j,index,nsize,tstore[64];
        int wordcount,eventcount;
	sprintf(nbuff,"__freqList_%d",lnum);
	cPatternEntry *tmp = find(nbuff, 8);
	// already defined??
	if (tmp != NULL)
	  return(tmp);
	// use a pattern for this
        wordcount = 0;
        eventcount = 0;
	patternCount++;
	pWaveformBuf->startSubSection(PATTERNHEADER+patternCount);
	//
	//
	for (index = 0; index < nfreqs; index++)
	{
           nsize = mySynthesizer.encodeFreq(systemIF+baseFreq+0.000001*(*(offsetArray+index)), tstore);
           for (j = 0; j < nsize; j++) tstore[j] |= AUX | LATCHKEY;
           pWaveformBuf->putCodes(tstore,nsize);
           wordcount += nsize;
           eventcount++;
	}
	tmp = new cPatternEntry(nbuff,8,eventcount,wordcount);
        addPattern(tmp);
        sendPattern(tmp);
    return(tmp);
}

// returns the duration of the element in ticks
int RFController::useRTFreq(int lnum,int vvar)
{
   char nbuff[256];
   int num, evntc, wordc;
   int abuff[4];
   sprintf(nbuff,"__freqList_%d",lnum);
   cPatternEntry *tmp = find(nbuff, 8);
   if (tmp == NULL)
     text_error("did not find frequency list");
   num = tmp->getReferenceID();
   evntc = tmp->getNumberStates();
   wordc = tmp->getNumberWords();
   if (evntc < 1) 
     text_error("frequency list construction error");
   wordc = wordc/evntc;
   abuff[0] = vvar;
   abuff[1] = num;
   abuff[2] = wordc; 
   outputACode(RTFREQ,3,abuff); 
   return(wordc*4);
} 
    
#endif

void RFController::powerMonLoopStartAction()
{
  if (progDecInterLock) return;
  powerWatch.saveStartOfLoop(getLoopLevel(),0.0000000125L*getBigTicker());
  //powerWatch.calculateEnergyPerInc(getBigTicker());
  //powerMonLoopStack[getLoopLevel()][0] = powerWatch.getPowerIntegral();
}
//
//
//
void RFController::powerMonLoopEndAction()
{
  int lvl; 
  double cnt;
  if (progDecInterLock) return;
  cnt = (double) getcurrentloopcount();
  lvl = getLoopLevel();
  powerWatch.calculateEnergyPerInc(getBigTicker());
  powerWatch.computeEndOfLoop(lvl,(double) cnt, 0.0000000125L*getBigTicker());
}
//
//
//
void RFController::showPowerIntegral()
{
  printf("%6s:        Energy=%10.4g\n",getName(),powerWatch.getPowerIntegral());
}
//
//
//
void RFController::showEventPowerIntegral(const char *comment)
{
  printf("Power info: %s %5s Energy=   %10.4g\n",comment,getName(),powerWatch.getEventPowerIntegral());
}
//
//
//
void RFController::enablePowerCalculation()
{
  powerWatch.enablePowerCalculation();
}
//
//
//
void RFController::suspendPowerCalculation()
{
  powerWatch.suspendPowerCalculation();
}

//
//
//
void RFController::eventStartAction()
{
  powerWatch.eventStartAction();
}
//
//
//
void RFController::resetPowerEventStart()
{
  powerWatch.initialize(getBigTicker());
}
//
//
//
void RFController::PowerActionsAtStartOfScan()
{
  powerWatch.saveStartOfScan(0.0000000125L*getBigTicker());
  //trs = getBigTicker();
  //powerWatch.calculateEnergyPerInc(getBigTicker());
  //powerAtStartOfScan[0] = powerWatch.getPowerIntegral();
}
//
//
//
void RFController::computePowerAtEndOfScan()
{
  double x;
  powerWatch.calculateEnergyPerInc(getBigTicker());
  x = 0.0000000125*getBigTicker(); 
  powerWatch.computeEndOfScan(getval("nt"),x);
}

//
//
//
cPatternEntry *RFController::resolveHomoDecPattern(char *nm, int flag, double pw_90, int dwTicks, int rfonTicks, long long acqtimeticks, int rof1Ticks, int rof2Ticks, int rof3Ticks)
{
    int i,l;
    double f1,f2,f3,f4,f5,scalef;
    int wcount, eventcount, repeats;
    char tname[200], dbname[200];
    double ampScale;
    int phaseWord, ampWord, gateWord, rawwfgsize,dur_ticks;
    double temp;
    unsigned int *pRawWfg, *pRawWfgStart;
    cPatternEntry *tmp;

    ampScale = 1023.0;
    rawwfgsize = 262144;
    strcpy(tname,nm);
    strcat(tname,".DEC");
    scalef = 4095.0/1023.0;
    if (flag < 1)   // safety..
      flag = 1;
    sprintf(dbname,"%s_hd %g-%d",nm,pw_90,dwTicks); // construct a pattern id
    // see if we can find dbname if so return it...
    // else construct,  and enter into database...
    tmp = find(dbname, flag);
    if (tmp != NULL)
      return(tmp);

    i = findPatternFile(tname);
    if (i != 1)
       abort_message("unable to find waveform file %s for homodecoupling", tname);


    if (P_getreal(CURRENT, "homodecwfgsize", &temp, 1) != 0 )
    {
      rawwfgsize = 262144;
    }
    else
    {
      rawwfgsize = (int) getval("homodecwfgsize");
      if ( (rawwfgsize < 32) || (rawwfgsize > 1e6) )
         abort_message("homodecouple waveform size parameter 'homodecwfgsize' invalid. abort!\n");
    }


    pRawWfg = new unsigned int[rawwfgsize];
    if (! pRawWfg)
        abort_message("unable to allocate array for homodecouple waveform. abort!\n");

    pRawWfgStart = pRawWfg;

    dur_ticks = (int) calcTicks(pw_90*flag/90.0);
    // read in the file
    do
    {
       if ( (pRawWfg - pRawWfgStart) >= rawwfgsize )
          abort_message("homodecouple wfg size too large. create & set 'homodecwfgsize' to a value greater than the default size of 262144. abort!\n");

      l = scanLine(&f1,&f2,&f3,&f4,&f5);
      if ((int) f1 == 0)
      {
         ampScale = f3;
         continue;
      }
      if ((l > -1) && (l < 2))
         abort_message("homodecouple wfg file requires amplitude & gate fields. abort!\n");
      gateWord = GATEKEY | RFGATEON(X_OUT);
      ampWord  = RFAMPKEY | amp2Binary(4095.0) ;


       switch( l )
       {
         case 4:
               if (((int)f4) & 0x1)
                  gateWord = GATEKEY | (RFGATEON(X_OUT)  & 0xffffff);
               else
                  gateWord = GATEKEY | (RFGATEOFF(X_OUT) & 0xffffff);
            // fall through

         case 3:
               ampWord = RFAMPKEY | amp2Binary(f3*scalef) ;
            // fall through

         case 2:
               phaseWord  = degrees2Binary(f2) | RFPHASEKEY ;
               *pRawWfg++ = gateWord;
               *pRawWfg++ = ampWord;
               *pRawWfg++ = phaseWord;

               repeats    = (int) ((f1/flag)+0.5);
               if (repeats <= 0)
                  abort_message("homodecouple tip angle res (hdres) on %s of %d is too large for waveform %s\n",logicalName,flag,tname);

               *pRawWfg++ = LATCHKEY | DURATIONKEY | (dur_ticks * repeats) ;


               break;

         default:
               break;
       }

    } while (l > -1);


    if ( pRawWfg >= (pRawWfgStart + rawwfgsize) )
          abort_message("homodecouple wfg size too large. create & set 'homodecwfgsize' to a value greater than the default size of 131072. abort!\n");


    int num_rawwords = pRawWfg - pRawWfgStart;

 // now make up the gate array

    unsigned int gatearr[10];
    int num_gatewords;

    int rfoffTicks = dwTicks - rfonTicks - rof1Ticks - rof2Ticks - rof3Ticks;

    if ( (rof1Ticks <= 0) || (rof2Ticks <= 0) || (rof3Ticks <= 0) )
       abort_message("homodecouple homorof1, homorof2 or homorof3 set to zero. abort!\n");

    if (rfoffTicks <= 0)
       abort_message("homodecouple rf off duration too short (try decreasing dutyc, homorof1, homorof2, homorof3) abort!\n");

    if (rfonTicks <= 0)
       abort_message("homodecouple rf on duration too short (try increasing dutyc) abort!\n");

    gatearr[0] = GATEKEY  | (RFGATEOFF(get_RF_PRE_GATES()   & 0xffffff) );
    gatearr[1] = LATCHKEY | DURATIONKEY | rfoffTicks ;                          /*  rf off time */
    gatearr[2] = GATEKEY  | (RFGATEON(get_RF_PRE_GATES()    & 0xffffff) );
    gatearr[3] = LATCHKEY | DURATIONKEY | rof1Ticks  ;                          /*  rof1        */
    gatearr[4] = GATEKEY  | (RFGATEON(get_RF_PULSE_GATES()  & 0xffffff) );
    gatearr[5] = LATCHKEY | DURATIONKEY | rfonTicks ;                           /*  rf on  time */
    gatearr[6] = GATEKEY  | (RFGATEOFF(get_RF_PULSE_GATES() & 0xffffff) );
    gatearr[7] = LATCHKEY | DURATIONKEY | rof2Ticks  ;                          /*  rof2        */
    gatearr[8] = GATEKEY  | (RFGATEOFF(get_RF_PRE_GATES()   & 0xffffff) );
    gatearr[9] = LATCHKEY | DURATIONKEY | rof3Ticks  ;                          /*  rof3        */

    num_gatewords = 10;


 // make up the final homo wfg after merging in raw wfg & gate array

    int A_DUR_WORD=0, A_GATE_WORD=0, A_PHASE_WORD=0, A_AMP_WORD=0;
    int B_DUR_WORD=0, B_GATE_WORD=0;
    int runWord  = 0;
    unsigned int *pA, *pB;
    pA = pRawWfgStart;
    pB = &gatearr[0];


    patternCount++;
    pWaveformBuf->startSubSection(PATTERNHEADER+patternCount);

    wcount     = 0;
    eventcount = 0;

    int stopLoad, lastAmpWord, lastGateWord, lastPhaseWord ;
    stopLoad = 0; lastAmpWord=0; lastGateWord=0; lastPhaseWord=0;

    while ( 1 )                     // over the entire acq time
    {

      // read in one state from A stream  ( raw wfg )

      if ( pA >= (pRawWfgStart + num_rawwords))
        pA = pRawWfgStart;

     if ( A_DUR_WORD == 0)
     {
       A_GATE_WORD = (*pA);
       pA++;
       A_AMP_WORD  = (*pA);
       pA++;
       A_PHASE_WORD= (*pA);
       pA++;
       A_DUR_WORD  = (*pA) & 0x3ffffff ;
       pA++;
     }

    // read in one state from B stream  ( gate array )

    if ( pB >= (&gatearr[0] + num_gatewords) )
      pB = &gatearr[0];

    if ( B_DUR_WORD == 0 )
    {
        B_GATE_WORD = (*pB);
        pB++;
        B_DUR_WORD  = (*pB) & 0x3ffffff;
        pB++;
     }

  // check for any zero duration words
  if ( (A_DUR_WORD <= 0) || (B_DUR_WORD <= 0))
  {
    abort_message("psg internal error homodecouple waveform merge duration word is 0. abort!\n");
  }

//   Now do the merging of the two streams

     // logic to determine if we need to stop!

     if ( A_DUR_WORD <=  B_DUR_WORD )
        runWord = A_DUR_WORD;  // choose shortest
     else
        runWord = B_DUR_WORD;
//
     if (acqtimeticks <= runWord)
     {
         runWord = acqtimeticks;
         stopLoad = 1;
     }
     else
         stopLoad = 0;

     if (A_AMP_WORD != lastAmpWord)
     {
        putPattern(A_AMP_WORD);
        wcount++;
        lastAmpWord = A_AMP_WORD;
     }

     if (A_PHASE_WORD != lastPhaseWord)
     {
        putPattern(A_PHASE_WORD);
        wcount++;
        lastPhaseWord = A_PHASE_WORD;
     }

     // gateWord comes from the B stream (gate array)
     if (B_GATE_WORD != lastGateWord)
     {
        putPattern(B_GATE_WORD);
        wcount++;
        lastGateWord = B_GATE_WORD;
     }

     // Now examine the two durations: A_DUR_WORD or B_DUR_WORD, which is shorter?
     // GATEWORD from B stream (gate array) has precedence

     if (runWord <= 1)   // catch a single or zero tick
        runWord = 2;

     if (runWord >= 2)  // insure legal time..
     {
       putPattern(LATCHKEY | DURATIONKEY | runWord);
       wcount++;
       eventcount++;
     }
     else
       printf("psg internal error homodecouple waveform merge duration word invalid. abort!\n");

     acqtimeticks -= runWord;
     B_DUR_WORD   -= runWord;
     A_DUR_WORD   -= runWord;

     if (A_DUR_WORD < 0) A_DUR_WORD = 0;
     if (B_DUR_WORD < 0) B_DUR_WORD = 0;

     if (stopLoad || (acqtimeticks <= 0) ) break;

   }
   // add the waveform to Pattern List

     if (eventcount > 0)
     {
        tmp = new cPatternEntry(dbname,flag,eventcount,wcount);
        addPattern(tmp);
        sendPattern(tmp);
     }
     else
        abort_message("pattern %s has no events",nm);

    pRawWfg      = NULL;
    delete[] pRawWfgStart;
    pRawWfgStart = NULL;

    return(tmp);
}
//
//
//
cPatternEntry *RFController::resolveSWIFTpattern(char *nm, int dwTicks, int rfonTicks, long long acqtimeTicks, int preacqTicks)
{
   int rawwfgsize,  numberStates, nInc, listId, num_inpwords;
   unsigned int *pRawWfg, *pOutWfg, *pFinalWfg;
   long long dur1, divs;
   double Tau,Delta;
   char pname[MAXSTR];
   // temporary variables
   int i;
   double temp;

   if ( strcmp(nm, "") == 0 )
   {
      abort_message("waveform shape name not defined for swift_acquire. abort!\n");
   }

   strcpy(pname, nm);
   strcat(pname,".RF");
   i = findPatternFile(pname);
   if (i != 1)
   {
      abort_message("unable to find waveform file %s for swift_acquire", pname);
   }

   if (P_getreal(CURRENT, "largewfgsize", &temp, 1) != 0 )
   {
     rawwfgsize = 524288;
   }
   else
   {
     rawwfgsize = (int) getval("largewfgsize");
     if ( (rawwfgsize < 32) || (rawwfgsize > 1048576) )
        abort_message("swift_acquire waveform size parameter 'largewfgsize' invalid. abort!\n");
   }

   pRawWfg = new unsigned int[rawwfgsize];

   if (! pRawWfg)
       abort_message("unable to allocate array for swift_acquire waveform. abort!\n");

   pOutWfg = new unsigned int[rawwfgsize];
   if (! pOutWfg)
       abort_message("unable to allocate array for swift_acquire waveform. abort!\n");

   cPatternEntry *origPat = WaveformUtility::readRFWaveform(pname, this, pRawWfg, rawwfgsize, (acqtimeTicks*12.5e-9));

   num_inpwords = origPat->getNumberWords();

   numberStates = origPat->getNumberStates();
   Delta = (acqtimeTicks*12.5e-9)/((double) numberStates);
   Tau   = OFFSETPULSE_TIMESLICE;         // 200 nS
   nInc = (int)  ((acqtimeTicks*12.5e-9)/(((double) numberStates)*Tau)+0.49);
   if (nInc < 1)
      nInc=1;
   if (Delta < Tau)
      text_message("advisory: minimum duration per state of swift_acquire waveform %s reset to 200ns\n",pname);
   dur1 = calcTicks(Tau);
   divs = dur1*nInc*numberStates;

 // now make up the gate array
    int num_gatewords = 10;
    unsigned int GatePattern[num_gatewords];

    int rof1Ticks = 80; int rof2Ticks = 80; int rof3Ticks= 80;   /* 1 usec */
    int rfoffTicks = dwTicks - rfonTicks - rof1Ticks - rof2Ticks - rof3Ticks;

    if ( (rof1Ticks <= 0) || (rof2Ticks <= 0) || (rof3Ticks <= 0) )
       abort_message("swift swiftrof1, swiftrof2 or swiftrof3 set to zero. abort!\n");

    if (rfoffTicks <= 0)
       abort_message("swift rf off duration too short (try decreasing dutyc, swiftrof1, swiftrof2, swiftrof3) abort!\n");

    if (rfonTicks <= 0)
       abort_message("swift rf on duration too short (try increasing dutyc) abort!\n");

    GatePattern[0] = GATEKEY  | (RFGATEOFF(get_RF_PRE_GATES()   & 0xffffff) );
    GatePattern[1] = LATCHKEY | DURATIONKEY | rfoffTicks ;                          /*  rf off time */
    GatePattern[2] = GATEKEY  | (RFGATEON(get_RF_PRE_GATES()    & 0xffffff) );
    GatePattern[3] = LATCHKEY | DURATIONKEY | rof1Ticks  ;                          /*  rof1        */
    GatePattern[4] = GATEKEY  | (RFGATEON(get_RF_PULSE_GATES()  & 0xffffff) );
    GatePattern[5] = LATCHKEY | DURATIONKEY | rfonTicks ;                           /*  rf on  time */
    GatePattern[6] = GATEKEY  | (RFGATEOFF(get_RF_PULSE_GATES() & 0xffffff) );
    GatePattern[7] = LATCHKEY | DURATIONKEY | rof2Ticks  ;                          /*  rof2        */
    GatePattern[8] = GATEKEY  | (RFGATEOFF(get_RF_PRE_GATES()   & 0xffffff) );
    GatePattern[9] = LATCHKEY | DURATIONKEY | rof3Ticks  ;                          /*  rof3        */

    pFinalWfg = new unsigned int[rawwfgsize];
    if (! pFinalWfg)
        abort_message("unable to allocate array for swift_acquire waveform. abort!\n");

    //printf("rfoff=%d rof1T=%d rfon=%d rof2T=%d rof3T=%d\n",rfoffTicks,rof1Ticks,rfonTicks,rof2Ticks,rof3Ticks);

    cPatternEntry *tmp = WaveformUtility::weaveGatePattern(pFinalWfg, pRawWfg, num_inpwords, &GatePattern[0], num_gatewords, acqtimeTicks);
    if (tmp == NULL)
       abort_message("error in rf/receive gate pattern creation in swift_acquire command. abort!\n");

    registerPatternFromArray(tmp, pFinalWfg);

    listId = tmp->getReferenceID();

    delete []pRawWfg; pRawWfg = NULL;

    //delete [] pOutWfg;
    delete []pFinalWfg; pFinalWfg = NULL;

   return(tmp);
}
//
//
//
int RFController::get_RF_PRE_GATES()
{
  return(RF_pre_gates);
}

int RFController::get_RF_PULSE_GATES()
{
  return(RF_pulse_gates);
}

int RFController::get_RF_POST_GATES()
{
  return(RF_post_gates);
}

void RFController::adviseFreq(int count, int* values)
{
   outputACode(ADVISE_FREQ,count,values);
   noteDelay(count * MINTIMERWORD);
}

// in MHz units!
int RFController::setFrequency(double frequency)
{
  int i,j;

  int auxA[30]; // plenty of room

  i = mySynthesizer.encodeFreq(frequency+systemIF,auxA);
  if (i < 1)
    return(-1);  // flag error ...
  if (mySynthesizer.adviseFreq())
    adviseFreq(i,auxA);
  else 
    for (j=0; j < i; j++)
      setAux(auxA[j]);
  return i*4;   // 50 ns each...
}


// iCAT attenuator codes 
static const int AttenuatorCodesICAT[201]=
    {
        0xFF, 0xFE, 0xBF, 0xBE, 0xDF, 0xDE, 0x9F, 0x9E, 0xEF, 0xEE,
        0xAF, 0xAE, 0xCF, 0xCE, 0x8F, 0x8E, 0xF7, 0xF6, 0xB7, 0xB6,
        0xD7, 0xD6, 0x97, 0x96, 0xE7, 0xE6, 0xA7, 0xA6, 0xC7, 0xC6,
        0x87, 0x86, 0xFB, 0xFA, 0xBB, 0xBA, 0xDB, 0xDA, 0x9B, 0x9A,
        0xEB, 0xEA, 0xAB, 0xAA, 0xCB, 0xCA, 0x8B, 0x8A, 0xF3, 0xF2,
        0xB3, 0xB2, 0xD3, 0xD2, 0x93, 0x92, 0xE3, 0xE2, 0xA3, 0xA2,
        0xC3, 0xC2, 0x83, 0x82, 0xFD, 0xFC, 0xBD, 0xBC, 0xDD, 0xDC,
        0x9D, 0x9C, 0xED, 0xEC, 0xAD, 0xAC, 0xCD, 0xCC, 0x8D, 0x8C,
        0xF5, 0xF4, 0xB5, 0xB4, 0xD5, 0xD4, 0x95, 0x94, 0xE5, 0xE4,
        0xA5, 0xA4, 0xC5, 0xC4, 0x85, 0x84, 0xF9, 0xF8, 0xB9, 0xB8,
        0xD9, 0xD8, 0x99, 0x98, 0xE9, 0xE8, 0xA9, 0xA8, 0xC9, 0xC8,
        0x89, 0x88, 0xF1, 0xF0, 0xB1, 0xB0, 0xD1, 0xD0, 0x91, 0x90,
        0xE1, 0xE0, 0xA1, 0xA0, 0xC1, 0xC0, 0x81, 0x80, 0x79, 0x78,
        0x39, 0x38, 0x59, 0x58, 0x19, 0x18, 0x69, 0x68, 0x29, 0x28,
        0x49, 0x48, 0x9, 0x8, 0x71, 0x70, 0x31, 0x30, 0x51, 0x50,
        0x11, 0x10, 0x61, 0x60, 0x21, 0x20, 0x41, 0x40, 0x1, 0x43,
        0x42, 0x35, 0x34, 0x33, 0x32, 0x3D, 0x3C, 0x2D, 0x2C, 0x2B,
        0x2A, 0x4D, 0x4C, 0x25, 0x24, 0x23, 0x22, 0x75, 0x74, 0x1D,
        0x1C, 0x1B, 0x1A, 0x55, 0x54, 0x15, 0x14, 0x13, 0x12, 0x65,
        0x64, 0xD, 0xC, 0xB, 0xA, 0x45, 0x44, 0x5, 0x4, 0x3, 0x2
    };



int RFController::setAttenuator(double atten)
{
  int result = 0;

  if (!rfIsEnabled) atten = mintpwr;
  if (atten > maxAllowedUserPower)
    abort_message("user safety power level exceeded on %s at value %2.0fdb limit %2.0fdb (maxattench%d)\n",coarsePowerName,atten,maxAllowedUserPower,logicalID);
  powerWatch.eventCoarsePowerChange(atten-47.0,getBigTicker());
  if (isICAT())
    result = setAttenuatorICAT(atten);
  else
    result = setAttenuatorVnmrs(atten);
  return result;
}


int RFController::getAttenuator(double atten) {
	int result = 0;
	if (isICAT())
		result = computeAttenuatorICAT(atten);
	else
		result = computeAttenuatorVnmrs(atten);
	return result;
}

int RFController::computeAttenuatorICAT(double atten)
{
  int value;
  int lkup_index=158;   // lkup_index 158 corresponds to tpwr=-16 (0x1)
  int outword = 0x1;

  value = (int)(atten*10.0);
  lkup_index = (630-value)/5;
  if ( (lkup_index >= 0) && (lkup_index <= 201) )
  {
    outword = AttenuatorCodesICAT[lkup_index] & 0xff;

    // check with Vnmrs calculation if tpwr in range +63 to -16 dB
    if ((atten >= -16) && (atten <= 63) && (fabs(atten - (int)atten)<=0.0) )
    {
      int outword2 = computeAttenuatorVnmrs(atten);
      if (outword != outword2)
        abort_message("error in attenuator value calculation for power! values= %5.1f  S=0x%X  V=0x%X  abort!",atten,outword,outword2);
    }
    return outword;
  }
  else
    abort_message("invalid attenuator power value specified\n");
}

int RFController::setAttenuatorICAT(double atten)
{
  int outword =computeAttenuatorICAT(atten);
  setAux(outword | AUXATTEN);
  return (4);
}


int RFController::computeAttenuatorVnmrs(double atten)
{
  int val,outword;

  val = 63 - (int) atten; /* power -> attenuator */
  if (val > 79) val = 79;
  if (val <  0) val = 0;
  outword = 0;
  if (val > 63)
  {
    outword = 0x80;  /* 16 db atten */
    val -= 16;
  }

  if ( val & 0x20)  outword |= 0x2;
  if ( val & 0x10)  outword |= 0x4;
  if ( val & 0x08)  outword |= 0x8;
  if ( val & 0x04)  outword |= 0x10;
  if ( val & 0x02)  outword |= 0x20;
  if ( val & 0x01)  outword |= 0x40;
  outword = (~outword) & 0xff;
  return(outword);
}


int RFController::setAttenuatorVnmrs(double atten)
{
  int val,outword;
  val = 63 - (int) atten; /* power -> attenuator */
  if (val > 79) val = 79;
  if (val <  0) val = 0;
  outword = 0;
  if (val > 63)
  {
    outword = 0x80;  /* 16 db atten */
    val -= 16;
  }

  if ( val & 0x20)  outword |= 0x2;
  if ( val & 0x10)  outword |= 0x4;
  if ( val & 0x08)  outword |= 0x8;
  if ( val & 0x04)  outword |= 0x10;
  if ( val & 0x02)  outword |= 0x20;
  if ( val & 0x01)  outword |= 0x40;
  outword = (~outword) & 0xff;
  setAux(outword | AUXATTEN);
  return(4);
}

int RFController::setSwitchOnAttenuator(int state)
{
  setAux(state | 0x700);
  return(4);
}

void RFController::setRFInfo(double freq)
{
int streamCodes[10],i;
double power;
   i=0;
   streamCodes[i++] = ( freq > whatamphibandmin(0, 0.0));
   streamCodes[i++] = ( freq > xmtrHiLowFreq);
   if (P_getreal(CURRENT, tunePwrName, &power, 1) != 0 )
      streamCodes[i++] = -1;
   else
      streamCodes[i++] = (int)power;
   outputACode(TINFO,3,streamCodes);
}
/*
**  this initializes the variable names in the object
**  must preceed initailizeIncrement ...
*/

//-- ICAT temperature compensation ---
void RFController::setTempComp(int t){
	int enable_tcomp=t;

	outputACode(TEMPCOMP,1,&enable_tcomp);
}

int RFController::initializeExpStates(int setupflag)
{
  int ticks,rftcomp;
  int switchVal;
  char ampModeStr[256], ch1, nucStr[256];
  int buffer[4];
  char estring[MAXSTR+9];
  long long auxticks;


  if (pAcodeBuf == NULL)
     abort_message("psg internal unable to allocate acode buffer for RF controller %s. abort!\n",Name);

  if (getparmd(freqName,"real",CURRENT,&baseFreq,1) == 1)
               baseFreq = 0.0;
  if (getparmd(freqOffsetName,"real",CURRENT,&baseOffset,1)   == 1)
               baseOffset  = 0.0;
  if (getparmd(coarsePowerName,"real",CURRENT,&baseTpwr,1) == 1)
               baseTpwr = 0.0;
  if (P_getreal(CURRENT, finePowerName, &baseTpwrF, 1) != 0 )
  {
    if (P_getreal(CURRENT, finePowerName2, &baseTpwrF, 1) != 0 )
    {
      baseTpwrF = 4095.0;
    }
    else
    {
      baseTpwrF = getval(finePowerName2);
    }
  }
  else
  {
    baseTpwrF = getval(finePowerName);
  }

/*
 *  Temperature COmpensation for iCAT RF
 *  n - off ( 0 )
 *  y - on ( 1 )
 *  c - continuous ( 2 )
 */
  if ( P_getstring(GLOBAL, "rftempcomp", estring, 1, MAXSTR) < 0)
     rftcomp = 0;
  else
  {
     if ( estring[0]=='n' )
        rftcomp = 0;
     else if ( estring[0]=='y' )
        rftcomp = 1;
     else if ( estring[0]=='c' )
        rftcomp = 2;
     else
        rftcomp = 0;
  }
  auxticks = 4LL;
  outputACode(TEMPCOMP,1,&rftcomp);
  noteDelay(auxticks);


  switchVal = 0;
  baseOffset /= 1000000.0L;
  baseFreq -= baseOffset;     //
  defAmpScale = 4095.0;
  setAmpScale(baseTpwrF);     // fine power
  defAmp = 4095.0;
  setAmp(defAmp);             // wfg amp
  defVAmpStep     = 16.0;
  currentVAmpStep = defVAmpStep;
  defPhase = defPhaseCycle = currentPhaseStep = 0.0;
  setPhaseCycle(defPhaseCycle);
  setPhase(defPhase);
  powerWatch.initialize(getBigTicker());
  auxHomoDecTicker = 0LL;

  if ( P_getstring(CURRENT,"ampmode",ampModeStr,1,16) < 0)
  {
     strcpy(ampModeStr,"dddddddddddddddddddddddd");
  }
  strcpy(nucStr,"none");
  if (strcmp(logicalName,"obs")==0)  P_getstring(CURRENT,"tn",nucStr,1,256);
  if (strcmp(logicalName,"dec")==0)  P_getstring(CURRENT,"dn",nucStr,1,256);
  if (strcmp(logicalName,"dec2")==0)  P_getstring(CURRENT,"dn2",nucStr,1,256);
  if (strcmp(logicalName,"dec3")==0)  P_getstring(CURRENT,"dn3",nucStr,1,256);
  if (strcmp(logicalName,"dec4")==0)  P_getstring(CURRENT,"dn4",nucStr,1,256);

  initAmpLinearization(nucStr);

  //------------------------------------

// ampmode is RF_CW_MODE
  ch1 = ampModeStr[logicalID-1];
  if (bgflag) { cout << "amp mode " << ampModeStr << " pick " << ch1 << "for " << logicalID <<endl; }
  /////////////////////
     gateDefault = RFGATEOFF(RFAMP_UNBLANK)  | RFGATEON(RF_TR_GATE) | RFGATEOFF(RF_OBS_PULSE_LIQUIDS);
  if  (baseFreq > xmtrHiLowFreq )
    gateDefault |= RFGATEON(RF_MIXER);
  else
    gateDefault |= RFGATEOFF(RF_MIXER);
  if ((strlen(nucStr) < 1) || (nucStr[0] == ' '))
  {
  	rfIsEnabled = FALSE;
  }
  if (ch1=='i') ch1 = 'd';
  // more defaults... if not override  and H2 or "" then go pulse mode
  // cout << logicalName << "  " << strlen(nucStr) << " is length of " << nucStr << endl;
  if ((ch1 != 'c') && ((strlen(nucStr) == 0) || (strncmp(nucStr,"H2",2)==0)))
      ch1 = 'p';

  if (bgflag) { cout << "amp mode for " << logicalID << " is " << ch1 <<endl; }
  if (!isObserve())
  {
    if ((ch1 == 'd') || (ch1 == 'c'))
       gateDefault |= RFGATEON(RF_CW_MODE) | RFGATEON(R_LO_NOT) | RFGATEON(RFAMP_UNBLANK);
    else  // PULSE MODE
       gateDefault |= RFGATEOFF(RF_CW_MODE) | RFGATEON(R_LO_NOT);

     RF_pre_gates   = RF_DEC_PRE_LIQUIDS;
     RF_pulse_gates = RF_DEC_PULSE_LIQUIDS;
     RF_post_gates  = RF_DEC_POST_LIQUIDS;
  }
  else
  {
     if (ch1 != 'c')  // pulse and default
     {
       gateDefault &= ~RF_CW_MODE;
       gateDefault |= RFGATEOFF(RF_CW_MODE);
     }
     else
       gateDefault |= RFGATEON(RF_CW_MODE);

     RF_pre_gates   = RF_OBS_PRE_LIQUIDS;
     RF_pulse_gates = RF_OBS_PULSE_LIQUIDS;
     RF_post_gates  = RF_OBS_POST_LIQUIDS;
  }
  //
  if (P2TheConsole->RFShared == 2)
  {
      if (logicalID == 2)
      {
        if (ix == 1) text_message("channel 2 shares high band amplifier");
        switchVal = 1;
       }
      if (logicalID == 1)
      {
        unblankAmplifier(); // sets the unblankCalled flag.
        rfSharingEnabled = 1; // also lets the decoupling be switched
      }
  }

  // On ProPulse INNOVA, switch channel 3 output to MAIN/AUX depending on HB/LB
  if (P2TheConsole->getConsoleSubType() == 's')
  {
     if ((P2TheConsole->getConsoleRFInterface()==3)||(P2TheConsole->getConsoleRFInterface()==4))
     {
        if (logicalID == 3) 
        {
           if (getBaseFrequency() <= 405.0)
           {
              switchVal = 1;
              buffer[0] = 0;
              //printf("Channel 3 output switched to AUX\n");
           }
           else
           {
              switchVal = 0;
              buffer[0] = 1;
              //printf("Channel 3 output switched to MAIN\n");
           }

           // select rf3 amplifier band on ProPulse INNOVA systems
           outputACode(SELECTRFAMP,1,buffer);
           /*
           if (buffer[0])
             printf("HB amplifier selected on RF 3 channel\n");
           else
             printf("LB amplifier selected on RF 3 channel\n");
           */
        }
     }
  }
  
  unblankCalled = 0;
  // blank_is_on = 1; ???
  buffer[0] = gateDefault & RF_CW_MODE;  // preserve the pulse/CW state
  if (P_getstring(GLOBAL,"blankmode",ampModeStr,1,16) >= 0)
  {
     if (ampModeStr[0] == 'u')
     {
       gateDefault |= RFGATEON(RFAMP_UNBLANK);
       // don't bounce RF_CW_MODE and set RFAMP_UNBLANK on 
       // everybody else OFF
       buffer[0] = gateDefault & (RFAMP_UNBLANK | RF_CW_MODE); 
       blank_is_on   = 0;
       unblankCalled = 1;
     }
  }
  setGates(gateDefault);
  setDelay(LATCHDELAY);   // send ahead of aux traffic
  // this is not in the fifo stream!!
  outputACode(SAFESTATE,1,buffer); // solid mode fully set from host...
  prevSyncType       = 1;
  prevAsyncScheme    = 0;
  ticks  = setFrequency(baseFreq+baseOffset);
  setCurrentOffset(baseOffset*1000000.0L);
  ticks += setAttenuator( baseTpwr);
  ticks += setSwitchOnAttenuator(switchVal);
  setUserGate(7, 0);            /* zero all arg1 > 3 */
  unblankCalled = 0;
  setRFInfo(baseFreq);		/* nmr console should save some values */

  setDelay(LATCHDELAY);             /* 4 usec to latch in */
  ticks += 2*LATCHDELAYTICKS ;        /* 4usec = 320 ticks  */
  return(ticks);
}

#define AMPCORRECT  // enable when fpga is ready
#define NUMAMPTBLS 64
#define AMPTBLSIZE 64
#define MAXDAC 65535

#define MAXTPWRTBLS 256
//#define P2INDX(t) (32+(t)/tpwrstep)
#define P2INDX(t) ((t-mintpwr)/tpwrstep)

#define HDRSIZE 3
#define ACODESIZE (HDRSIZE + NUMAMPTBLS * AMPTBLSIZE + 3*256)
static int acodedata[ACODESIZE];
static uint auxtbls[256][3];
static float tpwrmap[MAXTPWRTBLS][2]; // tpwr errors map
static double info[MAXTPWRTBLS][4]; // tpwr_in, tpwr_out, scale, tblmap

static int use_tbls = 1;
static int use_tpwr = 1;

void RFController::outputMapTables(int &index){
    int i,id,err_id,maptbl;
    float tpwr_in,tpwr_out,tpwr_err,tpwr_adj,tpwr_new,scale,scale_adj;
    int numpwrtables=(int)((maxtpwr-mintpwr)/tpwrstep)+1;
    if (use_tbls||use_tpwr){
        if (debug_ampfit > 1){
            printf("%-6s %-6s %-6s %-6s %-6s %-3s\n","in","out","err","new","scale","map");
            printf("---------------------------------------------\n");
        }
         for(i=0;i<numpwrtables;i++){
            tpwr_in=info[i][0];
            tpwr_out=info[i][1];
            scale=info[i][2];
            maptbl=(int)info[i][3];
            tpwr_err=tpwrmap[0][1];
            if(tpwr_out<mintpwr){
                scale=exp(0.115129*(tpwr_out-mintpwr-tpwr_err));
            }
            
            scale_adj=scale;
            tpwr_adj=tpwr_out;

            if(use_tpwr){
                tpwr_new=tpwr_adj;
                if(tpwr_adj>=mintpwr && tpwr_adj<=maxtpwr){
                    err_id= (int) P2INDX(tpwr_adj);
                    tpwr_err=tpwrmap[err_id][1];
                    tpwr_new=tpwr_adj+tpwr_err;
                    while(tpwr_new<tpwr_out && tpwr_new<maxtpwr){
                        err_id++;
                        tpwr_err=tpwrmap[err_id][1];
                        tpwr_adj+=tpwrstep;
                        tpwr_new=tpwr_adj+tpwr_err;
                    }
                    scale_adj=scale*exp(0.115129*(tpwr_out-tpwr_new));
                }
            }
            tpwr_adj=tpwr_adj<mintpwr?mintpwr:tpwr_adj;
            scale=scale>1.0?1.0:scale;
            if (debug_ampfit > 1)
                printf("%-6.1f %-6.1f %-6.3f %-6.1f %1.5f %-3d\n",tpwr_in,tpwr_out,tpwr_err,tpwr_adj,scale_adj,maptbl);
            id=getAttenuator(tpwr_in);  // assign a unique address for table look-ups
            auxtbls[id][0]=maptbl;  // modulator table lut map
            auxtbls[id][1]=(uint)(scale_adj*0x10000);  // scale multiplier
            auxtbls[id][2]=(uint)(getAttenuator(tpwr_adj));  // tpwr in - out map
        }
    }
    if (debug_ampfit > 2){
        printf("tpwr attn  scale\n");
        printf("-------------------\n");
        for(i=0;i<256;i++){
            printf("%4d %4d 0x%-8X\n",auxtbls[i][2],auxtbls[i][0],auxtbls[i][1]);
        }
    }
    for (i = 0; i < 256; i++) {
        acodedata[index++] = auxtbls[i][0];
    }
    // add tpwr out map table to acode data
    for (i = 0; i < 256; i++) {
        acodedata[index++] = auxtbls[i][2];
    }
    // add scale table to acode data
    for (i = 0; i < 256; i++) {
        acodedata[index++] = auxtbls[i][1];
    }
}
void RFController::initAmpLinearization(char *nucStr)
{
#ifdef AMPCORRECT
    uint ntbls = 0;
    uint nmaps = 0;
    uint tsize = 0;
    float tmin = 31;
    float tmax = 63;
    float tstep = 0.5;
    float scale1=0;
    char ampdir[256]={0};
    char path[256]={0};
    char dirpath[256]={0};
    char strbuff[256]={0};
    float a,p;
    float tpwr_in, lutbl,tpwr_out, scale,delta_tpwr=0;
    int i,j,k;
    int index = 0;
    int vstat=-1;
    int numpwrtables=(int)((maxtpwr-mintpwr)/tpwrstep)+1;
	double tmp;
    use_tbls = 1;
    use_tpwr = 1;

    tpwr_in=mintpwr;
    vstat=var_active("amptables", GLOBAL);
	if (vstat==0) 
	    use_tbls = 0;
	else if(vstat>0){
	    P_getreal(GLOBAL, "amptables", &tmp, 1);
	    use_tbls = (int) tmp;
	}
	vstat=var_active("pwrtables", GLOBAL);
 	if (vstat==0) 
	    use_tpwr = 0;
	else if(vstat>0){
	    P_getreal(GLOBAL, "pwrtables", &tmp, 1);
        use_tpwr = (int) tmp;
    }
	if (var_active("ampdebug", GLOBAL) == 1) {
		if (P_getreal(GLOBAL, "ampdebug", &tmp, 1) == 0)
			debug_ampfit = (int) tmp;
	}
	if (var_active("simicat", GLOBAL) == 1) {
		if (P_getreal(GLOBAL, "simicat", &tmp, 1) == 0)
			debug_icat = (int) tmp;
	}
	if(strlen(nucStr)==0)
		use_tbls=0;

	if(debug_ampfit>0)
        printf("Name:%s Nuc:%s useTables:%d\n",Name,nucStr,use_tbls);

    // ------------------------------------------------------
    // initialize fpga auxiliary tables
    // ------------------------------------------------------
    for (j = 0; j < 256; j++) {
        auxtbls[j][0]=0;         // LUT map - assign all  tpwr values to use table 0
        auxtbls[j][1]= 0x10000;  // scale - set to 1.0
        auxtbls[j][2]=j;         // tpw rmap - set out = in
    }

    index = 0;

    strcpy(ampdir,nucStr);
    if (P_getstring(GLOBAL, "hipwrampenable", strbuff, 1, 32) >= 0) {
       int i=Name[2]-'0'; // rf1=1 rf2=2 ..
       if(i>0 && strbuff[i-1]=='y')
           strcat(ampdir,"-HP");
    }

    // search user and system directories for tpwr map file
    if(use_tpwr>0){
        sprintf(dirpath, "%s/amptables/%s/%s", userdir,Name,ampdir);
        sprintf(path, "%s/tpwr.map",dirpath);
        FILE *fp = fopen(path, "r");
        if (fp == NULL){
            sprintf(dirpath, "%s/amptables/%s/%s", systemdir,Name,ampdir);
            sprintf(path, "%s/tpwr.map",dirpath);
            fp = fopen(path, "r");
        }
        if (fp == NULL){
            sprintf(dirpath, "%s/amptables/%s", userdir,Name);
            sprintf(path, "%s/tpwr.map",dirpath);
            fp = fopen(path, "r");
        }
        if (fp == NULL){
            sprintf(dirpath, "%s/amptables/%s", systemdir,Name);
            sprintf(path, "%s/tpwr.map",dirpath);
            fp = fopen(path, "r");
        }
        if (fp == NULL){
            use_tpwr=0;
            if (debug_ampfit > 0)
                printf("tpwr calibration file not found :%s\n", path);
        }
        else {
            if(debug_ampfit > 0)
                printf("tpwr calibration file found :%s\n", path);
            int j=1;
            int i=0;
            float a,b;
            j = fscanf(fp,"%g %g\n",&a,&b);
            if(j<2)
                printf("tpwr file format error : %d\n", j);
            else{
                tpwr_in=mintpwr;
                for(i=0;i<numpwrtables;i++){
                    tpwrmap[i][0]=a;
                    tpwrmap[i][1]=0.0;
                    tpwr_in+=tpwrstep;
                }
                while (j>0){
                    int indx= (int) P2INDX(a);
                    if(indx>=0 && indx<numpwrtables){
                        tpwrmap[indx][0]=a;
                        tpwrmap[indx][1]=b;
                    }
                    j = fscanf(fp,"%g %g\n",&a,&b);
                }
                if(debug_ampfit > 1){
                    printf("%-3s %6s\n","tpwr","err");
                    printf("-------------------\n");
                    for(i=0;i<numpwrtables;i++){
                        printf("%5.1f %-1.4f\n",tpwrmap[i][0],tpwrmap[i][1]);
                    }
                }
            }
            fclose(fp);
        }
    }

    // search user and system directories for table info file
    sprintf(dirpath, "%s/amptables/%s/%s", userdir,Name,ampdir);
    sprintf(path, "%s/tables.map",dirpath);
    FILE *fp = fopen(path, "r");
    if (fp == NULL){
        sprintf(dirpath, "%s/amptables/%s/%s", systemdir,Name,ampdir);
        sprintf(path, "%s/tables.map",dirpath);
        fp = fopen(path, "r");
    }
    if (fp == NULL){
        use_tbls=0;
        if (debug_ampfit > 0)
            printf("amplifier calibration file not found :%s\n", path);
    }
    else if (debug_ampfit > 0)
        printf("amplifier calibration file found :%s\n", path);
    // initialize info structure
    if (use_tbls||use_tpwr){
        tpwr_in=mintpwr;
        for(i=0;i<numpwrtables;i++){
            tpwr_out=tpwr_in+delta_tpwr;
            uint indx= (uint) P2INDX(tpwr_in);
            info[indx][0]=tpwr_in;
            info[indx][1]=tpwr_in;
            info[indx][2]=1.0;
            info[indx][3]=0;
            tpwr_in+=tpwrstep;
        }
    }
    if (use_tbls) { // extract data from tables.map
        acodedata[index++] = 1;
        acodedata[index++] = AMPTBLSIZE; // always 64
        acodedata[index++] = NUMAMPTBLS; // always 64
        char tmp[256];
        char tblfile[256]="tables.bin"; // if file ends in .txt assume ascii
        fscanf(fp,"%s %s\n",tmp,tblfile);
        fscanf(fp,"%s %g\n",tmp,&tmin);
        fscanf(fp,"%s %g\n",tmp,&tmax);
        fscanf(fp,"%s %g\n",tmp,&tstep);
        fscanf(fp,"%s %d\n",tmp,&tsize);
        fscanf(fp,"%s %d\n",tmp,&ntbls);
        fscanf(fp,"%s %d\n",tmp,&nmaps);
        if (debug_ampfit > 0)
            printf("tsize:%d tmin:%g max_tpwr:%g ntbls:%d nmaps:%d\n", tsize,tmin,tmax,ntbls,nmaps);
        uint i=0,j=1,first=1;
        while (i<nmaps && j>0){
            j = fscanf(fp,"%g %g %g %g\n",&tpwr_in, &lutbl,&tpwr_out,&scale);
            if(first){
                delta_tpwr=tpwr_out-tpwr_in;
                scale1=scale;
            }
            if (j> 0){
                (void) getAttenuator(tpwr_in);
                if(lutbl>=0 && lutbl<=63){
                    uint indx= (uint) P2INDX(tpwr_in);
                    info[indx][0]=tpwr_in;
                    info[indx][1]=tpwr_out;
                    info[indx][2]=scale;
                    info[indx][3]=lutbl;
               }
               else
                    warn_message("amptables lut out of allowed range %d\n",(uint)lutbl);
            }
            i++;
            first=0;
        }
        // adjust tpwr_out for values <tmin if tmin tpwr was scaled
        if(delta_tpwr!=0){
            tpwr_in=mintpwr;//-delta_tpwr;
            while(tpwr_in<tmin){
                tpwr_out=tpwr_in+delta_tpwr;
                scale=scale1;
                uint indx= (uint) P2INDX(tpwr_in);
                info[indx][0]=tpwr_in;
                info[indx][1]=tpwr_out;
                info[indx][2]=scale;
                info[indx][3]=0;
                tpwr_in+=tstep;
            }
        }

        bool bintables=false;
        if(strcmp(tblfile,"inline")!=0){
            fclose(fp);
            sprintf(path, "%s/%s", dirpath,tblfile);
            fp = fopen(path, "rb");
            if(fp==NULL){
                use_tbls=0;
                //if (debug_ampfit > 0)
                    printf("could not open amptable file %s\n",path);
                outputACode(AMPTBLS, 1, &use_tbls);
                return;
            }
            char *ext=tblfile+strlen(tblfile)-3;
			if(strcmp("bin",ext)==0)
		        bintables=true;
            if (debug_ampfit > 0)
                printf("\namptable file:%s ext:%s\n", tblfile,ext);
        }
        size_t words_read=0;

        uint datasize = ntbls * tsize;
        uint *tbldata = new uint[datasize];

        outputMapTables(index);

        if(bintables)
			words_read += fread(tbldata, 4, datasize, fp);
		else{
            j=1;
            k=0;
            i=0;
            while (j>0 && i<datasize){
                j = fscanf(fp,"%g %g\n",&a,&p);
                if((i%AMPTBLSIZE)==0)
                    k++;
                if((k==1 && debug_ampfit > 2) || debug_ampfit > 3)
                    printf("%-3d %1.5f %2.4f\n",k-1,a,p);
                uint ai=(uint)(a*MAXDAC);
                uint pi=(uint)((p+180)*MAXDAC/360.0);
                tbldata[i]=htonl((ai<<16)+(pi&0xffff));
                i++;
            }
        }

        fclose(fp);

        int tbls_skipped = (ntbls - NUMAMPTBLS);
        int tbls_missing = -tbls_skipped;

        tbls_missing = tbls_missing < 0 ? 0 : tbls_missing;
        tbls_skipped = tbls_skipped < 0 ? 0 : tbls_skipped;
        ntbls -= tbls_skipped;
        tmin = tmax - ntbls + 1;

        //  Construct an acode containing table info, maps and data ..
        //  HEADER (3 values)
        //  TABLEMAP (256 values)
        //  TPWRMAP  (256 values)
        //  SCALEMAP (256 values)
        //  TBLDATA(64x64 values)
        //  ------------------------------------------------------
        //  format for acode-download exactly NUMAMPTBLS(64) correction tables
        //  ------------------------------------------------------
        // - read in tables data file (binary or ascii)
        // - use first table as default (used for tpwr values <=tmin)
        // - if a tpwr correction was applied to tmin table apply it to non-mapped tables <tmin
        // - if the number of tables < 64 duplicate first table for missing tables
        // - if the number of tables >= 64 remove excess tables between first and last
        //  ------------------------------------------------------
        if (debug_ampfit >1 )
            printf("tables missing:%d skipped:%d\n", tbls_missing,tbls_skipped);
        // duplicate first table if needed to replace missing tables
        if (tbls_missing == 0) {
            tbls_missing = 1;
            tbls_skipped += 1;
        }
        // add first and missing tables
        for (int k = 0; k < tbls_missing; k++) {
            for (j = 0; j < tsize; j++) {
                if (debug_ampfit >3 )
                    printf("0x%.8X\n", htonl(tbldata[j]));
                acodedata[index++] = htonl(tbldata[j]);
            }
        }
        // add remaining tables
        int start = tbls_skipped * tsize;
        for (uint i = start; i < datasize; i++) {
            if (debug_ampfit >3 )
                printf("0x%.8X\n", htonl(tbldata[i]));
            acodedata[index++] = htonl(tbldata[i]);
        }
        if (debug_ampfit >1 )
            printf("sent %d:%d acode values\n", index,ACODESIZE);
        outputACode(AMPTBLS, index, acodedata);
        delete tbldata;
    }
    else { // send identity tables
        acodedata[index++] = 1; // NB: always enabled
        acodedata[index++] = AMPTBLSIZE; // always 64
        acodedata[index++] = 1; // use single table
        // add atten map table to acode data
        outputMapTables(index);
        // construct an identity table
        //for(i=0;i<NUMAMPTBLS;i++)
        double scale=(double)(MAXDAC-1)/MAXDAC/AMPTBLSIZE;
        //double scale=(double)MAXDAC/(AMPTBLSIZE);
        for (j = 0; j < AMPTBLSIZE; j++) {
           double a=(j+1)*scale;
           //printf("%g\n",a);
            uint ai=(uint)(a*MAXDAC);
            acodedata[index++] =(ai<<16);
        }
        outputACode(AMPTBLS, index, acodedata);
	}
#endif

}

int RFController::writeDefaultGates()
{
  setGates(gateDefault);
  return 0;
}


int  RFController::initializeIncrementStates(int total)
{
  long long ticks;

  double tmpval;
  defAmpScale = 4095.0;
  defAmp      = 4095.0;
  baseOffset = getval(freqOffsetName);
  baseOffset /= 1000000.0L;
  baseTpwr = getval(coarsePowerName);

  if (P_getreal(CURRENT, finePowerName, &tmpval, 1) != 0 )
  {
    if (P_getreal(CURRENT, finePowerName2, &tmpval, 1) != 0 )
    {
      baseTpwrF = defAmpScale;
    }
    else
    {
      baseTpwrF = getval(finePowerName2);
    }
  }
  else
  {
    baseTpwrF = getval(finePowerName);
  }

  // event time begins..
  clearSmallTicker();
  setTickDelay((long long)13);
  setAmpScale(baseTpwrF);    // fine power
  setAmp(defAmp);            // wfg amp

  defPhase = defPhaseCycle = 0.0;
  setPhaseCycle(defPhaseCycle);
  setPhase(defPhase);
  setGates(gateDefault);
  setTickDelay((long long) 4);
  setFrequency(baseFreq+baseOffset);
  setCurrentOffset(baseOffset*1000000.0L);
  setAttenuator( baseTpwr);
  setTickDelay((long long) 4);            /* 50 nsec to latch in */
  ticks = getSmallTicker();
  if (ticks > total)
     abort_message("timing synchronization tick count mismatch during initialize increment on %s. abort!\n",Name);

  setTickDelay((long long)(total - ticks));
  clearSmallTicker(); // this should be unnecessary
  auxHomoDecTicker = 0LL;

  unblankCalled = 0;

  // reset status dm, dmm indices between fids
  prevStatusDmIndex  = -1;
  prevStatusDmmIndex = -1;
  prevSyncType       = 1;
  prevAsyncScheme    = 0;

  return(0);
}


int RFController::doStatusPeriodActions(int currStatusIndex)
{
  char dm[256], dmm[256], dmmWfg[256];
  double dmf, dres;
  int statusRFOn = 0;
  int syncType=2, asyncScheme=1;
  char cAsyncScheme[256];

  if (P_getstring(CURRENT, "decasynctype", cAsyncScheme, 1, 255) != 0 )
  {
     asyncScheme = 1;       /* progressive */
  }
  else
  {
     getstr("decasynctype", cAsyncScheme);
     if (cAsyncScheme[0] == 'r')
        asyncScheme = 2;   /* random */
     else if (cAsyncScheme[0] == 'b')
        asyncScheme = 3;   /* bit reversed */
     else
        asyncScheme = 1;   /* progressive */
  }

  // get all decoupler & status parameter values

  // get current index into dm string
  int dmindex = currStatusIndex;
  if (P_getstring(CURRENT, dmName, dm, 1, 255) != 0)
  {
    return (0);
  }
  getstr(dmName,  dm);

  int dmsize  = strlen(dm);
  if (currStatusIndex >= dmsize)
  {
    dmindex = dmsize - 1;
  }

  if (P2TheConsole->RFShared == logicalID) //
  {
         if ((dm[dmsize -1] == 'y') && (dmindex >= (dmsize-1)))
         {
  	    dm[dmsize-1] = 'n';
            P2TheConsole->RFSharedDecoupling = 1;
         }
         else
            P2TheConsole->RFSharedDecoupling = 0;
  }
  // get current index into dmm string
  if (P_getstring(CURRENT, dmmName, dmm, 1, 255) != 0)
  {
    abort_message("decoupler status parameter %s does not exist. abort!\n", dmmName);
  }
  getstr(dmmName, dmm);

  int dmmindex = currStatusIndex;
  int dmmsize  = strlen(dmm);
  if (currStatusIndex >= dmmsize)
  {
    dmmindex = dmmsize - 1;
  }


 if ( (dm[dmindex] == 'a')||(dm[dmindex] == 's')||(dm[dmindex] == 'y')||
      (dm[dmindex] == 'A')||(dm[dmindex] == 'S')||(dm[dmindex] == 'Y') )
 {
   statusRFOn = 1;
 }



  // look up translation between dmm chars and waveform names


  switch (dmm[dmmindex])
  {
    case 'c':
        strcpy(dmmWfg,"hard");
        break;

    case 'f':
        strcpy(dmmWfg,"hard");   // this should be swept square wave
        text_message("swept square wave modulation not implemented. using cw on %s\n",Name);
        break;

    case 'g':
        strcpy(dmmWfg, "garp1");
        syncType=2;
        break;

    case 'm':
        strcpy(dmmWfg, "mlev16");
        syncType=2;
        break;

    case 'n':
        strcpy(dmmWfg, "hard");  // this should be noise modulation
        text_message("noise modulation not implemented. using cw on %s\n",logicalName);
        break;

    case 'p':
        if (P_getstring(CURRENT, dseqName, dmmWfg, 1, 255) != 0)
        {
          text_error("decoupler status parameter %s does not exist", dseqName);
          if (statusRFOn) psg_abort(1);
        }
        getstr(dseqName,dmmWfg);

        syncType=2;
        break;

    case 'r':
        strcpy(dmmWfg, "hard");  // this should be square wave modulation
        text_message("square wave modulation not implemented. using cw on %s\n",logicalName);
        break;

    case 'u':
        strcpy(dmmWfg, "hard");  // this should be user set modulation thru ext hardware
        text_message("user set ext hw dec modulation not implemented. using cw on %s\n",logicalName);
        break;

    case 'w':
        strcpy(dmmWfg, "waltz16");
        syncType=2;
        break;

    case 'x':
        strcpy(dmmWfg, "xy32");
        syncType=2;
        break;

    default:
        text_error("status: invalid value %c for parameter %s",dmm[dmmindex], dmmName);
        if (statusRFOn) psg_abort(1);
        break;

  }



  switch ( dm[dmindex] )
  {
     case 'a':
     case 's':
     case 'y':
     case 'A':
     case 'S':
     case 'Y':

        if ( (dm[dmindex] == 's') || (dm[dmindex] == 'S') )
        {
           syncType=1; asyncScheme=0;
        }

        if ( (prevStatusDmIndex >= 0) &&                                                                        \
             ((dm[prevStatusDmIndex] == 'a')||(dm[prevStatusDmIndex] == 's')||(dm[prevStatusDmIndex] == 'y')||  \
              (dm[prevStatusDmIndex] == 'A')||(dm[prevStatusDmIndex] == 'S')||(dm[prevStatusDmIndex] == 'Y')) )
        {
          if ((dmm[dmmindex] != dmm[prevStatusDmmIndex]) || (syncType != prevSyncType) )
          {
             progDecOff(DECSTOPEND, "off", prevSyncType, prevAsyncScheme);

             if (P_getreal(CURRENT, dmfName, &dmf, 1) != 0 )
             {
               text_error("decoupler status parameter %s does not exist", dmfName);
               if (statusRFOn) psg_abort(1);
             }
             dmf  = getval(dmfName);

             if (P_getreal(CURRENT, dresName, &dres, 1) != 0 )
             {
               text_error("decoupler status parameter %s does not exist", dresName);
               if (statusRFOn) psg_abort(1);
             }
             dres = getval(dresName);
             // turn on NEW  dec modulation & gate
             progDecOn(dmmWfg, 1.0/dmf, dres, syncType, "ProgDecOn ") ;   // with new dseq pattern
          }
          else
          {
             if (bgflag) { cout << "RFController::doStatusPeriodAction(): current status same as previous. keep dec going\n"; }
          }
        }
        else
        {
           if (P_getreal(CURRENT, dmfName, &dmf, 1) != 0 )
           {
             text_error("decoupler status parameter %s does not exist", dmfName);
             if (statusRFOn) psg_abort(1);
           }
           dmf  = getval(dmfName);

           if (P_getreal(CURRENT, dresName, &dres, 1) != 0 )
           {
             text_error("decoupler status parameter %s does not exist", dresName);
             if (statusRFOn) psg_abort(1);
           }
           dres = getval(dresName);

           // turn on NEW  dec modulation & gate
           progDecOn(dmmWfg, 1.0/dmf, dres, syncType,"ProgDecOn ") ;   // new decoupling
        }
        break;


     case 'n':
     case 'N':
        if (progDecInterLock)
        {
          // turn off existing dec modulation & gate
          progDecOff(DECSTOPEND, "off", prevSyncType, prevAsyncScheme);
        }
        break;


     default:
        text_error("decoupler status invalid character (%c) in dm string", dm[dmindex] );
        if (statusRFOn) psg_abort(1);
        break;

  }
  prevStatusDmIndex  = dmindex;
  prevStatusDmmIndex = dmmindex;
  prevSyncType       = syncType;
  prevAsyncScheme    = asyncScheme;
  return(0);
}




void RFController::resolveEndOfScanStatusAction(int endStatusIndex, int firstStatusIndex)
{
  char dm[256], dmm[256];

  int statusRFOn = 0;
  int firstSyncType;

  // get current index into dm string
  int dmindex = endStatusIndex;
  if (P_getstring(CURRENT, dmName, dm, 1, 255) != 0)
  {
    // no status action as dmName does not exist
    return ;
  }
  getstr(dmName,  dm);

  int dmsize  = strlen(dm);
  if (endStatusIndex >= dmsize)
  {
    dmindex = dmsize - 1;
  }
  if (dmindex <= 0)
  {
    if (bgflag) { cout << "RFController::resolveEndOfScanStatusAction(): " << dmName << " has zero characters!\n"; }
  }

  // get current index into dmm string
  {
    if (bgflag) { cout << "RFController::resolveEndOfScanStatusAction(): " << dmmName << " parameter has invalid value\n"; }
  }
  getstr(dmmName, dmm);

  int dmmindex = endStatusIndex;
  int dmmsize  = strlen(dmm);
  if (endStatusIndex >= dmmsize)
  {
    dmmindex = dmmsize - 1;
  }
  if (dmmindex <= 0)
  {
    if (bgflag) { cout << "RFController::resolveEndOfScanStatusAction(): " << dmmName << " has zero characters!\n"; }
  }

  int dm1stindex  = 0;
  int dmm1stindex = 0;

  if ( (dm[0] == 's') || (dm[0] == 'S') )
     firstSyncType=1;
  else
     firstSyncType=2;

  int continuous_on = 1;
  for(int i=dm1stindex; i <= dmindex; i++)
  {
    if ( (dm[i] == 'n') || (dm[i] == 'N') )
       continuous_on = 0;
  }
  for(int i=dmm1stindex; i < dmmindex; i++)
  {
    if ( dmm[i] != dmm[i+1] )
       continuous_on = 0;
  }

 if ( (dm[dmindex] == 'a')||(dm[dmindex] == 's')||(dm[dmindex] == 'y')||
      (dm[dmindex] == 'A')||(dm[dmindex] == 'S')||(dm[dmindex] == 'Y') )
 {
   statusRFOn = 1;
 }


  switch ( dm[dmindex] )
  {
     case 'a':
     case 's':
     case 'y':
     case 'A':
     case 'S':
     case 'Y':
        if ( (dm[dm1stindex] == 'a')||(dm[dm1stindex] == 's')||(dm[dm1stindex] == 'y')||
             (dm[dm1stindex] == 'A')||(dm[dm1stindex] == 'S')||(dm[dm1stindex] == 'Y') )
        {
          if (continuous_on)
          {
            if ((dm[dm1stindex] == 's') || (dm[dm1stindex] == 'S') )
            {
               // Policy = DECSTOPEND, since synchronous
               progDecOff(DECSTOPEND, "synchronous at start", 1, 0);
            }
            else
            {
               // Policy = DECCONTINUE
               progDecOff(DECCONTINUE, "continue at end of scan", 1, 0);
            }
          }
          else
          {
            // Policy = DECSTOPEND
            progDecOff(DECSTOPEND, "stop at end of scan", prevSyncType, prevAsyncScheme);
          }
        }
        else
        {
          // dec is not on for 1st status.  turn dec off at end of last status
          // Policy = DECSTOPEND
          progDecOff(DECSTOPEND, "stop at end of scan. dec not on for 1st status", prevSyncType, prevAsyncScheme);
        }
        break;


     default:
        // dec was not on for last status period. nothing to do
        break;

  }
  return;
}

void RFController::setHiLowFreq(double hlf)
{
  xmtrHiLowFreq = hlf;
}

double RFController::getHiLowFreq()
{
  return(xmtrHiLowFreq);
}

void RFController::setObserve(int onFlag)
{
  observeFlag = onFlag;
}

int RFController::isObserve()
{
  return(observeFlag);
}

void RFController::setSystemIF(double sysIF)
{
  systemIF=sysIF;
}

double RFController::getSystemIF()
{
  return(systemIF);
}

void RFController::setOffsetFrequency(double offset)
{
  baseOffset = offset/1000000.0; /* set to MHZ */
}

void RFController::setBaseFrequency(double bfreq)
{
  baseFreq = bfreq;
}

double RFController::getBaseFrequency()
{
  return(baseFreq);
}

double RFController::getOffsetFrequency()
{
  return(baseOffset);
}

double RFController::getCurrentOffset()
{
  return(currentOffset);
}

void RFController::setCurrentOffset(double offset)
{
  currentOffset = offset;
}

double RFController::getDefAmp()
{
  return(defAmp);
}

void RFController::setCWMode(int on_is_1)
{
  CW_mode= on_is_1;
  if (on_is_1)
    setGates(RFGATEON(RF_CW_MODE));
  else
    setGates(RFGATEOFF(RF_CW_MODE));
}

int RFController::getCWmode()
{
  return(CW_mode);
}
//
//  experimetal method
//
void RFController::VPhaseStep(double value,int rtvar)
{
  int buffer[4];
  buffer[0] = degrees2Binary(currentPhaseStep);  // phase multiplier..
  buffer[1] = rtvar;    // the rtvar to uses
  outputACode(VPHASE,2,buffer); // out they go..
}

//
//  auto offset a shaped pulse experimental..
//
#define TAU_S (0.0000001L)
//  phss is phase per step in full precision ..
//
cPatternEntry *RFController::makeOffsetPattern(char *nm, int nInc, double phss, double pacc, int flag, char mode,
                                               const char *tag, const char *emsg, int action)
{
  int i,l,value;
  int pInc;
  double f1,f2,f3,f4,f5,scalef,pDinc;
  int wcount, eventcount, repeats,trepeats;
  char qname[200],tname[200];
  pInc = degrees2Binary(phss); //  --- rounding .....
  sprintf(qname,"ols_%s:%d,%g_%5.3f_%s",nm,nInc,phss,pacc,tag);

  cPatternEntry *tmp;

  tmp = find(qname, flag);
  if (tmp != NULL)
  {
     // if mode == standard, no need to enter pattern into alias list
     if (mode == 's')
        return(tmp);

     // if mode == compressed|indexed, enter pattern into alias list
     int newPatID = tmp->allocatePatID();
     int oldPatID = tmp->getReferenceID();
     addToPatAliasList(newPatID, oldPatID);
     return(tmp);
  }

  scalef = 1023.0;

  pDinc = phss*((double) nInc);
  wcount = 0;
  eventcount = 0;
  strcpy(tname,nm);
  strcat(tname,".RF");
  //
  i = findPatternFile(tname);
  if (i != 1)
  {
      text_error("%s could not find %s", emsg, tname);
      if (action > -1)
        psg_abort(1);
      else
        return(NULL);
   }

   patternCount++;
   pWaveformBuf->startSubSection(PATTERNHEADER+patternCount);


   // these patterns are fixed duration at TAU_S-
   //
   // read in the file
   do
   {
      l = scanLine(&f1,&f2,&f3,&f4,&f5);
      if ((l > 2) && (f3 < 0.5))
      {
        scalef = f2;
        continue;
      }

      repeats = 1;
      if (l > 2) //
        repeats = ((int) f3) & 0xfff;
      repeats *= nInc;
      //
      if (l > 1)
      {
        // adjust this to the scalef
        value = RFAMPKEY | amp2Binary(f2*4095.0/scalef) ;
        putPattern( value );
        wcount++;
      }
     if (l > 0)
     {
       // each repeated start is exact ..
       value = degrees2Binary(f1+pacc) | RFPHASEKEY ;
       pacc += pDinc; // ready for next one..
       putPattern( value );
       eventcount += repeats;
       wcount++;
       // phase word choice to utilize RF phase ramp possibility...
       while (repeats > 0)  // might be problem..
       {
        trepeats = repeats;
        if (repeats > 255)
          trepeats = 255;

         putPattern( LATCHKEY | RFPHASEINCWORD(pInc, trepeats, 1) );
         repeats -= trepeats;
         wcount++;
       }
     }
   }  while (l > -1);
  if (eventcount > 0)
  {
     tmp = new cPatternEntry(qname,flag,eventcount,wcount);
     addPattern(tmp);
     sendPattern(tmp);
   }
 return(tmp);
}

//
//
//
int RFController::analyzeRFPattern(char *nm, int flag, const char *emsg, int action)
{
    int i,l;
    double f1,f2,f3,f4,f5;
    int wcount, eventcount, repeats;
    char tname[200];

    // do we already know the pattern??
    cPatternEntry *tmp = find(nm, flag);
    if (tmp != NULL)
    {
      return tmp->getNumberStates();
    }
    strcpy(tname,nm);
    strcat(tname,".RF");
    wcount = 0; eventcount = 0;

       i = findPatternFile(tname);
       if (i != 1)
       {
           text_error("%s could not find %s", emsg, tname);
           if (action > -1)
             psg_abort(1);
           else
             return(0);
       }

       // read in the file
       do
       {
         l = scanLine(&f1,&f2,&f3,&f4,&f5);
         if ((l > 2) && (f3 < 0.5))
             continue;

         repeats = 1;
         if (l > 2) // 255 is low??
           repeats = ((int) f3) & 0xff;
         if (l > 1)
           wcount++;
        if  (l > 0)
          while(repeats-- > 0)
          {
           wcount++;
           eventcount++;
          }
       }  while (l > -1);

    return(eventcount);
}

//
//
//
//*********************************************
//

  // this is outer pattern list composition - it does fraction math and initial
  // phase an handles the LIST mechanism - makeOffsetPatern builds the pattern
  //
int RFController::rRFOffsetShapeList(char *baseshape, double pw, double *offsetList, double frac ,double basePhase, double num, char mode)
{
   char pname[256],tagname[256];
   double Tau,pdelta,Delta,xdur,parg,tsum;
   double rmspower;
   int nInc,i;
   int numberStates, listId;
   long long dur1, divs;

   cPatternEntry *refPat, *tmp;
   if (pw <= 0.0)
     return -1;

   int numEntries = (int)num;

   if (strcmp(baseshape,"") == 0) strcpy(pname,"hard"); else strcpy(pname,baseshape);

   refPat = resolveRFPattern(pname,1,"RFoffset_shapelist",1);
   if (refPat == NULL)
       abort_message("unable to find %s shape file. abort!\n",pname);
   rmspower = refPat->getPowerFraction(); // everyone's got the same power.. 
   strcpy(tagname,"");
   // generates a signature of the offset list ..
   if (numEntries > 1)
   {
	   tsum = 0.0;
	   for (i=1; i < numEntries; i++)
		   tsum += *(offsetList+i);
	   sprintf(tagname,"%7.1f",tsum);
   }
   numberStates = refPat->getNumberStates();
   listId = -1;
   Delta = pw/((double) numberStates);
   Tau   = OFFSETPULSE_TIMESLICE;         // 200 nS
   nInc = (int)  (pw/(((double) numberStates)*Tau)+0.49);
   if (nInc < 1)
      nInc=1;
   if (Delta < Tau)
      text_message("advisory: minimum duration per state of waveform %s reset to 200ns\n",pname);
   dur1 = calcTicks(Tau);
   divs = dur1*nInc*numberStates;

   xdur = ((double) divs) *0.0000000125;
   parg = fmod((-360.0*frac*xdur*offsetList[0] + basePhase),(double)360.0);
   pdelta =(360.0*offsetList[0]*Tau);  // how many degrees does the phase change in Tau ns?
   tmp  = makeOffsetPattern((char *)pname,nInc,pdelta,parg, 1,mode,tagname,"gen_shapelist",1);
   if (tmp == NULL)
       abort_message("unable to find internal pattern file in gen_shapelist. abort!\n");
   tmp->setTotalDuration(divs);
   tmp->setDurationWord(dur1);
   tmp->setPowerFraction(rmspower);  
   listId = tmp->getReferenceID();
   if ((mode == 'c') || (mode == 'i'))
   {
      for (int i=1; i < numEntries; i++)
	{
          pdelta =(360.0*offsetList[i]*Tau);  // how many degrees does the phase change in Tau ns?
          parg = fmod((-360.0*frac*xdur*offsetList[i] + basePhase),360.0);
          // subtract to END at the correct phase
          tmp  = makeOffsetPattern(pname,nInc,pdelta,parg,1,mode,tagname,"gen_shapelist",1);
          if (tmp == NULL)
              abort_message("unable to find internal pattern file in gen_shapelist. abort!\n");
          tmp->setTotalDuration(divs);
          tmp->setDurationWord(dur1);
          tmp->setPowerFraction(rmspower);  
        }
   }
   if (listId <= -1)
      abort_message("error in gen_shapelist command. abort!\n");
   return(listId);
}

long long RFController::gslx(int listId, double pw, int rtvar, double rof1, double rof2, char mode, int vvar, int on)
{
   long long dur1, divs, ticks;
   cPatternEntry *tmp = NULL;
   int buffer[2];
   int patID = 0;

    setActive();
    clearSmallTicker();
    if ((mode != 'c') && (mode != 's') && (mode != 'i'))
      abort_message("bad mode furnished gen shaped list pulse");
    if ((mode == 'c') || (mode == 's'))
      tmp = findByID(listId);
    if (mode == 'i')
    {
      patID = listId + vvar;
      tmp = findByID(patID);
      if (tmp == NULL)
	{
          patID = getPatternAlias(patID);
          if (patID <= 0)
	    abort_message("Unable to find pattern referenced in shapedpulselist command\n");
          tmp = findByID(patID);
	}
    }
    if (tmp == 0)
       abort_message("Unable to find pattern referenced in shapedpulselist command\n");
    setPowerFraction(tmp->getPowerFraction());
    //cout << "RF chan onOff " << on << " is on off sw" << endl;
    pulsePreAmble(rof1, rtvar);   // turns ON the xmtr!  NEEDS ON?OFF
    // WORKS!
    if (!on)
       setGates(RFGATEOFF(X_LO|X_OUT|X_IF|RFAMP_UNBLANK));
    if (on == 2)
      setGates(RFGATEOFF(R_LO_NOT));  // lo on so concurrent will work
    dur1 = tmp->getDurationWord();
    divs = tmp->getTotalDuration();
    if ((dur1 <= 0) || (divs <= 0))
      abort_message("duration zero for gen_shapedpulselist. Abort!\n");

    add2Stream(DURATIONKEY | dur1);
    noteDelay(divs);
    if (mode == 'c')
    {
       buffer[0] = listId;
       buffer[1] = vvar;
       outputACode(EXECRTPATTERN, 2, buffer);
    }
    if (mode == 's')
        executePattern(listId);
   if (mode == 'i')
       executePattern(patID);
   pulsePostAmble(rof2);
   setPowerFraction(1.0);  // reset power calculation done in resolveRFPattern
   ticks = getSmallTicker();
   return(ticks);  // update 4 sync in calling routine..
}

// prototype
int RFController::registerUserRFShape(char *name, struct _RFpattern *pa, int steps)
{
    int i,value;
    int wordStore[16],wordCount;
    double f1,f2,f3,scalef;
    // double f4,f5;
    double lastamp,lastphase;
    int latchphase;
    int wcount, eventcount, repeats;
    // do we already know the pattern??
    return(5);
    cPatternEntry *tmp = find(name, 1 /* flag */);
    if (tmp != NULL)
      return(-1);  // this is an error..
    // another pattern ID please
    patternCount++;
    // and a buffer
    pWaveformBuf->startSubSection(PATTERNHEADER+patternCount);
    // initialize first level optimizer..
    for (i=0;i<16; i++)  wordStore[i]  = 0;
    wordCount = 0;
    latchphase = TRUE;
    lastamp = -10000.0;    // illegal value
    lastphase = -100012.3; // unlikely value
    eventcount = 0;
    wcount = 0;
    scalef = 4095.0/1023.0;
    for (i=0; i < steps; i++)
      {
         f1 = pa->phase;  /* phase */
         f2 = pa->amp;    /* amplitude */
         f3 = pa->time;   /* dwell */
         //f4 = pa->phase_inc;
         //f5 = pa->amp_inc;
         pa++;
         latchphase = TRUE;
         if ((lastamp != f2) && (lastphase == f1))
            latchphase = FALSE;
         repeats = ((int) f3) & 0xff;
         if (lastamp != f2)
         {
           // adjust this to the scalef
           lastamp = f2;
           value = RFAMPKEY | amp2Binary(f2*4095.0/scalef) ;
           if (latchphase == FALSE)
             value |= LATCHKEY;
           wordStore[wordCount++] = value;
           //putPattern( value );
           wcount++;
         }
         if (latchphase == TRUE)
         {
           value = degrees2Binary(f1) | RFPHASEKEY | LATCHKEY ;
           lastphase = f1;
           //putPattern( value );
           wordStore[wordCount++] = value;
           wcount++;
         }
         eventcount += repeats;
         repeats--;
         if (repeats > 0)
         {
              value = LATCHKEY | RFPHASEINCWORD(0, repeats, 1) ;
              //putPattern( value );
              wordStore[wordCount++] = value;
              wcount++;
         }
         // add inc stuff here...
         for (i=0; i < wordCount; i++) putPattern(wordStore[i]);
         wordCount=0;
      }  // end of for loop..
     // note eventcount may be greater than steps!
     if (eventcount > 0)
     {
        tmp = new cPatternEntry(name,/* flag*/1,eventcount,wcount);
        addPattern(tmp);
        sendPattern(tmp);
     }
     return(9);
}


// improved for mode = 1 minimizes words for highest speed operation
int RFController::registerUserDECShape(char *name, struct _DECpattern *pa, double dres, int mode, int steps)
{
    char lname[256];
    int i,j,firstTime;
    double f1,f2,f3,f4,f5,f6,scalef,tipResolution;
    int wcount, eventcount, repeats;
    int phaseWord, ampWord, gateWord;
    double last_amp, last_phase, last_gate;
    int wordStore[16], storeCount;
    int pcntrl;
    // do we already know the pattern??
    if (dps_flag) return(0);
    sprintf(lname,"%s_%d",name,mode); 
    cPatternEntry *tmp = find(lname, 2);  /* flag?? */
    if (tmp != NULL)
      return(0);
    scalef = 4095.0/1023.0;

    tipResolution = dres;
    wcount = 0;
    eventcount = 0;
    patternCount++;
    pWaveformBuf->startSubSection(PATTERNHEADER+patternCount);

    firstTime=1;
    // read in the file
    storeCount = 0;
    last_amp = -1.0;
    last_phase = -21839.7;
    last_gate = -1.0;
    for (i=0; i < steps; i++)
    {
       pcntrl = mode;  // use pcntrl instead of mode 
       storeCount = 0;
       f1 = pa->tip;  // tip
       f2 = pa->phase;  // phase
       f3 = pa->amp;  // amplitude
       f4 = pa->gate;  //
       f5 = pa->phase_inc; // NEW
       f6 = pa->amp_inc; // NEW
       pa++;
       repeats   = ((int) ((f1/tipResolution)+0.5)) - 1;
       if ( firstTime && (repeats < 0) )
       {
         text_message("tip angle resolution (dres) on %s of %g may be too coarse for waveform %s\n",logicalName,tipResolution,name);
         firstTime = 0;
       }
       if (repeats < 0) continue;
       if (((repeats > 1) && (f5 != 0.0)) || (f6 != 0.0)) 
         pcntrl=2; 
       // gate processing
       if ((pcntrl == 0) || (f4 != last_gate))
       {
         if (((int)f4) & 0x1)
          gateWord = GATEKEY | (RFGATEON(X_OUT)  & 0xffffff);
         else
          gateWord = GATEKEY | (RFGATEOFF(X_OUT) & 0xffffff);
         wordStore[storeCount++] = gateWord;
	 last_gate = f4;
       }
       // amplitude processing
       if ((pcntrl != 1) || (f3 != last_amp))
       {
         ampWord = RFAMPKEY | amp2Binary(f3*scalef) ;
	 wordStore[storeCount++] = ampWord;
	 last_amp = f3;
       }
       // phase processing
       if ((pcntrl != 1) || (f2 != last_phase))
       {
          phaseWord = (degrees2Binary(f2) | RFPHASEKEY) ;
	  wordStore[storeCount++] = phaseWord;
	  last_phase = f2;
       }
       // working point
       if (pcntrl == 2) 
       {
          if (f5 != 0.0) {
             phaseWord = RFPHASEINCWORD(degrees2Binary(f5),repeats+1,1); 
	     wordStore[storeCount++] = phaseWord;
             last_amp = -99; // illegal
          }
          if ((f6 != 0.0) || (f5 == 0.0)) //ensure 1 kind happens
          {
              ampWord = RFAMPINCWORD(amp2Binary(f6*scalef),repeats+1,1); 
	     wordStore[storeCount++] = ampWord;
             last_phase=21839.0;  // unlikely
          }
       }
       wordStore[storeCount-1] |= LATCHKEY;
       // write it out ..
       for (j = 0; j < storeCount; j++)
          putPattern(wordStore[j]);
       wcount += storeCount;
       eventcount++;
      if (pcntrl == 0)
         while(repeats >= 1)
         {
           for (j = 0; j < storeCount; j++)
             putPattern(wordStore[j]);
           wcount += storeCount;
           eventcount++;
           repeats--;
         }
       if (pcntrl == 1) // 
       {
         int rclip,value;
         while (repeats > 0)
         {
	  if (repeats > 255)
	      rclip = 255;
	  else
	      rclip = repeats;
          value = LATCHKEY | RFPHASEINCWORD(0, rclip, 1) ;
          putPattern( value );
          wcount++;
          eventcount += rclip;
	  repeats -= rclip;
         }
       }
       if (pcntrl == 2) eventcount += repeats;
     }
     // add the waveform to Pattern List

     if (eventcount > 0)
     {
        int kk = 1; /* pWaveformBuf->duplicate(200,wcount); */
        tmp = new cPatternEntry(lname,2,kk*eventcount,kk*wcount);
        addPattern(tmp);
        sendPattern(tmp);
     }
     else
       return(0);
    return(tmp->getPatID());
}

void RFController::markGate4Calc(int on)
{
   powerWatch.eventGateChange(on,getBigTicker());
}

void RFController::pprobe(const char *pp)
{
   powerWatch.print(pp);
}


void RFController::setPowerAlarm(int op,  double pwrc,  double alarm, double tc)
{
    powerWatch.setAlarmLevel(alarm,op,"thermal alarm");
    powerWatch.setTimeConstant(tc);
    powerWatch.setMainCalibrate(pwrc);
}


int RFController::getProgDecInterlock()
{
  return progDecInterLock;
}


void RFController::setAuxHomoDecTicker(long long ticks)
{
    auxHomoDecTicker = ticks;
}


long long RFController::getAuxHomoDecTicker()
{
    return auxHomoDecTicker;
}

    
//
// RF pulse group support. - Specific Info for each Controller..
//
RFSpecificPulse::RFSpecificPulse(int RFChanNum, int onOff, char cmode, char *name, double pwidth, \
                  double cpwr, double fpwr, double sphase, double *Freqarray, int nF)
{
  rfChanID = RFChanNum;
  p2rfc = P2TheConsole->getRFControllerByLogicalIndex(rfChanID);
  onFlag = onOff;
  strncpy(shapeName,name,256);
  pw = pwidth;
  mode[0] = cmode;
  tpwr = cpwr;
  tpwrf = fpwr;
  freqA = Freqarray;
  NFreq  = nF;
  startPhase = sphase;
  listID = -1;
}

int RFSpecificPulse::generate()
{
  //cout << "RF Spec generate " << rfChanID << endl;
  listID = p2rfc->rRFOffsetShapeList(shapeName, pw, freqA,/* double frac*/ 0.0 ,startPhase, \
				   NFreq,  mode[0]);
  return(listID);
}

// fixed phase shift moved to generate stage
long long RFSpecificPulse::execute(double r1, double r2, int rtvar, int vvar)
{
  long long tmp;
  p2rfc->setAttenuator(tpwr);
  p2rfc->setAmpScale(tpwrf);
  //  p2rfc->setPhaseStep(startPhase);   ->  MOVED to generate..
  // p2rfc->setVFullPhase(one);

  tmp = p2rfc->gslx(listID,pw, rtvar, r1, r2, mode[0] , vvar, onFlag);
  return(tmp + 4);
};


void RFSpecificPulse::tell(char *amsg)
{
int i;
  cout << "RF Specific " << endl;
  cout << amsg << endl;
  cout << "Channel #is " << rfChanID << endl;
  cout << "Its physical name is " << p2rfc->getName() << endl;
  cout << "NFreq " << NFreq << endl;
  cout << "Pattern name " << shapeName << endl;
  cout << "CW Mode " << p2rfc->getCWmode() << endl;
  for (i=0; i < NFreq; i++)
   cout << "index " << i << "  " << *(freqA+i) << endl;
}

////
//// specific configuration imaging excitation
///  all controllers use same LO, have same logical
///  function.
///  serializes the pulse creation.

int RFGroupPulse::tracker=100;

RFGroupPulse::RFGroupPulse(int nn)
{
  int i;
  numberInGroup = nn;
  isGend = 0;
  tracker++;
  myNum = tracker;
  next = NULL;
  for (i=0; i<  MAXINGROUP; i++ ) pulseArray[i] = NULL;
}

//////////
///  puts one of the channels into the group
///  modify by getControllerData..
///  0 BASED COUNT
///
int RFGroupPulse::addSpecificPulse(RFSpecificPulse *p2rfsi,int num)
{
  if (num >= MAXINGROUP) abort_message("Only %d group allowed",MAXINGROUP);
  if (num < 0) abort_message("Negative channel error");
  if  (pulseArray[num] != NULL) abort_message("group channel initialization conflict");
  pulseArray[num] = p2rfsi;
  return(1);
}
//
//  modify the data base - generate no codes
// the off by one
RFSpecificPulse *RFGroupPulse::getControllerData(int number)
{
  if ((number < 1) || (number > numberInGroup))
    abort_message("GroupPulse: controller number %d out of range",number);
  return(pulseArray[number-1]);
}

//  causes the patterns to be generated.
int RFGroupPulse::generate()
{
  int i;
  if (isGend)
     return(0);
  for (i=0; i < numberInGroup; i++)
  {
    pulseArray[i]->generate();
  }
  isGend = 1;
  return(0);
}
//
// does the acodes and the timing
//
long long RFGroupPulse::run(double r1, double r2, int vvar1, int vvar2)
{
  int i, flag; // need to put in power...
  long long tickA[16];
  long long tickMax;
  generate();

  //P2TheConsole->newEvent();
  tickMax = 0L;
  flag = 0;
  for (i=0; i < numberInGroup; i++)
    {
    tickA[i] = pulseArray[i]->execute(r1,r2,vvar1, vvar2);
    if (tickMax < tickA[i])
      tickMax = tickA[i];
    }
  //cout << "TickMax " << tickMax << endl;
  for (i=0; i < numberInGroup; i++)
    {
      //cout << tickA[i] << " is ticks on " << i << endl;
      tickA[i] = tickMax-tickA[i]; // difference..
      if ((tickA[i] < 4L) && (tickA[i] > 1L)) flag=1;
      //cout << tickA[i] << " is differential ticks on " << i << endl;
    }
  for (i = 0; i < numberInGroup; i++)
    {
      if (flag)
      {
         tickA[i] += 4L;
         tickMax += 4L;
      }
      //cout << tickA[i] << "is ticks on " << i << endl;
      if (tickA[i] > 0L)
	(pulseArray[i]->p2rfc)->setTickDelay(tickA[i]);
    }
  //P2TheConsole->update4Sync(tickMax);
  return(tickMax);
}

int RFGroupPulse::append(RFGroupPulse *pp)
{
   int k;
   if (next == NULL)
   {
      next = pp;
      return(pp->myNum);
   }
   k = next->append(pp);
   return(k);
}

void RFGroupPulse::tell(char *amsg)
{
  cout << "Hi Im a group pulse " << endl;
  cout << "Message: " <<amsg << endl;
}

//
// RFCHANNEL GROUP - a container class
//
int RFChannelGroup::addPulse(RFGroupPulse *p2myPulse)
{

  int j;
  if (pulsell == NULL)
  {
    pulsell = p2myPulse;
    j = 1;
  }
  else
  {
    j = pulsell->append(p2myPulse);
  }
  return(j);
}


RFGroupPulse *RFChannelGroup::getPulse(int num)
{
  RFGroupPulse *p1;
  p1 = pulsell;
  while (p1 != NULL)
  {
    if ( p1->myNum == num)
    {
      return(p1);
    }
    p1 = p1->next;
  }
  return(p1);
}
// con is user -1 Bridge call does subtract.
RFSpecificPulse *RFChannelGroup::getSpecific(int num, int con)
{
   RFGroupPulse *p1;
   p1 = getPulse(num);
   return(p1->getControllerData(con));
}

RFChannelGroup::RFChannelGroup(char *configStr)
{
  /* this is an ugly way to do it */
  int i,j,k;
  k = strlen(configStr);
  numberInGroup = 0;
  for (i=0;i<k; i++)  // configStr has little error tolerance
    {
      j = configStr[i] - '0';  // convert from char to int
      rfchannelIndexList[numberInGroup] = i+1;
      if ((j > 0) && (j < 32)) //
	{
          if (j == (i+1)) // he's in and the master
	    {
	      numberOfMaster=i+1;
            }
           numberInGroup++;
        }
    }
    if (numberInGroup > MAXINGROUP)
      abort_message("more group channels allotted than %d",MAXINGROUP);
    pulsell = NULL;  // empty linked list.
    pulsesCreated = 0;
}

void RFChannelGroup::tell(char *emsg)
{
  int i;
  RFController *rfc;
  cout << "RF Channel Group " << endl;
  cout << emsg << endl;
  cout << "group elements = " << numberInGroup << "  master is " << numberOfMaster << endl;
  for (i = 0; i < numberInGroup; i++)
    {
       rfc = P2TheConsole->getRFControllerByLogicalIndex(rfchannelIndexList[i]);
       cout << "-----------------------------" << endl;
       cout << "ordinal " << i << "physical " << rfchannelIndexList[i] << endl;
       cout << "The controller thinks its name is " << rfc->getName() << endl;
       cout << "Observe is " << rfc->isObserve() << endl;
       cout << "CW mode is " << rfc->getCWmode() << endl;
   }
}

