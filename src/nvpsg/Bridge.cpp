/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <string.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <unistd.h>

#include "Console.h"
#include "AcodeManager.h"
#include "FFKEYS.h"
#include "Bridge.h"
#include "acqparms.h"
#include "ACode32.h"
#include "lc.h"
#include "cpsg.h"
#include "RFController.h"

extern "C" {

#include "safestring.h"

}

extern unsigned int ix;
extern int dps_flag;
extern int statusindx;
extern int rcvroff_flag;
extern int rcvr_is_on_now;
extern int OBSch;
extern int DECch;
extern int DEC2ch;
extern int DEC3ch;
extern int nomessageflag;
extern int DEC4ch;
extern double oldpad;
extern double preacqtime;
#ifdef LINUX
using namespace std;
#endif

/* RF decouping flags / policy */
#define DECCONTINUE     (1)
#define DECSTOPEND      (2)
#define DECRESERVED     (12)
#define DECLAST         (16)
#define DECFRYM         (32)

/* Time Slice for Offset List Pulse 200ns */
#define OFFSETPULSE_TIMESLICE (0.00000020L)

/** Length of a tick in seconds. */
#define SECONDS_PER_TICK (12.5e-9)

/* Tickers                       */
static long long startScanTicker = 0L;

/** Minimum time between setting frequencies. */
#define MIN_SECS_PER_FREQUENCY (10e-6)

extern int grad_flag;
extern int bgflag;
extern int zerofid;
int readuserbyte; //SAS

// called from rtcontrol.c
extern "C"{
   void ddrpushloop(int);
   void ddrpoploop(int);
   void zero_all_gradients();
   void rgradient(char, double);
   void settmpgradtype(char *);
   void ifzero(int);
   void endif(int);
}

static int NOWAIT_loopcount = 0;
static void readMRIByte(int rtvar, int ticks);

// local utility functions

inline RFController *obschannel()
{
   return (RFController*)P2TheConsole->getRFControllerByLogicalIndex(OBSch);
}

inline DDRController *ddrchannel(int i)
{
   return (DDRController *) (P2TheConsole->DDRUserTable[i]);
}

inline RFController *rfchannel(int i)
{
   return (RFController *) (P2TheConsole->RFUserTable[i]);
}

inline MasterController *master()
{
   return (MasterController *)P2TheConsole->getControllerByID("master1");
}

inline long long calcticks(double d)
{
   return  (P2TheConsole->DDRUserTable[0])->calcTicks(d);
}

inline bool isICAT() {
        if (P2TheConsole->getConsoleSubType() == 's')
                return true;
        else
                return false;
}

//
//
// Interface to sequence elements
//
void delay(double dur)
{
   if(dur<=0)
       return;
   long long count;
   Controller *tmp;
   if (P2TheConsole->isParallel())
      tmp = P2TheConsole->getParallelController();
   else
      tmp = P2TheConsole->getControllerByID("master1");
   P2TheConsole->newEvent();
   tmp->clearSmallTicker();
   if (tmp->isAsync())
   {
      tmp->setAsyncDelay(dur);
   }
   else
   {
      tmp->setActive();
      tmp->armBlaf(1);
      tmp->setDelay(dur);
      tmp->clearBlaf();
   }
   count = tmp->getSmallTicker();
   P2TheConsole->update4Sync(count);
}

double calcDelay(double xx)
{
  return( (double) (calcticks(xx) ) / 80000000.0 );
}

double pwCorr(double xx)
{
   return( calcDelay( 2.0 * xx / PI ) );
}

// a non-shimmable delay - internal use only.
void no_shim_delay(double dur)
{
   if(dur<=0)
       return;
   long long count;
   Controller *tmp;
   if (P2TheConsole->isParallel())
      tmp = P2TheConsole->getParallelController();
   else
      tmp = P2TheConsole->getControllerByID("master1");
   P2TheConsole->newEvent();
   tmp->clearSmallTicker();
   if (tmp->isAsync())
   {
      tmp->setAsyncDelay(dur);
   }
   else
   {
      tmp->setActive();
      tmp->setDelay(dur);
   }
   count = tmp->getSmallTicker();
   P2TheConsole->update4Sync(count);
}

void hsdelay(double dur)
{
   int       codeStream[5];
   int       index;
   long long count;


   index = statusindx;
   if (statusindx >= hssize)
        index = hssize - 1;

   MasterController *tmp = (MasterController *) P2TheConsole->getControllerByID("master1");
   P2TheConsole->newEvent();
   tmp->setActive();
   tmp->clearSmallTicker();

   if ( (hs[index] == 'y') || (hs[index] =='Y') )
   {
       if (dur < hst)
       {  if (ix == 1)
          {  text_error("Delay time is less than homospoil time (hst)\n");
             text_error("No homospoil pulse produced.\n");
          }
       }
       else
       {
          dur = dur - hst;
          if (hst < 1e-7)
          {  text_error("Homospoil time is less than 100 ns (hst)\n");
             text_error("No homospoil pulse produced.\n");
          }
          codeStream[0] = 1;
          tmp->outputACode(HOMOSPOIL,1,codeStream);    // 4 clocks
          tmp->setDelay(hst - 1e-7);
          codeStream[0] = 0;
          tmp->outputACode(HOMOSPOIL,1,codeStream);    // 4 clocks
          tmp->noteDelay(8);
       }
   }

   tmp->armBlaf(1);
   tmp->setDelay(dur);
   tmp->clearBlaf();
   count = tmp->getSmallTicker();
   P2TheConsole->update4Sync(count);
}

//
// statusdelay
//
void statusDelay(int index, double statusdelay)
{
  status(index);
  delay(statusdelay);
}

//
// pad delay
//
void preacqdelay(int padactive)
{
   long long count;
   double paddelay = 0.0;
   if (padactive)
   {
     getRealSetDefault(CURRENT,"pad",&paddelay,0.0);
     if ((paddelay > LATCHDELAY) && (oldpad != paddelay))
     {
       Controller *tmp = P2TheConsole->getControllerByID("master1");
       P2TheConsole->newEvent();
       tmp->setActive();
       tmp->clearSmallTicker();
       tmp->setDelay(paddelay);
       count = tmp->getSmallTicker();
       P2TheConsole->update4Sync(count);
       preacqtime = paddelay;
       oldpad     = paddelay;
     }
   }
}

void status(int index)
{

 if (index < 0 || index > MAXSTATUSES)
    index = 0;                      /* set to first char */
 else
 {
    statusindx = index;
    int numRFChan = P2TheConsole->numRFChan;
    for(int i=0;i<numRFChan;i++)
    {
      ((RFController *)(P2TheConsole->RFUserTable[i]) )->doStatusPeriodActions(statusindx);
    }
  }
}

//+++++++++++++++++++++  DDR specific ++++++++++++++++++++++++++++++
// "explicit" acquire utility call (rtcontrol.c)
CEXTERN void pushloop(int n)
{
  for(int i=0;i<P2TheConsole->numDDRChan;i++)
      ddrchannel(i)->pushloop(n);
}

// "explicit" acquire utility call (rtcontrol.c)
CEXTERN void poploop(int n)
{
  for(int i=0;i<P2TheConsole->numDDRChan;i++)
      ddrchannel(i)->poploop(n);
}

// turn ON "receiver gate" (DDR_RG)
CEXTERN void rgon()
{
  for(int i=0;i<P2TheConsole->numDDRChan;i++)
      ddrchannel(i)->setReceiverGate(1);

  rcvroff_flag   = 0;
  rcvr_is_on_now = 1;
}

// turn OFF "receiver gate" (DDR_RG)
CEXTERN void rgoff()
{
  for(int i=0;i<P2TheConsole->numDDRChan;i++)
      ddrchannel(i)->setReceiverGate(0);

  rcvroff_flag   = 1;
  rcvr_is_on_now = 0;
}

// turn ON "data gate" (DDR_IEN)
CEXTERN void dgon()
{
  for(int i=0;i<P2TheConsole->numDDRChan;i++)
      ddrchannel(i)->setDataGate(1);
}

// turn OFF "data gate" (DDR_IEN)
CEXTERN void dgoff()
{
  for(int i=0;i<P2TheConsole->numDDRChan;i++)
      ddrchannel(i)->setDataGate(0);
}

// turn ON "acquisition gate" (DDR_ACQ)
CEXTERN void agon()
{
  for(int i=0;i<P2TheConsole->numDDRChan;i++)
      ddrchannel(i)->setAcqGate(1);
}

// turn OFF "acquisition gate" (DDR_ACQ)
CEXTERN void agoff()
{
  for(int i=0;i<P2TheConsole->numDDRChan;i++)
      ddrchannel(i)->setAcqGate(0);
}
// set DDR acquisition options (see acqparm.h for opcodes)
CEXTERN void setacqmode(int m)
{
  for(int i=0;i<P2TheConsole->numDDRChan;i++)
      ddrchannel(i)->setAcqMode(m);
}

// set DDR acquisition control variable
CEXTERN void setacqvar(int m)
{
  for(int i=0;i<P2TheConsole->numDDRChan;i++)
      ddrchannel(i)->setAcqVar(m);
}
// set DDR real time phase properties
CEXTERN void setRcvrPhaseStep(double step)
{
  for(int i=0;i<P2TheConsole->numDDRChan;i++)
      ddrchannel(i)->setRcvrPhaseStep(step);
}
CEXTERN void setRcvrPhaseVar(int var)
{
  for(int i=0;i<P2TheConsole->numDDRChan;i++)
      ddrchannel(i)->setRcvrPhaseVar(var);
}
// set DDR acquisition control variable
CEXTERN void sendzerofid(int arrayDim)
{
  for(int i=0;i<P2TheConsole->numDDRChan;i++)
      ddrchannel(i)->sendZeroFid(ix,arrayDim);
}

// set DDR channel specific properties

//
//
// "explicit" acquire startup call
CEXTERN void startacq(double tm)
{
  int loChNum;
  long long ticks, ticks0;
  ticks = 0LL; ticks0 = 0LL;
  DDRController *ddrCh;
  RFController *observeCh;
  int numActiveDDRs = 0;
  int doneOnce = 0, firstDDR = 1;
  char PDDacquireStr[MAXSTR], volumercvStr[MAXSTR];

  /* PDD control signal for switched transmit-receive configuration imaging*/
  /* Global parameter PDDacquire determines whether switching is primarily about acquisition or RF pulses */
  getStringSetDefault(GLOBAL,"PDDacquire",PDDacquireStr,"u"); /* undefined if PDDacquire doesn't exist */
  getStringSetDefault(CURRENT,"volumercv",volumercvStr,"y");
  if (PDDacquireStr[0] == 'y' && volumercvStr[0] == 'n') splineon(3);
  if (PDDacquireStr[0] == 'n' && volumercvStr[0] == 'y') splineon(3);

  P2TheConsole->newEvent();

  for(int i=0;i<P2TheConsole->numDDRChan;i++)
  {
    ddrCh = ddrchannel(i);
    if ( ! ddrCh->ddrActive4Exp )
    {
      ddrCh->inactiveRcvrSetup();
      continue;
    }
    else
     numActiveDDRs++;

    if ( (i == 0) && (P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_SINGACQ) )
    {
      if (OBSch == RCVR2_RF_LO_CHANNEL)
        ddrCh = ddrchannel(1);
      else
        ddrCh = ddrchannel(0);
    }
    if ( ! ddrCh->ddrActive4Exp ) continue;

    loChNum = ddrCh->get_RF_LO_source();
    if (loChNum <= 0)
      abort_message("error in transmitter/receiver settings. check tn,dn,rcvrs,rcvrstype,rfchannel,probeConnect? abort!\n");

    observeCh = (RFController*)P2TheConsole->getRFControllerByLogicalIndex(loChNum);
    if (observeCh == NULL)
      abort_message("unable to determine observe RF channel (LO source) for ddr(%d). abort!\n",i);

    if ( !doneOnce )
    {
      observeCh->blankAmplifier();
      observeCh->setGates(RFGATEOFF(RF_TR_GATE));

      if ( P2TheConsole->getRcvrsConfigMap() == MULTIMGRCVR_MULTACQ )
        doneOnce = 1;
    }
    ticks =  ddrchannel(i)->startAcquire(tm);
    if (firstDDR == 1)
    {
      ticks0 = ticks;
      firstDDR = 0;
    }
    if (ticks != ticks0)
      abort_message("time tick mismatch between DDR Controllers. abort!\n");

    ddrchannel(i)->setActive();
  }
  rcvr_is_on_now = 1;
  P2TheConsole->update4Sync(ticks);
}

static long long startacqAlarm = 0L;
static long long endacqAlarm = 0L;
static int pAcq=0;

// "explicit" obs acquire startup call
CEXTERN void startacq_obs(double tm)
{
  int loChNum;
  long long ticks, ticks0;
  ticks = 0LL; ticks0 = 0LL;
  DDRController *ddrCh;
  RFController *observeCh;
  int numActiveDDRs = 0;
  int doneOnce = 0, firstDDR = 1;

  /* Detune transmit for switched transmit-receive configuration */
/*
  char volumercvStr[MAXSTR];
  getStringSetDefault(CURRENT,"volumercv",volumercvStr,"y");
   if (volumercvStr[0] == 'n') splineon(3);
 */

  if ( ! (P2TheConsole->isParallel()) )
     abort_message("startacq_obs only appropriate in parallel code sections");
  P2TheConsole->newEvent();

  for(int i=0;i<P2TheConsole->numDDRChan;i++)
  {
    ddrCh = ddrchannel(i);
    if ( ! ddrCh->ddrActive4Exp )
    {
      ddrCh->inactiveRcvrSetup();
      continue;
    }
    else
     numActiveDDRs++;

    if ( (i == 0) && (P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_SINGACQ) )
    {
      if (OBSch == RCVR2_RF_LO_CHANNEL)
        ddrCh = ddrchannel(1);
      else
        ddrCh = ddrchannel(0);
    }
    if ( ! ddrCh->ddrActive4Exp ) continue;

    loChNum = ddrCh->get_RF_LO_source();
    if (loChNum <= 0)
      abort_message("error in transmitter/receiver settings. check tn,dn,rcvrs,rcvrstype,rfchannel,probeConnect? abort!\n");

    observeCh = (RFController*)P2TheConsole->getRFControllerByLogicalIndex(loChNum);
    if (observeCh == NULL)
      abort_message("unable to determine observe RF channel (LO source) for ddr(%d). abort!\n",i);
    if (startacqAlarm)
    {
       ticks = observeCh->getBigTicker();
       if (ticks < startacqAlarm)
       {
          abort_message("%s is %g sec. earlier than %s\n",
                         (pAcq) ? "parallelacquire_obs" : "startacq_obs",
                         (double) (startacqAlarm - ticks) / 80000000.0,
                         (pAcq) ? "parallelacquire_rcvr" : "startacq_rcvr");
       }
       else if (ticks > startacqAlarm)
       {
          abort_message("%s is %g sec. later than %s\n",
                         (pAcq) ? "parallelacquire_obs" : "startacq_obs",
                         (double) (ticks - startacqAlarm) / 80000000.0,
                         (pAcq) ? "parallelacquire_rcvr" : "startacq_rcvr");
       }
       startacqAlarm = 0L;
    }
    else
    {
       startacqAlarm = observeCh->getBigTicker();
    }

    if ( !doneOnce )
    {
      observeCh->blankAmplifier();
      observeCh->setGates(RFGATEOFF(RF_TR_GATE));

      if ( P2TheConsole->getRcvrsConfigMap() == MULTIMGRCVR_MULTACQ )
        doneOnce = 1;
    }
/*
    ticks =  ddrchannel(i)->startAcquire(tm);
 */
    ticks =  ddrchannel(i)->startAcquireTicksOnly(tm);
    observeCh->setActive();
    observeCh->setTickDelay(ticks);
    if (firstDDR == 1)
    {
      ticks0 = ticks;
      firstDDR = 0;
    }
    if (ticks != ticks0)
      abort_message("time tick mismatch between DDR Controllers. abort!\n");

/*
    ddrchannel(i)->setActive();
 */
  }
  rcvr_is_on_now = 1;
  P2TheConsole->update4Sync(ticks);
}

// "explicit" ddr acquire startup call
CEXTERN void startacq_rcvr(double tm)
{
  int loChNum;
  long long ticks, ticks0;
  ticks = 0LL; ticks0 = 0LL;
  DDRController *ddrCh;
  RFController *observeCh;
  int numActiveDDRs = 0;
  int firstDDR = 1;

  /* Detune transmit for switched transmit-receive configuration */
/*
  char volumercvStr[MAXSTR];
  getStringSetDefault(CURRENT,"volumercv",volumercvStr,"y");
   if (volumercvStr[0] == 'n') splineon(3);
 */

  if ( ! (P2TheConsole->isParallel()) )
     abort_message("startacq_rcvr only appropriate in parallel code sections");
  P2TheConsole->newEvent();

  for(int i=0;i<P2TheConsole->numDDRChan;i++)
  {
    ddrCh = ddrchannel(i);
    if ( ! ddrCh->ddrActive4Exp )
    {
      ddrCh->inactiveRcvrSetup();
      continue;
    }
    else
     numActiveDDRs++;

    if ( (i == 0) && (P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_SINGACQ) )
    {
      if (OBSch == RCVR2_RF_LO_CHANNEL)
        ddrCh = ddrchannel(1);
      else
        ddrCh = ddrchannel(0);
    }
    if ( ! ddrCh->ddrActive4Exp ) continue;

    loChNum = ddrCh->get_RF_LO_source();
    if (loChNum <= 0)
      abort_message("error in transmitter/receiver settings. check tn,dn,rcvrs,rcvrstype,rfchannel,probeConnect? abort!\n");

    observeCh = (RFController*)P2TheConsole->getRFControllerByLogicalIndex(loChNum);
    if (observeCh == NULL)
      abort_message("unable to determine observe RF channel (LO source) for ddr(%d). abort!\n",i);
    if (startacqAlarm)
    {
       ticks = ddrCh->getBigTicker();
       if (ticks < startacqAlarm)
       {
          abort_message("%s is %g sec. later than %s\n",
                         (pAcq) ? "parallelacquire_obs" : "startacq_obs",
                         (double) (startacqAlarm - ticks) / 80000000.0,
                         (pAcq) ? "parallelacquire_rcvr" : "startacq_rcvr");
       }
       else if (ticks > startacqAlarm)
       {
          abort_message("%s is %g sec. earlier than %s\n",
                         (pAcq) ? "parallelacquire_obs" : "startacq_obs",
                         (double) (ticks - startacqAlarm) / 80000000.0,
                         (pAcq) ? "parallelacquire_rcvr" : "startacq_rcvr");
       }
       startacqAlarm = 0L;
    }
    else
    {
       startacqAlarm = ddrCh->getBigTicker();
    }


/*
    if ( !doneOnce )
    {
      observeCh->blankAmplifier();
      observeCh->setGates(RFGATEOFF(RF_TR_GATE));

      if ( P2TheConsole->getRcvrsConfigMap() == MULTIMGRCVR_MULTACQ )
        doneOnce = 1;
    }
 */
    ticks =  ddrchannel(i)->startAcquire(tm);
    if (firstDDR == 1)
    {
      ticks0 = ticks;
      firstDDR = 0;
    }
    if (ticks != ticks0)
      abort_message("time tick mismatch between DDR Controllers. abort!\n");

    ddrchannel(i)->setActive();
  }
/*
  rcvr_is_on_now = 1;
 */
  P2TheConsole->update4Sync(ticks);
}

