/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <iomanip>
#include <fstream>
#include <math.h>
#include "cpsg.h"
#include "FFKEYS.h"
#include "ACode32.h"
#include "Console.h"
#include "Controller.h"
#include "PSGFileHeader.h"

extern "C" {
#include "vnmrsys.h"
#include "vfilesys.h"
extern char curexp[];
extern int dps_flag;
}

extern int bgflag;
extern FILE *shapeListFile;
extern int shapesWritten;


#define WriteWord( x ) pAcodeBuf->putCode( x )

// phil hornung 15 august 2003
//
// if we wish to re-merge the pattern data and the acode data
// pattern out stream -> ofs...
// I have left an inline acode for this purpose.
//

int cPatternEntry::patID = 1;

cPatternEntry::cPatternEntry()
{
   strcpy(name,"none");
   keys=0; numberWords = 0;
   numberStates = 0;
   durationWord = 0L;
   totalDuration= 0L;
   numReplications = 1;
   lastElement=0;
   referenceID = patID++;
   pNext = NULL;
}
//
//  name - pattern Name...
//  ky - as yet unused key...
//  nstates - fifo words created...
//  nW  - pattern length in words..
//
cPatternEntry::cPatternEntry(const char *nm, int ky, int nstates, int nW)
{
   strcpy(name,nm);
   keys = ky;
   numberWords = nW;
   numberStates = nstates;
   durationWord = 0L;
   totalDuration= 0L;
   numReplications = 1;
   lastElement=0;
   referenceID = patID++;
   pNext = NULL;
}

//
// CAUTION
// the following constructor with refID arg is only for writing pattern alias file
// and not for general use
//
cPatternEntry::cPatternEntry(const char *nm, int ky, int refID, int nstates, int nW)
{
   strcpy(name,nm);
   keys = ky;
   numberWords = nW;
   numberStates = nstates;
   durationWord = 0L;
   totalDuration= 0L;
   numReplications = 1;
   lastElement=0;
   referenceID = refID;
   patID++;
   pNext = NULL;
}

cPatternEntry::cPatternEntry(cPatternEntry *orig)
{
   strcpy(name,orig->name);
   keys = orig->keys;
   numberWords = orig->numberWords;
   numberStates = orig->numberStates;
   durationWord = orig->durationWord;
   totalDuration= orig->totalDuration;
   numReplications = orig->numReplications;
   lastElement=orig->lastElement;
   referenceID = orig->referenceID;
   pNext = orig->pNext;
}

cPatternEntry *cPatternEntry::match(const char *nm, int ky)
{
   if (this == NULL)
     return(NULL);
   if(strcmp(name,nm))
   {
      if ( ! pNext )
         return(NULL);
      else
         return(pNext->match(nm,ky));
   }
   else
     return(this);
}


cPatternEntry *cPatternEntry::matchID(int id)
{
   if (this == NULL)
     return(NULL);
   if(referenceID == id)
     return(this);
   else
     return(pNext->matchID(id));
}


void cPatternEntry::setNumberWords(int num)
{
   numberWords = num;
}
//
void cPatternEntry::setNumberStates(int num)
{
   numberStates = num;
}

int cPatternEntry::getNumberWords()
{
   return(numberWords);
}

int cPatternEntry::getNumberStates()
{
   return(numberStates);
}

int cPatternEntry::getReferenceID()
{
  return(referenceID);
}

int cPatternEntry::allocatePatID()
{
  int value = patID;
  patID++;
  return(value);
}

int cPatternEntry::getPatID()
{
  return(patID);
}

long long cPatternEntry::getTotalDuration()
{
   return totalDuration;
}

long long cPatternEntry::getDurationWord()
{
   return durationWord;
}

void cPatternEntry::setTotalDuration(long long ticks)
{
   totalDuration = ticks;
}

void cPatternEntry::setDurationWord(long long ticks)
{
   durationWord = ticks;
}

void cPatternEntry::setNumReplications(int rep)
{
   numReplications = rep;
}

int cPatternEntry::getNumReplications()
{
   return numReplications;
}

void cPatternEntry::setNext(cPatternEntry *next)
{
   pNext = next;
}

void cPatternEntry::tell()
{
   cout << "Pattern Entry : " << name << "  " << referenceID << "  " << keys << endl;
   if (pNext != NULL)
     pNext->tell();
}

void cPatternEntry::setPowerFraction(double frac)
{
  powerFraction = frac;
}

double cPatternEntry::getPowerFraction()
{
  return(powerFraction);
}

void cPatternEntry::setLastElement(int value)
{
  lastElement = value;
}

int cPatternEntry::getLastElement()
{
  return lastElement;
}

//----

PowerMonitor::PowerMonitor(double tc, double al, const char *msg)
{
   strncpy(message,msg,256);
   strncpy(message2,msg,256);
   energyMax = 0.0;   // largest recorded value..
   energyAccumulated = 0.0;
   eventStartEnergyAccumulated = energyAccumulated;
   mainPowerFactor = 1.0;     //tpwr or 1.0 for gradients.
   finePowerFactor = 0.0; //tpwrf or gradient demand CHECK CHECK
   gateONOFF = 0.0;       //tx gate or 1.0 for gradients.
   mainCalibrate = 1.0;   //just a scale factor
   fineCalibrate = 1.0;   // weight of a pattern
   alarmLevel = -1.0;      // threshold for alarm abort.
   timeConstant = tc;    // leakage rate
   incrementMarker = 0l; //
   eventStart = 0L;   // internal variable
   trip_action = POWER_ACTION_IGNORE; // until checked
   energyAtScanStart=0.0;
   for (int i = 0; i < 16; i++) 
   {
      energyAtLoopStart[i]=0.0; 
      markerAtLoopStart[i]=0.0; 
      exptTimeCorrStack[i]=0.0;
   }
   exptTimeCorrStack[16]=0.0;
   enablePowerCalc=1;
}

