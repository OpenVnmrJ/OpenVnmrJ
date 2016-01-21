/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef CONSOLE_H
#define CONSOLE_H

#include "Controller.h"
#include "RFController.h"
#include "MasterController.h"
#include "GradientController.h"
#include "PFGController.h"
#include "LockController.h"
#include "DDRController.h"
#include "InitAcqObject.h"

#define LOOP_TICKER         1
#define IFZ_TRUE_TICKER     2
#define IFZ_FALSE_TICKER    3
#define NOWAIT_LOOP_TICKER  5
#define RL_LOOP_TICKER      6

#define SINGRCVR_SINGACQ    1
#define MULTNUCRCVR_SINGACQ 2
#define MULTNUCRCVR_MULTACQ 3
#define MULTIMGRCVR_SINGACQ 4
#define MULTIMGRCVR_MULTACQ 5
/* RF channel where LO is switched into 2nd Receiver */
#define RCVR2_RF_LO_CHANNEL 3


const int MAXCHAN = 32;
const int OPT_DMAXFER_WORDS = 8192;

// Feature Sets not yet integrated..
// Select via Feature Set - choose matching controller..

/* get getController should change to a channel scan for the Feature/Assignment
   desired ... returns a controller so Bridge code should be transparent...
   internals of console need to migrate.. */

class Console
{
 public:
  Controller *PhysicalTable[MAXCHAN];
  RFController *RFUserTable[MAXCHAN];
  Controller *GRADUserTable;
  Controller *PFGUserTable;
  Controller *DDRUserTable[MAXCHAN];
  RFChannelGroup *theRFGroup;
  int nValid;
  int nActive;
  int numRFChan;
  int numDDRChan;
  int numActiveRcvrs;
  int firstActiveRcvrIndex;
  int numGRDChan;
  char configName[400];
  char configString[1000];
  char rfChannelStr[64];
  int RFShared;
  int RFSharedDecoupling;
  int channelBitsConfigured[2];
  int channelBitsActive[2];
  Console(const char *nm);
  int getNumberControllers();
  int RFConfigMap();
  int addController(Controller *mine);
  RFController *getRFControllerByLogicalIndex(int k);
  Controller *getDDRControllerByIndex(int k);
  Controller *getFirstActiveDDRController();
  Controller *getGRADController();
  Controller *getPFGController();
  Controller *getControllerByID(const char *key);
  GradientBase *getConfiguredGradient();
  InitAcqObject *initAcqObject;
  InitAcqObject *getInitAcqObject();

  void build(const char *name);
  void getChannelBits(int *channelbits);
  char getConsoleSubType();
  int  getConsoleRFInterface();
  char getConsoleHFMode();
  int  getConsoleHBMask();
  int  getConsoleCryoUsage();
  void setChannelMapping();
  void initializeExpStates(int setupflag);
  void postInitScanCodes();
  void getrfChannelStr();
  int  getIndexToRFChannel(char* name);
  void describe(char *pntr);
  void describe2(char *pntr);
  void newEvent();
  void update4Sync(long long tickCount);
  void update4SyncNoteDelay(long long tickCount);
  void resetSync();
  void flushAllEvents();
  int verifySync(const char *lbl);
  int getNumActiveRcvrs();
  void setNumActiveRcvrs(int numactive);
  void setAcodeBuffers();
  void setWaveformBuffers();
  /* void startDDRs(); */
  int turnOffRFDuplicates();
  int getFirstActiveRcvr();
  long long getMinTimeEventTicks();

  /* Power monitoring related */
  void showPowerIntegral();
  void showEventPowerIntegral(const char *);
  void suspendPowerCalculation();
  void enablePowerCalculation();

  void eventStartAction();

  /* following are Acode related */
  void broadcastCodes(int code, int many, int *stream);
  void broadcastSystemSync(int many, int prepflag);
  void flushAllControllerDload();
  void closeWaveformFiles();

  void closeAllFiles(int num);

  int  syncCompVar(int modTicks, int myTicks);
  void setLogicalParamNames();
  void printSyncStatus();
  int cleanConfigString();
  double duration();
  void checkForErrors();

  /* following are obs/decprg nested with loops, ifzero, vdelay related */
  void startTicker(int);
  long long stopTicker(int);
  long long  getTicker(int);
  int  getNestedLoopDepth();
  void setProgDecInterLock();
  void clearProgDecInterLock();
  int  getProgDecInterLock();

  void setAuxHomoDecTicker(long long);

  char getRcvrsType();
  void setRcvrsConfigMap();
  int  getRcvrsConfigMap();

  /* parallel events */
  void parallelEvents(const char *chanType, int chanIndex);
  double parallelEnd();
  int  isParallel();
  void parallelElapsed( const char *name, long long ticks);
  void parallelSync();
  void parallelShow();
  Controller *getParallelController();
  void setParallelInfo(int count);
  int getParallelInfo();
  void setParallelKzDuration(double duration);
  double getParallelKzDuration();


  private:
   long long HostSystemMemory;
   static char obs[16][8] ;
   char obsStr[16][8] ;
   char decStr[16][8] ;
   char dec2Str[16][8] ;
   char dec3Str[16][8] ;
   char dec4Str[16][8] ;
   int posIndex(char *a,char *b);
   int next_free();
   int clean(int ch);
   int usedMask;
   int Xunused;
   char consoleSubType;
   int  consoleRFInterface;
   char consoleHFmode;
   int  consoleHBmask;
   int  consoleCryoUsage;
   int MinTimeEventTicks;
   /* following are obs/decprg nested with loops, ifzero, vdelay related */
   long long loop_Ticker;
   long long ifz_True_Ticker;
   long long ifz_False_Ticker;
   long long nowait_loop_Ticker;
   int  nestedLoopDepth;
   int  progDecInterLock;
   char rcvrsType;
   int receiverConfig;
   int parallelIndex;
   double parallelKzDuration;
   int parallelInfo;
   long long parallelTicks[MAXCHAN];
   int parallelSyncPos[MAXCHAN];
   Controller *parallelController;
   int countSpecificChars(char *string, char character);
   void getSystemRamSize();
   int chk4EnoughHostBufferMemory();
};

extern Console *P2TheConsole;
#endif