//
// explicit acquire sample(tm)
CEXTERN void sample(double tm)
{
    int loChNum;
    long long ticks, ticks0;
    ticks = 0LL; ticks0 = 0LL;
    RFController  *observeCh;
    DDRController *ddrCh ;
    int doneOnce = 0, firstDDR = 1;

    P2TheConsole->newEvent();
    for(int i=0;i<P2TheConsole->numDDRChan;i++)
    {
      ddrCh = ddrchannel(i);
      if ( ! ddrCh->ddrActive4Exp ) continue;
      if ( (i == 0) && (P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_SINGACQ) )
      {
        if (OBSch == RCVR2_RF_LO_CHANNEL)
          ddrCh = ddrchannel(1);
        else
          ddrCh = ddrchannel(0);
      }
      if ( ! ddrCh->ddrActive4Exp ) continue;

      loChNum = ddrCh->get_RF_LO_source();
      if (loChNum <= 0)
        abort_message("error in transmitter/receiver settings. check tn,dn,rcvrs,rcvrstype,rfchannel,probeConnect? abort!\n");

      observeCh = (RFController*)P2TheConsole->getRFControllerByLogicalIndex(loChNum);
      if (observeCh == NULL)
        abort_message("unable to determine observe RF channel (LO source) for ddr(%d). abort!\n",i);

      if(!(ddrCh->acq_started))
      {
        if ( !doneOnce )
        {
          observeCh->blankAmplifier();
          observeCh->setGates(RFGATEOFF(RF_TR_GATE));
          if ( P2TheConsole->getRcvrsConfigMap() == MULTIMGRCVR_MULTACQ )
            doneOnce = 1;
        }
        rcvr_is_on_now = 1;
      }
      ticks = ddrchannel(i)->Sample(tm);
      if (firstDDR == 1)
      {
        ticks0 = ticks;
        firstDDR = 0;
      }
      if (ticks != ticks0)
        abort_message("time tick mismatch between DDR Controllers. abort!\n");
      ddrchannel(i)->setActive();
    }
    P2TheConsole->update4Sync(ticks);
}
// explicit obs acquire sample(tm)
static void sample_obs(double tm)
{
    int loChNum;
    long long ticks, ticks0;
    ticks = 0LL; ticks0 = 0LL;
    RFController  *observeCh;
    DDRController *ddrCh ;
    int firstDDR = 1;

    P2TheConsole->newEvent();
    for(int i=0;i<P2TheConsole->numDDRChan;i++)
    {
      ddrCh = ddrchannel(i);
      if ( ! ddrCh->ddrActive4Exp ) continue;
      if ( (i == 0) && (P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_SINGACQ) )
      {
        if (OBSch == RCVR2_RF_LO_CHANNEL)
          ddrCh = ddrchannel(1);
        else
          ddrCh = ddrchannel(0);
      }
      if ( ! ddrCh->ddrActive4Exp ) continue;

      loChNum = ddrCh->get_RF_LO_source();
      if (loChNum <= 0)
        abort_message("error in transmitter/receiver settings. check tn,dn,rcvrs,rcvrstype,rfchannel,probeConnect? abort!\n");

      observeCh = (RFController*)P2TheConsole->getRFControllerByLogicalIndex(loChNum);
      if (observeCh == NULL)
        abort_message("unable to determine observe RF channel (LO source) for ddr(%d). abort!\n",i);

/*
      if(!(ddrCh->acq_started))
      {
        if ( !doneOnce )
        {
          observeCh->blankAmplifier();
          observeCh->setGates(RFGATEOFF(RF_TR_GATE));
          if ( P2TheConsole->getRcvrsConfigMap() == MULTIMGRCVR_MULTACQ )
            doneOnce = 1;
        }
        rcvr_is_on_now = 1;
      }
      ticks = ddrchannel(i)->Sample(tm);
 */
      if( ! rcvr_is_on_now )
      {
        abort_message("startacq_obs must be called before using sample_obs pulse elements\n");
      }
      observeCh->setActive();
      ticks = observeCh->calcTicks(tm);
      observeCh->setTickDelay(ticks);
      if (firstDDR == 1)
      {
        ticks0 = ticks;
        firstDDR = 0;
      }
      if (ticks != ticks0)
        abort_message("time tick mismatch between DDR Controllers. abort!\n");
/*
      ddrchannel(i)->setActive();
 */
    }
    P2TheConsole->update4Sync(ticks);
}
// explicit ddr acquire sample(tm)
static void sample_rcvr(double tm)
{
    int loChNum;
    long long ticks, ticks0;
    ticks = 0LL; ticks0 = 0LL;
    RFController  *observeCh;
    DDRController *ddrCh ;
    int firstDDR = 1;

    P2TheConsole->newEvent();
    for(int i=0;i<P2TheConsole->numDDRChan;i++)
    {
      ddrCh = ddrchannel(i);
      if ( ! ddrCh->ddrActive4Exp ) continue;
      if ( (i == 0) && (P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_SINGACQ) )
      {
        if (OBSch == RCVR2_RF_LO_CHANNEL)
          ddrCh = ddrchannel(1);
        else
          ddrCh = ddrchannel(0);
      }
      if ( ! ddrCh->ddrActive4Exp ) continue;

      loChNum = ddrCh->get_RF_LO_source();
      if (loChNum <= 0)
        abort_message("error in transmitter/receiver settings. check tn,dn,rcvrs,rcvrstype,rfchannel,probeConnect? abort!\n");

      observeCh = (RFController*)P2TheConsole->getRFControllerByLogicalIndex(loChNum);
      if (observeCh == NULL)
        abort_message("unable to determine observe RF channel (LO source) for ddr(%d). abort!\n",i);

/*
      if(!(ddrCh->acq_started))
      {
        if ( !doneOnce )
        {
          observeCh->blankAmplifier();
          observeCh->setGates(RFGATEOFF(RF_TR_GATE));
          if ( P2TheConsole->getRcvrsConfigMap() == MULTIMGRCVR_MULTACQ )
            doneOnce = 1;
        }
        rcvr_is_on_now = 1;
      }
 */
      ticks = ddrchannel(i)->Sample(tm);
      if (firstDDR == 1)
      {
        ticks0 = ticks;
        firstDDR = 0;
      }
      if (ticks != ticks0)
        abort_message("time tick mismatch between DDR Controllers. abort!\n");
      ddrchannel(i)->setActive();
    }
    P2TheConsole->update4Sync(ticks);
}
//
//
// "explicit" acquire cleanup call
CEXTERN void endacq()
{
  int loChNum;
  long long ticks, ticks0;
  ticks = 0LL; ticks0 = 0LL;
  RFController  *observeCh;
  DDRController *ddrCh;
  int doneOnce = 0, firstDDR = 1;
  char PDDacquireStr[MAXSTR], volumercvStr[MAXSTR];

  /* PDD control signal for switched transmit-receive configuration */
  /* Global parameter PDDacquire determines whether switching is primarily about acquisition or RF pulses */
  getStringSetDefault(GLOBAL,"PDDacquire",PDDacquireStr,"u"); /* undefined if PDDacquire doesn't exist */
  getStringSetDefault(CURRENT,"volumercv",volumercvStr,"y");
  if (PDDacquireStr[0] == 'y' && volumercvStr[0] == 'n') splineoff(3);
  if (PDDacquireStr[0] == 'n' && volumercvStr[0] == 'y') splineoff(3);

  P2TheConsole->newEvent();
  for(int i=0;i<P2TheConsole->numDDRChan;i++)
  {
      ddrCh = ddrchannel(i);
      if ( ! ddrCh->ddrActive4Exp ) continue;
      if ( (i == 0) && (P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_SINGACQ) )
      {
        if (OBSch == RCVR2_RF_LO_CHANNEL)
          ddrCh = ddrchannel(1);
        else
          ddrCh = ddrchannel(0);
      }
      if ( ! ddrCh->ddrActive4Exp ) continue;

      loChNum = ddrCh->get_RF_LO_source();
      if (loChNum <= 0)
        abort_message("error in transmitter/receiver settings. check tn,dn,rcvrs,rcvrstype,rfchannel,probeConnect? abort!\n");

      observeCh = (RFController*)P2TheConsole->getRFControllerByLogicalIndex(loChNum);
      if (observeCh == NULL)
        abort_message("unable to determine observe RF channel (LO source) for ddr(%d). abort!\n",i);

      if ( !doneOnce )
      {
        observeCh->setTickDelay(ticks);
        observeCh->setGates(RFGATEON(RF_TR_GATE));
        observeCh->setActive();
        if ( P2TheConsole->getRcvrsConfigMap() == MULTIMGRCVR_MULTACQ )
          doneOnce = 1;
      }

      ticks = ddrCh->endAcquire();
      if (firstDDR == 1)
      {
        ticks0 = ticks;
        firstDDR = 0;
      }
      if (ticks != ticks0)
        abort_message("time tick mismatch between DDR Controllers. abort!\n");

      ddrCh->setActive();
  }
  P2TheConsole->update4Sync(ticks);
  rcvr_is_on_now = 0;
}
//
// "explicit" obs acquire cleanup call
CEXTERN void endacq_obs()
{
  int loChNum;
  long long ticks, ticks0;
  ticks = 0LL; ticks0 = 0LL;
  RFController  *observeCh;
  DDRController *ddrCh;
  int doneOnce = 0;

  /* Detune transmit for switched transmit-receive configuration */
/*
  char volumercvStr[MAXSTR];
  getStringSetDefault(CURRENT,"volumercv",volumercvStr,"y");
  if (volumercvStr[0] == 'n') splineoff(3);
 */

  if ( ! (P2TheConsole->isParallel()) )
     abort_message("endacq_obs only appropriate in parallel code sections");
  P2TheConsole->newEvent();
  for(int i=0;i<P2TheConsole->numDDRChan;i++)
  {
      ddrCh = ddrchannel(i);
      if ( ! ddrCh->ddrActive4Exp ) continue;
      if ( (i == 0) && (P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_SINGACQ) )
      {
        if (OBSch == RCVR2_RF_LO_CHANNEL)
          ddrCh = ddrchannel(1);
        else
          ddrCh = ddrchannel(0);
      }
      if ( ! ddrCh->ddrActive4Exp ) continue;

      loChNum = ddrCh->get_RF_LO_source();
      if (loChNum <= 0)
        abort_message("error in transmitter/receiver settings. check tn,dn,rcvrs,rcvrstype,rfchannel,probeConnect? abort!\n");

      observeCh = (RFController*)P2TheConsole->getRFControllerByLogicalIndex(loChNum);
      if (observeCh == NULL)
        abort_message("unable to determine observe RF channel (LO source) for ddr(%d). abort!\n",i);
      if (endacqAlarm)
      {
         ticks = observeCh->getBigTicker();
         if (ticks < endacqAlarm)
         {
            abort_message("%s is %g sec. earlier than %s\n",
                           (pAcq) ? "parallelacquire_obs" : "endacq_obs",
                           (double) (endacqAlarm - ticks) / 80000000.0,
                           (pAcq) ? "parallelacquire_rcvr" : "endacq_rcvr");
         }
         else if (ticks > endacqAlarm)
         {
            abort_message("%s is %g sec. later than %s\n",
                           (pAcq) ? "parallelacquire_obs" : "endacq_obs",
                           (double) (ticks - endacqAlarm) / 80000000.0,
                           (pAcq) ? "parallelacquire_rcvr" : "endacq_rcvr");
         }
         endacqAlarm = 0L;
      }
      else
      {
         endacqAlarm = observeCh->getBigTicker();
      }


      if ( !doneOnce )
      {
        observeCh->setTickDelay(ticks);
        observeCh->setGates(RFGATEON(RF_TR_GATE));
        observeCh->setActive();
        if ( P2TheConsole->getRcvrsConfigMap() == MULTIMGRCVR_MULTACQ )
          doneOnce = 1;
      }

/*
      ticks = ddrCh->endAcquire();
      if (firstDDR == 1)
      {
        ticks0 = ticks;
        firstDDR = 0;
      }
      if (ticks != ticks0)
        abort_message("time tick mismatch between DDR Controllers. abort!\n");

      ddrCh->setActive();
 */
  }
  P2TheConsole->update4Sync(ticks);
/*
  rcvr_is_on_now = 0;
 */
}
//
// "explicit" ddr acquire cleanup call
CEXTERN void endacq_rcvr()
{
  int loChNum;
  long long ticks, ticks0;
  ticks = 0LL; ticks0 = 0LL;
  RFController  *observeCh;
  DDRController *ddrCh;
  int firstDDR = 1;

  /* Detune transmit for switched transmit-receive configuration */
/*
  char volumercvStr[MAXSTR];
  getStringSetDefault(CURRENT,"volumercv",volumercvStr,"y");
  if (volumercvStr[0] == 'n') splineoff(3);
 */

  if ( ! (P2TheConsole->isParallel()) )
     abort_message("endacq_rcvr only appropriate in parallel code sections");
  P2TheConsole->newEvent();
  for(int i=0;i<P2TheConsole->numDDRChan;i++)
  {
      ddrCh = ddrchannel(i);
      if ( ! ddrCh->ddrActive4Exp ) continue;
      if ( (i == 0) && (P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_SINGACQ) )
      {
        if (OBSch == RCVR2_RF_LO_CHANNEL)
          ddrCh = ddrchannel(1);
        else
          ddrCh = ddrchannel(0);
      }
      if ( ! ddrCh->ddrActive4Exp ) continue;

      loChNum = ddrCh->get_RF_LO_source();
      if (loChNum <= 0)
        abort_message("error in transmitter/receiver settings. check tn,dn,rcvrs,rcvrstype,rfchannel,probeConnect? abort!\n");

      observeCh = (RFController*)P2TheConsole->getRFControllerByLogicalIndex(loChNum);
      if (observeCh == NULL)
        abort_message("unable to determine observe RF channel (LO source) for ddr(%d). abort!\n",i);
      if (endacqAlarm)
      {
         ticks = ddrCh->getBigTicker();
         if (ticks < endacqAlarm)
         {
            abort_message("%s is %g sec. later than %s\n",
                           (pAcq) ? "parallelacquire_obs" : "endacq_obs",
                           (double) (endacqAlarm - ticks) / 80000000.0,
                           (pAcq) ? "parallelacquire_rcvr" : "endacq_rcvr");
         }
         else if (ticks > endacqAlarm)
         {
            abort_message("%s is %g sec. earlier than %s\n",
                           (pAcq) ? "parallelacquire_obs" : "endacq_obs",
                           (double) (ticks - endacqAlarm) / 80000000.0,
                           (pAcq) ? "parallelacquire_rcvr" : "endacq_rcvr");
         }
         endacqAlarm = 0L;
      }
      else
      {
         endacqAlarm = ddrCh->getBigTicker();
      }


/*    
      if ( !doneOnce )
      {
        observeCh->setTickDelay(ticks);
        observeCh->setGates(RFGATEON(RF_TR_GATE));
        observeCh->setActive();
        if ( P2TheConsole->getRcvrsConfigMap() == MULTIMGRCVR_MULTACQ )
          doneOnce = 1;
      }
 */

      ticks = ddrCh->endAcquire();
      if (firstDDR == 1)
      {
        ticks0 = ticks;
        firstDDR = 0;
      }
      if (ticks != ticks0)
        abort_message("time tick mismatch between DDR Controllers. abort!\n");

      ddrCh->setActive();
  }
  P2TheConsole->update4Sync(ticks);
  rcvr_is_on_now = 0;
}
//
//
// set receive mode
void recon()
{
  double rof3;
  long long ticks;
  RFController *observeCh = obschannel();

  if ((P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_SINGACQ) || (P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_MULTACQ))
  {
    for(int i=1;i<P2TheConsole->numDDRChan;i++)  // excludes 1st receiver
    {
      if (ddrchannel(i)->ddrActive4Exp)
        abort_message("pulse sequence command recon is not valid for receiver %d. abort!\n",(i+1));
    }
  }
  getRealSetDefault(CURRENT,"rof3",&rof3,2.0e-6);

  observeCh->setGates(RFGATEOFF(RF_TR_GATE | X_IF | R_LO_NOT));     // set T/R to R

  ticks = calcticks(rof3);

  P2TheConsole->newEvent();
  for(int i=0;i<P2TheConsole->numDDRChan;i++)
  {
      ddrchannel(i)->setDelay(rof3);
  	  ddrchannel(i)->setActive();
  }
  rgon(); // set DDR_RG gate and globals flags
  P2TheConsole->update4Sync(ticks);
}

// set receive mode (with amplifier blanking)
void rcvron()
{
  if ((P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_SINGACQ) || (P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_MULTACQ))
  {
    for(int i=1;i<P2TheConsole->numDDRChan;i++)  // excludes 1st receiver
    {
      if (ddrchannel(i)->ddrActive4Exp)
        abort_message("pulse sequence command rcvron is not valid for receiver %d. abort!\n",(i+1));
    }
  }

  obschannel()->blankAmplifier();   // blank obs amplifier
  recon();                          // set receive mode
}

// set transmit mode
void recoff()
{
  if ((P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_SINGACQ) || (P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_MULTACQ))
  {
    for(int i=1;i<P2TheConsole->numDDRChan;i++)  // excludes 1st receiver
    {
      if (ddrchannel(i)->ddrActive4Exp)
        abort_message("pulse sequence command recoff is not valid for receiver %d. abort!\n",(i+1));
    }
  }

  rgoff(); // clear DDR_RG gate
  obschannel()->setGates(RFGATEON(RF_TR_GATE | X_IF | R_LO_NOT));   // set T/R to T
}

// set transmit mode (with amplifier unblanking)
void rcvroff()
{
  if ((P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_SINGACQ) || (P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_MULTACQ))
  {
    for(int i=1;i<P2TheConsole->numDDRChan;i++)  // excludes 1st receiver
    {
      if (ddrchannel(i)->ddrActive4Exp)
        abort_message("pulse sequence command rcvroff is not valid for receiver %d. abort!\n",(i+1));
    }
  }

  recoff();
  obschannel()->unblankAmplifier();
}

// "explicit" acquire(pts,tm) in pulse-sequence
void acquire(double cnt, double tm)
{
    sample(0.5*cnt*tm);
}
void acquire_obs(double cnt, double tm)
{
    if ( ! (P2TheConsole->isParallel()) )
       abort_message("acquire_obs only appropriate in parallel code sections");
    sample_obs(0.5*cnt*tm);
}

void acquire_rcvr(double cnt, double tm)
{
    if ( ! (P2TheConsole->isParallel()) )
       abort_message("acquire_rcvr only appropriate in parallel code sections");
    sample_rcvr(0.5*cnt*tm);
}


void parallelacquire_obs(double alfa, double cnt, double tm)
{
    if ( ! (P2TheConsole->isParallel()) )
       abort_message("parallelacquire_obs only appropriate in parallel code sections");
    pAcq=1;
    startacq_obs(alfa);
    sample_obs(0.5*cnt*tm);
    endacq_obs();
    pAcq=0;
}
void parallelacquire_rcvr(double alfa, double cnt, double tm)
{
    if ( ! (P2TheConsole->isParallel()) )
       abort_message("parallelacquire_rcvr only appropriate in parallel code sections");
    pAcq = 1;
    startacq_rcvr(alfa);
    sample_rcvr(0.5*cnt*tm);
    endacq_rcvr();
    pAcq = 0;
}


// standard "implicit" acquire

static long long DDRAlarmTimer = 0L;
//
//
//
void dfltstdacq()
{
  int loChNum;
  long long ticks, ticks0;
  DDRController *ddrCh = NULL;
  RFController  *observeCh;

  observeCh = obschannel();
  ticks = observeCh->getBigTicker();

  if (DDRAlarmTimer != 0LL)
  {
    if (ticks - DDRAlarmTimer < 6400)
      abort_message("need at least 80 micro-seconds between acquires. abort!\n");
  }

  P2TheConsole->newEvent();

  ticks = 0LL; ticks0 = 0LL;
  int doneOnce = 0, firstDDR = 1;

  for(int i=0; i<P2TheConsole->numDDRChan; i++)
  {
    ddrCh = ddrchannel(i);

    if ( ! ddrCh->ddrActive4Exp )
    {
      ddrCh->inactiveRcvrSetup();
      continue;
    }

    if (   ddrCh->acq_completed ) continue;

    if ( (i == 0) && (P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_SINGACQ) )
    {
      if (OBSch == RCVR2_RF_LO_CHANNEL)
        ddrCh = ddrchannel(1);
      else
        ddrCh = ddrchannel(0);
    }

    if ( ! ddrCh->ddrActive4Exp ) continue;
    if (   ddrCh->acq_completed ) continue;

    ddrCh->setActive();

    loChNum = ddrCh->get_RF_LO_source();
    if (loChNum <= 0)
      abort_message("error in transmitter/receiver settings. check tn,dn,rcvrs,rcvrstype,rfchannel,probeConnect? abort!\n");

    observeCh = (RFController*)P2TheConsole->getRFControllerByLogicalIndex(loChNum);
    if ( observeCh == NULL )
      abort_message("unable to determine observe RF channel (LO source) for ddr(%d). abort!\n",i);
    observeCh->setActive();             // corresponding RF LO channels also marked Active

    if ( !(ddrCh->acq_completed) )
    {
      if ( !doneOnce )
      {
        observeCh->blankAmplifier();
        observeCh->setGates(RFGATEOFF(RF_TR_GATE));
        ticks = ddrCh->Acquire();
        if (firstDDR == 1)
        {
          ticks0 = ticks;
          firstDDR = 0;
        }
        if (ticks != ticks0)
          abort_message("time tick mismatch between DDR Controllers. abort!\n");
        observeCh->setTickDelay(ticks);
        observeCh->setGates(RFGATEON(RF_TR_GATE));
        if ( P2TheConsole->getRcvrsConfigMap() == MULTIMGRCVR_MULTACQ )
          doneOnce = 1;
      }
      else
      {
        ticks = ddrCh->Acquire();
        if (ticks != ticks0)
          abort_message("time tick mismatch between DDR Controllers. abort!\n");
      }
      // cout <<"acq done for ddr="<<ddrCh->getName()<<"  and rf="<<observeCh->getName() <<" setActive()\n";
    }
  }

   P2TheConsole->update4Sync(ticks);

   DDRAlarmTimer = observeCh->getBigTicker();
   if (ddrCh)
      ddrCh->setChannelActive(1);
}
//
void sharedChannelWfgHDec();
//
//
// "implicit" acquire types need to be sorted out
//
void dfltacq()
{
  char homo[256], hdseq[256];
  int homodecflag=0;

  if ( zerofid )
     return;
  getstr("homo",homo);
  if (strcmp(homo,"y") == 0)
     homodecflag=1;
  if (P2TheConsole->RFSharedDecoupling == 1)
  {
     if (homodecflag)
        abort_message("simultaneous RF shared decoupling and homo decoupling cannot be executed. abort!\n");

     sharedChannelWfgHDec();
     return;
  }
  if (homodecflag)
  {
    getStringSetDefault(CURRENT,"hdseq",hdseq,"");
    if (strlen(hdseq) < 1)
    {
      dflthomoacq();
      return;
    }
    dfltwfghomoacq();
    return;
  }
  
  dfltstdacq();
}
//
// "implicit" acquire w/ homo decoupling (acquire(..) not in pulse-sequence)
//

void dflthomoacq()
{
  double sw, dutycycle;
  double pwr, pwrf, offset, toffset, origpwr, origpwrf;
  double alfa=6.0e-6;

  // the following are special for homo decoupling
  double homo_rof1 = 2.0e-6;
  double homo_rof2 = 2.0e-6;
  double homo_rof3 = 2.0e-6;

  // check for multiple acquire in multi-nuclear receiver
  if ( P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_MULTACQ )
    abort_message("homo decoupling is not implemented for multi-nuclear multi-receiver configuration. abort!\n");

  getRealSetDefault(CURRENT,"homorof1",&homo_rof1,2.0e-6);
  getRealSetDefault(CURRENT,"homorof2",&homo_rof2,1.0e-6);
  getRealSetDefault(CURRENT,"homorof3",&homo_rof3,2.0e-6);
  if (homo_rof1 < 1.0e-6)
       abort_message("homo decoupling parameter homorof1 should be longer than 1 usec. abort!\n");
  if (homo_rof2 < 1.0e-6)
       abort_message("homo decoupling parameter homorof2 should be longer than 1 usec. abort!\n");
  if (homo_rof3 < 1.0e-6)
       abort_message("homo decoupling parameter homorof3 should be longer than 1 usec. abort!\n");

  // get relevant parameters
  sw = getvalnwarn("sw");
  if ((sw <= 300.0) || (sw > 500.001e3))
    abort_message("invalid sw %g for homo decoupling. (valid range 300Hz - 500KHz) abort!\n",sw);
  getRealSetDefault(CURRENT,"dutyc",&dutycycle,0.1);

  if ((dutycycle > 0.4) || (dutycycle < 0.01))
    abort_message("invalid dutyc value %g specified for homo decoupling. (valid range 0.01 - 0.40) abort!\n",dutycycle);

  if (var_active("alfa",CURRENT)==1)
    alfa = getvalnwarn("alfa");


   // check tn, tof parameters


  getRealSetDefault(CURRENT,"tof",&toffset,0.0);

  // save the original tpwr & tpwrf on obs channel to reset with at end
  getRealSetDefault(CURRENT,"tpwr", &origpwr,  -16.0);
  getRealSetDefault(CURRENT,"tpwrf",&origpwrf,4095.0);
  // get homo decoupling parameters

  if (P_getreal(CURRENT, (char*) "hdpwr", &pwr, 1) != 0 )
     abort_message("homo decoupling parameter hdpwr does not exist. abort!\n");
  else
     pwr = getval("hdpwr");
  getRealSetDefault(CURRENT,(char *)"hdpwrf",&pwrf,4095.0);

  if (P_getreal(CURRENT, (char*) "hdof", &offset, 1) != 0 )
     abort_message("homo decoupling parameter hdof does not exist. abort!\n");
   else
     offset = getval("hdof");

  // Observe Controller: set T/R to R mode, blank amplifier, turn off any Tx
  //                     set power, fine power etc

  RFController *observeCh = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(OBSch);
  observeCh->setGates(RFGATEOFF(observeCh->get_RF_PRE_GATES() | observeCh->get_RF_PULSE_GATES()));
  observeCh->setAmpScale(pwrf);                           // fine power
  long long rfticks = observeCh->setAttenuator(pwr);      // attenuator value

  // Currently, we are using the observe channel to produce the rf. Any frequency
  // shift would have to be implemented by phase-ramp, using the phase-increment feature

  double freqshift;
  freqshift = offset - toffset;

  // Now, PINSYNC code for a single (first) DDR

  DDRController *ddrC ;

  if ( P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_SINGACQ )
  {
    ddrC = (DDRController *) (P2TheConsole->getFirstActiveDDRController());
    if (ddrC == NULL)
      abort_message("unable to determine receiver for homo-nuclear decoupling. check transmit/receiver settings. abort!\n");
  }
  else
    ddrC = (DDRController *) (P2TheConsole->DDRUserTable[0]) ;

  // set parse ahead control on VxWorks for inactive DDR controllers
  DDRController *ddrChannel;
  for(int i=0;i<P2TheConsole->numDDRChan;i++)
  {
    ddrChannel = ddrchannel(i);
    if ( ! ddrChannel->ddrActive4Exp )
      ddrChannel->inactiveRcvrSetup();
  }

  long long ddrTicks  = ddrC->startAcquire(alfa);

  P2TheConsole->newEvent();
  ddrC->setActive();
  observeCh->setActive();        // because we need to sync observeCh specially,
                                 // since some ticks have already been done
  P2TheConsole->update4Sync(ddrTicks);

  // now do the observe controller sync-ing
  observeCh->setTickDelay(ddrTicks-rfticks);  // in 12.5ns ticks

  // now all controllers have been synchronized, till the end of PINSYNC


  // establish appropriate chop rate for rf & ddr
  // the chop rate should be an integer multiple of sw
  // chop_factor = 2 * sw
  // so that aliased sidebands do not show up  in-band

  // compute the rfON and rfOFF times from dutycycle and the new chop rate (2*sw)
  double rfONdelay  = 0.5 * (1.0/sw) * (dutycycle);
  double rfOFFdelay = 0.5 * (1.0/sw) * (1.0-dutycycle);
  double acqtime    = ddrC->aqtm;

  double phaseramp  = 360.0 * freqshift * (rfONdelay+rfOFFdelay);   // in one pass of loop


  // compute the number of times the homo dec/ddr gating has to play out

  int homoloops   = (int)(acqtime/(rfONdelay + rfOFFdelay));


  if ((homoloops < 1) || (homoloops > 1e6))
    abort_message("invalid value for homo decoupling loop count. (valid range 1 - 1e6) abort!\n");

  long long rfTicks, ddrElapsTicks, rfElapsTicks;

  ddrC->flushDload();
  observeCh->flushDload();

  ddrTicks = ddrC->getBigTicker();
  rfTicks  = observeCh->getBigTicker();

  if (ddrTicks != rfTicks)
    abort_message("homo decoupling receiver and rf channel ticks do not match at start of acquisition, abort!\n");

  if ( (rfOFFdelay - homo_rof1 - homo_rof2 - homo_rof3) < LATCHDELAY )
  {
    text_message("homo decoupling gating duration is %6.3g us\n",(rfOFFdelay - homo_rof1 - homo_rof2 - homo_rof3)*1e6);
    abort_message("homo decoupling gating durations too short. suggest decreasing dutyc, sw, homorof1, homorof2, homorof3. abort!\n");
  }

  if ( (homo_rof1 + rfONdelay + homo_rof2 + homo_rof3) < LATCHDELAY )
    abort_message("homo decoupling gating durations too short. suggest increasing dutyc or decreasing sw. abort!\n");

  if ( rfONdelay  < LATCHDELAY )
    abort_message("homo decoupling gating durations too short. suggest increasing dutyc or decreasing sw. abort!\n");


  // the following loop is run through the period aqtm
  // duration aqtm = homoloops * (rfOFFdelay + rfONdelay) + residual

  ddrC->clearSmallTicker();
  observeCh->clearSmallTicker();

  F_initval((double)homoloops, res_hdec_lcnt);

  loop(res_hdec_lcnt, res_hdec_cntr);          // res_hdec_lcnt is a special temporary rtvar for homo dec loop

     // DDR: set DDR gating pattern

     ddrC->setAllGates(DDR_RG | DDR_IEN | DDR_ACQ);
     ddrC->setDelay(rfOFFdelay - homo_rof1 - homo_rof2 - homo_rof3);
     ddrC->setAllGates(DDR_ACQ);
     ddrC->setDelay(homo_rof1 + rfONdelay + homo_rof2 + homo_rof3);

     // Observe: set up phase ramp increment
     int buffer[2];
     buffer[0] = observeCh->degrees2Binary(phaseramp);
     buffer[1] = res_hdec_cntr;
     observeCh->outputACode(VPHASE, 2, buffer);

     // Observe: set chopped gating pattern for T/R gate on observe controller

     observeCh->setGates(RFGATEOFF(observeCh->get_RF_PRE_GATES()));          // T/R in R mode
     observeCh->setDelay( rfOFFdelay - homo_rof1 - homo_rof2 - homo_rof3 );
     observeCh->setGates(RFGATEON(observeCh->get_RF_PRE_GATES()));           // T/R in T mode
     observeCh->setDelay( homo_rof1 );
     observeCh->setGates(RFGATEON(observeCh->get_RF_PULSE_GATES()));         // Tx Gate ON
     observeCh->setDelay( rfONdelay );                                       // homo RF ON
     observeCh->setGates(RFGATEOFF(observeCh->get_RF_PULSE_GATES()));        // Tx Gate OFF
     observeCh->setDelay( homo_rof2 );
     observeCh->setGates(RFGATEOFF(observeCh->get_RF_PRE_GATES()));          // T/R in R mode
     observeCh->setDelay( homo_rof3 );

     // sync delay for all other controllers
     P2TheConsole->newEvent();
     ddrC->setActive();
     observeCh->setActive();

     ddrElapsTicks = ddrC->getSmallTicker();
     rfElapsTicks  = observeCh->getSmallTicker();
     if (ddrElapsTicks != rfElapsTicks)
        abort_message("homo decoupling receiver and rf ticks do not match during acquisition. abort!\n");

     P2TheConsole->update4Sync(ddrElapsTicks);

  endloop(res_hdec_cntr);

  // noteDelay the extra ticks in (homoloops-1) loops for all controllers
  P2TheConsole->update4SyncNoteDelay((homoloops-1)*ddrElapsTicks);
  P2TheConsole->setAuxHomoDecTicker((homoloops-1)*ddrElapsTicks);

  observeCh->setGates(RFGATEOFF(observeCh->get_RF_PRE_GATES() | observeCh->get_RF_PULSE_GATES()));    // blank + Tx OFF

  ddrElapsTicks *= homoloops;
  rfElapsTicks  *= homoloops;


  // set the residual as a simple delay on ddr & rf

  double residual = acqtime - (ddrElapsTicks * 12.5e-9);
  ddrC->clearSmallTicker();
  observeCh->clearSmallTicker();

  if (residual >= LATCHDELAY)
  {
    ddrC->setDelay(residual);
    observeCh->setDelay(residual);
  }
  ddrElapsTicks += ddrC->getSmallTicker();
  rfElapsTicks  += observeCh->getSmallTicker();

  ddrC->setAllGates(0);
  observeCh->setGates(RFGATEON(RF_TR_GATE));                    // T/R gate in T mode


  if (ddrElapsTicks != rfElapsTicks)
    abort_message("homo decoupling receiver and rf ticks do not match at end of acquisition. abort!\n");

  if (ddrElapsTicks <  ddrC->calcTicks(acqtime))
  {
    text_message("homo decoupling receiver ticks (in loop) is less than aqtm ticks by %gus. abort!\n",(ddrC->calcTicks(acqtime)-ddrElapsTicks)*12.5e-3);
    text_message("ddrElapsTicks= %lld  acqtime= %g  acqtime(in ticks)= %lld\n",ddrElapsTicks,acqtime, ddrC->calcTicks(acqtime));
    exit(-1);
  }

  P2TheConsole->newEvent();
  ddrC->setActive();
  observeCh->setActive();
  P2TheConsole->update4Sync(ddrC->getSmallTicker());

  rcvr_is_on_now = 0;
  ddrC->reset_acq();

  // now reset the power on the observe channel
  observeCh->setAmpScale(origpwrf);                           // fine power
  rfticks = observeCh->setAttenuator(origpwr);                // attenuator value
  buffer[0] = 0;  // reset the phase counter
  buffer[1] = res_hdec_cntr;
  observeCh->outputACode(VPHASE, 2, buffer);
  P2TheConsole->newEvent();
  observeCh->setActive();
  P2TheConsole->update4Sync(rfticks);
}