// RF and PFG verified.
//
double PowerMonitor::calculateEnergyPerInc(long long now)
{
  double xx,yy,incPower,incDuration,expFactor;
  if (! enablePowerCalc) return(0.0);
  if (now <= eventStart) // nothing to do.
    return(energyAccumulated);
  xx = mainCalibrate*pow(10.0,mainPowerFactor/10);
  yy = finePowerFactor*fineCalibrate;
  incPower = xx*yy*yy; // now in power
  incDuration = ((double) (now - eventStart))* 0.0000000125L; //in seconds
  expFactor = exp(-incDuration/timeConstant);
  energyAccumulated *= expFactor;
  eventStart = now;
  if (gateONOFF)
  {
    if (timeConstant < 1000.0)
    {
      energyAccumulated += incPower*(1.0-expFactor)*timeConstant;
    }
    else
    {
      energyAccumulated += incPower*incDuration;
    }
  }
  if (energyAccumulated > energyMax) energyMax = energyAccumulated;
  //
   if (trip_action & POWER_ACTION_DEBUG)
   {
        cout << message << " E=" << energyAccumulated << " t=" << now*0.0000000125L << " incPwr=" << incPower << " Emax=" << energyMax << endl;
        text_message("%s: %f is accumulated energy / %f max",message,energyAccumulated,energyMax);
   }
   if (trip_action & POWER_ACTION_WARN)
   {
       if (energyAccumulated > 0.9*alarmLevel)
          text_message("Power * duration approaching/exceeding Limit for %s!",message);
   } 
   if (trip_action & POWER_ACTION_LIMIT)
   {
      if (energyAccumulated > alarmLevel)
          abort_message("Power * duration over Limit for %s!",message);
   }
  ///
  return(energyAccumulated);
}

void PowerMonitor::checkAlarmCondition()
{
   if (trip_action & (POWER_ACTION_DEBUG|POWER_ACTION_TELL))
   {
        text_message("%s: %f is accumulated energy / %f max at end of loop or increment",message,energyAccumulated,energyMax);
   }
   if (trip_action & POWER_ACTION_WARN)
   {
       if (energyAccumulated > 0.9*alarmLevel)
          text_message("Power * duration approaching/exceeding Limit for %s!",message);
   } 
   if (trip_action & POWER_ACTION_LIMIT)
   {
      if (energyAccumulated > alarmLevel)
          abort_message("Power * duration over Limit for %s!",message);
   }
}


void PowerMonitor::eventStartAction()
{
   eventStartEnergyAccumulated = energyAccumulated;
}

void PowerMonitor::eventFinePowerChange(double fine,long long ticks)
{
  double xx;
  if (! enablePowerCalc) return;
  if (fine == finePowerFactor)
    return;
  xx = calculateEnergyPerInc(ticks); //
  finePowerFactor=fine;
  return;
}

void PowerMonitor::eventCoarsePowerChange(double coarse,long long ticks)
{
  double xx;
  if (! enablePowerCalc) return;
  if (coarse == mainPowerFactor)
    return;
  xx = calculateEnergyPerInc(ticks); //
  mainPowerFactor= coarse;
  return;
}

void PowerMonitor::eventGateChange(int gate, long long ticks)
{
  double xx;
  if (! enablePowerCalc) return;
  if (gate == gateONOFF)
    return;
  xx = calculateEnergyPerInc(ticks); //
  gateONOFF = gate;
  return;
}

void PowerMonitor::eventFineCalibrateUpdate(double frac, long long ticks)
{
  double xx;
  if (! enablePowerCalc) return;
  if (frac == fineCalibrate)
    return;
  xx = calculateEnergyPerInc(ticks); //
  fineCalibrate = frac;
}

void PowerMonitor::setGateONOFF(double val)
{
    gateONOFF = val;
}

void PowerMonitor::setMainCalibrate(double xx)
{
  mainCalibrate = xx;
}

void PowerMonitor::setMainPowerFactor(double xx)
{
  mainPowerFactor = xx;
}

void PowerMonitor::setAlarmLevel(double threshold,int action, const char *emsg)
{
  alarmLevel = threshold;
  trip_action = action;
  strncpy(message2,emsg,256);
}

void PowerMonitor::setTimeConstant(double tc)
{
  timeConstant = tc;
}

double PowerMonitor::getPowerIntegral()
{
  return(energyAccumulated);
}


double PowerMonitor::getEventPowerIntegral()
{
  return(energyAccumulated-eventStartEnergyAccumulated);
}


void PowerMonitor::addToPowerIntegral(double value)
{
  if (! enablePowerCalc) return;
  energyAccumulated += value;
}


void PowerMonitor::enablePowerCalculation()
{
  enablePowerCalc=1;
}


void PowerMonitor::suspendPowerCalculation()
{
  enablePowerCalc=0;
}
//
// using the binomial theorem forecast the energy at the end
// of loops repeats. accumulates the power and tests the limit.
// returns the scaling factor if you are interested.
//

double PowerMonitor::scalePower4Loops(double Estart, double Eend,  double loops, double delta)
{
    double xx,yy,zz, e1, e2;
    if (! enablePowerCalc) return(0.0);

    e1 = Estart*exp(-(loops*delta)/timeConstant);

    double energyPerLoop = (Eend - Estart*exp(-delta/timeConstant));
    xx = exp(-delta/timeConstant);
    yy = pow(xx,loops);
    zz = (1.0-yy)/(1.0-xx);
    e2 = (zz)*energyPerLoop;

    energyAccumulated = e1 + e2;

    if (energyAccumulated > energyMax) energyMax = energyAccumulated;
    checkAlarmCondition();
    return(zz);
}


void PowerMonitor::initialize(long long now)
{
  eventStart = now;
}

void PowerMonitor::print(const char *heading)
{
  cout << heading << endl;
  cout << "Power Accumulated is " << energyAccumulated << endl;
  cout << "TimeConstant is " << timeConstant << endl;
}

double PowerMonitor::getAlarmRatio()
{
  if (alarmLevel > 0.0)
      return(energyAccumulated/alarmLevel);
  else
      return(0.0);
}

long long PowerMonitor::getMarker()
{
  return(incrementMarker);
}

