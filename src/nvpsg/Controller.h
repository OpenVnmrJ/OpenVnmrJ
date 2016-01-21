/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INC_CONTROLLER_H
#define INC_CONTROLLER_H

#include <iostream>
using namespace std;
#include <fstream>
#include "AcodeManager.h"

// this should be the start of ControllerReg.h
#define BREAKDELAY (0.8388)
#define SUBDELAY   (0.8000)
#define TIMERLIMIT 0x4000000
#define MINTIMERWORD (4)
#define BIGTIMERWORD 0x3800000

/*  from aptable.h */
#define BASEINDEX 1024

/* names of various acode and waveform stages */
#define ACODE_INIT 0
#define ACODE_PRE  1
#define ACODE_PS   2
#define ACODE_POST 3
#define WFG_FLAG   4

#define POWER_ACTION_IGNORE 0
#define POWER_ACTION_WARN   1
#define POWER_ACTION_LIMIT  2
#define POWER_ACTION_DEBUG  4
#define POWER_ACTION_TELL   8
#define POWER_ACTION_RTLOOP  16

/* max level of ifzero nesting */
#define MAXIFZ 5

/* max number of nested loops */
#define NESTEDLOOP_DEPTH 10

/* maximum number of waveforms */
#define MAX_PATTERN_COUNT	4096

/* maximum waveform size warn at 30MB */
#define MAX_PATTERN_SIZE	0x1E00000
//
// cPatternEntry is a pattern data base generic to
// RF / Gradient / PFG needs...
// implemented
// Phil Hornung 15 Aug 2003
//
class cPatternEntry
{
   private:
      char name[400];
      int  keys;
      // NYI - for SAR etc...
      double powerFraction;
      int  numberWords;
      int  numberStates;

      long long totalDuration;
      long long durationWord;
      int  numReplications;
      static int patID;
      int referenceID;
      cPatternEntry *pNext;
      int lastElement;
   public:

      cPatternEntry();
      cPatternEntry(const char *nm, int nW, int nT, int ky);
//
// CAUTION the  following constructor with refID arg is only for
// writing pattern alias file and not for general use
      cPatternEntry(const char *nm, int ky, int refID, int nstates, int nW);
      cPatternEntry(cPatternEntry *orig);

      cPatternEntry *match(const char *nm, int ky);
      cPatternEntry *matchID(int id);
      void tell();
      int  getPatID();
      void setNumberWords(int num);
      void setNumberStates(int num);
      void setPowerFraction(double fract);
      // count of 32 bit words produces..
      int getNumberWords();
      // count of number fifo loads
      int getNumberStates();
      int getReferenceID();
      void setLastElement(int element);
      int getLastElement();
      double getPowerFraction();
      long long getTotalDuration();
      long long getDurationWord();
      void setTotalDuration(long long ticks);
      void setDurationWord(long long ticks);
      void setNumReplications(int rep);
      int  getNumReplications();

      void setNext(cPatternEntry *next);
      int allocatePatID();
};
//
class PowerMonitor
{
  private:
   int enablePowerCalc;
   char message[256];
   char message2[256];
   double energyMax;
   double energyAccumulated;
   double eventStartEnergyAccumulated;
   double mainPowerFactor; // tpwr or 1.0 for gradients.
   double finePowerFactor; // tpwrf or gradient demand
   double gateONOFF;       // tx gate or 1.0 for gradients.
   double mainCalibrate;   // just a scale factor
   double fineCalibrate;   // weight of a pattern
   double alarmLevel;      // threshold for alarm abort.
   double timeConstant;    // leakage rate
   long long eventStart;   // internal variable
   long long incrementMarker;   // internal variable - unused
   double energyAtScanStart;
   double markerAtScanStart;
   double energyAtLoopStart[16]; 
   double markerAtLoopStart[16]; 
   double exptTimeCorrStack[17];
  public:
   int  trip_action;
   PowerMonitor(double timeconstant,double alarmlevel,const char *emsg);
   double calculateEnergyPerInc(long long now);
   void eventFinePowerChange(double fine,long long now);
   void eventCoarsePowerChange(double coarse,long long now);
   void eventFineCalibrateUpdate(double frac,long long now);
   void setMainCalibrate(double xx);
   void setMainPowerFactor(double xx);
   void eventGateChange(int gate,long long now);
   void setAlarmLevel(double threshold,int action, const char *emsg);
   void setTimeConstant(double tc);
   void setGateONOFF(double val);
   double getPowerIntegral();
   double getEventPowerIntegral();
   void addToPowerIntegral(double);
   double scalePower4Loops(double Estart, double Eend, double loops, double delta);
   void checkAlarmCondition();
   double getAlarmRatio();
   void initialize(long long now);
   long long getMarker();
   void print(const char *heading);
   void saveStartOfScan(double t);
   void computeEndOfScan(double ntv,double t);
   void saveStartOfLoop(int lvl, double t);
   void computeEndOfLoop(int lvl, double lps, double t);
   void eventStartAction();
   void enablePowerCalculation();
   void suspendPowerCalculation();
};
//
//
//  Controller has common functionality for the
//  various kinds of channels which inherit it.
//
//
//  usage values..
//  OFF-Controller is present but NOT Timing
//  ACTIVE - i/f routine performs detailed timing
//  SYNCHRONOUS - Console need to add delays to keep Sync
//  ASYNCHRONOuS -  Delays need to be noted to time status
//                  see progDecOn/Off
//
//  implemented and preliminary testing except for start Exp  and start Inc
//
#define OFF  (1)
#define ACTIVE (2)
#define SYNCHRONOUS  (3)
#define ASYNCHRONOUS (4)
#define NOWAIT_MODE  (5)