////////
//
// "implicit" acquire w/ waveform homo decoupling (acquire(..) not in pulse-sequence)
//

void dfltwfghomoacq()
{
  double sw, hdmf, dutycycle, hdres;
  char hdseq[256];

  double alfa=6.0e-6;

  // the following are special for homo decoupling
  double homo_rof1 = 2.0e-6;
  double homo_rof2 = 2.0e-6;
  double homo_rof3 = 2.0e-6;

  // check for multiple acquire in multi-nuclear receiver
  if ( P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_MULTACQ )
    abort_message("homo decoupling is not implemented for multi-nuclear multi-receiver configuration. abort!\n");
  getRealSetDefault(CURRENT,(char*) "homorof1", &homo_rof1, 2.0e-6);
  if (homo_rof1 < 1.0e-6)
    abort_message("homo decoupling parameter homorof1 should be longer than 1 usec. abort!\n");
  getRealSetDefault(CURRENT,(char*) "homorof2", &homo_rof2, 2.0e-6);
  if (homo_rof2 < 1.0e-6)
    abort_message("homo decoupling parameter homorof2 should be longer than 1 usec. abort!\n");
  getRealSetDefault(CURRENT, (char*) "homorof3", &homo_rof3, 2.0e-6);

  // get relevant parameters
  sw = getvalnwarn("sw");
  if ((sw <= 300.0) || (sw > 500.001e3))
    abort_message("invalid sw %g for homo decoupling. (valid range 300Hz - 500KHz) abort!\n",sw);

  getRealSetDefault(CURRENT, (char*) "dutyc", &dutycycle, 0.1);
  if ((dutycycle > 0.4) || (dutycycle < 0.01))
  {
    abort_message("invalid dutyc value %g specified for homo decoupling. (valid range 0.01 - 0.40) abort!\n",dutycycle);
  }

  if (var_active("alfa",CURRENT)==1)
    alfa = getvalnwarn("alfa");


   // check tn, tof parameters

   double pwr, pwrf, toffset, origpwr, origpwrf;
   char tnString[256];

  if (P_getstring(CURRENT, (char*) "tn", tnString, 1, 256) == 0 )
    getstr("tn",tnString);
  else
    abort_message("homo decoupling: parameter tn not defined. abort!\n");

  if (P_getreal(CURRENT, (char*) "tof", &toffset, 1) != 0 )
    abort_message("homo decoupling parameter tof does not exist. abort!\n");
  else
    toffset = getval("tof");


  // save the original tpwr & tpwrf on obs channel to reset with at end

  if (P_getreal(CURRENT, (char*) "tpwr", &origpwr, 1) != 0 )
    abort_message("homo decoupling parameter tpwr does not exist. abort!\n");
  else
    origpwr = getval("tpwr");

  getRealSetDefault(CURRENT, (char*) "tpwrf", &origpwrf,4095.0);
  // get homo decoupling parameters

  if (P_getreal(CURRENT, (char*) "hdpwr", &pwr, 1) != 0 )
     abort_message("homo decoupling parameter hdpwr does not exist. abort!\n");
  else
     pwr = getval("hdpwr");

  getRealSetDefault(CURRENT, (char*) "hdpwrf", &pwrf, 4095.0);

  if (P_getstring(CURRENT, (char*) "hdseq", hdseq, 1, 256) == 0 )
    getstr("hdseq",hdseq);
  else
    abort_message("homo decoupling parameter hdseq not defined. abort!\n");

  if (P_getreal(CURRENT, (char*) "hdmf", &hdmf, 1) != 0 )
     abort_message("homo decoupling parameter hdmf does not exist. abort!\n");
   else
     hdmf = getval("hdmf");

  getRealSetDefault(CURRENT, (char*) "hdres", &hdres, 9.0);

  RFController *observeCh = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(OBSch);
  observeCh->setGates(RFGATEOFF(observeCh->get_RF_PRE_GATES() | observeCh->get_RF_PULSE_GATES()));
  observeCh->setAmpScale(pwrf);                           // fine power
  long long rfticks = observeCh->setAttenuator(pwr);      // attenuator value

  DDRController *ddrC ;

  if ( P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_SINGACQ )
  {
    ddrC = (DDRController *) (P2TheConsole->getFirstActiveDDRController());
    if (ddrC == NULL)
      abort_message("unable to determine receiver for homo-nuclear decoupling. check transmit/receiver settings. abort!\n");
  }
  else
    ddrC = (DDRController *) (P2TheConsole->DDRUserTable[0]) ;


  // Now, PINSYNC code for a single (first) DDR

  long long ddrTicks = ddrC->startAcquire(alfa);

  P2TheConsole->newEvent();
  ddrC->setActive();
  observeCh->setActive();        // because we need to sync observeCh specially,
                                 // since some ticks have already been done
  P2TheConsole->update4Sync(ddrTicks);

  // now do the observe controller sync-ing
  observeCh->setTickDelay(ddrTicks-rfticks);  // in 12.5ns ticks

  // now all controllers have been synchronized, till the end of PINSYNC

  double acqtime         = ddrC->aqtm;
  long long acqtimeticks = ddrC->calcTicks(acqtime);
  int dwTicks            = (int) ddrC->calcTicks(1.0/sw);
  int rfonTicks          = (int) (dwTicks * dutycycle);
  int rof1Ticks          = (int) ddrC->calcTicks(homo_rof1);
  int rof2Ticks          = (int) ddrC->calcTicks(homo_rof2);
  int rof3Ticks          = (int) ddrC->calcTicks(homo_rof3);
  long long rfoffTicks   = (long long) (dwTicks-rfonTicks-rof1Ticks-rof2Ticks-rof3Ticks);

  cPatternEntry *homopat = observeCh->resolveHomoDecPattern(hdseq, (int)hdres, (1.0/hdmf), dwTicks, rfonTicks, acqtimeticks, rof1Ticks, rof2Ticks, rof3Ticks);
  observeCh->setPowerFraction(dutycycle);
  //.... assume full power in pattern and mult by dutyc
  //.... refine
  // start the observe RFController running the long homo dec gated waveform

  P2TheConsole->newEvent();
  ddrC->setActive();
  observeCh->setActive();
  // the pattern driver does not enable power tracker 
  observeCh->markGate4Calc(1);
  observeCh->executePattern(homopat->getReferenceID());
  observeCh->noteDelay(acqtimeticks);
  observeCh->setGates(RFGATEOFF(observeCh->get_RF_PRE_GATES() | observeCh->get_RF_PULSE_GATES()));
  observeCh->setGates(RFGATEON(RF_TR_GATE));
  // completes the RF program.  Do not let it span the loops part or the power detection will be off.

  ddrC->clearSmallTicker();
  long long ddrElapsTicks = 0;

  // start the DDR loop

  int homoloops   = acqtimeticks/dwTicks;
  F_initval((double)homoloops, res_hdec_lcnt);

  P2TheConsole->suspendPowerCalculation();

  loop(res_hdec_lcnt, res_hdec_cntr);

     ddrC->setAllGates(DDR_RG | DDR_IEN | DDR_ACQ);
     ddrC->setTickDelay(rfoffTicks);
     ddrC->setAllGates(DDR_ACQ);
     ddrC->setTickDelay((long long) (rfonTicks+rof1Ticks+rof2Ticks+rof3Ticks));

  endloop(res_hdec_cntr);

  P2TheConsole->enablePowerCalculation();

  // printf("homoloops=%d acqtimeticks=%lld  dwTicks=%d  rfONTicks=%d rfoffTicks=%lld\n",homoloops,acqtimeticks,dwTicks, rfonTicks, rfoffTicks);

  // end of DDR loop
  // execute the residual delay after loops on DDR

  ddrElapsTicks = homoloops * (ddrC->getSmallTicker());

  ddrC->noteDelay((homoloops-1)*(ddrC->getSmallTicker()));

  long long residualTicks = acqtimeticks - ddrElapsTicks;
  if (residualTicks > 0 )
  {
     ddrC->setTickDelay(residualTicks);
     ddrElapsTicks += residualTicks;
  }

  ddrC->setAllGates(0);
  rcvr_is_on_now = 0;

  if (ddrElapsTicks != acqtimeticks)
     abort_message("wfghomodec DDR tick count does not match the aqtm ticks. abort!\n");

  observeCh->setActive();
  observeCh->setPowerFraction(1.0);
  P2TheConsole->update4Sync(acqtimeticks);

  ddrC->reset_acq();
  ddrC->acq_completed = 1;
  // now reset the power on the observe channel
  observeCh->setAmpScale(origpwrf);                           // fine power reset
  observeCh->setAmp(4095.0);                                  // wfg amp reset
  rfticks = observeCh->setAttenuator(origpwr);                // attenuator value reset
  P2TheConsole->newEvent();
  observeCh->setActive();
  P2TheConsole->update4Sync(rfticks);

}

////////
//
// "implicit" acquire w/ waveform homo decoupling
//  for shared (combination) RF amplifier configurations...

void sharedChannelWfgHDec()
{
  double sw, hdmf, dutycycle, hdres;
  char hdseq[256];
  long long rfonTicks,rof1Ticks,rof2Ticks,rof3Ticks,rfoffTicks;
  long long ddrTicks,acqtimeticks,dwTicks;
  DDRController *ddrC;
  RFController *observeCh, *decoupleCh;
  cPatternEntry *homopat;
  // the following are special for homo decoupling
  double homo_rof1 = 2.0e-6;
  double homo_rof2 = 2.0e-6;
  double homo_rof3 = 2.0e-6;
  double alfa=6.0e-6;
  long long ddrElapsTicks = 0;
  long long obsElapsTicks = 0;

  if ((OBSch != 1) && (DECch != 2))
    abort_message("illegal configuration for MR400 HF operation!");
  observeCh= (RFController *) P2TheConsole->getRFControllerByLogicalIndex(OBSch);
  decoupleCh= (RFController *) P2TheConsole->getRFControllerByLogicalIndex(DECch);
  ddrC = (DDRController *) (P2TheConsole->DDRUserTable[0]) ;
  // check for multiple acquire in multi-nuclear receiver
  if ( P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_MULTACQ )
    abort_message("shared decoupling is not implemented for multi-receiver configuration. abort!\n");
  //
  getRealSetDefault(CURRENT,(char*) "homorof1", &homo_rof1, 2.0e-6);
  if ( (homo_rof1 < 1.0e-6) && !nomessageflag )
    abort_message("shared decoupling parameter homorof1 should be longer than 1 usec. abort!\n");
  getRealSetDefault(CURRENT,(char*) "homorof2", &homo_rof2, 2.0e-6);
  if ( (homo_rof2 < 1.0e-6) && !nomessageflag )
    abort_message("shared decoupling parameter homorof2 should be longer than 1 usec. abort!\n");
  getRealSetDefault(CURRENT, (char*) "homorof3", &homo_rof3, 2.0e-6);
  if ( (homo_rof3 < 1.0e-6) && !nomessageflag )
    abort_message("shared decoupling parameter homorof1 should be longer than 1 usec. abort!\n");
  //
  sw = getvalnwarn("sw");
  if ( ((sw <= 300.0) || (sw > 60.001e3)) && !nomessageflag )
    abort_message("invalid sw %g for combination decoupling. (valid range 300Hz - 60KHz) abort!\n",sw);

  getRealSetDefault(CURRENT, (char*) "dutyc", &dutycycle, 0.4);
  // should tighten range
  if ( ((dutycycle > 0.9) || (dutycycle < 0.1)) && !nomessageflag )
  {
    abort_message("invalid dutyc value %g specified for shared decoupling. (valid range 0.10- 0.90) abort!\n",dutycycle);
  }

  if (var_active("alfa",CURRENT)==1)
    alfa = getvalnwarn("alfa");

  //-- should just use dseq or fetch from object..
  //-- might check dmm last
  if (P_getstring(CURRENT, (char*) "dseq", hdseq, 1, 256) == 0 )
    getstr("dseq",hdseq);
  else
    OSTRCPY( hdseq, sizeof(hdseq), "garp1");
  //-- should just use dmf or fetch from object
  hdmf = getval("dmf");
  if ( (hdmf > 50000.0) && !nomessageflag )
     abort_message("shared decoupling limits dmf < 50k");
  if ( (hdmf <= 0.0) && !nomessageflag )
     abort_message("shared decoupling parameter dmf does not exist. abort!\n");
  // should just use dres
  getRealSetDefault(CURRENT, (char*) "dres", &hdres, 9.0);
  //
  P2TheConsole->newEvent();
  ddrC->setActive();
  observeCh->setActive();
  decoupleCh->setActive();
  observeCh->setGates(RFGATEOFF(observeCh->get_RF_PRE_GATES() | observeCh->get_RF_PULSE_GATES()));
  ddrTicks = ddrC->startAcquire(alfa);
  observeCh->setTickDelay(ddrTicks); // no event time on observe.
  decoupleCh->setTickDelay(ddrTicks);
  P2TheConsole->update4Sync(ddrTicks);

  // we could postpone update...

  acqtimeticks = ddrC->calcTicks(ddrC->aqtm);
  dwTicks      = 1600;  /* 50 Khz FIXED chop rate */
  rfonTicks    = (long long )((double) dwTicks * dutycycle);
  rof1Ticks    = ddrC->calcTicks(homo_rof1);
  rof2Ticks    = ddrC->calcTicks(homo_rof2);
  rof3Ticks    = ddrC->calcTicks(homo_rof3);
  rfoffTicks   = dwTicks-rfonTicks-rof1Ticks-rof2Ticks-rof3Ticks;

  homopat = decoupleCh->resolveHomoDecPattern(hdseq, (int)hdres, (1.0/hdmf), \
                     dwTicks, rfonTicks, acqtimeticks, rof1Ticks, rof2Ticks, rof3Ticks);

  // start the observe RFController running the long homo dec gated waveform

  P2TheConsole->newEvent();
  ddrC->setActive();
  observeCh->setActive();
  decoupleCh->setActive();
  observeCh->clearSmallTicker();
  decoupleCh->clearSmallTicker();
  ddrC->clearSmallTicker();

  decoupleCh->markGate4Calc(1);
  decoupleCh->noteDelay(acqtimeticks);
  decoupleCh->executePattern(homopat->getReferenceID());
  decoupleCh->setGates(RFGATEOFF(decoupleCh->get_RF_PRE_GATES() | decoupleCh->get_RF_PULSE_GATES()));
// as with home dec shut off the gates or the loops will confuse the power calc.

  int homoloops   = acqtimeticks/dwTicks;
  F_initval((double)homoloops, res_hdec_lcnt);
  //  X_OUT X_IF are OFF ALREADY...
  observeCh->setGates(RFGATEOFF(RF_TR_GATE | RFAMP_UNBLANK | R_LO_NOT ));
  loop(res_hdec_lcnt, res_hdec_cntr);
     ddrC->setAllGates(DDR_RG | DDR_IEN | DDR_ACQ);
     ddrC->setTickDelay(rfoffTicks);
     ddrC->setAllGates(DDR_ACQ);
     ddrC->setTickDelay((long long) (rfonTicks+rof1Ticks+rof2Ticks+rof3Ticks));
     observeCh->setTickDelay(rfoffTicks);
     observeCh->setGates(RFGATEON(RF_TR_GATE | RFAMP_UNBLANK | R_LO_NOT));
     observeCh->setTickDelay(rfonTicks+rof1Ticks+rof2Ticks);
     observeCh->setGates(RFGATEOFF(RF_TR_GATE | RFAMP_UNBLANK | R_LO_NOT));
     observeCh->setTickDelay(rof3Ticks);
  endloop(res_hdec_cntr);

  // end of DDR loop
  // execute the residual delay after loops on DDR

  ddrElapsTicks = (homoloops - 1) * (ddrC->getSmallTicker());
  obsElapsTicks = (homoloops - 1) * (observeCh->getSmallTicker());
  if (obsElapsTicks  != ddrElapsTicks)
  cout << "obs elapsed " << (double)obsElapsTicks << "   ddr elapsed " << (double)ddrElapsTicks << endl;

  ddrC->noteDelay(ddrElapsTicks);
  observeCh->noteDelay(obsElapsTicks);
  ddrElapsTicks = ddrC->getSmallTicker();
  obsElapsTicks = observeCh->getSmallTicker();
  if (obsElapsTicks  != ddrElapsTicks)
  cout << "obs elapsed " << (double)obsElapsTicks << "   ddr elapsed " << (double)ddrElapsTicks << endl;
  //  Check out code...
  long long residualTicks = acqtimeticks - ddrElapsTicks;
  if (residualTicks > 0 )
  {
     ddrC->setTickDelay(residualTicks);
     ddrElapsTicks += residualTicks;
     observeCh->setTickDelay(residualTicks);
     obsElapsTicks += residualTicks;
  }
  if (residualTicks < 0)
    abort_message("shared decoupling: internal error.  abort!\n");

  ddrC->setAllGates(0);
  rcvr_is_on_now = 0;
  // restore initial / safe conditions.
  observeCh->setGates(RFGATEOFF(observeCh->get_RF_PRE_GATES() | observeCh->get_RF_PULSE_GATES()));
  observeCh->setGates(RFGATEON(RF_TR_GATE));
  decoupleCh->setGates(RFGATEOFF(decoupleCh->get_RF_PRE_GATES() | decoupleCh->get_RF_PULSE_GATES()));
  // tr gate is ch1 shared..
  decoupleCh->setPhase(0.0);
  decoupleCh->setAmp(4095.0);
  if (ddrElapsTicks != acqtimeticks)
     abort_message("shared decoupling: DDR tick count does not match the aqtm ticks. abort!\n");

  P2TheConsole->update4Sync(acqtimeticks);
  ddrC->reset_acq();
}
//
//
//
void setactivercvrs(char *str)
{
  int i, active, numactive=0;

  for (i=0; i< P2TheConsole->numDDRChan; i++)
  {
    active = (parmToRcvrMask(str,2) >> i) & 1;
    ((DDRController *) (P2TheConsole->DDRUserTable[i]))->setDDRActive4Exp(active);
    if (active == 1)
      numactive++;
  }
  P2TheConsole->setNumActiveRcvrs(numactive);
}
//
//
//
void set4Tune(int chan, double gain)
{
  double frq;
  int flag,tchan;
  Controller *masterC = P2TheConsole->getControllerByID("master1");
  RFController *rf2Tune =  (RFController *) P2TheConsole->getRFControllerByLogicalIndex(chan);
  flag = 0;
  frq = rf2Tune->getBaseFrequency();
  if (frq > 405.0)
    flag = 1;
  tchan = chan;
  P2TheConsole->newEvent();
  if ((P2TheConsole->RFShared > 0) and (chan == 2))
     tchan = 1;
  masterC->setActive();
  masterC->clearSmallTicker();
  // LO selection moved to master object.
  ((MasterController *)masterC)->set4Tune(chan,tchan,(int) gain,flag);
  // now sync all controllers
  P2TheConsole->update4Sync(masterC->getSmallTicker());
}

//
// Acquire while transmitting on observe. Padded to
// last acquistion time.
//