void PowerMonitor::saveStartOfScan(double t)
{
   if (! enablePowerCalc) return;
   energyAtScanStart = energyAccumulated;
   markerAtScanStart = t;
}

void PowerMonitor::computeEndOfScan(double ntv, double t)
{
   if (! enablePowerCalc) return;
   double singleScanTime = (t-markerAtScanStart) + exptTimeCorrStack[0];
   scalePower4Loops(energyAtScanStart, energyAccumulated, ntv, singleScanTime);  
}

void PowerMonitor::saveStartOfLoop(int lvl, double t)
{
   if (! enablePowerCalc) return;
   double xx;
   long long now = (long long)(t/(0.0000000125L));
   xx = calculateEnergyPerInc(now);
   energyAtLoopStart[lvl] = energyAccumulated;
   markerAtLoopStart[lvl] = t;
}

void PowerMonitor::computeEndOfLoop(int lvl, double lps, double t)
{
   if (! enablePowerCalc) return;
   double singleLoopTime = t-markerAtLoopStart[lvl];
   exptTimeCorrStack[lvl] += singleLoopTime * (lps-1.0);
   scalePower4Loops(energyAtLoopStart[lvl], energyAccumulated, lps, (singleLoopTime+exptTimeCorrStack[lvl+1]) );  
}

// here is a channel not so basic ...

int Controller::SeqID= 0;

//
Controller::Controller()
{
    strcpy(Name,"generic");
    kind = GENERIC_TAG;
    usage = SYNCHRONOUS;  // default to SYNCHRONOUS
    dataWordCount = 0;
    bigTicker = 0;
    smallTicker = 0;
    NOWAIT_eventTicker = 0;
    UniqueID =  SeqID++;
    pHook = NULL;
    patternDataStore = NULL;
    patternDataStoreSize = 0;
    patternDataStoreUsed = 0;
    patternCount = 0;
    patternSize  = 0;
    outputControlFlag = 2;
    pAcodeMgr = NULL;
    pAcodeBuf = NULL;
    pWaveformBuf = NULL;
    rtStackCounter=0;
    ifzLvl = -1;
    for (int i=0; i<MAXIFZ; i++)
    {
      ifzVvar[i]    = -1;
      ifzPosIf[i]   = -1;
      ifzPosElse[i] = -1;
    }
    loopLvl = -1;
    for (int i=0; i<NESTEDLOOP_DEPTH; i++)
      loopPos[i]    = -1;
    channelActive = 0;
}

Controller::Controller(const char *nm, int num)
{
    strcpy(Name,nm);
    kind = GENERIC_TAG;
    dataWordCount = 0;
    bigTicker = 0;
    smallTicker = 0;
    NOWAIT_eventTicker = 0;
    usage = SYNCHRONOUS;  // default to SYNCHRONOUS
    UniqueID = SeqID++;
    pHook = NULL;
    patternDataStore = NULL;
    patternDataStoreSize = 0;
    patternDataStoreUsed = 0;
    outputControlFlag = 2;
    patternCount = 0;
    patternSize  = 0;
    pAcodeMgr = AcodeManager::getInstance();

/* #ifdef LINUX */
    if (strcmp(nm,"dummy"))
      setAcodeBuffer();
/* #else
 *    setAcodeBuffer();
 * #endif
 */

    pWaveformBuf = NULL;
    rtStackCounter=0;
    ifzLvl = -1;
    for (int i=0; i<MAXIFZ; i++)
    {
      ifzVvar[i]    = -1;
      ifzPosIf[i]   = -1;
      ifzPosElse[i] = -1;
    }
    loopLvl = -1;
    for (int i=0; i<NESTEDLOOP_DEPTH; i++)
      loopPos[i]    = -1;
    channelActive = 0;
}


void Controller::setAcodeBuffer()
{
  pAcodeBuf = pAcodeMgr->setAcodeBuffer(Name,UniqueID);
  pAcodeBuf->cHeader.comboID_and_Number = ACODEHEADER;
}


void Controller::setWaveformBuffer()
{
  pWaveformBuf = setAcodeBuffer((char*)"pat");
}


AcodeBuffer* Controller::setAcodeBuffer(const char *type)
{
  char fileName[64];
  strcpy(fileName,type);
  strcat(fileName,".");
  strcat(fileName,Name);
  AcodeBuffer *pSpecialBuf = pAcodeMgr->setAcodeBuffer(fileName,UniqueID);
  pSpecialBuf->cHeader.comboID_and_Number = UNDEFINEDHEADER;
  return pSpecialBuf;
}


void Controller::describe(const char *what)
{
   cout << "CH: " << what << "   " << Name << "  " << SeqID << endl;
#ifndef __INTERIX
   cout << "--- " << bigTicker << "   " << smallTicker << endl;
#endif
   if (pHook != NULL)
     pHook->tell();
   (*pAcodeBuf).describe();
}


void Controller::setOutputControl(int flag)
{
  outputControlFlag = flag;
}


int Controller::matchID(const char *test)
{
  if (!strncmp(test,Name,strlen(Name)))
    return(1);
  else
    return(0);
}


char* Controller::getName()
{
  return Name;
}


void Controller::setActive()
{
   usage = ACTIVE;
}


void Controller::setOff()
{
   usage = OFF;
}


void Controller::setSync()
{
   usage = SYNCHRONOUS;
}


void Controller::setAsync()
{
  usage = ASYNCHRONOUS;
}


void Controller::setNOWAITMode()
{
  usage = NOWAIT_MODE;
}


void Controller::incr_NOWAIT_eventTicker(long long ticks)
{
  NOWAIT_eventTicker += ticks;
}


void Controller::resetSync()
{
   bigTicker   = 0;
   smallTicker = 0;

   resetPowerEventStart();
}


int Controller::isActive()
{
   return(usage == ACTIVE);
}


int Controller::isOff()
{
   return(usage == OFF);
}


int Controller::isSync()
{
   return(usage == SYNCHRONOUS);
}


int Controller::isAsync()
{
   return(usage == ASYNCHRONOUS);
}


