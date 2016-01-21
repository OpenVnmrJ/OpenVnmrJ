/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INC_MASTERCONTROLLER_H
#define INC_MASTERCONTROLLER_H
#include "Controller.h"

//
// selector in back of console hooked to master via aux.
//
class LO_Selector
{ 
  private:
  int code;
  public:
  LO_Selector();
  int select_LO(int which);
  int select_LO_A56(int which);
  int getCode();
};

class TR_Selector
{
  private:
    int code;
  public:
    TR_Selector();
    int select_TR(int which);
    int getCode();
};

class NSR
{
 private:
   unsigned int NSR_words[16];
   unsigned int gainsetting[4];
   int myNum;
  
 public:
   NSR();
   //NSR(int i);
   void setNum(int i); // puts in the chipselect..
   int programBitField(int word, int top, int bot, int value);
   void setPreAmpSelect(int pat);
   void setTRSelect(int pat);
   void setNarrowBand(int pat);
   void setTuneModeSelect(int pat);
   void setMixerHighBand(int mixer,int pat); 
   void setMixerGain(int mixer, int pat);
   void setGain(int mixernum, int gain);
   void setMixerInput(int mixer,int pat);
   void setLockAtten(int pat);
   void setLockHiLow(int pat);
   void setDiplexRelay(int pat);
   void setPreSig(int pat);
   void setCoilBias(int pat);
   void setPP2HFBypassSwitch(int pat);
   void setPP2HFCombinerSwitch(int pat);
   void setPP2Ch3TuneSelect(int pat);
   void setPP2HFandCryoInterface();
   int getGainSetting(int mixernum);
   int getNSRWord(int index);
};
//
// -- local encodes for convenience.
//
#define MULTI_RECEIVER  1
#define MULTI_NUCLEUS   2
#define PRESIGBIT       4
#define COILBIASBIT     8
#define LOCKATTENBIT    16
#define HBLOCKBIT       32
#define VOLUMERCVRBIT	64
#define IMGWLOCKBIT	128
// etc.
#define PACK4TUNE       1
#define MAXNUMNSRS      (8)
// 0 to 7 32 receivers
#define AUXCNTRLNUM      (8)

class TR_INTERCONNECT
{
  public:
  LO_Selector the_LO_Selector;
  TR_Selector the_TR_Selector;
  NSR the_NSR_Array[MAXNUMNSRS];
  NSR auxNSRControl;
  int numNSRS;
  int numRcvrs;
  int modeFlags;
  int usageFlag;
  int gain[4];
  int observe1ch;
  int observe2ch;
  int hibandflag;
  int lockflag;
  int preAmpMask;
  // max 2 multinuclear...
  char rcvrS[64]; 
  // end refactor...
  TR_INTERCONNECT();
  // new calls
  void setSingleReceiver();  // Normal operation
  void setA56Receiver();     // ProPulse configurations
  void setA56_3ch_Receiver();     // ProPulse configurations
  void setMultiNucSingle();  // Normal operation on a Multi-nuclear system
  void setMultiNuc2();       // 2 receiver operation on a Multi-nuclear system.
  void setArrayedRcvrs();    // phased array & multi-mouse
  // end new calls
  // changes arg list
  int initialize(/*int numReceivers, char *c, double gain, int i, int ModeFlag, int hiband, int lkflag*/); // maps out but no output.
  int setRTGainData(int gain);  // returns numRcvrs..
  int setRTGainOneData(int gain, int choice); // returns a single word..
  int getGainCode(int rcvrNum);
  int packOutputBuffer(int *bufferp, int flag);
  void setBankGainCode(int igain, int nsrNum);
  int getBankGainCode(int nsrNum);
  void tell();
};

  
/*
methods
set Tune
set LockObserve
set Lockstuff hi/low lock atten 
set Observe
set preSig
set Gain all/individual.
set attached premap on/off.. 
coil bias .. 
make an NSR array and a active mask?? or just pump
tr goes to which preamp(s)..
*/
class MasterController:public Controller
{
   private:
     // there probably should be some duty cycle 
   double ReceiverGainDefault;
   int numberReceivers;
   double spinRate;
   int spinFlag;
   double Temp;
   int tempFlag;
   int   codes[54];	// 48 shims + 1 count + extra
   //int TRSelectCode;
   //int LOSelectCode;
   int LKMode;
   int Extra;
   int XYZgradflag;
   int shimset;
   int bound(char, int, int);
   int blafMode;
   
   public:
     MasterController(const char *name,int flags);
     // NSR myRouter;
     //  this is the constructor...
     TR_INTERCONNECT trInterconnect;
     void setGates(int GatePattern);
     void wait4XGate(int count);
     void SystemSync(int num);        //  master will override...
     void SystemSync(int num, int prepflag);        //  master will override...
     void setNoDelay(double dur); //SAS
     void giveSyncSignal();
     void setShimInRT(int shimID, int shimVal);
     // these are NYI !!! 
     void setStatus(char ch);   // master might need this??
     // lock TC & lock S/H  wait4VT insert/eject 
     //
     void rotorSync(int arg1, int arg2);
     int xgate(int counts);
     int RT_spi(int number, int value);
     int set4Tune(int channel, int selectchan, int gain, int hilow);
     int set4TuneA56(int channel, int gain, int hilow, int cryo);
     int set4TuneA56_3ch(int channel, int gain, int hilow, int cryo);
     int setFPowerMagnus(int gain);
     int setRTGain(int mixer,double gain);
     void vtOps(int what);
     void spinOps(int what);
     void insetEjectOps(int what);
     void setConsoleMap(int kind, int obsch, int decch, int hilow, int lkflag);
     int initializeExpStates(int setupflag);
     int initializeIncrementStates(int total);
     int endExperimentState();
     void RollCall(char *configString);
     void homospoilGradient(char, double);
     void shimGradient(char,double);
     int * loadshims(void);
     // setDiplexerRelay is real time element
     void setDiplexerRelay(int state);
     void enableBlaf();
     void armBlaf(int arg);
     void clearBlaf();
     void setVDelayList(cPatternEntry *cPattern, int *list, int nvals);
     cPatternEntry *createVDelayPattern(int *list, int nvals);
};

#endif