void XmtNAcquire(double pw, int phase,double rof1)
{
  long long rfTicks;
  long long ticks=0L;
  double freq;
  int startWord,stopWord,i;

  RFController *observeCh = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(OBSch);
  freq = observeCh->getBaseFrequency();
  if (freq > 150.0)
    startWord = RFGATEON(RF_MIXER);
  else
    startWord = RFGATEOFF(RF_MIXER);
  stopWord = startWord;
  startWord |= RFGATEON(X_LO|X_OUT|X_IF|RFAMP_UNBLANK) | RFGATEOFF(R_LO_NOT|RF_TR_GATE);
  stopWord |= RFGATEOFF(X_LO|X_OUT|X_IF|RFAMP_UNBLANK | R_LO_NOT| RF_TR_GATE);
  // timeline ..
  P2TheConsole->newEvent();
  observeCh->setActive();
  observeCh->clearSmallTicker();

  observeCh->setVQuadPhase(phase);
  observeCh->setDelay(rof1);
  observeCh->setGates(startWord);
  for (i=0; i< P2TheConsole->numDDRChan; i++)
    {
      if (((DDRController *) (P2TheConsole->DDRUserTable[i]))->ddrActive4Exp)
      {
      ((DDRController *) (P2TheConsole->DDRUserTable[i]))->setActive();
      ((DDRController *) (P2TheConsole->DDRUserTable[i]))->clearSmallTicker();
      ticks = ((DDRController *) (P2TheConsole->DDRUserTable[i]) )->Sample(np*0.5/sw);
      }
    }
  observeCh->setDelay(pw);
  observeCh->setGates(stopWord);
  rfTicks = observeCh->getSmallTicker();
  observeCh->setTickDelay(ticks-rfTicks);
  P2TheConsole->update4Sync(ticks);
}
/////
//   Acquire while transmitting on observe. Padds time to AT.
//   Pulse tune functionality.
/////
void ShapedXmtNAcquire(char *aname, double pw, int phase, double rof1,int chan)
{
  long long rfTicks,duration;
  long long ticks=0L;
  double freq;
  int startWord,stopWord,divs,i;
  cPatternEntry *patDes;
  char tname[200];
  OSTRCPY( tname, sizeof(tname), aname);
  RFController *observeCh = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(chan);
  freq = observeCh->getBaseFrequency();
  if (freq > 150.0)
    startWord = RFGATEON(RF_MIXER);
  else
    startWord = RFGATEOFF(RF_MIXER);
  stopWord = startWord;
  startWord |= RFGATEON(X_LO|X_OUT|X_IF|RFAMP_UNBLANK) | RFGATEOFF(R_LO_NOT|RF_TR_GATE);
  stopWord |= RFGATEOFF(X_LO|X_OUT|X_IF|RFAMP_UNBLANK | R_LO_NOT| RF_TR_GATE);

  P2TheConsole->newEvent();
  observeCh->setActive();
  observeCh->clearSmallTicker();
  observeCh->setVQuadPhase(phase);
  observeCh->setDelay(rof1);
  observeCh->setGates(startWord);
  for (i=0; i < P2TheConsole->numDDRChan; i++) {
     if (!((DDRController *) (P2TheConsole->DDRUserTable[i]))->ddrActive4Exp) continue;
     ((DDRController *) (P2TheConsole->DDRUserTable[i]))->setActive();
     ((DDRController *) (P2TheConsole->DDRUserTable[i]))->clearSmallTicker();
     ticks = ((DDRController *) (P2TheConsole->DDRUserTable[i]))->Sample(np*0.5/sw);}
  /////////
  patDes = observeCh->resolveRFPattern(tname,1,"genRFShapedPulse",1);
  if (patDes == 0)
  {
    abort_message("NO Pattern found for ShapedXmtNAcquire.");
  }
  observeCh->setPowerFraction(patDes->getPowerFraction());
  
   // this element allows the observeCh to time independently...
   int minTicks = (int)(P2TheConsole->getMinTimeEventTicks());
   divs = patDes->getNumberStates();
   duration = observeCh->calcTicksPerState(pw,divs,minTicks,0.1,"genShaped  & Acquire Pulse",0);
   if (duration < 0)
     duration = 0L;
   if (duration > 0x3ffffffL)
     {
       abort_message("pattern element > 0.8 sec\n");
     }

  observeCh->add2Stream(DURATIONKEY | duration);  // unlatched...wait for first element
  observeCh->noteDelay(duration*((long long)divs));
  observeCh->executePattern(patDes->getReferenceID());
  observeCh->setGates(stopWord);
  observeCh->setPowerFraction(1.0);  // reset power calculation
  rfTicks = observeCh->getSmallTicker();
  if (rfTicks <= ticks)
  {
    observeCh->setTickDelay(ticks-rfTicks);
    P2TheConsole->update4Sync(ticks);
  }
  else
  {
    for (i=0; i < P2TheConsole->numDDRChan; i++)
    {
       if (!((DDRController *) (P2TheConsole->DDRUserTable[i]))->ddrActive4Exp) continue;
       ((DDRController *) (P2TheConsole->DDRUserTable[i]))->setTickDelay(rfTicks-ticks);
    }
    P2TheConsole->update4Sync(rfTicks);
  }

}

/**
 * Steps across a given frequency range while observing with the DDR.
 * Note that the number of frequency steps to take is specified here (nfreqs),
 * while the number of data points taken is specified by the "np" parameter
 * (np/2 acquires). If np/2 is not divisible by nfreqs, the timing of the
 * acquisitions relative to the frequency changes will vary.
 * @param fstartMHz The first frequency to set.
 * @param fendMHz The last frequency to set.
 * @param nfreqs The number of frequencies to set, including fstartMHz
 * and fendMHz.
 * @param chanBitmap Bitmap of which RFController channels to use,
 * with (bit0 set) = (use chan1), (bit1 set) = (use chan2), etc.
 * @param offset_sec The delay between setting a frequency and acquiring
 * a data point.
 */
void MultiChanSweepNAcquire(double fstartMHz, double fendMHz, int nfreqs,
                            unsigned int chanBitmap, double offset_sec)
{
  const int MAX_OBS_CHANS = 8;
  RFController *obsChans[MAX_OBS_CHANS];
  int startWords[MAX_OBS_CHANS];
  int stopWords[MAX_OBS_CHANS];
  int nObsChans;
  long long rfTicks,compT;
  long long ticks=0L;
  double cfreq;
  long long offsetTicks;


  nObsChans = 0;
  for (int chan = 1; chan <= MAX_OBS_CHANS; chan++) {
    int chanIdx = chan - 1;
    if ( ((1 << chanIdx) & chanBitmap) != 0) {
      // This channel is ON in chanBitmap
      obsChans[nObsChans] = P2TheConsole->getRFControllerByLogicalIndex(chan);
      if (obsChans[nObsChans]->getBaseFrequency() > 150.0) {
        startWords[nObsChans] = RFGATEON(RF_MIXER);
      } else {
        startWords[nObsChans] = RFGATEOFF(RF_MIXER);
      }
      stopWords[nObsChans] = startWords[nObsChans];
      startWords[nObsChans] |= (RFGATEON(X_LO|X_OUT|X_IF | RFAMP_UNBLANK)
                                | RFGATEOFF(R_LO_NOT));
      stopWords[nObsChans] |= (RFGATEOFF(X_LO|X_OUT|X_IF | RFAMP_UNBLANK
                                         | R_LO_NOT| RF_TR_GATE));
      nObsChans++;
    }
  }

  P2TheConsole->newEvent();
  for (int i = 0; i < nObsChans; i++) {
    obsChans[i]->setActive();
    obsChans[i]->setGates(startWords[i]);
    obsChans[i]->clearSmallTicker();
  }
  offsetTicks = (int)(offset_sec / SECONDS_PER_TICK);

  // Sample element does
  for (int i = 0; i < P2TheConsole->numDDRChan; i++)
  {
    DDRController *ddrC = (DDRController *)P2TheConsole->DDRUserTable[i];
    ddrC->setActive();
    ddrC->clearSmallTicker();
    ddrC->setTickDelay(offsetTicks);
    ticks = ddrC->Sample(np * 0.5 / sw);
  }

  rfTicks = ticks / nfreqs;
  if (rfTicks * SECONDS_PER_TICK < MIN_SECS_PER_FREQUENCY)
  {
    abort_message("MultiChanSweepNAcquire: Less than %g s between freq changes",
                  MIN_SECS_PER_FREQUENCY);
  }

  for (int i = 0; i < nfreqs; i++)
  {
    cfreq = fstartMHz + i * (fendMHz - fstartMHz) / (nfreqs - 1);
    for (int j = 0; j < P2TheConsole->numDDRChan; j++)
    {
      compT = obsChans[j]->setFrequency(cfreq);
      obsChans[j]->setTickDelay(rfTicks-compT);
    }
  }
  for (int i = 0; i < nObsChans; i++) {
    obsChans[i]->setGates(stopWords[i]);
    rfTicks = obsChans[i]->getSmallTicker();
    obsChans[i]->setTickDelay(offsetTicks + ticks - rfTicks);
  }
  P2TheConsole->update4Sync(offsetTicks + ticks);
}

/**
 * Steps across a given frequency range while observing with the DDR.
 * Note that the number of frequency steps to take is specified here (nfreqs),
 * while the number of data points taken is specified by the "np" parameter
 * (np/2 acquires). If np/2 is not divisible by nfreqs, the timing of the
 * acquisitions relative to the frequency changes will vary.
 * @param fstartMHz The first frequency to set.
 * @param fendMHz The last frequency to set.
 * @param nfreqs The number of frequencies to set, including fstartMHz
 * and fendMHz.
 * @param chan The index of the RFController channel to use.
 * @param offset_sec The delay between setting the first frequency and acquiring
 * the first data point.
 */
void SweepNOffsetAcquire(double fstartMHz, double fendMHz, int nfreqs,
                         int chan, double offset_sec)
{
  long long rfTicks,compT;
  long long ticks=0L;
  double freq,cfreq;
  int startWord,stopWord,i;
  long long offsetTicks;

  RFController *observeCh
          = (RFController *)P2TheConsole->getRFControllerByLogicalIndex(chan);
  freq = observeCh->getBaseFrequency();
  if (freq > 150.0)
  {
    startWord = RFGATEON(RF_MIXER);
  }
  else
  {
    startWord = RFGATEOFF(RF_MIXER);
  }
  stopWord = startWord;
  startWord |= RFGATEON(X_LO|X_OUT|X_IF|RFAMP_UNBLANK) | RFGATEOFF(R_LO_NOT);
  stopWord  |= RFGATEOFF(X_LO|X_OUT|X_IF|RFAMP_UNBLANK | R_LO_NOT| RF_TR_GATE);

  P2TheConsole->newEvent();
  observeCh->setActive();
  observeCh->setGates(startWord);
  observeCh->clearSmallTicker();
  offsetTicks = (int)(offset_sec / SECONDS_PER_TICK);

  // Sample element does
  for (i=0; i <P2TheConsole->numDDRChan; i++)
  {
    DDRController *ddrC = (DDRController *)P2TheConsole->DDRUserTable[i];
    if (!(ddrC->ddrActive4Exp)) continue;
    ddrC->setActive();
    ddrC->clearSmallTicker();
    ddrC->setTickDelay(offsetTicks);
    ticks = ddrC->Sample(np * 0.5 / sw);
  }

  rfTicks = ticks / nfreqs;
  if (rfTicks * SECONDS_PER_TICK < MIN_SECS_PER_FREQUENCY)
  {
    abort_message("SweepNOffsetAcquire: Less than %g s between frequency steps",
                  MIN_SECS_PER_FREQUENCY);
  }

  for (i = 0; i < nfreqs; i++)
  {
    cfreq = fstartMHz + i * (fendMHz - fstartMHz) / (nfreqs - 1);
    if (cfreq > 150.0) {
      observeCh->setGates(RFGATEON(RF_MIXER));
    } else {
      observeCh->setGates(RFGATEOFF(RF_MIXER));
    }
    compT = observeCh->setFrequency(cfreq);
    observeCh->setTickDelay(rfTicks-compT);
  }
  observeCh->setGates(stopWord);
  rfTicks = observeCh->getSmallTicker();
  observeCh->setTickDelay(offsetTicks + ticks - rfTicks);
  P2TheConsole->update4Sync(offsetTicks + ticks);
}

//
// sweep rf on chan over freq range... observe using DDR.
//
void SweepNAcquire(double fstartMHz, double fendMHz, int nfreqs,int chan)
{
    SweepNOffsetAcquire(fstartMHz, fendMHz, nfreqs, chan, 0);
}

//
// Set up Receiver LO signal for RF channels
// (this method is only used for console tests)
//
void setRcvrLO(int chan)
{
  int startWord;
  // int stopWord;
  long long ticks;
  double freq;

  RFController *observeCh
          = (RFController *)P2TheConsole->getRFControllerByLogicalIndex(chan);
  freq = observeCh->getBaseFrequency();
  if (freq > 150.0)
  {
    startWord = RFGATEON(RF_MIXER);
  }
  else
  {
    startWord = RFGATEOFF(RF_MIXER);
  }
  // stopWord = startWord;
  startWord |= RFGATEON(X_LO|X_IF) | RFGATEOFF(R_LO_NOT);
  // stopWord  |= RFGATEOFF(X_LO|X_OUT|X_IF|RFAMP_UNBLANK | R_LO_NOT| RF_TR_GATE);

  P2TheConsole->newEvent();
  observeCh->setActive();
  observeCh->setGates(startWord);
  observeCh->clearSmallTicker();

  if (freq > 150.0) {
    observeCh->setGates(RFGATEON(RF_MIXER));
  } else {
    observeCh->setGates(RFGATEOFF(RF_MIXER));
  }
  observeCh->setTickDelay(4LL);
  ticks = observeCh->getSmallTicker();
  P2TheConsole->update4Sync(ticks);

  int flag=0, tchan;
  Controller *masterC = P2TheConsole->getControllerByID("master1");
  if (freq > 405.0)
    flag = 1;
  tchan = chan;

  // set up LO selection
  P2TheConsole->newEvent();
  masterC->setActive();
  masterC->clearSmallTicker();
  // LO selection moved to master object.
  ((MasterController *)masterC)->set4Tune(chan, tchan, (int)50, flag);
  // now sync all controllers
  P2TheConsole->update4Sync(masterC->getSmallTicker());
}

//
// this element is directly "C" callable...
//
//
void genRFPulse(double pw, int phaseC, double rof1, double rof2, int rfch)
{
   long long ticks;

   if (pw <= 0.0)
     return;

   RFController *Work = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfch);
   if (Work->getProgDecInterlock() > 0)
   {
      if (bgflag)
      {
         fprintf(stdout,"advisory: pulse not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
      }
      return;
   }

   P2TheConsole->newEvent();
   // we mark the time on this channel ..
   Work->clearSmallTicker();
   Work->setActive();
   Work->pulsePreAmble(rof1, phaseC);
   Work->setDelay(pw);
   Work->pulsePostAmble(rof2);
   ticks = Work->getSmallTicker();
   Work->setChannelActive(1);
   // this will generate timing error... Work->setDelay(rof2);
   P2TheConsole->update4Sync(ticks);
}


////////////////////
//
//  gensim_pulse/8
//  two centered square RF pulses
//
//  receiver gate on shorter just follows larger...
//
void gensim_pulse(double width1, double width2,
            int phase1, int phase2, double rx1, double rx2,
            int rfdevice1,int rfdevice2)
{
   long long aw1,aw2, awmax, totalTicks;
   long long extra1, extra2;
   long long rx1T, rx2T;
   long long rx1bT, rx2bT;

   aw1=0;    aw2=0;
   rx1T=0;   rx2T=0;
   rx1bT=0;  rx2bT=0;
   extra1=0; extra2=0;

   RFController *S1=NULL, *S2=NULL;
   P2TheConsole->newEvent();

   if (width1 > 0.0)
   {
     S1 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice1);
     if (S1->getProgDecInterlock() > 0)
     {
         if (bgflag)
         {
            fprintf(stdout,"advisory: psg cmd simpulse not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
         }
         return;
     }

     S1->setActive();
     S1->clearSmallTicker();
     aw1   = S1->calcTicks(width1);
     if (aw1 < 4LL) aw1 = 0LL;
     rx1T  = S1->calcTicks(rx1);
     rx1bT = S1->calcTicks(rx2);
   }

   if (width2 > 0.0)
   {
     S2 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice2);
     if (S2->getProgDecInterlock() > 0)
     {
         if (bgflag)
         {
            fprintf(stdout,"advisory: psg cmd simpulse not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
         }
         return;
     }

     S2->setActive();
     S2->clearSmallTicker();
     aw2   = S2->calcTicks(width2);
     if (aw2 < 4LL) aw2 = 0LL;
     rx2T  = S2->calcTicks(rx1);
     rx2bT = S2->calcTicks(rx2);
   }


   // aw1 aw2 is the full widths in ticks...
   awmax = 0LL;
   if (aw1 > aw2)
     awmax = aw1;
   else
     awmax = aw2;

   rx1T += (awmax - aw1)/2;
   if ((rx1T > 0LL) && (rx1T < 4LL))
   {
     rx1T  += 4LL;
     rx2T  += 4LL;
   }
   rx2T += (awmax - aw2)/2;
   if ((rx2T > 0LL) && (rx2T < 4LL))
   {
     rx1T  += 4LL;
     rx2T  += 4LL;
   }

   rx1bT += (awmax - aw1)/2+(awmax -aw1) %2;
   if ((rx1bT > 0) && (rx1bT < 4))
   {
     rx1bT += 4LL;
     rx2bT += 4LL;
   }
   rx2bT += (awmax - aw2)/2+(awmax -aw2) %2;
   if ((rx2bT > 0) && (rx2bT < 4))
   {
     rx1bT += 4LL;
     rx2bT += 4LL;
   }

   if (aw1 >= 4LL)
   {
     S1->pulsePreAmble(rx1T, phase1);
     S1->setTickDelay(aw1);
     S1->pulsePostAmble(rx1bT);
     extra1 = S1->getSmallTicker();
     S1->setChannelActive(1);
   }

   if (aw2 >= 4LL)
   {
     S2->pulsePreAmble(rx2T, phase2);
     S2->setTickDelay(aw2);
     S2->pulsePostAmble(rx2bT);
     extra2 = S2->getSmallTicker();
     S2->setChannelActive(1);
   }



   // check for timing tick match between active controllers

   totalTicks=0;

   if (aw1 >= 4LL)
     totalTicks = extra1;

   if (aw2 >= 4LL)
   {
     if (totalTicks != 0)
     {
       if (totalTicks != extra2)
       {
         abort_message("simpulse: timing mismatch error. abort!\n");
       }
     }
     else
       totalTicks = extra2;
   }

   // notify all other channels of duration ....

   P2TheConsole->update4Sync(totalTicks);
}


////////////////////
//
//
void genRFShapedPulse(double pw, char *name, int rtvar, double rof1, double rof2, double g1, double g2, int rfch)
{
   cPatternEntry *tmp;
   long long extra,duration;
   double value;
   int divs;
   char pname[MAXSTR];

   if (pw <= 0.0)
     return;
   value = pw;

   P2TheConsole->newEvent();
   RFController *ShapedChan = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfch);

   if (strcmp(name,"") == 0)
     OSTRCPY( pname, sizeof(pname), "hard");
   else
     OSTRCPY( pname, sizeof(pname), name);

   tmp = ShapedChan->resolveRFPattern(pname,1,"genRFShapedPulse",1);
   // this element allows the ShapedChan to time independently...
   ShapedChan->setPowerFraction(tmp->getPowerFraction());
   ShapedChan->setActive();
   // track the duration using the small ticker...
   ShapedChan->clearSmallTicker();
   //
   int minTicks = (int)(P2TheConsole->getMinTimeEventTicks());
   divs = tmp->getNumberStates();
   duration = ShapedChan->calcTicksPerState(value,divs,minTicks,0.1,"gen Shaped Pulse",0);
   if (duration < 0)
     abort_message("state duration negative is pattern\n");
   if (duration > 0x3ffffffL)
     {
      abort_message("Pattern state exceeds 0.8 sec\n");
     }
   ShapedChan->pulsePreAmble(rof1+g1, rtvar);   // turns ON the xmtr!

   ShapedChan->add2Stream(DURATIONKEY | duration);  // unlatched...wait for first element
   ShapedChan->noteDelay(duration*((long long)divs));            // maybe plus 1..
   ShapedChan->executePattern(tmp->getReferenceID());
   // the pattern has finished xgate off wait rof2.. blank
   ShapedChan->pulsePostAmble(rof2+g2);
   ShapedChan->setPowerFraction(1.0);  // reset
   ShapedChan->setChannelActive(1);
   extra = ShapedChan->getSmallTicker();
   // notify all other channels of duration ....
   P2TheConsole->update4Sync(extra);
}
//
// EXPERIMENTAL ELEMENT..
//
#define F50NS  (0.00000005L)
#define F200NS (0.00000020L)
#define F500NS (0.00000050L)
//
//
// not an offset list call..
// MAY BE REMOVED SOON..
//
void genRFShapedPulseWithOffset(double pw, char *name, int rtvar, double rof1, double rof2, double offset, int rfch)
{
   cPatternEntry *tmp,*refPat;
   long long extra,dur1,divs;
   double value,Tau,pdelta,Delta;
   int nInc;
   int numberStates;
   char pname[MAXSTR];

   if (pw <= 0.0)
     return;
   value = pw;

   P2TheConsole->newEvent();
   RFController *ShapedChan = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfch);

   if (strcmp(name,"") == 0)
     OSTRCPY( pname, sizeof(pname), "hard");
   else
     OSTRCPY( pname, sizeof(pname), name);
   //
   refPat = ShapedChan->resolveRFPattern(pname,1,"genRFSpecific_Reference",1);
   //
   if (refPat == NULL)
    abort_message("could not find %s\n",pname);
   //
   numberStates = refPat->getNumberStates(); //
   ShapedChan->setPowerFraction(refPat->getPowerFraction()); // a phase ramp doenot change the power
   Delta = pw/((double) numberStates);
   if (Delta < F500NS)
     Tau = Delta;
   else
     if (fabs(offset) > 50000.0)
       Tau = F50NS;
     else
       Tau = F200NS;
   pdelta =(360.0*offset*Tau);  // how many degrees does the phase change in Tau ns?
   nInc = (int) rint(pw/(((double) numberStates)*Tau));
   if (nInc < 1)
   {
     nInc=1;
   }
   // cout << "nInc = " << nInc << "  Tau = " << Tau << endl;
   dur1 = ShapedChan->calcTicks(Tau);
   divs = dur1*nInc*numberStates;


   tmp = ShapedChan->makeOffsetPattern(pname,nInc,pdelta,0.0,1,'s',"","RFShapePulseWithOffset",1);
   ShapedChan->setActive();
   ShapedChan->clearSmallTicker();

   ShapedChan->pulsePreAmble(rof1, rtvar);   // turns ON the xmtr!

   ShapedChan->add2Stream(DURATIONKEY | dur1);  // unlatched...wait for first element

   ShapedChan->noteDelay(divs);
   ShapedChan->executePattern(tmp->getReferenceID());

   ShapedChan->pulsePostAmble(rof2);
   ShapedChan->setPowerFraction(1.0); // restore power level
   ShapedChan->setChannelActive(1);
   extra = ShapedChan->getSmallTicker();

   P2TheConsole->update4Sync(extra);
}

//
//  gets the correct gyromagnetic ratio for the observe nucleus
//
double nuc_gamma()
{
   int    i;
   char tn[MAXSTR];

   struct TN_GAMMA
   {
      const char   *tnstring;
      double  gamma;
   } tn_gamma[] =
     {
      /*  nuclei    Gamma */
         {"H1",   4257.707747},    /* gammas in  [Hz/G] */
         {"H2",    653.6},
         {"He3",  3243.5},
         {"Li7",  1654.7},
         {"C13",  1070.6},
         {"N15",   431.6},
         {"O17",   577.2},
         {"F19",  4006.2},
         {"Na23", 1126.2},
         {"P31",  1723.5},
         {"Xe129",1184.1},
         {"K39",   198.7}
     };

   getstr("tn",tn);           /* Import name of observe nuclei */

   /* find nuclei in tn_gamma and assign correct gamma value */
   for (i = 0; i < (int) (sizeof(tn_gamma)/sizeof(struct TN_GAMMA)); i++)
   {
      if (!strcmp(tn, tn_gamma[i].tnstring))
      {
         return(tn_gamma[i].gamma);
      }
   }
      return(sfrq*1e6/B0);
}