int Controller::isNoWaitMode()
{
   return(usage == NOWAIT_MODE);
}


// one of these gets overridden in the inheriting class...
int Controller::isMaster()
  { return(kind == MASTER_TAG); }
int Controller::isRF()
  { return(kind == RF_TAG); }
int Controller::isDDR()
  { return(kind == DDR_TAG); }
int Controller::isGradient()
  { return(kind == GRAD_TAG); }
int Controller::isLock()
  { return(kind == LOCK_TAG); }
int Controller::isPFG()
  { return(kind == PFG_TAG); }

//
//  this is for everything but patterns and (perhaps) tables...
//
//  the structure is a little clumsy
//
#define ASSERT_COUNT(arg)  if (many != arg) { cout << "expected " << arg << " got " << many << endl; exit(-1);}

int Controller::outputACode(int Code, int many, int *stream)
{
  int i;
   if (Code != (int) DLOAD)   // it's a little weird but is secure and unifies outputs....
      flushDload();

   if (bgflag && (outputControlFlag & 1))
   {
     cout << Name << " emits " << many << " codes" << endl;
     switch (Code)
     {
     case (int) DLOAD:  cout << "DLOAD with " << many << setw(8) << setfill('0') << endl;
          for (i=0; i < many; i++)
             cout  << setw(5) << setfill('0') << i << "   " << setw(8) << hex << *(stream+i)<< dec << endl;
          break;
     case (int) PFGSETZCORR: ASSERT_COUNT(2); cout << "PFGSETZCORR high=" << hex << *(stream) << "low= " << *(stream+1) << endl;   break;
     case (int) VDELAY: ASSERT_COUNT(2); cout << "V DELAY with " << hex << *(stream) << " and " << *(stream+1) << endl;   break;
     case (int) EXECUTEPATTERN:   ASSERT_COUNT(1); cout << "Link 2 pattern # " << hex << *(stream) << endl; break;
     case (int) TABLEDEF:   cout << "TABLE DEFINE -- NYI ----" << endl;  break;
     case (int) VRFAMPS:   ASSERT_COUNT(2); cout << " V RF AMPS  factor => " << hex << *(stream) << "  vvar = "<< dec << *(stream+1) << endl; break;
     case (int) VPHASEC:   ASSERT_COUNT(2); cout << " V RF PHASEC factor => " << hex << *(stream) << " vvar ->"<< dec << *(stream+1) << endl; break;
     case (int) ADVISE_FREQ: cout << " ADVISE FREQ => " << hex;
          for (int i=0; i<many; i++)
             cout  << setw(5) << setfill('0') << i << "   " << setw(8) << hex << *(stream+i)<< dec << endl;
          break;
     default:  cout << "unknown ACODE " << hex << Code << endl;  break;
     }
     cout.setf(ios::dec);
   }
   // add to output buffer - flushed once per increment...
   if (!(outputControlFlag & 2)) return(many);
   WriteWord(Code);
   if (Code == (int) DLOAD || Code == (int) ADVISE_FREQ)
       WriteWord(many);   // only necessary for variable length codes...
   for (i=0; i < many; i++)
       WriteWord(*(stream+i));

   return many;
}



int Controller::findPatternFile(char *fname)
{
    char p1[400];

    if (ifs.is_open())
      ifs.close();

    if (appdirFind(fname,"shapelib",p1,"",R_OK|F_OK) != 0)
    {
       ifs.open(p1);
       if (ifs.is_open())
       {
          ifs.clear();
          if (shapeListFile != NULL)
          {
             fprintf(shapeListFile,"%s\n",p1);
             shapesWritten++;
          }
          return(1);
       }
       else
          return(0);  // error !!
    }
    else
    {
       return(0);  // error !!
    }
}


//
//   send pattern prints and write if ON..
//   send pattern put the ID, number words, and number states
//   from the cPatternEntry class.. in case RT engine needs it...
//

int Controller::sendPattern(cPatternEntry *myPat)
{
   int i,id, len, numStates;

   id = myPat->getReferenceID();
   len = myPat->getNumberWords();
   numStates = myPat->getNumberStates();

   if ( len != (int) ((pWaveformBuf->getAcodeSize())-(sizeof(pWaveformBuf->cHeader)/4)) )
      abort_message("psg internal waveform buffer has invalid size for waveform pattern ID %d. abort!\n",id);

   if (bgflag && (outputControlFlag & 1))
   {
       cout << "pattern ID = " << id << " with " << len << "elements first 100 shown" << endl;
       for (i = 0; i < len; i++)
       {
          if (i > 100) continue; // debugging...
          //cout << "p(" << i << ")  0x" << hex << *(patternDataStore+i) << endl; //
        }

   }
   if (bgflag)
   {
     cout << dec;
     cout << "Controller::sendPattern(): outputControlFlag = " << outputControlFlag << endl;
   }

   // the following is necessary to get the correct patID for the waveform into acode stream
   pWaveformBuf->setHeaderWord(COMBOID_NUM_WORD, (PATTERNHEADER+id));

   pWaveformBuf->endSubSection();

   pAcodeMgr->setAcodeStageWriteFlag(WFG_FLAG,1);

   return(0);
}


void Controller::executePattern(int id)
{
  outputACode(EXECUTEPATTERN,1,&id);
}


void Controller::executePattern(int id, int npasses, int remwords, int remticks)
{
  int buffer[4];
  buffer[0] = id;
  buffer[1] = npasses;
  buffer[2] = remwords;
  buffer[3] = remticks;
  outputACode(MULTIPATTERN,4,buffer);
}


// eats comment lines too..
int Controller::scanLine(double *pf1, double *pf2, double *pf3, double *pf4, double *pf5)
{
   char buffer[400];
   int l;
   do
   {
     ifs.getline(buffer,sizeof(buffer));
     if (ifs.eof())
       {
       ifs.close();
       return(-1);
       }
   } while ( (buffer[0] == '#') || (strlen(buffer) == 0) );


   // phase amplitude duration gates  ... if duration == 0 phase is
   // a scale parameter...
   l = sscanf(buffer,"%lf  %lf  %lf  %lf  %lf",pf1,pf2,pf3,pf4,pf5);

   return(l);
}

