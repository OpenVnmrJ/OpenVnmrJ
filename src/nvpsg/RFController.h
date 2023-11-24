/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INC_RFCONTROLLER_H
#define INC_RFCONTROLLER_H

#include "Controller.h"
#include "Synthesizer.h"
//
// Gate definitions in FFKEYS.h
//
//
//   all of these are implemented..
//
typedef struct _RFpattern {
    double  phase;
    double  phase_inc;
    double  amp;
    double  amp_inc;
    double  time;
} RFpattern;

typedef struct _DECpattern {
    double  tip;
    double  phase;
    double  phase_inc;
    double  amp;
    double  amp_inc;
    int gate;
} DECpattern;

typedef struct _Gpattern {
    double  amp;
    double  time;
    int ctrl;
} Gpattern;

class Amp
{
 public:
   double amppowermax;
   char ampT[256];
   Amp(int slot);
};

class RFController: public Controller
{
 private:
     // there probably should be some duty cycle
     // things here...
     static double h1freq;
     static double systemIF;
     static double xmtrHiLowFreq;
     static double ampHiLowFreq;

     cPatternEntry *p2Dec;
     int *increaseDataStore(int *origbuffer,int origsize, int finalsize);
     int observeFlag;
     int decTicks;
     int progDecInterLock;
     long long progDecMarker;
     long long auxHomoDecTicker;
     int blank_is_on;
     int unblankCalled;
     double defAmp;
     double defAmpScale;
     double defVAmpStep;
     double defPhase;
     double defPhaseCycle;
     double currentPhaseStep;  // for legacy small angle handling..
     double currentVAmpStep;
     double baseFreq;
     double baseOffset;
     double baseTpwr;
     double baseTpwrF;
     double currentFreq;
     double currentOffset;
     long long trs;
     int RF_pre_gates;
     int RF_pulse_gates;
     int RF_post_gates;
     int gateDefault;
     int userGates;
     int rfIsEnabled; //
     Synthesizer mySynthesizer;
     PowerMonitor powerWatch;
     char coarsePowerName[100];
     char finePowerName[100];
     char finePowerName2[100];
     char tunePwrName[100];
     char freqName[100];
     char freqOffsetName[100];
     char dmName[100];
     char dmmName[100];
     char dmfName[100];
     char dresName[100];
     char dseqName[100];
     char homoName[100];
     char nucleusName[100];

     int logicalID;
     int prevStatusDmIndex;
     int prevStatusDmmIndex;
     int prevSyncType;
     int prevAsyncScheme;
     int CW_mode;
     int *patAliasList;
     int  patAliasListSize;
     int  patAliasListIndex;
     double maxAllowedUserPower;

     void outputMapTables(int &index);

    public:
     char logicalName[256];
     int rfSharingEnabled;
     //  this is the constructor...
     RFController(char *name,int flags);

     //  low level drivers...
     void setMaxUserLevel(double lvl);
     void setPowerFraction(double pwrf);
     void setAmpScale(double scale);
     void setVAmpScale(double scale, int rtvar);
     void setVAmpStep(double stepsize);
     void setAmp(double amp);
     void setPhaseCycle(double phase);
     void setPhase(double phase);
     void setVQuadPhase(int rtvar);
     void setVFullPhase(int rtvar);
     int degrees2Binary(double xx);
     // use ONLY with RT elements
     int degrees2XBinary(double xx);
     int amp2Binary(double xx);