//
//
//
double gen_poffset(double pos, double grad, int rfch)
{
   double nucgamma;

   nucgamma = nuc_gamma();

   return (nucgamma*grad*pos);
}

//
//
//
double gen_shapelistpw(char  *baseshape, double pw, int rfch)
{
   char pname[MAXSTR];
   double Tau,Delta;
   int nInc;
   int numberStates;
   long long dur1, divs;

   if (pw <= 0.0)
     return 0.0;

   RFController *ShapedChan = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfch);
   if (strcmp(baseshape,"") == 0)
     OSTRCPY( pname, sizeof(pname), "hard");
   else
     OSTRCPY( pname, sizeof(pname), baseshape);

   numberStates = ShapedChan->analyzeRFPattern(pname,1,"gen_shapelistpw",1);
   if (numberStates <= 0)
      abort_message("RF pattern %s has no events. abort!\n",pname);

   Delta = pw/((double) numberStates);
   Tau   = OFFSETPULSE_TIMESLICE;         // 200 nS
   if (Delta < Tau)
      text_message("advisory: minimum duration per state of waveform %s is shorter than 200ns\n",pname);
   nInc = (int) rint(pw/(((double) numberStates)*Tau));
   if (nInc < 1)
      nInc=1;

   dur1 = ShapedChan->calcTicks(Tau);
   divs = dur1*nInc*numberStates;
   //return ((double)divs)*12.5e-9 + 6.25e-9; //computes incorrect durations for shapedpulseoffset
   return ((double)divs)*12.5e-9;
}

//
//   This element initializes the elements required by
//   gen_shapepulselist. It returns a listId (int)
//
int gen_shapelist_init(char *baseshape, double pw, double *offsetList, double num, double frac, char mode, int rfch)
{
  RFController *ShapedChan = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfch);
  return(ShapedChan->rRFOffsetShapeList(baseshape, pw, offsetList,\
    frac ,/*double fixedphase*/0.0,  num,  mode));
}


//
// executes the pulse defined in the previous calls
//
void gen_shapedpulselist(int listId, double pw, int rtvar, double rof1, double rof2, char mode, int vvar, int rfch)
{
   long long ticks;

   if ( ! validrtvar(rtvar) )
      abort_message("invalid real time variable specified for phase in gen_shapedpulselist. abort!\n");

   RFController *ShapedChan = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfch);
   if (ShapedChan->getProgDecInterlock() > 0)
   {
       if (bgflag)
       {
          fprintf(stdout,"advisory: psg cmd shapedpulselist not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
       }
       return;
   }

   P2TheConsole->newEvent();

   ticks = ShapedChan->gslx(listId,pw,rtvar,rof1,rof2,mode,vvar,1);  // 1 = on
   P2TheConsole->update4Sync(ticks);
}
//
// -- not a offset list..
//
void gen_shapedpulseoffset(char *name, double pw, int rtvar, double rof1, double rof2, double offset,
 int rfch)
{
   cPatternEntry *tmp,*refPat;
   long long extra,dur1,divs;
   double value,Tau,pdelta,Delta;
   int nInc;
   int numberStates;
   char pname[MAXSTR];

   if (pw <= 0.0)
     return;
   value = pw;

   RFController *ShapedChan = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfch);
   if (ShapedChan->getProgDecInterlock() > 0)
   {
       if (bgflag)
       {
          fprintf(stdout,"advisory: psg cmd shapedpulseoffset not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
       }
       return;
   }

   P2TheConsole->newEvent();

   if (strcmp(name,"") == 0)
     OSTRCPY( pname, sizeof(pname), "hard");
   else
     OSTRCPY( pname, sizeof(pname), name);

   refPat = ShapedChan->resolveRFPattern(pname,1,"gen_shapedpulseoffset",1);
   if (refPat == NULL)
    abort_message("unable to find shape file %s. abort!\n",pname);

   numberStates = refPat->getNumberStates(); // number of events
   ShapedChan->setPowerFraction(refPat->getPowerFraction());

   Delta = pw/((double) numberStates);
   Tau   = OFFSETPULSE_TIMESLICE;            // 200 nS
   if (Delta < Tau)
      text_message("advisory: minimum duration per state of waveform %s reset to 200ns\n",pname);
   pdelta =(360.0*offset*Tau);  // how many degrees does the phase change in Tau ns?
   nInc = (int) rint(pw/(((double) numberStates)*Tau));
   if (nInc < 1)
     nInc=1;

   dur1 = ShapedChan->calcTicks(Tau);
   divs = dur1*nInc*numberStates;
   tmp  = ShapedChan->makeOffsetPattern(pname,nInc,pdelta,0.0,1,'s',"","gen_shapedpulseoffset",1);

   ShapedChan->setActive();
   ShapedChan->clearSmallTicker();

   ShapedChan->pulsePreAmble(rof1, rtvar);   // turns ON the xmtr!

   ShapedChan->add2Stream(DURATIONKEY | dur1);  // unlatched...wait for first element

   ShapedChan->noteDelay(divs);
   ShapedChan->executePattern(tmp->getReferenceID());

   ShapedChan->pulsePostAmble(rof2);
   ShapedChan->setPowerFraction(1.0);  // reset power calculation
   ShapedChan->setChannelActive(1);
   extra = ShapedChan->getSmallTicker();

   P2TheConsole->update4Sync(extra);
}
//
//
////////////////////
//
//
void
swift_acquire(char *shape, double pwon, double preacqdelay)
{
  // the following are special for swift_acquire
  double swift_rof1 = 0.5e-6;
  double swift_rof2 = 0.5e-6;
  double swift_rof3 = 0.5e-6;

  // check for multiple acquire in multi-nuclear receiver

  if (P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_MULTACQ)
    abort_message(
        "swift_acquire is not implemented for multi-nuclear multi-receiver configuration. abort!\n");

  getRealSetDefault(CURRENT, (char*) "swiftrof1", &swift_rof1, 0.5e-6);
  if (swift_rof1 < 0.5e-6)
    abort_message(
        "swift_acquire parameter swiftrof1 should be >= 1 usec. abort!\n");

  getRealSetDefault(CURRENT, (char*) "swiftrof2", &swift_rof2, 0.5e-6);
  if (swift_rof2 < 0.5e-6)
    abort_message(
        "swift_acquire parameter swiftrof2 should be >= 1 usec. abort!\n");

  getRealSetDefault(CURRENT, (char*) "swiftrof3", &swift_rof3, 0.5e-6);
  if (swift_rof3 < 0.5e-6)
    abort_message(
        "swift_acquire parameter swiftrof3 should be >= 0.5 usec. abort!\n");

  // get relevant parameters
  sw = getvalnwarn("sw");

  double alfa = 4.0e-6;
  if (var_active("alfa", CURRENT) == 1)
    alfa = getvalnwarn("alfa");

  // Controller objects
  RFController *observeCh = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(OBSch);
  if (observeCh->getProgDecInterlock() > 0)
  {
      if (bgflag)
      {
         fprintf(stdout,"advisory: psg cmd swift_acquire not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
      }
      return;
  }

  observeCh->setGates(RFGATEOFF(observeCh->get_RF_PRE_GATES() | observeCh->get_RF_PULSE_GATES()));

  DDRController *ddrC ;

  if ( P2TheConsole->getRcvrsConfigMap() == MULTNUCRCVR_SINGACQ )
  {
    ddrC = (DDRController *) (P2TheConsole->getFirstActiveDDRController());
    if (ddrC == NULL)
      abort_message("unable to determine receiver for swift_acquire. check transmit/receiver settings. abort!\n");
  }
  else
    ddrC = (DDRController *) (P2TheConsole->DDRUserTable[0]) ;

  // Now, PINSYNC code for a single (first) DDR

  long long ddrTicks = ddrC->startAcquire(alfa);

  P2TheConsole->newEvent();
  ddrC->setActive();
  observeCh->setActive();        // because we need to sync observeCh specially,
                                 // since some ticks have already been done
  P2TheConsole->update4Sync(ddrTicks);

  // now do the observe controller syncing
  observeCh->setTickDelay(ddrTicks);  // in 12.5ns ticks

  // select sw or swiftmodfreq for RF gating frequency
  double swift_modfreq;
  getRealSetDefault(CURRENT, (char*) "swiftmodfreq", &swift_modfreq, sw);

  if ((swift_modfreq < 300.0) || (swift_modfreq > 1.0e6))
    abort_message(
        "invalid swiftmodfreq %g for swift_acquire (valid range 300Hz - 1.0MHz). abort!\n",
        swift_modfreq);

  // now all controllers have been synchronized, till the end of PINSYNC

  double acqtime         = ddrC->aqtm;
  long long acqtimeTicks = ddrC->calcTicks(acqtime);
  int dwTicks            = (int) ddrC->calcTicks(1.0/swift_modfreq);  // make this chopping rate general
  int rfonTicks          = (int) ddrC->calcTicks(pwon);
  int rof1Ticks          = (int) ddrC->calcTicks(swift_rof1);
  int rof2Ticks          = (int) ddrC->calcTicks(swift_rof2);
  int rof3Ticks          = (int) ddrC->calcTicks(swift_rof3);

  if (rfonTicks >= (dwTicks-(rof1Ticks+rof2Ticks+rof3Ticks)) )
    {abort_message("invalid rf pulse width %g for swift_acquire. abort!\n",pwon);}
  else if ( rfonTicks >= 0.75*(dwTicks-(rof1Ticks+rof2Ticks+rof3Ticks)) )
    {warn_message("rf duty cycle may be too high in swift_acquire");}
  long long rfoffTicks   = (long long) (dwTicks-rfonTicks-rof1Ticks-rof2Ticks-rof3Ticks);
  if (rfoffTicks <= 0L)
    abort_message("invalid rf pulse width %g, rfoffTicks %lld for swift_acquire. abort!\n",pwon,rfoffTicks);

  cPatternEntry *swiftpat = observeCh->resolveSWIFTpattern(shape, dwTicks, rfonTicks, acqtimeTicks, 0);

  // start the observe RFController running the long swift gated waveform

  if (swiftpat == NULL)
    abort_message("swiftpat is null\n");

  P2TheConsole->newEvent();
  ddrC->setActive();
  observeCh->setActive();

  observeCh->noteDelay(acqtimeTicks);
  observeCh->executePattern(swiftpat->getReferenceID());

  ddrC->clearSmallTicker();
  long long ddrElapsTicks = 0;

  // start the DDR loop

  int swiftloops   = acqtimeTicks/dwTicks;
  F_initval((double)swiftloops, res_hdec_lcnt);

  ddrC->setAcqGate(1);

  loop(res_hdec_lcnt, res_hdec_cntr);

     ddrC->setGates(DDR_RG | DDR_IEN);
     ddrC->setTickDelay(rfoffTicks);
     ddrC->clrGates(DDR_RG | DDR_IEN);
     ddrC->setTickDelay((long long) (rfonTicks+rof1Ticks+rof2Ticks+rof3Ticks));

  endloop(res_hdec_cntr);

  ddrC->clrGates(DDR_RG | DDR_IEN | DDR_ACQ);

  // end of DDR loop
  // execute the residual delay after loops on DDR

  ddrElapsTicks = swiftloops * (ddrC->getSmallTicker());

  ddrC->noteDelay((swiftloops-1)*(ddrC->getSmallTicker()));

  long long residualTicks = acqtimeTicks - ddrElapsTicks;
  if (residualTicks > 0 )
  {
     ddrC->setTickDelay(residualTicks);
     ddrElapsTicks += residualTicks;
  }

  ddrC->setAllGates(0);
  rcvr_is_on_now = 0;
  observeCh->setGates(RFGATEOFF(observeCh->get_RF_PRE_GATES() | observeCh->get_RF_PULSE_GATES()));
  observeCh->setGates(RFGATEON(RF_TR_GATE));

  if (ddrElapsTicks != acqtimeTicks)
     abort_message("swift_acquire DDR tick count does not match the aqtm ticks. abort!\n");

  observeCh->setActive();

  P2TheConsole->update4Sync(acqtimeTicks);

  ddrC->reset_acq();
  ddrC->acq_completed = 1;
}
//
//
//
//  gensim3_pulse/11
//  three centered square RF pulses
//
//  receiver gate on shorter just follows larger...
//
void gensim3_pulse(double width1, double width2, double width3,
            int phase1, int phase2, int phase3, double rx1, double rx2,
            int rfdevice1,int rfdevice2, int rfdevice3)
{
   long long aw1,aw2,aw3, awmax, totalTicks;
   long long extra1, extra2, extra3;
   long long rx1T,   rx2T,   rx3T;
   long long rx1bT,  rx2bT,  rx3bT;

   aw1=0;    aw2=0;    aw3=0;
   rx1T=0;   rx2T=0;   rx3T=0;
   rx1bT=0;  rx2bT=0;  rx3bT=0;
   extra1=0; extra2=0; extra3=0;

   RFController *S1=NULL, *S2=NULL, *S3=NULL;

   if (width1 > 0.0)
   {
     S1 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice1);
     if (S1->getProgDecInterlock() > 0)
     {
         if (bgflag)
         {
            fprintf(stdout,"advisory: psg cmd sim3pulse not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
         }
         return;
     }
   }
   if (width2 > 0.0)
   {
     S2 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice2);
     if (S2->getProgDecInterlock() > 0)
     {
         if (bgflag)
         {
            fprintf(stdout,"advisory: psg cmd sim3pulse not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
         }
         return;
     }
   }
   if (width3 > 0.0)
   {
     S3 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice3);
     if (S3->getProgDecInterlock() > 0)
     {
         if (bgflag)
         {
            fprintf(stdout,"advisory: psg cmd sim3pulse not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
         }
         return;
     }
   }

   P2TheConsole->newEvent();

   if (width1 > 0.0)
   {
     S1 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice1);
     S1->setActive();
     S1->clearSmallTicker();
     aw1   = S1->calcTicks(width1);
     if (aw1 < 4LL) aw1 = 0LL;
     rx1T  = S1->calcTicks(rx1);
     rx1bT = S1->calcTicks(rx2);
   }

   if (width2 > 0.0)
   {
     S2 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice2);
     S2->setActive();
     S2->clearSmallTicker();
     aw2   = S2->calcTicks(width2);
     if (aw2 < 4LL) aw2 = 0LL;
     rx2T  = S2->calcTicks(rx1);
     rx2bT = S2->calcTicks(rx2);
   }

   if (width3 > 0.0)
   {
     S3 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice3);
     S3->setActive();
     S3->clearSmallTicker();
     aw3   = S3->calcTicks(width3);
     if (aw3 < 4LL) aw3 = 0LL;
     rx3T  = S3->calcTicks(rx1);
     rx3bT = S3->calcTicks(rx2);
   }

   // aw1, aw2, aw3 are the full widths in ticks...

   awmax = 0LL;
   if (aw1 > aw2)
     awmax = aw1;
   else
     awmax = aw2;
   if (aw3 > awmax)
     awmax = aw3;

   rx1T += (awmax - aw1)/2;
   if ((rx1T > 0LL) && (rx1T < 4LL))
   {
     rx1T  += 4LL;
     rx2T  += 4LL;
     rx3T  += 4LL;
   }

   rx2T += (awmax - aw2)/2;
   if ((rx2T > 0LL) && (rx2T < 4LL))
   {
     rx1T  += 4LL;
     rx2T  += 4LL;
     rx3T  += 4LL;
   }

   rx3T += (awmax - aw3)/2;
   if ((rx3T > 0LL) && (rx3T < 4LL))
   {
     rx1T  += 4LL;
     rx2T  += 4LL;
     rx3T  += 4LL;
   }

   rx1bT += (awmax - aw1)/2+(awmax -aw1) %2;
   if ((rx1bT > 0LL) && (rx1bT < 4LL))
   {
     rx1bT += 4LL;
     rx2bT += 4LL;
     rx3bT += 4LL;
   }

   rx2bT += (awmax - aw2)/2+(awmax -aw2) %2;
   if ((rx2bT > 0LL) && (rx2bT < 4LL))
   {
     rx1bT += 4LL;
     rx2bT += 4LL;
     rx3bT += 4LL;
   }

   rx3bT += (awmax - aw3)/2+(awmax -aw3) %2;
   if ((rx3bT > 0LL) && (rx3bT < 4LL))
   {
     rx1bT += 4LL;
     rx2bT += 4LL;
     rx3bT += 4LL;
   }


   if (aw1 >= 4LL)
   {
     S1->pulsePreAmble(rx1T, phase1);
     S1->setTickDelay(aw1);
     S1->pulsePostAmble(rx1bT);
     extra1 = S1->getSmallTicker();
     S1->setChannelActive(1);
   }

   if (aw2 >= 4LL)
   {
     S2->pulsePreAmble(rx2T, phase2);
     S2->setTickDelay(aw2);
     S2->pulsePostAmble(rx2bT);
     extra2 = S2->getSmallTicker();
     S2->setChannelActive(1);
   }

   if (aw3 >= 4LL)
   {
     S3->pulsePreAmble(rx3T, phase3);
     S3->setTickDelay(aw3);
     S3->pulsePostAmble(rx3bT);
     extra3 = S3->getSmallTicker();
     S3->setChannelActive(1);
   }


   // check for timing tick match between active controllers

   totalTicks=0;

   if (aw1 > 0)
     totalTicks = extra1;

   if (aw2 > 0)
   {
     if (totalTicks != 0)
     {
       if (totalTicks != extra2)
       {
         abort_message("sim3pulse: timing mismatch error. abort!\n");
       }
     }
     else
       totalTicks = extra2;
   }

   if (aw3 > 0)
   {
     if (totalTicks != 0)
     {
       if (totalTicks != extra3)
       {
         text_message("sim3pulse: timing mismatch error. abort!\n");
         exit(-1);
       }
     }
     else
       totalTicks = extra3;
   }
   // notify all other channels of duration ....

   P2TheConsole->update4Sync(totalTicks);
}

////////////////////
//
//  gensim4_pulse/14
//  four centered square RF pulses
//
//  receiver gate on shorter just follows larger...
//
void gensim4_pulse(double width1, double width2, double width3, double width4,
            int phase1, int phase2, int phase3, int phase4, double rx1, double rx2,
            int rfdevice1,int rfdevice2, int rfdevice3,int rfdevice4)
{
   long long aw1,aw2,aw3,aw4, awmax, totalTicks;
   long long extra1, extra2, extra3, extra4;
   long long rx1T, rx2T, rx3T, rx4T;
   long long rx1bT, rx2bT, rx3bT, rx4bT;

   aw1=0;    aw2=0;    aw3=0;    aw4=0;
   rx1T=0;   rx2T=0;   rx3T=0;   rx4T=0;
   rx1bT=0;  rx2bT=0;  rx3bT=0;  rx4bT=0;
   extra1=0; extra2=0; extra3=0; extra4=0;

   RFController *S1=NULL, *S2=NULL, *S3=NULL, *S4=NULL;
   if (width1 > 0.0)
   {
     S1 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice1);
     if (S1->getProgDecInterlock() > 0)
     {
         if (bgflag)
         {
            fprintf(stdout,"advisory: psg cmd sim4pulse not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
         }
         return;
     }
   }
   if (width2 > 0.0)
   {
     S2 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice2);
     if (S2->getProgDecInterlock() > 0)
       {
         if (bgflag)
         {
            fprintf(stdout,"advisory: psg cmd sim4pulse not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
         }
         return;
     }
   }
   if (width3 > 0.0)
   {
     S3 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice3);
     if (S3->getProgDecInterlock() > 0)
     {
         if (bgflag)
         {
            fprintf(stdout,"advisory: psg cmd sim4pulse not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
         }
         return;
     }
   }
   if (width4 > 0.0)
   {
     S4 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice4);
     if (S4->getProgDecInterlock() > 0)
     {
         if (bgflag)
         {
            fprintf(stdout,"advisory: psg cmd sim4pulse not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
         }
         return;
     }
   }

   P2TheConsole->newEvent();

   if (width1 >= LATCHDELAY)
   {
     S1 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice1);
     S1->setActive();
     S1->clearSmallTicker();
     aw1   = S1->calcTicks(width1);
     if (aw1 < 4LL) aw1 = 0LL;
     rx1T  = S1->calcTicks(rx1);
     rx1bT = S1->calcTicks(rx2);
   }

   if (width2 >= LATCHDELAY)
   {
     S2 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice2);
     S2->setActive();
     S2->clearSmallTicker();
     aw2   = S2->calcTicks(width2);
     if (aw2 < 4LL) aw2 = 0LL;
     rx2T  = S2->calcTicks(rx1);
     rx2bT = S2->calcTicks(rx2);
   }

   if (width3 >= LATCHDELAY)
   {
     S3 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice3);
     S3->setActive();
     S3->clearSmallTicker();
     aw3   = S3->calcTicks(width3);
     if (aw3 < 4LL) aw3 = 0LL;
     rx3T  = S3->calcTicks(rx1);
     rx3bT = S3->calcTicks(rx2);
   }

   if (width4 >= LATCHDELAY)
   {
     S4 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice4);
     S4->setActive();
     S4->clearSmallTicker();
     aw4   = S4->calcTicks(width4);
     if (aw4 < 4LL) aw4 = 0LL;
     rx4T  = S4->calcTicks(rx1);
     rx4bT = S4->calcTicks(rx2);
   }

   // aw1, aw2, aw3 and aw4 are the full widths in ticks...

   awmax = 0LL;
   if (aw1 > aw2)
     awmax = aw1;
   else
     awmax = aw2;
   if (aw3 > awmax)
     awmax = aw3;
   if (aw4 > awmax)
     awmax = aw4;

   rx1T += (awmax - aw1)/2;
   if ((rx1T > 0LL) && (rx1T < 4LL))
   {
     rx1T  += 4LL;
     rx2T  += 4LL;
     rx3T  += 4LL;
     rx4T  += 4LL;
   }

   rx2T += (awmax - aw2)/2;
   if ((rx2T > 0LL) && (rx2T < 4LL))
   {
     rx1T  += 4LL;
     rx2T  += 4LL;
     rx3T  += 4LL;
     rx4T  += 4LL;
   }

   rx3T += (awmax - aw3)/2;
   if ((rx3T > 0LL) && (rx3T < 4LL))
   {
     rx1T  += 4LL;
     rx2T  += 4LL;
     rx3T  += 4LL;
     rx4T  += 4LL;
   }

   rx4T += (awmax - aw4)/2;
   if ((rx4T > 0LL) && (rx4T < 4LL))
   {
     rx1T  += 4LL;
     rx2T  += 4LL;
     rx3T  += 4LL;
     rx4T  += 4LL;
   }

   rx1bT += (awmax - aw1)/2+(awmax -aw1) %2;
   if ((rx1bT > 0LL) && (rx1bT < 4LL))
   {
     rx1bT += 4LL;
     rx2bT += 4LL;
     rx3bT += 4LL;
     rx4bT += 4LL;
   }

   rx2bT += (awmax - aw2)/2+(awmax -aw2) %2;
   if ((rx2bT > 0LL) && (rx2bT < 4LL))
   {
     rx1bT += 4LL;
     rx2bT += 4LL;
     rx3bT += 4LL;
     rx4bT += 4LL;
   }

   rx3bT += (awmax - aw3)/2+(awmax -aw3) %2;
   if ((rx3bT > 0LL) && (rx3bT < 4LL))
   {
     rx1bT += 4LL;
     rx2bT += 4LL;
     rx3bT += 4LL;
     rx4bT += 4LL;
   }

   rx4bT += (awmax - aw4)/2+(awmax -aw4) %2;
   if ((rx4bT > 0LL) && (rx4bT < 4LL))
   {
     rx1bT += 4LL;
     rx2bT += 4LL;
     rx3bT += 4LL;
     rx4bT += 4LL;
   }

   if (aw1 >= 4LL)
   {
     S1->pulsePreAmble(rx1T, phase1);
     S1->setTickDelay(aw1);
     S1->pulsePostAmble(rx1bT);
     extra1 = S1->getSmallTicker();
     S1->setChannelActive(1);
   }

   if (aw2 >= 4LL)
   {
     S2->pulsePreAmble(rx2T, phase2);
     S2->setTickDelay(aw2);
     S2->pulsePostAmble(rx2bT);
     extra2 = S2->getSmallTicker();
     S2->setChannelActive(1);
   }

   if (aw3 >= 4LL)
   {
     S3->pulsePreAmble(rx3T, phase3);
     S3->setTickDelay(aw3);
     S3->pulsePostAmble(rx3bT);
     extra3 = S3->getSmallTicker();
     S3->setChannelActive(1);
   }

   if (aw4 >= 4LL)
   {
     S4->pulsePreAmble(rx4T, phase4);
     S4->setTickDelay(aw4);
     S4->pulsePostAmble(rx4bT);
     extra4 = S4->getSmallTicker();
     S4->setChannelActive(1);
   }

   // check for timing tick match between active controllers

   totalTicks=0;

   if (aw1 > 0)
     totalTicks = extra1;

   if (aw2 > 0)
   {
     if (totalTicks != 0)
     {
       if (totalTicks != extra2)
         abort_message("sim4pulse timing mismatch. abort!\n");
     }
     else
       totalTicks = extra2;
   }

   if (aw3 > 0)
   {
     if (totalTicks != 0)
     {
       if (totalTicks != extra3)
         abort_message("sim4pulse timing mismatch. abort!\n");
     }
     else
       totalTicks = extra3;
   }

   if (aw4 > 0)
   {
     if (totalTicks != 0)
     {
       if (totalTicks != extra4)
         abort_message("sim4pulse timing mismatch. abort!\n");
     }
     else
       totalTicks = extra4;
   }


   // notify all other channels of duration ....

   P2TheConsole->update4Sync(totalTicks);
}