int Controller::channelCount()
{
   return(UniqueID);
}
//  takes a load word and concatenates to array
//  when twenty elements or a flush command out it goes..
//

int Controller::add2Stream(int dataWord)
{
   dataWordBuffer[dataWordCount] = dataWord;
   dataWordCount ++;
   if(dataWordCount > 100)
     flushDload();
   return(1);
}


void Controller::flushDload()
{
    if (dataWordCount != 0)
    {
       outputACode(DLOAD,dataWordCount,dataWordBuffer);
       dataWordCount = 0;
    }
}


int Controller::openAcodeFile(int option)
{
  return(pAcodeBuf->openAcodeFile(option));
}


int Controller::closeAcodeFile(int option)
{
  pAcodeBuf->closeAcodeFile(option);
  return(3);
}


int Controller::closeWaveformFile(int option)
{
  /* Check waveform sizes and numbers so that they will fit into controller memory */
  if (getTotalPatternCount() > MAX_PATTERN_COUNT)
	  abort_message((char*) "total number of waveforms %d on %s exceeds maximum limit %d. abort!\n",getTotalPatternCount(),getName(),(int)MAX_PATTERN_COUNT);
  if (getTotalPatternSize() > MAX_PATTERN_SIZE)
	  warn_message((char*) "advisory: total waveform size of %6.2fMB on %s may exceed available acq memory (suggest limiting to 30MB)!\n",(getTotalPatternSize()/1048576.0),getName());
  if (bgflag)
  {
     printf("%s Total number patterns = %d  Total size of patterns = %d\n",getName(),getTotalPatternCount(),getTotalPatternSize());
  }
  pWaveformBuf->closeAcodeFile(option);
  return(3);
}


long long Controller::calcTicks(double xx)
{
  return( (long long) ((xx * 80000000.0L)+0.499999999999999999999999) );
}


long long Controller::calcTicksPerState(double xx, int nstates, int min, double fracError, const char *emsg, int action)
{
  long long perState;
  double dperState, ethreshold;

  dperState = xx / ((double) nstates); // all in full precision so very long patterns supported
  perState = calcTicks(dperState);     // 1 state is less than 0.8 sec!
  if (perState < min)
     abort_message((char*) "wfg state duration %g is less than minumum %g on controller %s. abort!\n",dperState,(min*12.5e-9),Name);

  if (perState >  TIMERLIMIT)
     abort_message((char*) "wfg state duration %g ms is greater than timer limit %g ms on %s. abort!\n",(dperState*1e3),(0x4000000*12.5e-6),Name);

  ethreshold = (dperState - ((double) perState)* 80000000.0)/dperState;
  //
  if (ethreshold > fracError)
  {
      if (action > 0)
        abort_message((char*) "wfg pattern has %g %% error on controller %s. abort!\n",
                      (ethreshold*100),Name);
      else
        text_message((char*) "wfg pattern has %g %% error on controller %s\n",
                      (ethreshold*100),Name);
  }
  return(perState);
}


//
// long long delay ..
//
//
int Controller::setTickDelay(long long ticks)
{
  union64 lscratch;
  int buffer[4];
  int k;
  if (ticks < P2TheConsole->getMinTimeEventTicks())
    return(0);
  bigTicker += ticks;
  smallTicker += ticks;
  lscratch.ll = ticks;
  buffer[0] = lscratch.w2.word0;
  buffer[1] = lscratch.w2.word1;

  if (ticks < TIMERLIMIT)
  {
     k = (buffer[1] & 0x3ffffff);
     add2Stream(DURATIONKEY | LATCHKEY | k);
     return(1);
  }
  else
  {
     outputACode(BIGDELAY,2,buffer);
     return(3);
  }
}


//  long long precision
//
int Controller::setDelay(double dd)
{
  long long temp;

  //if (dd < 0.0)
  //   abort_message("negative delay %g on controller %s. abort!\n",dd,Name);

  if (dd > 3600.0*24.0)
     abort_message("delay %g longer than 24 hours not allowed on controller %s. abort!\n",dd,Name);

  temp = calcTicks(dd);
  if (temp < P2TheConsole->getMinTimeEventTicks())
     return(0);
  return(setTickDelay(temp));
}

void Controller::setAsyncDelay(double dd)
{
  long long temp;

  if (dd > 3600.0*24.0)
     abort_message("delay %g longer than 24 hours not allowed on controller %s.\n",dd,Name);

  temp = calcTicks(dd);
  if (temp < P2TheConsole->getMinTimeEventTicks())
     return;
  noteDelay(temp);
}


//
void Controller::setVDelay(int rtVar,int factor)
{
  int buffer[4];
  buffer[0] = rtVar;
  buffer[1] = factor;
  outputACode(VDELAY,2, buffer);
  return;
}

void Controller::doVDelayList(int listId, int rtvar)
{
        if ( ! validrtvar(rtvar) )
             abort_message("invalid real time variable specified in vdelay_list. abort!\n");

        cPatternEntry *tmp = findByID(listId);
        if (tmp == NULL)
                abort_message("unable to find vdelay list for list %d",listId);

        int buffer[2];
        buffer[0]=listId; buffer[1]=rtvar;
        outputACode(VDELAY_LIST,2,buffer);
}


void Controller::setVDelayList(cPatternEntry *vdelayPattern, int *list, int nvals)
{
        int *pBuffer = &list[0];
        cPatternEntry *tmpVdelayPattern;
        if (pBuffer == NULL)
                abort_message("vdelay list empty. abort!\n");

        pWaveformBuf->startSubSection(PATTERNHEADER);
        pWaveformBuf->putCodes(pBuffer, nvals);
        if (vdelayPattern != NULL)
        {
                tmpVdelayPattern = new cPatternEntry(vdelayPattern);
                addPattern(tmpVdelayPattern);
                sendPattern(tmpVdelayPattern);
        }
        else
                abort_message("error in creating vdelay_list. abort!\n");

        return;
}