// these are internal def s for the kind field
// .. kind is protected with use isMaster() etc...
//
#define GENERIC_TAG    (0)
#define MASTER_TAG     (1)
#define RF_TAG         (2)
#define GRAD_TAG       (3)
#define DDR_TAG        (4)
#define LOCK_TAG       (5)
#define PFG_TAG        (6)

//
//
//
class Controller
{
   private:
     static int SeqID;
     int UniqueID;
     int  outputControlFlag;
     ifstream ifs;
     ofstream ofs;
     ofstream PatternOutStream;
     int dataWordBuffer[400];
     int dataWordCount;
     AcodeManager *pAcodeMgr;
     cPatternEntry *pHook;
     int rtStack[100];
     int rtStackCounter;
     int ifzLvl;
     int ifzVvar[MAXIFZ];
     int ifzPosIf[MAXIFZ];
     int ifzPosElse[MAXIFZ];
     int loopLvl;
     int loopPos[NESTEDLOOP_DEPTH];

   protected:
     int  kind;
     char Name[200];
     // defunct void setAuxI(int word);
     int usage;
     int *patternDataStore;
     int patternDataStoreSize;
     int patternDataStoreUsed;
     int patternCount;	  // total number of patterns
     int patternSize;     // total size of patterns in bytes
     long long bigTicker;
     long long smallTicker;
     long long NOWAIT_eventTicker;
     AcodeBuffer *pAcodeBuf;
     AcodeBuffer *pWaveformBuf;
     int channelActive;
     int writeAcodes(int many, int *stream);

   public:

   Controller();
   Controller(const char *nm, int num);
   virtual ~Controller() {};
   void describe(const char *what);
   int  matchID(const char *nn);
   char* getName();
   void setAcodeBuffer();
   void setWaveformBuffer();
   AcodeBuffer * setAcodeBuffer(const char *type);
   void setActive();
   void setOff();
   void setSync();
   void setAsync();
   void setNOWAITMode();
   void resetSync();
   void incr_NOWAIT_eventTicker(long long ticks);

   int isActive();
   int isOff();
   int isSync();
   int isAsync();
   int isNoWaitMode();
   //
   int isMaster();
   int isRF();
   int isDDR();
   int isGradient();
   int isLock();
   int isPFG();
   void setOutputControl(int j);

   // these two are
   // not yet implemented...
   void startExp(const char *label, int tmp);
   void startIncrement(int incNum, int ss, int bs, int nt);
   virtual int outputACode(int Code, int many, int *codes);
   // puts into buffer....
   void flushDload();

   int  openAcodeFile(int options);
   int  closeAcodeFile(int options);
   int  closeWaveformFile(int options);
   int  getAcodePosition();
   int  pushAcodePosition();
   int  popAcodePosition();
   int  getWfgBufPosition();
   void writeIncrement(int options);
   //
   void setAux(int word);
   int findPatternFile(/*const*/ char *filename);
   // 1 if OK, 0 is error private ifstream resolvent min, double fracError, char *emsg, int action)
   int sendPattern(cPatternEntry *tmp);
   void executePattern(int id);
   void executePattern(int id, int npasses, int remstates, int remticks);
   int scanLine(double *pf1, double *pf2, double *pf3, double *pf4, double *pf5);
   int getTotalPatternCount();
   int getTotalPatternSize();
   int channelCount();
   int add2Stream(int dataWord);
   long long calcTicks(double xx);
   // open questions long long or int -  Patterns are not yet multi word ..
   long long calcTicksPerState(double xx, int nstates, int min, double fracError, const char *emsg, int action);
   //  long long...
   virtual int setTickDelay(long long ticks);
   virtual int setDelay(double dd);
   virtual void setAsyncDelay(double dd);
   void setVDelay(int rtvar, int factor);  //
   virtual void setVDelayList(cPatternEntry *cPattern, int *list, int nvals);
   void doVDelayList(int listId, int rtvar);

   // virtual power monitor related
   virtual void powerMonLoopStartAction();
   virtual void powerMonLoopEndAction();
   virtual void showPowerIntegral();
   virtual void showEventPowerIntegral(const char *);
   virtual void PowerActionsAtStartOfScan();
   virtual void computePowerAtEndOfScan();
   virtual void resetPowerEventStart();
   virtual void eventStartAction();
   virtual void enablePowerCalculation();
   virtual void suspendPowerCalculation();

   // real time branch control
   void ifzero(int rtvar);
   void ifmod2zero(int rtvar);
   void elsenz(int rtvar);
   void endif(int rtvar);
   void loopStart();
   void loopEnd();
   void loopEnd(int count);
   int getLoopLevel();

   virtual void SystemSync(int num);        //  master will override...
   virtual void SystemSync(int num, int prepflag);        //  master will override...
   void noteDelay(long long ticks);
   long long getBigTicker();
   long long getSmallTicker();
   void clearSmallTicker();
   void update4Sync(long long ticks);
   void update4ParallelSync(long long ticks, int acodePos);
   void addPattern(cPatternEntry *pat);
   void registerPatternFromArray(cPatternEntry *pat, unsigned int *array);
   cPatternEntry *find(const char *nm, int ky);
   cPatternEntry *findByID(int id);
   void setSW_Int(int word); // int safeStateGates;
   virtual int initializeExpStates(int setupflag);
   //int postInitScanCodes();
   virtual int initializeIncrementStates(int num);
   virtual int endExperimentState();
   virtual void armBlaf(int arg);
   virtual void clearBlaf();
   void setChannelActive(int state);
   int getChannelActive();
};

#endif