////////////////////

//
/*-----------------------------------------------
|                                               |
|               genshaped_rtamppulse()/9       |
|                                               |
+----------------------------------------------*/
void genRFshaped_rtamppulse(char *nm, double pw, int rtTpwrf, int rtphase, double rx1, double rx2, double g1, double g2, int rfdevice)
{
   cPatternEntry *tmp;
   long long extra;
   double value;
   int divs,duration;
   if (pw <= 0.0)
     return;
   value = pw;

   RFController *ShapedChan = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice);
   if (ShapedChan->getProgDecInterlock() > 0)
   {
       if (bgflag)
       {
          fprintf(stdout,"advisory: psg cmd shapedrtamppulse not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
       }
       return;
   }

   P2TheConsole->newEvent();
   // -- test only ...
   tmp = ShapedChan->resolveRFPattern(nm,1,"genshaped_rfampulse",1);
   ShapedChan->setPowerFraction(1.0);  // estimate at full power
   // this element allows the ShapedChan to time independently...
   ShapedChan->setActive();
   // track the duration using the small ticker...
   ShapedChan->clearSmallTicker();
   int minTicks = (int)(P2TheConsole->getMinTimeEventTicks());
   divs = tmp->getNumberStates();
   duration = ShapedChan->calcTicksPerState(value,divs,minTicks,0.1,"gen Shaped RT Amp Pulse",0);
   if (duration < 0)
     abort_message("pattern time cannot be < 0\n");  // until we sort out ....
   if (duration > 0x3ffffffL)
     {
       abort_message("WARNING: SINGLE STATE DURATION EXCEEDES LIMIT\n");
     }
   ShapedChan->setVAmpScale(rtTpwrf,1);  // latched with rx1 delay...
   ShapedChan->pulsePreAmble(rx1+g1, rtphase);   // turns ON the xmtr!

   ShapedChan->add2Stream(DURATIONKEY | duration);  // unlatched...wait for first element
   ShapedChan->noteDelay(duration*divs);
   ShapedChan->executePattern(tmp->getReferenceID());
   // the pattern has finished xgate off wait rof2.. blank
   ShapedChan->pulsePostAmble(rx2+g2);
   ShapedChan->setChannelActive(1);
   extra = ShapedChan->getSmallTicker();
   // notify all other channels of duration ....
   P2TheConsole->update4Sync(extra);
}


//
// simultaneous shaped pulses on 2 rf channels
//
void gensim2shaped_pulse(char *name1, char *name2, double width1, double width2,\
                 int phase1, int phase2, double rx1, double rx2, double g1, double g2, \
                 int rfdevice1, int rfdevice2)
{
   cPatternEntry *pat1=NULL,*pat2=NULL;
   long long halfpwTicks, halfPadTicks, syncTicks, rof1Ticks, rof2Ticks;
   long long rf1pwTicks=0, rf2pwTicks=0, maxpwTicks=0;
   long long rf1TicksPerState, rf2TicksPerState;
   int rf1Active, rf2Active, div1, div2;
   char shp1[256], shp2[256];

   // get both rf controllers
   RFController *S1 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice1);
   RFController *S2 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice2);

   if ( (S1 == NULL) || (S2 == NULL) )
   {
     text_message("simshaped_pulse: unable to assign rf controllers. abort!\n");
     exit(-1);
   }

   if (width1 > 0.0)
   {
     S1 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice1);
     if (S1->getProgDecInterlock() > 0)
     {
         if (bgflag)
         {
            fprintf(stdout,"advisory: psg cmd simshapedpulse not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
         }
         return;
     }
   }
   if (width2 > 0.0)
   {
     S2 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice2);
     if (S2->getProgDecInterlock() > 0)
     {
         if (bgflag)
         {
            fprintf(stdout,"advisory: psg cmd simshapedpulse not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
         }
         return;
     }
   }

   // if shape name blank, use hard.RF
   if (strcmp(name1,""))
     OSTRCPY( shp1, sizeof(shp1), name1);
   else
     OSTRCPY( shp1, sizeof(shp1), "hard");
   if (strcmp(name2,""))
     OSTRCPY( shp2, sizeof(shp2), name2);
   else
     OSTRCPY( shp2, sizeof(shp2), "hard");

   // sort out the active rf channels
   // rf1Active = 1 means rf1 participates in event

   rf1pwTicks = S1->calcTicks(width1);
   rf1Active = 0;
   if (rf1pwTicks >= LATCHDELAYTICKS) rf1Active = 1;

   rf2pwTicks = S2->calcTicks(width2);
   rf2Active = 0;
   if (rf2pwTicks >= LATCHDELAYTICKS) rf2Active = 1;

   rf1TicksPerState=0; rf2TicksPerState=0;
   div1 = div2 = 0;

   if (rf1Active)
   {
     pat1 = S1->resolveRFPattern(shp1,1,"simshaped_pulse pattern 1 not found",1);
     S1->setPowerFraction(pat1->getPowerFraction());
     int minTicks = (int)(P2TheConsole->getMinTimeEventTicks());
     div1 = pat1->getNumberStates();
     rf1TicksPerState = S1->calcTicksPerState(width1,div1,minTicks,0.1,"simshaped_pulse arg1",0);
     if (rf1TicksPerState <= 0)
     {
        text_error("simshaped_pulse: shape 1 duration too short for pattern!");
        exit(-1);
     }
     else if (rf1TicksPerState > 0x3ffffffL)
     {
        text_error("simshaped_pulse: shape 1 duration per state exceeds limit");
        exit(-1);
     }
   }

   if (rf2Active)
   {
     pat2 = S2->resolveRFPattern(shp2,1,"simshaped_pulse pattern 2 not found",1);
     S2->setPowerFraction(pat2->getPowerFraction());
     int minTicks = (int)(P2TheConsole->getMinTimeEventTicks());
     div2 = pat2->getNumberStates();
     rf2TicksPerState = S2->calcTicksPerState(width2,div2,minTicks,0.1,"simshaped_pulse arg2",0);
     if (rf2TicksPerState <= 0)
     {
        text_error("simshaped_pulse: shape 2 duration too short for pattern!");
        exit(-1);
     }
     else if (rf2TicksPerState > 0x3ffffffL)
     {
        text_error("simshaped_pulse: shape 2 duration per state exceeds limit");
        exit(-1);
     }
   }

   // total ticks during the shaped pulse, excluding rof1 and rof2
   rf1pwTicks = rf1TicksPerState * (long long)div1;
   rf2pwTicks = rf2TicksPerState * (long long)div2;
   // Round to even number of ticks to avoid tick mismatches
   rf1pwTicks = (rf1pwTicks/2) * 2;
   rf2pwTicks = (rf2pwTicks/2) * 2;


   // find the maximum shape pulse width
   maxpwTicks = rf1pwTicks;
   if (rf2pwTicks > maxpwTicks) maxpwTicks = rf2pwTicks;
   halfpwTicks = maxpwTicks/2 ;

   // there may be truncation in halfpwTicks due to integer divide.
   // this is compensated using the modulo (halfPadTicks - (rf1pwTicks % 2))

   rof1Ticks = S1->calcTicks(rx1);
   rof2Ticks = S1->calcTicks(rx2);
   syncTicks = 0;

   // start the new rf event..
   P2TheConsole->newEvent();

   // for all or any of the three active rf controllers generate code
   // at end sets waveform amplitude (defAmp) back to default value (4095)
   // this could make centering of shaped pulses have an asymmetry of 12.5 ns

   if ( rf1Active )
   {
     S1->setActive();
     S1->clearSmallTicker();
     halfPadTicks = halfpwTicks - (rf1pwTicks/2);
     S1->setTickDelay(halfPadTicks);
     S1->pulsePreAmble(rof1Ticks, phase1);

     S1->add2Stream(DURATIONKEY | rf1TicksPerState);
     S1->executePattern(pat1->getReferenceID());
     S1->noteDelay(rf1pwTicks);

     S1->pulsePostAmble(rof2Ticks);
     S1->setAmp(S1->getDefAmp());
     S1->setTickDelay(halfPadTicks - (rf1pwTicks % 2));
     S1->setPowerFraction(1.0);
     S1->setChannelActive(1);
     syncTicks = S1->getSmallTicker();
   }

   if ( rf2Active )
   {
     S2->setActive();
     S2->clearSmallTicker();
     halfPadTicks = halfpwTicks - (rf2pwTicks/2);
     S2->setTickDelay(halfPadTicks);
     S2->pulsePreAmble(rof1Ticks, phase2);

     S2->add2Stream(DURATIONKEY | rf2TicksPerState);
     S2->executePattern(pat2->getReferenceID());
     S2->noteDelay(rf2pwTicks);

     S2->pulsePostAmble(rof2Ticks);
     S2->setAmp(S2->getDefAmp());
     S2->setTickDelay(halfPadTicks - (rf2pwTicks % 2));
     S2->setPowerFraction(1.0);
     S2->setChannelActive(1);
     syncTicks = S2->getSmallTicker();
   }


   // check rf channels for synchronization

   if (rf1Active && rf2Active)
   {
     if (S1->getSmallTicker() != S2->getSmallTicker())
     {
        text_message("sim3shaped_pulse: timing mismatch error. abort!\n");
        exit(-1);
     }
     else
        syncTicks = S1->getSmallTicker();
   }

   if (rf1Active)
     syncTicks = S1->getSmallTicker();

   if (rf2Active)
     syncTicks = S2->getSmallTicker();

   // sync all other channels

   P2TheConsole->update4Sync(syncTicks);

   // all events complete and syncronized
}

//
// simultaneous shaped pulses on 3 rf channels
//
void gensim3shaped_pulse(char *name1, char *name2, char *name3, double width1, double width2,\
                 double width3, int phase1, int phase2, int phase3, double rx1, double rx2, double g1, double g2, \
                 int rfdevice1, int rfdevice2, int rfdevice3)
{
   cPatternEntry *pat1=NULL,*pat2=NULL, *pat3=NULL;
   long long halfpwTicks, halfPadTicks, syncTicks, rof1Ticks, rof2Ticks;
   long long rf1pwTicks=0, rf2pwTicks=0, rf3pwTicks=0, maxpwTicks=0;
   long long rf1TicksPerState, rf2TicksPerState, rf3TicksPerState;
   int rf1Active, rf2Active, rf3Active, div1, div2, div3;
   char shp1[256], shp2[256], shp3[256];

   // get all three rf controllers
   RFController *S1 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice1);
   RFController *S2 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice2);
   RFController *S3 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice3);

   if ( (S1 == NULL) || (S2 == NULL) || (S3 == NULL) )
   {
     text_message("sim3shaped_pulse: unable to assign rf controllers. abort!\n");
     exit(-1);
   }

   if (width1 > 0.0)
   {
     S1 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice1);
     if (S1->getProgDecInterlock() > 0)
     {
         if (bgflag)
         {
            fprintf(stdout,"advisory: psg cmd sim3shaped_pulse not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
         }
         return;
     }
   }
   if (width2 > 0.0)
   {
     S2 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice2);
     if (S2->getProgDecInterlock() > 0)
     {
         if (bgflag)
         {
            fprintf(stdout,"advisory: psg cmd sim3shaped_pulse not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
         }
         return;
     }
   }
   if (width3 > 0.0)
   {
     S3 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice3);
     if (S3->getProgDecInterlock() > 0)
     {
         if (bgflag)
         {
            fprintf(stdout,"advisory: psg cmd sim3shaped_pulse not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
         }
         return;
     }
   }

   // if shape name blank, use hard.RF
   if (strcmp(name1,""))
     OSTRCPY( shp1, sizeof(shp1), name1);
   else
     OSTRCPY( shp1, sizeof(shp1), "hard");
   if (strcmp(name2,""))
     OSTRCPY( shp2, sizeof(shp2), name2);
   else
     OSTRCPY( shp2, sizeof(shp2), "hard");
   if (strcmp(name3,""))
     OSTRCPY( shp3, sizeof(shp3), name3);
   else
     OSTRCPY( shp3, sizeof(shp3), "hard");


   // sort out the active rf channels
   // rf1Active = 1 means rf1 participates in event

   rf1pwTicks = S1->calcTicks(width1);
   rf1Active = 0;
   if (rf1pwTicks >= LATCHDELAYTICKS) rf1Active = 1;

   rf2pwTicks = S2->calcTicks(width2);
   rf2Active = 0;
   if (rf2pwTicks >= LATCHDELAYTICKS) rf2Active = 1;

   rf3pwTicks = S3->calcTicks(width3);
   rf3Active = 0;
   if (rf3pwTicks >= LATCHDELAYTICKS) rf3Active = 1;

   rf1TicksPerState=0; rf2TicksPerState=0; rf3TicksPerState=0;
   div1 = div2 = div3 = 0;

   if (rf1Active)
   {
     pat1 = S1->resolveRFPattern(shp1,1,"sim3shaped_pulse pattern 1 not found",1);
     div1 = pat1->getNumberStates();
     S1->setPowerFraction(pat1->getPowerFraction());
     int minTicks = (int)(P2TheConsole->getMinTimeEventTicks());
     rf1TicksPerState = S1->calcTicksPerState(width1,div1,minTicks,0.1,"sim3shaped_pulse arg1",0);
     if (rf1TicksPerState <= 0)
     {
        text_error("sim3shaped_pulse: shape 1 duration too short for pattern!");
        exit(-1);
     }
     else if (rf1TicksPerState > 0x3ffffffL)
     {
        text_error("sim3shaped_pulse: shape 1 duration per state exceeds limit");
        exit(-1);
     }
   }

   if (rf2Active)
   {
     pat2 = S2->resolveRFPattern(shp2,1,"sim3shaped_pulse pattern 2 not found",1);
     div2 = pat2->getNumberStates();
     S2->setPowerFraction(pat2->getPowerFraction());
     int minTicks = (int)(P2TheConsole->getMinTimeEventTicks());
     rf2TicksPerState = S2->calcTicksPerState(width2,div2,minTicks,0.1,"sim3shaped_pulse arg2",0);
     if (rf2TicksPerState <= 0)
     {
        text_error("sim3shaped_pulse: shape 2 duration too short for pattern!");
        exit(-1);
     }
     else if (rf2TicksPerState > 0x3ffffffL)
     {
        text_error("sim3shaped_pulse: shape 2 duration per state exceeds limit");
        exit(-1);
     }
   }

   if (rf3Active)
   {
     pat3 = S3->resolveRFPattern(shp3,1,"sim3shaped_pulse pattern 3 not found",1);
     div3 = pat3->getNumberStates();
     S3->setPowerFraction(pat3->getPowerFraction());
     int minTicks = (int)(P2TheConsole->getMinTimeEventTicks());
     rf3TicksPerState = S3->calcTicksPerState(width3,div3,minTicks,0.1,"sim3shaped_pulse arg3",0);
     if (rf3TicksPerState <= 0)
     {
        text_error("sim3shaped_pulse: shape 3 duration too short for pattern!");
        exit(-1);
     }
     else if (rf3TicksPerState > 0x3ffffffL)
     {
        text_error("sim3shaped_pulse: shape 3 duration per state exceeds limit");
        exit(-1);
     }
   }

   // total ticks during the shaped pulse, excluding rof1 and rof2
   rf1pwTicks = rf1TicksPerState * (long long)div1;
   rf2pwTicks = rf2TicksPerState * (long long)div2;
   rf3pwTicks = rf3TicksPerState * (long long)div3;
   // Round to even number of ticks to avoid tick mismatches
   rf1pwTicks = (rf1pwTicks/2) * 2;
   rf2pwTicks = (rf2pwTicks/2) * 2;
   rf3pwTicks = (rf3pwTicks/2) * 2;


   // find the maximum shape pulse width
   maxpwTicks = rf1pwTicks;
   if (rf2pwTicks > maxpwTicks) maxpwTicks = rf2pwTicks;
   if (rf3pwTicks > maxpwTicks) maxpwTicks = rf3pwTicks;
   halfpwTicks = maxpwTicks/2 ;

   // there may be truncation in halfpwTicks due to integer divide.
   // this is compensated using the modulo (halfPadTicks - (rf1pwTicks % 2))

   rof1Ticks = S1->calcTicks(rx1);
   rof2Ticks = S1->calcTicks(rx2);
   syncTicks = 0;

   // start the new rf event..
   P2TheConsole->newEvent();

   // for all or any of the three active rf controllers generate code
   // at end sets waveform amplitude (defAmp) back to default value (4095)
   // this could make centering of shaped pulses have an asymmetry of 12.5 ns

   if ( rf1Active )
   {
     S1->setActive();
     S1->clearSmallTicker();
     halfPadTicks = halfpwTicks - (rf1pwTicks/2);
     S1->setTickDelay(halfPadTicks);
     S1->pulsePreAmble(rof1Ticks, phase1);

     S1->add2Stream(DURATIONKEY | rf1TicksPerState);
     S1->executePattern(pat1->getReferenceID());
     S1->noteDelay(rf1pwTicks);

     S1->pulsePostAmble(rof2Ticks);
     S1->setAmp(S1->getDefAmp());
     S1->setChannelActive(1);
     S1->setTickDelay(halfPadTicks - (rf1pwTicks % 2));
     syncTicks = S1->getSmallTicker();
   }

   if ( rf2Active )
   {
     S2->setActive();
     S2->clearSmallTicker();
     halfPadTicks = halfpwTicks - (rf2pwTicks/2);
     S2->setTickDelay(halfPadTicks);
     S2->pulsePreAmble(rof1Ticks, phase2);

     S2->add2Stream(DURATIONKEY | rf2TicksPerState);
     S2->executePattern(pat2->getReferenceID());
     S2->noteDelay(rf2pwTicks);

     S2->pulsePostAmble(rof2Ticks);
     S2->setAmp(S2->getDefAmp());
     S2->setChannelActive(1);
     S2->setTickDelay(halfPadTicks - (rf2pwTicks % 2));
     syncTicks = S2->getSmallTicker();
   }

   if ( rf3Active )
   {
     S3->setActive();
     S3->clearSmallTicker();
     halfPadTicks = halfpwTicks - (rf3pwTicks/2);
     S3->setTickDelay(halfPadTicks);
     S3->pulsePreAmble(rof1Ticks, phase3);

     S3->add2Stream(DURATIONKEY | rf3TicksPerState);
     S3->executePattern(pat3->getReferenceID());
     S3->noteDelay(rf3pwTicks);

     S3->pulsePostAmble(rof2Ticks);
     S3->setAmp(S3->getDefAmp());
     S3->setChannelActive(1);
     S3->setTickDelay(halfPadTicks - (rf3pwTicks % 2));
     syncTicks = S3->getSmallTicker();
   }


   // check rf channels for synchronization

   if (rf1Active && rf2Active)
   {
     if (S1->getSmallTicker() != S2->getSmallTicker())
     {
        text_message("sim3shaped_pulse: timing mismatch error. abort!\n");
        exit(-1);
     }
     else
        syncTicks = S1->getSmallTicker();
   }

   if (rf1Active && rf3Active)
   {
     if (S1->getSmallTicker() != S3->getSmallTicker())
     {
        text_message("sim3shaped_pulse: timing mismatch error. abort!\n");
        exit(-1);
     }
     else
        syncTicks = S1->getSmallTicker();
   }


   if (rf2Active && rf3Active)
   {
     if (S2->getSmallTicker() != S3->getSmallTicker())
     {
        text_message("sim3shaped_pulse: timing mismatch error. abort!\n");
        exit(-1);
     }
     else
        syncTicks = S2->getSmallTicker();
   }


   // reset power levels
   S1->setPowerFraction(1.0);  
   S2->setPowerFraction(1.0);
   S3->setPowerFraction(1.0);
   // sync all other channels

   P2TheConsole->update4Sync(syncTicks);

   // all events complete and syncronized
}


//
void genspinlock(char *name, double pw_90, double deg_res, int phsval, int ncycles, int rfdevice)
{
   cPatternEntry *pat1;
   long long ticksPerState;
   double duration;
   int states;

   RFController *S1 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice);
   if (S1->getProgDecInterlock() > 0)
   {
       if (bgflag)
       {
          fprintf(stdout,"advisory: psg cmd spinlock not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
       }
       return;
   }

   P2TheConsole->newEvent();

   // DecPattern puts states out with deg_res as a unit duration...
   pat1 = S1->resolveDecPattern(name,(int)deg_res,pw_90,0,1,"genspinlock pattern not found");
   S1->setPowerFraction(pat1->getPowerFraction());

   S1->setActive();
   states = pat1->getNumberStates();
   //  duration of each state is pw_90*deg_res/90
   duration      =  (pw_90*((double) deg_res)/90.0);
   ticksPerState = S1->calcTicks(duration);

   S1->setGates(RFGATEON(S1->get_RF_PRE_GATES()));
   S1->setVQuadPhase(phsval);

   S1->setGates(RFGATEON(S1->get_RF_PULSE_GATES()));

   if (ticksPerState > 0x3ffffffL)
   {
     text_message("single state duration exceeds limit in spinlock");
     exit(-112);
   }
   S1->add2Stream(DURATIONKEY | (int) ticksPerState);
   S1->noteDelay(ticksPerState*states*ncycles);

   // invokes multipattern acode...
   S1->executePattern(pat1->getReferenceID(),ncycles,0,0);

   S1->setGates(RFGATEOFF(S1->get_RF_PULSE_GATES()));

   S1->setGates(RFGATEOFF(S1->get_RF_POST_GATES()));

   if (S1->getBlank_Is_On())
     S1->setGates(RFGATEOFF(RFAMP_UNBLANK));

   S1->setPowerFraction(1.0);
   P2TheConsole->update4Sync(ticksPerState*states*ncycles);
}


/*-----------------------------------------------
|						|
|		 prg_dec_on()/4			|
|						|
|   name	-  name of pattern		|
|   timeper90	-  time for a 90 degree flip	|
|   deg_res	-  degree resolution (min. 0.7)	|
|   rfdevice	-  RF device			|
|						|
+----------------------------------------------*/
void prg_dec_on(char *name, double pw_90, double deg_res, int rfdevice)
{

   RFController *S1 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice);
   S1->progDecOn(name,pw_90,deg_res,1,"prg dec on");
}

void prg_dec_on_offset(char *name, double pw_90, double deg_res, double foff, int rfdevice)
{

    RFController *S1 = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfdevice);
	S1->progDecOnWithOffset(name,pw_90,deg_res,foff,1,"prg dec on");
}

void prg_dec_off(int rfdevice)
{
  RFController *S1= (RFController *)  P2TheConsole->getRFControllerByLogicalIndex(rfdevice);
  S1->progDecOff(DECSTOPEND, "prg dec off ", 1, 0);
}


//
// setstatus is identical to obs/decprgon/off and does rf gating in addition
//
void setstatus(int channel, int state, char mode, int sync, double modfrq)
{
  char modName[2];

  // mode (synchronus/asynchronus) is not implemented currently

  if (state)
  {
    modName[0]=mode; modName[1]='\0';
    prg_dec_on(modName, (1.0/modfrq), 90.0, channel);
    genTxGateOnOff( 1, channel);
  }
  else
  {
    prg_dec_off(channel);
    genTxGateOnOff( 0, channel);
  }
}