long long Controller::getBigTicker()
{
   return(bigTicker);
}


long long Controller::getSmallTicker()
{
   return(smallTicker);
}


void Controller::clearSmallTicker()
{
   smallTicker = 0;
}


void Controller::noteDelay(long long ticks)
{
   bigTicker += ticks;
   smallTicker += ticks;
}


void Controller::update4Sync(long long ticks)
{
   if (P2TheConsole->isParallel())
   {
      Controller *p2Ch = P2TheConsole->getParallelController();
      if (isActive())
      {
         if ( ! strcmp(getName(), p2Ch->getName() ) )
            P2TheConsole->parallelElapsed( getName(), ticks);
         else
            abort_message("Illegal %s event on parallel channel %s",
                          getName(), p2Ch->getName());
      }
      else if (isAsync() && ! strcmp(getName(), p2Ch->getName()) )
      {
         P2TheConsole->parallelElapsed( getName(), ticks);
      }
      return;
   }
   if (isSync())
      setTickDelay(ticks);
   else if (isAsync())
      noteDelay(ticks);
   else if ( usage == NOWAIT_MODE )
   {
     if (ticks >= NOWAIT_eventTicker)
     {
       usage  = SYNCHRONOUS;
       ticks -= NOWAIT_eventTicker;
       setTickDelay(ticks);
       NOWAIT_eventTicker = 0;
     }
     else
     {
       NOWAIT_eventTicker -= ticks;
     }
   }
}

void Controller::update4ParallelSync(long long ticks, int acodePos)
{
   if (isSync())
   {
      union64 lscratch;
      int buffer[4];
      if (ticks < P2TheConsole->getMinTimeEventTicks())
        return;
      bigTicker += ticks;
      smallTicker += ticks;
      lscratch.ll = ticks;
      buffer[0] = lscratch.w2.word0;
      buffer[1] = lscratch.w2.word1;
      pAcodeBuf->putCodesAt( buffer, 2, acodePos );
   }
   else if (isAsync())
   {
      noteDelay(ticks);
   }
   else if ( usage == NOWAIT_MODE )
   {
     if (ticks >= NOWAIT_eventTicker)
     {
       usage  = SYNCHRONOUS;
       ticks -= NOWAIT_eventTicker;
       setTickDelay(ticks);
       NOWAIT_eventTicker = 0;
     }
     else
     {
       NOWAIT_eventTicker -= ticks;
     }
   }
}

void Controller::SystemSync(int many, int prepflag)
{
  int buffer[4];
  buffer[0] = many;
  outputACode(WAIT4ISYNC,1,buffer);
}

void Controller::SystemSync(int many)
{
  int buffer[4];
  buffer[0] = many;
  outputACode(WAIT4ISYNC,1,buffer);
}


void Controller::addPattern(cPatternEntry *tmp)
{
   tmp->setNext(pHook);
   pHook = tmp;

   patternSize += ((tmp->getNumberWords())*4) + 24; // total pattern size in bytes w/ 24byte header
   // patternCount is already being incremented
}


void Controller::registerPatternFromArray(cPatternEntry *pat, unsigned int *array)
{

   if (pat == NULL)
     abort_message("error in creating pattern from array on %s. abort!\n",getName());

   int numwords = pat->getNumberWords();

   patternCount++;
   pWaveformBuf->startSubSection(PATTERNHEADER+patternCount);

   int *pInputArray = (int *)array;
   pWaveformBuf->putCodes(pInputArray,numwords);

   addPattern(pat);

   sendPattern(pat);
}


cPatternEntry *Controller::find(const char *nm, int ky)
{
     if (pHook == NULL)
       return(NULL);
     return(pHook->match(nm,ky));
}


cPatternEntry *Controller::findByID(int id)
{
     if (pHook == NULL)
       return(NULL);
     return(pHook->matchID(id));
}

//
//  protected - Aux won't work w/o latch
//
void Controller::setAux(int word)
{
   add2Stream(LATCHKEY | AUX | (0xfff & word));
   bigTicker += MINTIMERWORD;
   smallTicker += MINTIMERWORD;
}

//  DEFUNCT??
// protected
//
//void Controller::setAuxI(int word)
//{
//   add2Stream(AUX | word );
//   bigTicker += MINTIMERWORD;
//   smallTicker += MINTIMERWORD;
//}
//
// protected...
//
void Controller::setSW_Int(int word)
{
  setAux(word);
}
//
//  At the start of each fid, startIncrement sets these loop control values
//
void Controller::startIncrement(int incNum, int ss, int bs, int nt)
{
  int buffer[4];
  buffer[0] = incNum;
  buffer[1] = ss;
  buffer[2] = bs;
  buffer[3] = nt;
  outputACode(INITSCAN,4,buffer);
}

//
// PSG uses this for internal jump calculations...
//
int Controller::getAcodePosition()
{
  // flushDload is important
  flushDload();
  return(pAcodeBuf->getAcodeSize());
}

//
int Controller::getWfgBufPosition()
{
  // flushDload is important
  flushDload();
  return(pWaveformBuf->getAcodeSize());
}

//
int Controller::getTotalPatternCount()
{
	return (patternCount);
}

//
int Controller::getTotalPatternSize()
{
	return (patternSize);
}

//
int Controller::pushAcodePosition()
{
  // put the current position on the stack
  if (rtStackCounter > 99)
    exit(-1);  // improve..

  // flushDload is important
  flushDload();
  int numCodes = pAcodeBuf->getAcodeSize();
  rtStack[rtStackCounter++] = numCodes;
  return(numCodes);
}

//
int Controller::popAcodePosition()
{
  if (rtStackCounter-- < 0) exit(-1); // improve
  return(rtStack[rtStackCounter]);
}