     void setPhaseStep(double value);
     void setGates(int GatePattern);
     //  common packages of events...
     void pulsePreAmble(double rof1, int Vvar4Phase);
     void pulsePostAmble(double rof2);
     void pulsePreAmble(long long ticks, int Vvar4Phase);
     void pulsePostAmble(long long ticks);
     void unblankAmplifier();
     void blankAmplifier();
     int  getBlank_Is_On();
     void setTxGateOnOff(int state);
     void setStatus(char ch);
     //  decoupler stuff...
     //  probably explicit status functions...
     void progDecOn(char *name, double pw_90, double deg_res, int mode, const char *emsg);
     void progDecOnWithOffset(char *name, double pw_90, double deg_res, double foff, int mode, const char *emsg);
     void progDecOff(int decPolicy, const char *emsg, int syncType, int asyncScheme);
     cPatternEntry *resolveRFPattern( char *nm, int flag, const char *emsg, int action);
     cPatternEntry *resolveDecPattern(char *nm, int flag, double pw90, int wfgdup, int mode, const char *emsg);
     cPatternEntry *resolveHomoDecPattern(char *nm, int hdres, double pw_90, int dwTicks, int rfonTicks, long long acqtimeticks, int rof1Ticks, int rof2Ticks, int rof3Ticks);
     cPatternEntry *resolveSWIFTpattern(char *nm, int dwTicks, int rfonTicks, long long acqtimeTicks, int preacqTicks);
     cPatternEntry *resolveDecOffsetPattern(char *nm, int flag, double pw_90, double offset,
                                            int mode, const char *emsg);
     cPatternEntry *resolveFreqList(int lnum, double *offsetArray, int nfreqs);
     int useRTFreq(int lnum, int vvar);
     int setFrequency(double freq);  // Ticks to perform returned...
     // in hz ... at the user level .. internal to MHZ.
     void setOffsetFrequency(double offset);
     void setBaseFrequency(double bfreq);  //
     double getCurrentFrequency();
     double getBaseFrequency();
     double getOffsetFrequency();
     double getCurrentOffset();
     void   setCurrentOffset(double offset);
     double getDefAmp();
     int getAttenuator(double attn); // return AUX code for tpwr value
     int setAttenuator(double attn); // tick to perform..
     int setAttenuatorICAT(double atten);
     int setAttenuatorVnmrs(double atten);
     int computeAttenuatorVnmrs(double atten);
     int computeAttenuatorICAT(double atten);
     void setRFInfo(double freq);
     int setSwitchOnAttenuator(int state);
     int initializeExpStates(int setupflag);
     //  int postInitScanCodes(); - mapped into below
     int initializeIncrementStates(int num);
     int writeDefaultGates();
     int doStatusPeriodActions(int index);
     int getLogicalID();
     void getNucleusName(char *nucname);
     void setLogicalNames(int logicID, char RFnames[][8]);
     void resolveEndOfScanStatusAction(int current, int first);
     void setHiLowFreq(double hlf);
     double getHiLowFreq();
     void setObserve(int onFlag);
     int isObserve();
     void setSystemIF(double value);
     double getSystemIF();
     void setCWMode(int on_is_1);
     int getCWmode();
     void setUserGate(int line, int on_or_off);
     // experimental..
     long long gslx(int listId, double pw, int rtvar, double rof1, double rof2, char mode, int vvar, int on);
     void VPhaseStep(double value,int rtvar);
     cPatternEntry *makeOffsetPattern(char *nm, int nfast, double dphase, double bphase, int flag, char mode,
                                      const char *tag, const char *emsg, int action);
     int rRFOffsetShapeList(char *baseshape, double pw, double *offsetList, double frac ,double basePhase, double num, char mode);
     int analyzeRFPattern(char *nm, int flag, const char *emsg, int action);
     void addToPatAliasList(int newpat, int oldpat);
     int getPatternAlias(int newpatid);
     void writePatternAlias();
     int get_RF_PRE_GATES();
     int get_RF_PULSE_GATES();
     int get_RF_POST_GATES();
     void loopAction();
     int registerUserRFShape(char *name, struct _RFpattern *pa, int steps);
     int registerUserDECShape(char *name, struct _DECpattern *pa, double dres, int mode, int steps);
     void setPowerAlarm(int op,  double pwrc,  double alarm, double tc);
     void powerMonLoopStartAction();
     void powerMonLoopEndAction();
     void PowerActionsAtStartOfScan();
     void computePowerAtEndOfScan();
     void showPowerIntegral();
     void showEventPowerIntegral(const char*);
     void enablePowerCalculation();
     void suspendPowerCalculation();
     void eventStartAction();
     void resetPowerEventStart();
     void setTempComp(int t);
     void initAmpLinearization(char *);
     void markGate4Calc(int on);
     void pprobe(const char *);
     void adviseFreq(int, int*);
     int getProgDecInterlock();
     long long getAuxHomoDecTicker();
     void setAuxHomoDecTicker(long long);
};

// RF pulse group support. keep small for debugging..


class RFSpecificPulse
{
 public:
  RFController *p2rfc;
  int rfChanID;
  double tpwr, tpwrf;
  int onFlag;
  long long myDuration; // if needed..
  int numbInArray;
  double startPhase;
  char shapeName[256];
  double *freqA;
  char mode[4];  //
  int NFreq;
  double pw;
  double offset;
  int listID;    // gets set by generate..
  RFSpecificPulse(int rfChanNum, int onOff, char mode, char *name, double pwidth, \
                  double cpwr, double fpwr, double sphase, double *Freqarray, int nF);
  int generate();
  long long execute(double r1, double r2, int rtphase, int rtselect);
  void tell(char *emsg);
};

#define MAXINGROUP (8)

class RFGroupPulse
{
 public:
  RFGroupPulse *next;
  static int tracker;
  int myNum;
  int numberInGroup;
  int isGend;
  RFSpecificPulse *pulseArray[MAXINGROUP];
  RFGroupPulse(int numInG);
  // num is 0 to numInG - 1
  int addSpecificPulse(RFSpecificPulse *p2spc, int num);
  // number is 1 to numInG user nomenclature.
  RFSpecificPulse *getControllerData(int number);
  int generate();
  long long run(double r1, double r2, int vvar1, int vvar2);
  int append(RFGroupPulse *p);
  void tell(char *emsg);
};

class RFChannelGroup
{
  private:
   RFGroupPulse *pulsell;
  public:
  int numberInGroup;
  int pulsesCreated;
  int rfchannelIndexList[128];
  int numberOfMaster;
  RFChannelGroup(char *configStr);
  int addPulse(RFGroupPulse* p2myPulse);
  RFGroupPulse *getPulse(int num);
  RFSpecificPulse *getSpecific(int pulsen, int con);
  void tell(char *emsg);
};
#endif