//
// displays power integral values on all controllers
//
void showpowerintegral()
{
  int numcntrlrs = P2TheConsole->nValid;
  printf("Power integral info:\n");
  for (int i=0; i < numcntrlrs; i++)
  {
    Controller *contr = P2TheConsole->PhysicalTable[i];
    contr->showPowerIntegral();
  }
}



void  resolve_initscan_actions()
{
  RFController *rfC = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(OBSch);
  startScanTicker = rfC->getBigTicker();

  int numcntrlrs = P2TheConsole->nValid;
  for (int i=0; i < numcntrlrs; i++)
  {
    Controller *contr = P2TheConsole->PhysicalTable[i];
    contr->PowerActionsAtStartOfScan();
  }
}


/* all end of scan actions go into this method */

void resolve_endofscan_actions()
{
  /* resolve status decoupling for RF controllers */

  RFController *rfC;
  int numRFChan = P2TheConsole->numRFChan;
  for(int i=0;i<numRFChan;i++)
  {
    rfC = (RFController *)(P2TheConsole->RFUserTable[i]);
    if (rfC->isAsync())
       rfC->resolveEndOfScanStatusAction(statusindx,0);
  }
  if (grad_flag) zero_all_gradients();  // pfg, homospoil, dacs

  rfC = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(OBSch);
  long long totalScanTime = rfC->getBigTicker()-startScanTicker;

  (void) totalScanTime;
  int numcntrlrs = P2TheConsole->nValid;
  for (int i=0; i < numcntrlrs; i++)
  {
    Controller *contr = P2TheConsole->PhysicalTable[i];
    contr->computePowerAtEndOfScan();
  }
  //showpowerintegral();

}


/* all end of expt actions go into this method */

void resolve_endofexpt_actions()
{
  /* turn off the Tx gate on all RF Controllers */

  RFController *rfC;
  int numRFChan = P2TheConsole->numRFChan;
  for(int i=0;i<numRFChan;i++)
  {
    rfC = (RFController *)(P2TheConsole->RFUserTable[i]);
    rfC->pulsePostAmble(0.0);
    rfC->writeDefaultGates();
  }
  delay(4.0e-6);     // 4usec will work on all systems, including GradientControllers

  int gradenergyaction = 0;
  gradenergyaction = (int)(getvalnwarn("gradenergyaction")+0.49);
  if ( (gradenergyaction & 0x1) ) P2TheConsole->showPowerIntegral();
  //showpowerintegral();
}


// first state was 0.0
static double mgrad[21]= {0.07,0.24,0.42,0.68,0.94,1.0,
                             1.0,1.0,1.0,1.0,1.0,1.0,1.0,
                             1.0,0.94,0.68,0.42,0.24,0.07,0.0,0.0};

void zgradpulse(double amp, double duration)
{
  char gstr[MAXSTR];
  double sdur;
  int i;
  if (duration < 2.4e-6) return;  // won't work - used as skip .. no error

  if ( P_getstring(GLOBAL,(char*) "gradientdisable",gstr,1,2) != 0)
    OSTRCPY( gstr, sizeof(gstr), "n");

  if (strcmp(gstr,"y") == 0)
    amp = 0.0;

  if ( P_getstring(GLOBAL,(char*) "gradientshaping",gstr,1,2) != 0 )
    OSTRCPY( gstr, sizeof(gstr), "s");
  if (gstr[0]!='y')
    {
      rgradient('z',amp);
      no_shim_delay(duration);
      rgradient('z',0.0);
      return;
    }
  sdur = duration/19.0;  // was 21.
  if (sdur < 2.4e-6)
    abort_message("Gradient too short to wurst shape. abort!\n");
  for (i=0; i < 19; i++)
    {
      rgradient('z',amp*mgrad[i]);
      no_shim_delay(sdur);
    }
  rgradient('z',0.0);
  return;
}

void notImplemented()
{
  cout << "called notImplemented() ..." << endl;
}


void InitAcqObject_writecode(int word)
{
  InitAcqObject *initAcqObject = P2TheConsole->getInitAcqObject();
  initAcqObject->WriteWord(word);
}


void InitAcqObject_startTableSection(ComboHeader ch)
{
  InitAcqObject *initAcqObject = P2TheConsole->getInitAcqObject();
  initAcqObject->startTableSection(ch);
}


void InitAcqObject_endTableSection()
{
  InitAcqObject *initAcqObject = P2TheConsole->getInitAcqObject();
  initAcqObject->endTableSection();
}


void broadcastCodes(int code, int many, int *codeptr)
{
  P2TheConsole->broadcastCodes(code, many, codeptr);
}

void setKzDuration(double duration)
{
   P2TheConsole->setParallelKzDuration(duration);
}

int calcKzLoop(long long ticker, double *remainingTime)
{
   double duration;
   long long ticks;
   long long count = 0;
   long long rem;

   *remainingTime = 0.0;
   duration = P2TheConsole->getParallelKzDuration();
   rem = ticks = calcticks(duration);
   if (ticks >= ticker)
   {
      count = ticks / ticker;
      if (P2TheConsole->isParallel())
         P2TheConsole->setParallelInfo( (int) count);
      rem = ticks - (count * ticker);
   }
   if (rem > 0)
      *remainingTime = (double) rem / 80000000.0;
   return((int) count);
}

void broadcastLoopCodes(int code, int many, int *codeptr,
                        int count, long long ticks)
{
   if (P2TheConsole->isParallel())
   {
      Controller *contr = P2TheConsole->getParallelController();
      contr->outputACode(code, many, codeptr);
      if (ticks == -1)
      {
         P2TheConsole->setParallelInfo(count);
      }
      else
      {
         count = P2TheConsole->getParallelInfo();
         if (count <= 0)
         {
            P2TheConsole->update4Sync(-ticks);
            contr->noteDelay(-ticks);
         }
         else if (count > 1)
         {
            P2TheConsole->update4Sync((count - 1) * ticks);
            contr->noteDelay((count - 1) * ticks);
         }
      }
   }
   else
      P2TheConsole->broadcastCodes(code, many, codeptr);
}

void AcodeManager_startSubSection(int header)
{
  AcodeManager *acodeMgr = AcodeManager::getInstance();
  acodeMgr->startSubSection(header);
}

void AcodeManager_endSubSection()
{
  AcodeManager *acodeMgr = AcodeManager::getInstance();
  acodeMgr->endSubSection();
}

int AcodeManager_getAcodeStageWriteFlag(int stage)
{
  AcodeManager *acodeMgr = AcodeManager::getInstance();
  int flag = acodeMgr->getAcodeStageWriteFlag(stage);
  return flag;
}


/* if blank on state = 1, state = 0
   the state sense needs to be verified
   gates take no time to set..
*/
void genBlankOnOff(int state, int rfch)
{
   RFController *Work = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfch);
   if (Work->getProgDecInterlock() > 0)
   {
       if (bgflag)
       {
          fprintf(stdout,"advisory: psg cmd obs/decblank/unblank not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
       }
       return;
   }

   if (state == 1)
     Work->blankAmplifier();
   else
     Work->unblankAmplifier();
}


/* if tx on state = 1, else for tx off  state = 0  */

void genTxGateOnOff(int state, int rfch)
{
  RFController *Work = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfch);
  if (Work->getProgDecInterlock() > 0)
  {
      if (bgflag)
      {
         fprintf(stdout,"advisory: psg cmd xmtr/decon/off not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
      }
      return;
  }

  if (state == 1)
  {
    Work->setChannelActive(1);

    // any check for rcvr on currently?
    if ( (Work->isObserve()) && rcvr_is_on_now )
    {
      abort_message("TX Gate cannot be turned on if receiver is on for %s  abort",Work->getName());
    }
  }

  Work->setTxGateOnOff(state);
}


/*
    sets coarse attenuator with 50 ns duration
    AUX BUS
*/
void genPower(double power, int rfch)
{
   long long ticks;
   if (rfch > P2TheConsole->numRFChan) return;

   RFController *Work = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfch);
   if (Work->getProgDecInterlock() > 0)
   {
       if (bgflag)
       {
          fprintf(stdout,"advisory: psg cmd obs/decpower not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
       }
       return;
   }

   P2TheConsole->newEvent();
   Work->clearSmallTicker();
   Work->setActive();
   Work->setAttenuator(power);
   ticks = Work->getSmallTicker();
   P2TheConsole->update4Sync(ticks);
}

/*
   "Fine Power" takes no time to set..
*/

void genPowerF(double finepower, int rfch)
{
    if (rfch > P2TheConsole->numRFChan) return;
    RFController *Work = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfch);
    if (Work->getProgDecInterlock() > 0)
    {
        if (bgflag)
        {
           fprintf(stdout,"advisory: psg cmd obs/decpwrf not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
        }
        return;
    }

    Work->setAmpScale(finepower);
}


/*
    sets fine power in real-time
    takes no time to set..
*/

void genVPowerF(int vindex, int rfch)
{
   if (rfch > P2TheConsole->numRFChan) return;

   RFController *Work = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfch);
   if (Work->getProgDecInterlock() > 0)
   {
       if (bgflag)
       {
          fprintf(stdout,"advisory: psg cmd vobs/decpwrf not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
       }
       return;
   }

   if ( ! validrtvar(vindex) )
      abort_message("invalid real time variable specified for real-time fine power on %s. abort!\n",Work->getName());

   Work->setVAmpScale(1.0, vindex);
}

void defineVAmpStep(double vstep, int rfch)
{
    if (rfch > P2TheConsole->numRFChan) return;
    RFController *Work = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfch);
    if (Work->getProgDecInterlock() > 0)
    {
        if (bgflag)
        {
           fprintf(stdout,"advisory: psg comd vobs/decpwrfstepsize not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
        }
        return;
    }

    Work->setVAmpStep(vstep);
}

/*
   Phase takes no time to set.. This is all angles.. however it
   "fishes" the phase step from currentPhaseStep.
*/

void definePhaseStep(double step, int rfch)
{
    if (rfch > P2TheConsole->numRFChan) return;
    RFController *Work = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfch);
    if (Work->getProgDecInterlock() > 0)
    {
        if (bgflag)
        {
           fprintf(stdout,"advisory: psg cmd obs/decstepsize not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
        }
        return;
    }

    Work->setPhaseStep(step);
}

void genFullPhase(int vvar, int rfch)
{
    if (rfch > P2TheConsole->numRFChan) return;
    RFController *Work = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfch);
    if (Work->getProgDecInterlock() > 0)
    {
        if (bgflag)
        {
           fprintf(stdout,"advisory: psg cmd xmtr/dcplrphase not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
        }
        return;
    }

    Work->setVFullPhase(vvar);
}

void genPhase(int phase, int rfch)
{
   if (rfch > P2TheConsole->numRFChan) return;
    RFController *Work = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfch);
    if (Work->getProgDecInterlock() > 0)
    {
        if (bgflag)
        {
           fprintf(stdout,"advisory: psg cmd tx/decphase not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
        }
        return;
    }

    Work->setVQuadPhase(phase);
}

/*
    sets frequency with N * 50 ns duration
    AUX BUS
*/
void genOffset(double offset, int rfch)
{
   long long ticks;
   double temp;
   /* maybe some large hop warning?? */
   P2TheConsole->newEvent();
   RFController *Work = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfch);
   if (Work->getProgDecInterlock() > 0)
   {
       if (bgflag)
       {
          fprintf(stdout,"advisory: psg cmd obs/decoffset not allowed inside obs/decprgon/decoupling period (ignoring..)\n");
       }
       return;
   }

   Work->clearSmallTicker();
   Work->setActive();
   temp = Work->getBaseFrequency();
   temp += offset/1000000.0L;

   Work->setFrequency(temp);
   Work->setCurrentOffset(offset);
   ticks = Work->getSmallTicker();  /* use the ticker.. */
   P2TheConsole->update4Sync(ticks);
}
//
// genOffsetI use with implicit arrays i.e. func4 calls
// they get executed by f reset at increment start. w no add't overhead
//
void genOffsetI(double offset, int rfch)
{
   return;
}
//
// genSfrqI use with implicit arrays i.e. func4 calls
// they get executed by f reset at increment start. w no add't overhead
//
void genSfrqI(double sfrq, int rfch)
{
   RFController *Work = (RFController *) P2TheConsole->getRFControllerByLogicalIndex(rfch);
   Work->setFrequency(sfrq);
   // this might need offset and setbase..
}

//
//
void gen_offsetlist(double *pssval, double gssval, double resfrq, double *freqList, double nslices, char mode, int rfch)
{
    double nucgamma;

    int nsval   = (int)nslices;
    nucgamma   = nuc_gamma();

    if ( (mode == 'c') || (mode == 'i') )
    {
       for (int j=0; j < nsval; j++)
       {
         freqList[j] = resfrq + nucgamma*gssval*pssval[j] ;
       }
    }
    else if (mode == 's')
    {
       freqList[0] = resfrq + nucgamma*gssval*pssval[0] ;
    }
    else
       abort_message("invalid value for mode in gen_offsetlist. abort!\n");
}

//
//
void gen_offsetglist(double *pssval, double *gssvalarr, double resfrq, double *freqList, double nslices, char mode, int rfch)
{
    double nucgamma;

    int nsval   = (int)nslices;

    nucgamma   = nuc_gamma();

    if ( (mode == 'c') || (mode == 'i') )
    {
       for (int j=0; j < nsval; j++)
       {
         freqList[j] = resfrq + nucgamma*gssvalarr[j]*pssval[j] ;
       }
    }
    else if (mode == 's')
    {
       freqList[0] = resfrq + nucgamma*gssvalarr[0]*pssval[0] ;
    }
    else
       abort_message("invalid value for mode in gen_offsetglist. abort!\n");
}

//
//
void ifzero_bridge(int rtvar)
{
  int numcntrlrs = P2TheConsole->nValid;
  for (int i=0; i < numcntrlrs; i++)
  {
    if (! (P2TheConsole->PhysicalTable[i])->isOff() )
    {
      (P2TheConsole->PhysicalTable[i])->ifzero(rtvar);
    }
  }
}
//
//
void ifmod2zero_bridge(int rtvar)
{
  int numcntrlrs = P2TheConsole->nValid;
  for (int i=0; i < numcntrlrs; i++)
  {
    if (! (P2TheConsole->PhysicalTable[i])->isOff() )
    {
      (P2TheConsole->PhysicalTable[i])->ifmod2zero(rtvar);
    }
  }
}
//
//
void elsenz_bridge(int rtvar)
{
  int numcntrlrs = P2TheConsole->nValid;
  for (int i=0; i < numcntrlrs; i++)
  {
    if (! (P2TheConsole->PhysicalTable[i])->isOff() )
    {
      (P2TheConsole->PhysicalTable[i])->elsenz(rtvar);
    }
  }
}
//
//
void endif_bridge(int rtvar)
{
  int numcntrlrs = P2TheConsole->nValid;
  for (int i=0; i < numcntrlrs; i++)
  {
    if (! (P2TheConsole->PhysicalTable[i])->isOff() )
    {
      (P2TheConsole->PhysicalTable[i])->endif(rtvar);
    }
  }
}
//
//
void nowait_loop(double dloops, int rtvar1, int rtvar2)
{
   int loops = (int)(dloops+0.5);
   NOWAIT_loopcount = loops;
   startTicker(NOWAIT_LOOP_TICKER);

   F_initval(loops, rtvar1);
   loop(rtvar1, rtvar2);
}
//
//
void nowait_endloop(int rtvar)
{
   long long ticks;

   endloop(rtvar);

   ticks = stopTicker(NOWAIT_LOOP_TICKER);
   GradientBase  *gC = P2TheConsole->getConfiguredGradient();

   if (gC != NULL)
   {
     int numcntrlrs = P2TheConsole->nValid;
     for (int i=0; i < numcntrlrs; i++)
     {
        if ( !(P2TheConsole->PhysicalTable[i])->isAsync() )
        {
           if (P2TheConsole->PhysicalTable[i] != gC)
              (P2TheConsole->PhysicalTable[i])->noteDelay(ticks*(NOWAIT_loopcount-1));
           else
              gC->incr_NOWAIT_eventTicker(-ticks*(NOWAIT_loopcount-1));

           P2TheConsole->newEvent();
           P2TheConsole->update4Sync(0);
        }
     }
   }

   NOWAIT_loopcount = 0;
}
//
//
void xgate(double events)
{
  int counts = (int) (events+0.5);
  if (counts < 0)
  {
    abort_message("invalid argument (negative) for xgate command\n");
  }
  else if (counts == 0)
  {
    return;
  }

  /* need to flush any 4usec grid delay words on the Gradient Controller first */
  GradientController *grad = (GradientController *)(P2TheConsole->getGRADController());
  if (grad != NULL)
     grad->flushGridDelay();

  int buffer[2];
  buffer[0] = counts;   // xgate counts
  buffer[1] = 0;        // type of controller 1=DD2 RFController  0=all others

  int numcntrlrs = P2TheConsole->nValid;

  // Loop starts from 1, avoids Master Controller for now (MasterController FIFO should not be halted)
  for (int i=1; i < numcntrlrs; i++)
  {
    Controller *contr = P2TheConsole->PhysicalTable[i];

    if ( ! contr->isOff() )
    {
        contr->add2Stream(LATCHKEY|DURATIONKEY|0);    // halt all FIFOs except master1

        if ( isICAT() )
        {
           if (! contr->isRF())
           {
              buffer[1] = 0;
              contr->outputACode(SYNC_XGATE_COUNT, 2, buffer);
              contr->setTickDelay(64L);
           }
           else
           {
              buffer[1] = 1;
              contr->outputACode(SYNC_XGATE_COUNT, 2, buffer);
              contr->noteDelay(64L);
           }
        }
    }
  }

  // xgate events for Master Controller
  // SYNC_XGATE_COUNT is only associated with generating a count for xgate
  // and not to do with the actual execution of xgate
  Controller *masterC = P2TheConsole->getControllerByID("master1");
  buffer[1] = 0;
  masterC->outputACode(SYNC_XGATE_COUNT, 2, buffer);

  if ( isICAT() )
    masterC->setTickDelay(64L);

  P2TheConsole->newEvent();
  masterC->setActive();

  int ticks = ((MasterController *)masterC)->xgate(counts);

  // now sync all controllers
  P2TheConsole->update4Sync(ticks);
}

//
//  support for 3x number channels.
//
void splineonoff(int whichone,int state)
{
  int ch, l;
  char stub[20];
  RFController *rfC;
  int numRFChan = P2TheConsole->numRFChan;
  if ((whichone < 1) || (whichone > 3*numRFChan))
    abort_message("sp line #%d invalid\n",whichone);
  ch = (whichone - 1) / 3 + 1;
  l = (whichone - 1) % 3 + 1;
  OSPRINTF( stub, sizeof(stub), "rf%d", ch);
  //cout << "stub = " << stub << endl;
  rfC = (RFController *)(P2TheConsole->getControllerByID(stub));
  if (rfC->isAsync())
  {
     text_message("channel is decoupling - no spare usage");
     return;
  }
  else
    rfC->setUserGate( l , state);
}

void splineon(int whichone)
{
   splineonoff(whichone,1);
}
//
void splineoff(int whichone)
{
   splineonoff(whichone,0);
}

//
// Turn temperature compensation on or off
//
void setTempComp(int state) {
        RFController *rfC;
        int numRFChan = P2TheConsole->numRFChan;
        long long count = 4LL;
        P2TheConsole->newEvent();

        for (int i = 0; i < numRFChan; i++) {
                rfC = (RFController *) (P2TheConsole->RFUserTable[i]);
                rfC->setActive();
                rfC->setTempComp(state);
                rfC->noteDelay(count);
        }

        P2TheConsole->update4Sync(count);
}
//
//
void postInitScanCodes()
{
  P2TheConsole->postInitScanCodes();
}

void lk_sample()
{
int count, i, streamCodes[20];
   Controller *tmp = P2TheConsole->getControllerByID("master1");
   P2TheConsole->newEvent();
   tmp->setActive();
   tmp->clearSmallTicker();

   i=0;
   streamCodes[i++]=0;
   tmp->outputACode(SMPL_HOLD,1,streamCodes);

   tmp->noteDelay(4);
   count = tmp->getSmallTicker();
   P2TheConsole->update4Sync(count);
}

void lk_hold()
{
int count, i, streamCodes[20];
   Controller *tmp = P2TheConsole->getControllerByID("master1");
   P2TheConsole->newEvent();
   tmp->setActive();
   tmp->clearSmallTicker();

   i=0;
   streamCodes[i++]=1;
   tmp->outputACode(SMPL_HOLD,1,streamCodes);

   tmp->noteDelay(4);
   count = tmp->getSmallTicker();
   P2TheConsole->update4Sync(count);
}

void lk_sampling_off()
{  }

void lk_autotrig()
{  }

/* turn off lock sampling */
/*
 * void lk_sampling_off()
 * {
 * int count, i, streamCodes[20];
 *    Controller *tmp = P2TheConsole->getControllerByID("master1");
 *    P2TheConsole->newEvent();
 *    tmp->setActive();
 *    tmp->clearSmallTicker();
 *
 *    i=0;
 *    streamCodes[i++]=3;
 *    tmp->outputACode(SMPL_HOLD,1,streamCodes);
 *
 *    tmp->noteDelay(4);
 *    count = tmp->getSmallTicker();
 *    P2TheConsole->update4Sync(count);
 * }
 *
 * void lk_autotrig()
 * {
 * int count, i, streamCodes[20];
 *    Controller *tmp = P2TheConsole->getControllerByID("master1");
 *    P2TheConsole->newEvent();
 *    tmp->setActive();
 *    tmp->clearSmallTicker();
 *
 *    i=0;
 *    streamCodes[i++]=4;
 *    tmp->outputACode(SMPL_HOLD,1,streamCodes);
 *
 *    tmp->noteDelay(4);
 *    count = tmp->getSmallTicker();
 *    P2TheConsole->update4Sync(count);
 * }
 */


void func4sfrq(double value)
{
  genSfrqI(value,1);
}

void func4dfrq(double value)
{
  genSfrqI(value,2);
}

void func4dfrq2(double value)
{
  genSfrqI(value,3);
}

void func4dfrq3(double value)
{
  genSfrqI(value,4);
}

void func4dfrq4(double value)
{
  genSfrqI(value,5);
}

void arrayedshims()
{
int *streamBuffer;
   MasterController *masterC =
                (MasterController *)P2TheConsole->getControllerByID("master1");
   streamBuffer = masterC->loadshims();
   if (streamBuffer != NULL)
   {
      masterC->outputACode(SETSHIMS, 47, streamBuffer);
   }
}

void vdelay(int timecode,int rtVar)
{
  // for all controllers ...
  int tc;
  int numcntrlrs = P2TheConsole->nValid;
  switch (timecode) {
  case 1:  tc = 1; break;
  case 2:  tc = 80; break;
  case 3:  tc = 80000; break;
  case 4:  tc = 80000000; break;
  default: tc = 1;
  }
  for (int i=0; i < numcntrlrs; i++)
  {
    Controller *contr = P2TheConsole->PhysicalTable[i];
    if ( ! contr->isOff() )
      contr->setVDelay(rtVar,tc);
  }
}


int genCreate_delay_list(double *list, int nvals)
{
        if (dps_flag) return(0);

    /* prepare delay list */

        if (list == NULL)
                abort_message("vdelay_list is not defined or empty. abort!\n");

        union64 lscratch;
        int intdelaylist[2*nvals];

        MasterController *masterContr = (MasterController *)P2TheConsole->getControllerByID("master1");
        for (int i=0; i<nvals; i++)
        {
                lscratch.ll = masterContr->calcTicks(list[i]);
                if (lscratch.ll <= 0LL)
                        abort_message("invalid delay value in vdelay_list, possibly zero or negative. abort!\n");

                intdelaylist[i*2+0]=lscratch.w2.word0;
                intdelaylist[i*2+1]=lscratch.w2.word1;
        }

        cPatternEntry *vdelayPattern  = masterContr->createVDelayPattern(intdelaylist, 2*nvals);
        int listId = -1;
        listId = vdelayPattern->getReferenceID();
        if ( (listId < 1) || (listId > 0x7FFFFF) )
                abort_message("unable to determine the vdelay_list id. abort!\n");

        int numcntrlrs = P2TheConsole->nValid;
        for (int i=0; i < numcntrlrs; i++)
        {
                if ( !(P2TheConsole->PhysicalTable[i])->isOff() )
                {
                        (P2TheConsole->PhysicalTable[i])->setVDelayList(vdelayPattern, intdelaylist, 2*nvals);
                }
        }
        return(listId);
}