//
// Real Time ifzero control branching
//
void Controller::ifzero(int rtvar)
{
  /* check if rtvar used in nested outer ifzero */
  for(int i=0; i<ifzLvl; i++)
  {
    if (rtvar == ifzVvar[i])
       abort_message((char*) "invalid argument in ifzero statement. abort!\n");
  }

  ifzLvl++;
  ifzVvar[ifzLvl] = rtvar;

  /* check if nested ifzeros exceed MAXIFZ  */

  if (ifzLvl >= MAXIFZ)
     abort_message((char*) "too many nested ifzero statements. abort!\n");


  ifzPosElse[ifzLvl] = -1;                           /* "found Else" to FALSE */


  /* format of acode
  IFZERO
  rtvar         (if rtvar == 0 then TRUE      )
  elseExists    (does else branch exist ?     )
  lengthToElse  (in words to the elsenz acode )
  lengthToEnd   (in words to the endif  acode )
  */

  int buffer[4];
  buffer[0] = rtvar;
  buffer[1] = 0;     /* elseExists   to be filled in later at endif stage */
  buffer[2] = 0;     /* lengthToElse to be filled in later at endif stage */
  buffer[3] = 0;     /* lengthToEnd  to be filled in later at endif stage */

  outputACode(IFZERO, 4, buffer);

  ifzPosIf[ifzLvl] = pAcodeBuf->getAcodeSize() ;
  /*
     ifzPosIf WILL CORRESPOND TO THE FIRST ACODE WORD AFTER IFZERO+args
     It is important that getAcodeSize() call comes after the outputACode()
     so that any preceding DLOAD codes would have been flushed and we get
     the correct Acode position
  */

  /* last 3 args to be filled in at ( ifzPosIf[ifzLvl] - 3 )              */
  /*                        i.e.  3 words back from end of "ifzero" block */
  /* this will be done by the endif function                              */
}


//
// Real Time ifmod2zero control branching
//
void Controller::ifmod2zero(int rtvar)
{
  /* check if rtvar used in nested outer ifmod2zero */
  for(int i=0; i<ifzLvl; i++)
  {
    if (rtvar == ifzVvar[i])
       abort_message((char*) "invalid argument in ifmod2zero statement. abort!\n");
  }

  ifzLvl++;
  ifzVvar[ifzLvl] = rtvar;

  /* check if nested ifzeros or ifmod2zeros exceed MAXIFZ  */

  if (ifzLvl >= MAXIFZ)
     abort_message((char*) "too many nested ifzero or ifmod2zero statements. abort!\n");


  ifzPosElse[ifzLvl] = -1;                           /* "found Else" to FALSE */


  /* format of acode
  IFZERO
  rtvar         (if rtvar == 0 then TRUE      )
  elseExists    (does else branch exist ?     )
  lengthToElse  (in words to the elsenz acode )
  lengthToEnd   (in words to the endif  acode )
  */

  int buffer[4];
  buffer[0] = rtvar;
  buffer[1] = 0;     /* elseExists   to be filled in later at endif stage */
  buffer[2] = 0;     /* lengthToElse to be filled in later at endif stage */
  buffer[3] = 0;     /* lengthToEnd  to be filled in later at endif stage */

  outputACode(IFMOD2ZERO, 4, buffer);

  ifzPosIf[ifzLvl] = pAcodeBuf->getAcodeSize() ;
  /*
     ifzPosIf WILL CORRESPOND TO THE FIRST ACODE WORD AFTER IFMOD2ZERO+args
     It is important that getAcodeSize() call comes after the outputACode()
     so that any preceding DLOAD codes would have been flushed and we get
     the correct Acode position
  */

  /* last 3 args to be filled in at ( ifzPosIf[ifzLvl] - 3 )              */
  /*                        i.e.  3 words back from end of "ifmod2zero" block */
  /* this will be done by the endif function                              */
}


//
// real time control function elsenz
//
void Controller::elsenz(int rtvar)
{
  /* check if there was a preceeding ifzero statment */
  if (ifzLvl == -1)
     abort_message((char*) "elsenz statement without a preceeding ifzero or ifmod2zero statement.\n");

  /* check if rtvar used in nested (outer) ifzero */
  for(int i=0; i<ifzLvl; i++)
  {
    if (rtvar == ifzVvar[i])
       abort_message((char*) "invalid argument in elsenz statement. abort!\n");
  }

  /* check if nested ifzeros exceed MAXIFZ  */
  if (ifzLvl >= MAXIFZ)
     abort_message((char*) "too many nested ifzero statements. abort!\n");

  /* check if there is rtvar mismatch between elsenz & ifzero */
  if (rtvar != ifzVvar[ifzLvl])
     abort_message((char*) "argument mismatch between ifzero and elsenz statements. abort!\n");


  /* format of acode
  ELSENZ
  rtvar         (if rtvar == 0 then TRUE                          )
  lengthToEnd   (in words to the first executable after end clause)
  */

  int buffer[2];
  buffer[0] = rtvar;
  buffer[1] = 0;     /* lengthToEnd  to be filled in later at endif stage */

  outputACode(ELSENZ, 2, buffer);

  ifzPosElse[ifzLvl] = pAcodeBuf->getAcodeSize() - 3 ;
  /*
     ifzPosIf WILL CORRESPOND TO THE START OF THE ELSENZ ACODE WORD
     It is important that getAcodeSize() call comes after the outputACode()
     so that any preceding DLOAD codes would have been flushed and we get
     the correct Acode position
  */



  /* last 1 arg  to be filled in at ( ifzPosElse[ifzLvl] + 2 )              */
  /*                        i.e.  2 words after the start of elsenz acode */
  /* this will be done by the endif function                              */

}


//
// real time control function endif
//