void vdelay_list(int listId, int rtvar)
{
  if (dps_flag) return;

  if ( ! validrtvar(rtvar) )
     abort_message("invalid real time variable specified in vdelay_list. abort!\n");

  int numcntrlrs = P2TheConsole->nValid;
  for (int i=0; i < numcntrlrs; i++)
  {
    if (! (P2TheConsole->PhysicalTable[i])->isOff() )
    {
      (P2TheConsole->PhysicalTable[i])->doVDelayList(listId, rtvar);
    }
  }
}


void startTicker(int type)
{
  int numcntrlrs;

  switch ( type )
  {
     case IFZ_TRUE_TICKER:
     case IFZ_FALSE_TICKER:
     case NOWAIT_LOOP_TICKER:

       P2TheConsole->startTicker(type);
       break;

     // to handle loops, hardloops
     case LOOP_TICKER:
       P2TheConsole->startTicker(type);
       numcntrlrs = P2TheConsole->nValid;
       for (int i=0; i < numcntrlrs; i++)
       {
         Controller *contr = P2TheConsole->PhysicalTable[i];

         if (! (contr->isOff()) )
           contr->loopStart();
       }
       break;

     case RL_LOOP_TICKER:
       P2TheConsole->startTicker(type);
       if ( P2TheConsole->isParallel())
       {
          Controller *contr = P2TheConsole->getParallelController();
          if (! (contr->isOff()) )
             contr->loopStart();
       }
       else
       {
          numcntrlrs = P2TheConsole->nValid;
          for (int i=0; i < numcntrlrs; i++)
          {
            Controller *contr = P2TheConsole->PhysicalTable[i];

            if (! (contr->isOff()) )
               contr->loopStart();
          }
       }
       break;

     default:
       text_message("psg error: startTicker(): invalid Console ticker\n");
       break;
  }
}


long long stopTicker(int type)
{
  int numcntrlrs;

  switch ( type )
  {
     case NOWAIT_LOOP_TICKER:
       return (P2TheConsole->stopTicker(type));
       break;

     // to handle loops, hardloops
     case LOOP_TICKER:
       numcntrlrs = P2TheConsole->nValid;
       for (int i=0; i < numcntrlrs; i++)
       {
          Controller *contr = P2TheConsole->PhysicalTable[i];

         /* need to flush any 4usec grid delay words on the Gradient Controller first */
         if ( strcmp(contr->getName(),"grad1") == 0)
           ((GradientController *)contr)->flushGridDelay();

         if (! (contr->isOff()) )
           contr->loopEnd();
       }
       return (P2TheConsole->stopTicker(type));
       break;

     case RL_LOOP_TICKER:
       if ( P2TheConsole->isParallel())
       {
          Controller *contr = P2TheConsole->getParallelController();
          if ( strcmp(contr->getName(),"grad1") == 0)
             ((GradientController *)contr)->flushGridDelay();

          if (! (contr->isOff()) )
             contr->loopEnd();
       }
       else
       {
          numcntrlrs = P2TheConsole->nValid;
          for (int i=0; i < numcntrlrs; i++)
          {
             Controller *contr = P2TheConsole->PhysicalTable[i];

            /* need to flush any 4usec grid delay words on the Gradient Controller first */
            if ( strcmp(contr->getName(),"grad1") == 0)
              ((GradientController *)contr)->flushGridDelay();

            if (! (contr->isOff()) )
              contr->loopEnd();
          }
       }
       return (P2TheConsole->stopTicker(type));
       break;

     case IFZ_TRUE_TICKER:
     case IFZ_FALSE_TICKER:
       return(0);
       break;


     default:
       text_message("psg error: stopTicker(): invalid Console ticker\n");
       return(0);
       break;

  }
}


long long stopTickerKzLoop( double *remTime)
{
  int numcntrlrs;
  long long ticker;
  long long count;
       ticker = P2TheConsole->stopTicker(RL_LOOP_TICKER);
       count = calcKzLoop(ticker, remTime);

       if ( P2TheConsole->isParallel())
       {
          Controller *contr = P2TheConsole->getParallelController();
          if ( strcmp(contr->getName(),"grad1") == 0)
             ((GradientController *)contr)->flushGridDelay();

          if (! (contr->isOff()) )
             contr->loopEnd((int) count);
       }
       else
       {
          numcntrlrs = P2TheConsole->nValid;
          for (int i=0; i < numcntrlrs; i++)
          {
             Controller *contr = P2TheConsole->PhysicalTable[i];

            /* need to flush any 4usec grid delay words on the Gradient Controller first */
            if ( strcmp(contr->getName(),"grad1") == 0)
              ((GradientController *)contr)->flushGridDelay();

            if (! (contr->isOff()) )
              contr->loopEnd((int) count);
          }
       }
       return (ticker);
}


void diplexer_override(int state)
{
  MasterController *masterC = (MasterController*) P2TheConsole->getControllerByID("master1");
  masterC->setDiplexerRelay(state); //
  delay(0.000025);
}

void rotorperiod(int vvar)
{
  int buffer[4];
  MasterController *masterC = (MasterController*) P2TheConsole->getControllerByID("master1");
  buffer[0] = vvar;
  masterC->outputACode(READHSROTOR,1,buffer);
}

void rotorsync(int vvar)
{
  int buffer[4];
  buffer[0] = 1;
  buffer[1] = vvar;
  P2TheConsole->broadcastCodes(ROTORSYNC_TRIG,2,buffer);
}

double getTimeMarker()
{
  long long ago;
   MasterController *masterC = (MasterController*) P2TheConsole->getControllerByID("master1");
   ago = masterC->getBigTicker();
   return(((double) ago) * 0.0000000125L);
}
//
//
//
int parmToRcvrMask(const char *parName, int calltype)
{
    /*
     * This version assumes the rcvrs parameter is a string of "y"s and "n"s
     * specifying whether each receiver is on or off.
     * Returned mask is guaranteed to be non-zero.
     */
    char rcvrStr[MAXSTR];
    int nRcvrs;
    int rtnMask = 0;

        double tmp;
        if (P_getreal(GLOBAL,"numrcvrs", &tmp, 1) < 0)
        {
            nRcvrs = 1;
        } else {
            nRcvrs = (int)tmp;
        }

        if ( P_getstring(CURRENT, parName, rcvrStr, 1, MAXSTR) && (calltype==1) )
        {
          rtnMask = 1;
          return rtnMask;
        }
        if (calltype == 1)
           P_getstring(CURRENT, parName, rcvrStr, 1, MAXSTR) ;
        else
           OSTRCPY( rcvrStr, sizeof(rcvrStr), parName);

        int c;
        int n;
        char *pc = rcvrStr;
        for (n=0; (c=*pc) && (n<nRcvrs); pc++, n++) {
                if (c == 'y' || c == 'Y') {
                    rtnMask |= 1 << n;
                }
        }
        if (!rtnMask) {
            rtnMask = 1;        /* Don't allow all rcvrs to be off! */
        }
        return rtnMask;
}
//
//
//
int isRcvrActive(int n)
{
    int rtn;

    rtn = (parmToRcvrMask("rcvrs",1) >> n) & 1;
    return rtn;
}

char *RcvrMapStr(const char *parmname, char *mapstr)
{
  // BDZ: note that parmname is never referenced: bug?
  
  int mrMask;
  int i,nRcvrs,fstflag;
  char tmpstr[256];
  double tmp;

  if (P_getreal(GLOBAL,"numrcvrs", &tmp, 1) < 0)
  {
     nRcvrs = 1;
  }
  else
  {
     nRcvrs = (int)tmp;
  }
  
  // BDZ: maybe parmname is supposed to be passed into this next method
  //   instead of "rcvrs"? That is what usage in the rest of the program
  //   kind of implies. Or parname param should just go away.
  
  mrMask = parmToRcvrMask((char*) "rcvrs", 1);
  fstflag = 1;
  for (i=0;i < nRcvrs; i++)
  {
     if ( (mrMask >> i) & 1 )
     {
       if (fstflag)
         OSPRINTF( tmpstr, sizeof(tmpstr), "ddr%d", i+1);
       else
         OSPRINTF( tmpstr, sizeof(tmpstr), ",ddr%d", i+1);
                    
       // BDZ - UNSAFE - could fix if this routine took mapstr's size as a param
       strcat( mapstr, tmpstr);
       
       fstflag = 0;
     }
  }
  return(mapstr);
}

int getNumActiveRcvrs()
{
   return P2TheConsole->getNumActiveRcvrs();
}

/*
 * triggerSelect selects the trigger to be used for the xgate.
 * The Labels on the hardware are Input1, Input2, and Input3.
 *  To select Input1, a 0 must be sent to the hardware.
 *  To select Input2, a 1 must be sent to the hardware.
 *  To select Input3, a 2 must be sent to the hardware.
 *  To select Internal, a 3 must be sent to the hardware.
 * Users typically start counting at 1.  Therefore, the input
 * to this function will match the users point of view.
 * That is, to select Input1, a 1 is passed as whichone, etc.
 * This function subtracts 1 before sending it to the hardware.
 */
void triggerSelect(int whichone)
{
   long long ticks;

   if (whichone == 0)
   {
      warn_message("trigger defaults to 1");
      whichone=1;
   }
   if ( (whichone<1) || (whichone>3) )
   {  abort_message("triggerSelect argument %d out of range 1-3, check trigger parameter",whichone);
   }
   P2TheConsole->newEvent();
   MasterController *Work =
            (MasterController *) P2TheConsole->getControllerByID("master1");
   Work->clearSmallTicker();
   Work->setActive();
   Work->setAux(( 7<<8) | ( (whichone - 1) & 3) );
   ticks = Work->getSmallTicker();
   P2TheConsole->update4Sync(ticks);
}

void readMRIUserByte(int rtvar, double dur)
{
  int ticks;

  readuserbyte=0;
  if (dur < 0.0025)             // less than 2.5 ms is to short
     abort_message("readMRIUSerByte() must have a delay > 2.5 ms");
  else if (dur > 0.5)
  {
      ticks = 40000000; dur -= 0.5;
      readMRIByte(rtvar, ticks);
      delay(dur);
  }
  else
  {
      ticks = (int)(dur*8e7 + 0.5);     // 12.5 ns ticks
      readMRIByte(rtvar,ticks);
  }
}

void readMRIUserBit(int rtvar, double dur)
{
  int ticks;

  readuserbyte=1;
  if (dur <= 0.00099)           // less than 1 ms is to short
     abort_message("FAST readMRIUSerByte() must have a delay > 0.99 ms");
  else if (dur > 0.5)
  {
      ticks = 40000000; dur -= 0.5;
      readMRIByte(rtvar, ticks);
      delay(dur);
  }
  else
  {
      ticks = (int)(dur*8e7 + 0.5);     // 12.5 ns ticks
      readMRIByte(rtvar,ticks);
  }
}

static void readMRIByte(int rtvar, int ticks)
{
int  i, codes[9];
   i=0;
if(readuserbyte==0) //SAS
   codes[i++] = 0;              // Read
else //SAS
   codes[i++] = 2;              // Fast Read SAS
   codes[i++] = rtvar;
   codes[i++] = ticks;		// 50 ms <= tick <= 500 ms
   broadcastCodes(MRIUSERBYTE, i, codes);
}

void writeMRIUserByte(int rtvar)
{
int  codes[9];
long long ticks;
   P2TheConsole->newEvent();
   MasterController *tmp =
            (MasterController *) P2TheConsole->getControllerByID("master1");
   tmp->clearSmallTicker();
   tmp->setActive();

   codes[0] = 1;		// Write
   codes[1] = rtvar;
   tmp->outputACode(MRIUSERBYTE, 2, codes);
   tmp->noteDelay(4);		// aux bus write takes 50 nsec
   tmp->setTickDelay(316);	// fill out to 4 usec

   ticks = tmp->getSmallTicker();
   P2TheConsole->update4Sync(ticks);
}


////
////  init and modify generate no codes.
////

int initRFGroupPulse(double pw, char *basename, char mode,  double coarsePower, double finePower,
       double startPhase, double *freqOffsetArray, int numberInArray)
{
  int i,j;
  RFChannelGroup *theRFGroup;
  RFSpecificPulse *p2s;
  RFGroupPulse *p2g;
  theRFGroup = P2TheConsole->theRFGroup;
  if (theRFGroup == NULL)
    {
      abort_message("not configured for group pulses-see rfGroupMap");
    }
  p2g = new RFGroupPulse(theRFGroup->numberInGroup);
  for (i=0; i < theRFGroup->numberInGroup; i++)
  {
    j = theRFGroup->rfchannelIndexList[i];
    p2s = new RFSpecificPulse(j,1,mode,basename,pw,coarsePower,finePower,startPhase,freqOffsetArray,numberInArray);
    p2g->addSpecificPulse(p2s,i);
  }
  j = theRFGroup->addPulse(p2g);
  //cout << "initRFGroupPulse made pulse # " << p2g->myNum << endl;
  return(p2g->myNum);
}

//
// routine aborts if there's a problem
//

void modifyRFGroupName(int list, int gnum, char *newName)
{
  RFChannelGroup *theRFGroup;
  RFSpecificPulse *p2s;
  RFGroupPulse *p2g;
  theRFGroup = P2TheConsole->theRFGroup;
  if (theRFGroup == NULL)
    {
      abort_message("not configured for group pulses-see rfGroupMap");
    }
  p2g = theRFGroup->getPulse(list);
  if (p2g == NULL) abort_message("Group Pulse not found");
  // get Controller Data is 1 to MAXINGROUP
  p2s = p2g->getControllerData(gnum);
  if (p2s == NULL) abort_message("Group Pulse: Controller not found");
  OSTRCPY( p2s->shapeName, sizeof(p2s->shapeName), newName);
}

// - tested
void modifyRFGroupOnOff(int pulseId, int gnum,int on)
{
  RFChannelGroup *theRFGroup;
  RFSpecificPulse *p2s;
  theRFGroup = P2TheConsole->theRFGroup;
  if (theRFGroup == NULL)
      abort_message("not configured for group pulses-see rfGroupMap");
  p2s = theRFGroup->getSpecific(pulseId,gnum);
  p2s->onFlag = on;
}

void modifyRFGroupPwrf(int pulseId, int gnum, double pwrf)
{
  RFChannelGroup *theRFGroup;
  RFSpecificPulse *p2s;
  theRFGroup = P2TheConsole->theRFGroup;
  if (theRFGroup == NULL)
      abort_message("not configured for group pulses-see rfGroupMap");
  p2s = theRFGroup->getSpecific(pulseId,gnum);
  p2s->tpwrf = pwrf;
}

void modifyRFGroupPwrC(int pulseId, int gnum, double pwr)
{
  RFChannelGroup *theRFGroup;
  RFSpecificPulse *p2s;
  theRFGroup = P2TheConsole->theRFGroup;
  if (theRFGroup == NULL)
      abort_message("not configured for group pulses-see rfGroupMap");
  p2s = theRFGroup->getSpecific(pulseId,gnum);
  if (p2s == NULL) abort_message("Group Pulse: Controller not found");
  p2s->tpwr = pwr;
}

void modifyRFGroupSPhase(int pulseId, int gnum, double phase)
{
  RFChannelGroup *theRFGroup;
  RFSpecificPulse *p2s;
  theRFGroup = P2TheConsole->theRFGroup;
  if (theRFGroup == NULL)
      abort_message("not configured for group pulses-see rfGroupMap");
  p2s = theRFGroup->getSpecific(pulseId,gnum);
  if (p2s == NULL) abort_message("Group Pulse: Controller not found");
  p2s->startPhase = phase;
}

void modifyRFGroupFreqList(int pulseId, int gnum, double *fl)
{
  RFChannelGroup *theRFGroup;
  RFSpecificPulse *p2s;
  theRFGroup = P2TheConsole->theRFGroup;
  if (theRFGroup == NULL)
      abort_message("not configured for group pulses-see rfGroupMap");
  p2s = theRFGroup->getSpecific(pulseId,gnum);
  if (p2s == NULL) abort_message("Group Pulse: Controller not found");
  p2s->freqA = fl;
}

void GroupPulse(int pulseid, double rof1, double rof2,  int vphase, int vselect)
{
  long long answer;
  RFChannelGroup *theRFGroup;
  RFGroupPulse *p2g;
  theRFGroup = P2TheConsole->theRFGroup;
  P2TheConsole->newEvent();
  if (theRFGroup == NULL)
    {
      abort_message("not configured for group pulses-see rfGroupMap");
    }
  p2g = theRFGroup->getPulse(pulseid);
  if (p2g == NULL)
    abort_message("Group Pulse not found");
  p2g->generate(); // self detects prior generation
  answer = p2g->run(rof1,rof2,vphase,vselect);
  P2TheConsole->update4Sync(answer);
}

// diagnostic mode incomplete..
void TGroupPulse(int pulseid, double rof1, double rof2,  int vphase, int vselect)
{
  int i,k,l;
  long long answer,delta;
  long long ticks=0L;
  RFController *p2rfc;
  RFChannelGroup *tRFG;
  RFGroupPulse *p2g;
  tRFG = P2TheConsole->theRFGroup;
  k = tRFG->numberInGroup;
  for (i=0; i < k; i++)
      modifyRFGroupOnOff(pulseid,i+1,2);  /* arg =2 leave LO on for external mixer - test use only */
  P2TheConsole->newEvent();
  for (i=0; i< P2TheConsole->numDDRChan; i++)
    {
      if (!((DDRController *) (P2TheConsole->DDRUserTable[i]))->ddrActive4Exp) continue;
      ((DDRController *) (P2TheConsole->DDRUserTable[i]))->setActive();
      ((DDRController *) (P2TheConsole->DDRUserTable[i]))->clearSmallTicker();
      ticks = ((DDRController *) (P2TheConsole->DDRUserTable[i]) )->Sample(np*0.5/sw);
    }
  if (tRFG == NULL)
    {
      abort_message("not configured for group pulses-see rfGroupMap");
    }
  p2g = tRFG->getPulse(pulseid);
  if (p2g == NULL)
    abort_message("Group Pulse not found");
  p2g->generate(); // self detects prior generation
  answer = p2g->run(rof1,rof2,vphase,vselect);
  if (answer > ticks)
     abort_message("TGroup - AT must be greater than pw+rof1+rof2");
  // turn ticks into remaining time for others
  delta = ticks - answer;
  if (ticks > 0)
    for (i=0;i<k;i++)
    {
      l = tRFG->rfchannelIndexList[i];
      p2rfc = P2TheConsole->getRFControllerByLogicalIndex(l);
      p2rfc->setTickDelay(delta);
    }
  P2TheConsole->update4Sync(ticks);
}

void GroupPhaseStep(double value)
{
  int i,k,l;
  RFChannelGroup *tRFG;
  tRFG = P2TheConsole->theRFGroup;
  k = tRFG->numberInGroup;
  for (i=0;i<k;i++)
    {
      l = tRFG->rfchannelIndexList[i];
      definePhaseStep(value,l);
    }
}

void GroupXmtrPhase(int rtvar)
{
 int i,k,l;
  RFChannelGroup *tRFG;
  tRFG = P2TheConsole->theRFGroup;
  k = tRFG->numberInGroup;
  for (i=0;i<k;i++)
    {
      l = tRFG->rfchannelIndexList[i];
      genFullPhase(rtvar,l);
    }
}

void MagnusSelectFPower()
{
  double ftmp;
  MasterController *tmp =
            (MasterController *) P2TheConsole->getControllerByID("master1");
  P2TheConsole->newEvent();
  tmp->setActive();
  tmp->clearSmallTicker();
  ftmp = getval("gain");
  tmp->setFPowerMagnus((int)ftmp);
  P2TheConsole->update4Sync(tmp->getSmallTicker());
}

// speculative
// do NOT break this up..
// may not be nestable..

static void ifrt_GEN(int logic, int rtarg1, int rtarg2, int rtarg3)
{
  int buffer[4];
  buffer[0] = logic;
  buffer[1] = rtarg1;
  buffer[2] = rtarg2;
  buffer[3] = rtarg3;
  broadcastCodes(RT3OP,4, buffer);
  ifzero(rtarg3);
}

void ifrtGT(int rtarg1, int rtarg2, int rtarg3)
{
  ifrt_GEN(LE,rtarg1,rtarg2, rtarg3);
}

void ifrtGE(int rtarg1, int rtarg2, int rtarg3)
{
   ifrt_GEN(LT,rtarg1,rtarg2, rtarg3);
}

void ifrtLT(int rtarg1, int rtarg2, int rtarg3)
{
   ifrt_GEN(GE,rtarg1,rtarg2, rtarg3);
}

void ifrtLE(int rtarg1, int rtarg2, int rtarg3)
{
   ifrt_GEN(GT,rtarg1,rtarg2, rtarg3);
}

void ifrtEQ(int rtarg1, int rtarg2, int rtarg3)
{
   ifrt_GEN(NE,rtarg1,rtarg2, rtarg3);
}

void ifrtNEQ(int rtarg1, int rtarg2, int rtarg3)
{
   ifrt_GEN(EQ,rtarg1,rtarg2, rtarg3);
}

void hdwshiminit()
{
  MasterController *masterC = (MasterController *) P2TheConsole->getControllerByID("master1");
  masterC->armBlaf(2);
}

// real time gain entry point

void mgain(int spec, double dgain)
{
  long long dur;
  MasterController *masterC = (MasterController *) P2TheConsole->getControllerByID("master1");
  P2TheConsole->newEvent();
  masterC->setActive();
  dur = masterC->setRTGain(spec,dgain);
  P2TheConsole->update4Sync(dur);
}

void enableHDWshim()
{
  MasterController *masterC = (MasterController *) P2TheConsole->getControllerByID("master1");
  masterC->enableBlaf();
}
// set blank/unblank at the nextscan point.
// allows solids unblank and shared amplifier
void setAmpBlanking()
{
  if (P2TheConsole->RFShared > 0)
  ((RFController *) P2TheConsole->getRFControllerByLogicalIndex(OBSch))->unblankAmplifier();
}
// support for direct access user patterns
void userRFShape(char *name, struct _RFpattern *pa, int nsteps, int channel)
{
      if (dps_flag) return;
	 RFController *rfC = (RFController *)(P2TheConsole->getRFControllerByLogicalIndex(channel));
      rfC->registerUserRFShape(name, pa, nsteps);
}

void userDECShape(char *name, struct _DECpattern *pa, double dres, int mode, int steps, int channel)
{
      if (dps_flag) return;
	RFController *rfC = (RFController *)(P2TheConsole->getRFControllerByLogicalIndex(channel));
    rfC->registerUserDECShape(name, pa, dres, mode, steps);
}

// this generates a table 
void create_offset_list(double *da, int num, int channel, int listnum)
{
    if (dps_flag) return;
    RFController *rfC = (RFController *)(P2TheConsole->getRFControllerByLogicalIndex(channel));
    rfC->resolveFreqList(listnum,da,num);
} 

void voffsetch(int listid, int vvar, int channel)
{
   int ticks;
   P2TheConsole->newEvent();
   RFController *rfC = (RFController *)(P2TheConsole->getRFControllerByLogicalIndex(channel));
   rfC->setActive();
   ticks = rfC->useRTFreq(listid,vvar);
   rfC->noteDelay(ticks);
   P2TheConsole->update4Sync(ticks);
}

void parallelstart(const char *chanType)
{
   if ( ! strcmp(chanType,"obs") )
      P2TheConsole->parallelEvents("rf", OBSch);
   else if ( ! strcmp(chanType,"dec") )
      P2TheConsole->parallelEvents("rf", DECch);
   else if ( ! strcmp(chanType,"dec2") )
      P2TheConsole->parallelEvents("rf", DEC2ch);
   else if ( ! strcmp(chanType,"dec3") )
      P2TheConsole->parallelEvents("rf", DEC3ch);
   else if ( ! strcmp(chanType,"dec4") )
      P2TheConsole->parallelEvents("rf", DEC4ch);
   else if ( ! strcmp(chanType,"grad") )
      P2TheConsole->parallelEvents("grad", 1);
   else if ( ! strcmp(chanType,"rcvr") )
      P2TheConsole->parallelEvents("rcvr", 1);
   else
      abort_message("parallelstart must be called with obs, dec, dec2, dec3, dec4, grad, or rcvr");
}

double parallelend()
{
   return(P2TheConsole->parallelEnd());
}

void parallelsync()
{
   P2TheConsole->parallelSync();
}


void parallelshow()
{
   P2TheConsole->parallelShow();
}