void Controller::endif(int rtvar)
{
  int elseExists, lenToElse, lenToEndif, posEndif;

  /* check if there was a preceeding ifzero statment */
  if (ifzLvl == -1)
     abort_message((char*) "elsenz statement without a preceeding ifzero statement. abort!\n");

  /* check if rtvar used in nested ifzero */
  for(int i=0; i<ifzLvl; i++)
  {
    if (rtvar == ifzVvar[i])
       abort_message((char*) "invalid argument in ifzero statement. abort!\n");
  }

  /* check if nested ifzeros exceed MAXIFZ  */
  if (ifzLvl >= MAXIFZ)
     abort_message((char*) "too many nested ifzero statements. abort!\n");


  /* check if there is rtvar mismatch between elsenz & ifzero */
  if (rtvar != ifzVvar[ifzLvl])
     abort_message((char*) "argument mismatch between ifzero and endif statements. abort!\n");


  // now write the ENDIFZERO acode block
  /* format of acode
  ENDIFZERO
  rtvar         (the rtvar involved in the ifzero-rndif block    )
  */


  int buffer[3];

  buffer[0] = rtvar;
  outputACode(ENDIFZERO, 1, buffer);

  posEndif  = pAcodeBuf->getAcodeSize() - 2;    /* start  of ENDIFZERO acode */



  if (ifzPosElse[ifzLvl] != -1)      /* Else part exists */
  {
    elseExists = 1;
    lenToElse = ifzPosElse[ifzLvl] - ifzPosIf[ifzLvl];

    lenToEndif= posEndif - ifzPosIf[ifzLvl];  /* length to ENDIFZERO from IFZERO */

    // fill in 3 args to the IFZERO acode block
    buffer[0] = elseExists;
    buffer[1] = lenToElse ;
    buffer[2] = lenToEndif;


    pAcodeBuf->putCodesAt( buffer, 3, (ifzPosIf[ifzLvl]-3) );

    /*
       fill in 1 word at ELSENZ acode start + 2
       the word is the length from elsenz to endif
    */
    buffer[0] = posEndif - ifzPosElse[ifzLvl] - 3; /* there are 3 words in ELSENZ code */

    pAcodeBuf->putCodeAt( buffer[0], (ifzPosElse[ifzLvl]+2) );
  }

  else                               /* Else part does not exist */

  {
    elseExists = 0;
    // Else does not exist.  lenToElse not computed

    lenToEndif= posEndif - ifzPosIf[ifzLvl];

    // fill in 3 args to the IFZERO acode block
    buffer[0] = elseExists;
    buffer[1] = lenToEndif;   /* note no Else, so both lenToElse & lenToEndif SAME */
    buffer[2] = lenToEndif;
    pAcodeBuf->putCodesAt( buffer, 3, (ifzPosIf[ifzLvl]-3) );

   // NO NEED to fill in 1 word to the elsenz acode block as Else does not exist
  }


  // now re initialise all the ifz array/stack variables */
  ifzVvar[ifzLvl]    = -1;
  ifzPosIf[ifzLvl]   = -1;
  ifzPosElse[ifzLvl] = -1;

  // now decrement ifLvl index, as one ifzero-endif block done
  ifzLvl--;
}

//
// to handle loops, hardloops
//
void Controller::loopStart()
{
   loopLvl++;
   if (loopLvl >= NESTEDLOOP_DEPTH)
      abort_message("too many nested loops in pulse sequence. abort!\n");

   loopPos[loopLvl] = getAcodePosition();   // loc of 1st word after NVLOOP acode
   powerMonLoopStartAction();
}

//
// to handle loops, hardloops
//
void Controller::loopEnd()
{
   if ((loopLvl < 0) || (loopLvl >= NESTEDLOOP_DEPTH))
      abort_message("invalid nesting of loops in pulse sequence. abort!\n");

   int lenToEnd = getAcodePosition() - loopPos[loopLvl] + 3 ;  // 3 is the no of words in ENDNVLOOP acode

   // write lenToEnd to correct pos in NVLOOP acode
   pAcodeBuf->putCodeAt(lenToEnd, loopPos[loopLvl]-1);         // last arg in NVLOOP acode is updated
                                                               // with num words to skip, to bypass loop
   powerMonLoopEndAction();
   loopLvl--;
}

//
// to handle kzloops
//
void Controller::loopEnd(int count)
{
   if ((loopLvl < 0) || (loopLvl >= NESTEDLOOP_DEPTH))
      abort_message("invalid nesting of loops in pulse sequence. abort!\n");

   int lenToEnd = getAcodePosition() - loopPos[loopLvl] + 3 ;  // 3 is the no of words in ENDNVLOOP acode

   // write lenToEnd to correct pos in NVLOOP acode
   pAcodeBuf->putCodeAt(lenToEnd, loopPos[loopLvl]-1);         // last arg in NVLOOP acode is updated
                                                               // with num words to skip, to bypass loop
   pAcodeBuf->putCodeAt(count, loopPos[loopLvl]-6);         // position of initval loop count
   powerMonLoopEndAction();
   loopLvl--;
}

//
//
//
int Controller::getLoopLevel()
{
  return loopLvl;
}

//
//  compression should be added here!
//
void Controller::writeIncrement(int j)
{
  pAcodeBuf->writeIncrement(j);
}


int Controller::initializeExpStates(int setupflag)
{
  if (bgflag) { cout << "Controller::InitializeExpStates - base class called by " << getName() << endl; }
  return(0);
}

int Controller::initializeIncrementStates(int num)
{
  if (bgflag) { cout << "InitializeIncrementStates - base class called by " << getName() << endl; }
  setTickDelay((long long) num);
  return(0);
}

int Controller::endExperimentState()
{
  if (bgflag) { cout << "endExperimentState - base class called by " << getName() << endl; }
  return(0);
}


void Controller::powerMonLoopStartAction()
{
  //cout <<"powerMonLoopStartAction called in Base Controller class" << endl;
}


void Controller::powerMonLoopEndAction()
{
  //cout <<"powerMonLoopEndAction called in Base Controller class" << endl;
}


void Controller::PowerActionsAtStartOfScan()
{
}


void Controller::computePowerAtEndOfScan()
{
}


void Controller::showPowerIntegral()
{
}


void Controller::showEventPowerIntegral(const char*)
{
}


void Controller::enablePowerCalculation()
{
}


void Controller::suspendPowerCalculation()
{
}

void Controller::eventStartAction()
{
}

void Controller::resetPowerEventStart()
{
}


void Controller::armBlaf(int arg)
{
}

void Controller::clearBlaf()
{
}

void Controller::setChannelActive(int state)
{
   channelActive = state;
}

int Controller::getChannelActive()
{
   return channelActive;
}
