/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifdef __INTERIX
#include <arpa/inet.h>
#endif
#ifdef LINUX
#include <netinet/in.h>
#endif
#include <string.h>

#include "MasterController.h"
#include "expDoneCodes.h"
#include "Console.h"
#include "shims.h"
#include "FFKEYS.h"
#include "ACode32.h"
#include "cpsg.h"
#include "spinner.h"

#ifndef MAXSTR
#define MAXSTR	256
#endif
#define ACQ_PAD 	25
#define ACQ_HOSTGAIN	55
#define ACQ_FINDZ0	65
#define ACQ_HOSTSHIM	75
#define ACQ_PROBETUNE	105

extern char interLock[];
extern char RollCallString[MAX_ROLLCALL_STRLEN];
extern int loc;
extern int locActive;
extern int ok2bumpflag;
extern int spinactive;
extern int OBSch;
extern int oneshimdelay;
extern int prepScan;
extern int readuserbyte;

extern double vttemp, oldvttemp;
extern int spin, oldspin;

extern "C" void setvt();
extern "C" void wait4vt(int finalcheck);
extern "C" void setspin();
extern "C" void wait4spin();
extern "C" void doAutolock();
extern "C" void getsamp();
extern "C" void loadsamp();
extern "C" void translate_string(int shimset);
extern "C" void initwshim(int buffer[]);
extern "C" int  spinnerStringToInt(void);
extern "C" int  isRRI(void);
extern "C" int  shimCount(void);
extern "C" int  download_master_decc_values(const char *);

// master controller defines for xgate
#define encode_MASTERSelectTimeBase(W,select)     \
                   ((3<<26)|((W&1)<<31)|((select&1)<<0))
#define encode_MASTERSetDuration(W,duration)      \
                   ((1<<26)|((W&1)<<31)|((duration&((1<<(25-0+1))-1))<<0))

// selector in back of console hooked to master via aux.

LO_Selector::LO_Selector()
{ 
  code = 0;  
}
// returns full aux word.
int LO_Selector::select_LO(int which)
{ 
   // fprintf(stdout,"select_LO() called with chan = %d\n",which);
   switch (which)
    {
    case 1:  code = 0; break;
    case 2:  code = 4; break;
    case 3:  code = 7; break;
    case 4:  code = 5; break;
    case 5:  code = 6; break;
    default: code = 0; 
    }
   return(0x600 | code); 
}

int LO_Selector::select_LO_A56(int which)
{
     // fprintf(stdout,"select_LO_A56() called with chan = %d\n",which);
     switch (which) 
     {
       case 1: code = 0x5; break;
       case 2: code = 0x6; break;
       case 3: code = 0x3; break;
       default: code = 0x5; 
     }
   return(0x600 | code); 
}

int LO_Selector::getCode()
{
  return(0x600 | code);
}

TR_Selector::TR_Selector()
{
  code = 0;
}

// returns full AUX WORD..
// code is a five bit mask inverted 
int TR_Selector::select_TR(int which)
{
   // fprintf(stdout,"select_TR() called with chan = %d\n",which);
   switch (which)
    {
    case 1:  code = 0x1e; break;
    case 2:  code = 0x1d; break;
    case 3:  code = 0x1b; break; 
    case 4:  code = 0x17; break;
    case 5:  code = 0x0f; break;
    default:
       code = 0x1e;
    }
   return((0x400| code));
}

int TR_Selector::getCode()
{
  return(0x400|code);
}

//
// NSR SUPPORT 
//
NSR::NSR()
{
  int i;
  for (i=0; i < 16; i++)
    NSR_words[i] = i<<11; 
  for (i=0;i < 4; i++) gainsetting[i] = 0; 
  /* gain hides here */
  myNum = 0;  
}

void NSR::setNum(int num)
{
  int i;
  for (i=0; i < 16; i++)
    NSR_words[i] = i<<11 | (num & 0xf) << 24;
   gainsetting[0] = 0;  /* gain hides here */ 
  for (i=0;i < 4; i++) gainsetting[i] = 0; 
  myNum=num; 
}
//
int NSR::programBitField(int word, int top, int bot, int value)
{
  int active, mask, data;
  unsigned int *p2Word;

  p2Word = &NSR_words[word];
  active = (1 << (top - bot + 1))-1;
  active <<= bot;
  mask = ~active; 
  data = (value << bot) & active; 
  *p2Word = (*p2Word & mask) | data;
  *p2Word = (*p2Word & 0xf0003ff) | (word << 11);
  return 0;
}

/* This is a mask 1 = pre amp#1 2 = pre amp#2 */
void NSR::setPreAmpSelect(int pat)
{     programBitField(2,3,0,pat); }

/* This is a mask 1 = tr#1 2 = tr#2 3= tr#1 and tr#2 etc*/
void NSR::setTRSelect(int pat)
{    programBitField(2,7,4,pat);  }

void NSR::setNarrowBand(int pat)
{    programBitField(1,7,0,pat); }

void NSR::setTuneModeSelect(int pat)
{    programBitField(8,0,0,pat); }
/* mixers go from 0 to 3 but */
/* write to mixer 4 to set all at once!! */

/* 1 turns high band ON - also used in tune selector */
void NSR::setMixerHighBand(int mixer, int pat)
{     // printf("setMixerHighBand: mixer:%d  input:%d\n",mixer,pat);
      programBitField(mixer+3,5,5,pat); }

// CONVERTS 0 to 60 to 0 to 30 code HERE 
void NSR::setMixerGain(int mixer,int pat)
{    programBitField(mixer+3,4,0,pat/2);
     gainsetting[(mixer%4)] = pat;
     // printf("setMixerGain called with mixer=%d  input=%d\n",mixer,pat);
}   // CPLD is in steps of 2 dB 

//  NSR table
//  input  mixer connector use tune connection use	tune ONLY
//	0	J5113	hb	J5511	tune 1		MixerHiBand = 1
//	1	J5114	lb	J5512	tune 2		MixerHiBand = 1
//	2	J5115	h2/lk	J5513	tune 3		MixerHiBand = 1
//	3	J5111	tune	J5514	tune 4		MixerHiBand = 1
//	0	n/a		J5505   extend preamp	MixerHiBand = 0
/* this is a count 0 = port 1, 1 = port 2, 2 = port 3 */
void NSR::setMixerInput(int mixer, int pat)
{    // printf("setMixerInput: mixer:%d  input port:%d\n",mixer,pat);
     programBitField(mixer+3,7,6,pat); }

void NSR::setLockAtten(int pat)
{    programBitField(13,0,0,pat); }

void NSR::setLockHiLow(int pat)
{    programBitField(12,0,0,pat); }

void NSR::setDiplexRelay(int pat)
{    programBitField(14,1,0,pat); }

void NSR::setPreSig(int pat)
{
  programBitField(11,0,0,pat);
}

void NSR::setCoilBias(int pat)
{
  programBitField(10,0,0,pat);
}

void NSR::setPP2HFBypassSwitch(int pat)
{
  // pat=0 selects Combined;  pat=1 selects Bypass

  if (pat)
  {
    programBitField(14,4,3,0);
    // fprintf(stdout,"setPP2HFBypassSwitch set to ByPass\n");
  }
  else
  {
    programBitField(14,4,3,3);
    // fprintf(stdout,"setPP2HFBypassSwitch set to Combined\n");
  }
}

void NSR::setPP2HFCombinerSwitch(int pat)
{
  // pat=0 selects CombinerSwitch set to rf1->F19/rf3->H1
  // pat=1 selects CombinerSwitch set to rf1->H1/rf3->F19

  if (pat)
  {
    programBitField(14,5,5,0);
    // fprintf(stdout,"setPP2HFCombinerSwitch set to rf1->H1/rf3->F19\n");
  }
  else
  {
    programBitField(14,5,5,1);
    // fprintf(stdout,"setPP2HFCombinerSwitch set to rf1->F19/rf3->H1\n");
  }
}

void NSR::setPP2Ch3TuneSelect(int pat)
{

  // pat=0 selects LB to Ch3 Tune
  // pat=1 selects HB to Ch3 Tune

  if (pat)
  {
    programBitField(14,2,2,1);
    // fprintf(stdout,"setPP2Ch3TuneSelect set to HB\n");
  }
  else
  {
    programBitField(14,2,2,0);
    // fprintf(stdout,"setPP2Ch3TuneSelect set to LB\n");
  }
}


void NSR::setPP2HFandCryoInterface()
{
      // fprintf(stdout,"setPP2HFandCryoInterface() called\n");

      char nucName[64];
      // set all the new ProPulse INNOVA H/F combiner & cryo switches
      if (P2TheConsole->getConsoleHFMode() == 'c')
      {
        // set ByPass sw to do combined
        setPP2HFBypassSwitch(0);

        RFController* rfC1 = P2TheConsole->getRFControllerByLogicalIndex(1);
        rfC1->getNucleusName(nucName);
        // printf("rf1 nucleus name is %s\n",nucName);

        if (strcmp(nucName, "H1") == 0)
        {
            // set Combiner input rf1->H1/rf3->F19
            setPP2HFCombinerSwitch(1);
        }
        else if (strcmp(nucName,"F19") == 0)
        {
            // set combiner input rf1->F19/rf3->H1
            setPP2HFCombinerSwitch(0);
        }
        else   // if tn not H1 or F19
        {
            // set Combiner input rf1->H1/rf3->F19
            setPP2HFCombinerSwitch(1);
            text_message("advisory: H/F combiner switch set for rf1 to H1 filter. Possible attenuation of transmit RF power?\n");
        }

      }
      else
      {
        // set ByPass sw to do independent
        setPP2HFBypassSwitch(1);

        RFController* rfC3 = P2TheConsole->getRFControllerByLogicalIndex(3);
        rfC3->getNucleusName(nucName);
        // printf("rf3 nucleus name is %s\n",nucName);


        if (strcmp(nucName,"H1") == 0)
        {
           // set combiner input rf1->F19/rf3->H1
           setPP2HFCombinerSwitch(0);
        }
        else
        {
           // set Combiner input rf1->H1/rf3->F19
           setPP2HFCombinerSwitch(1);
        }
      }

      // No special setttings necessary for Cryo transmit path routing

}

int NSR::getGainSetting(int j)
{
  return(gainsetting[j]); 
}

int NSR::getNSRWord(int index)
{
  return(NSR_words[index]);
}

// maps out but no output.
// presig coil bias..
// prototype MODEFLAG 

TR_INTERCONNECT::TR_INTERCONNECT()
{
  int i;
  for (i=0; i<4; i++)  the_NSR_Array[i].setNum(i); // initializes chip selects and address fields.
  numRcvrs =1;
  numNSRS = 1; 
  auxNSRControl.setNum(AUXCNTRLNUM);  // the ninth controller 
}

// normal operation
void TR_INTERCONNECT::setSingleReceiver()
{
  int k;
  k = 1 << (observe1ch-1); // a mask
  the_NSR_Array[0].setCoilBias((modeFlags & COILBIASBIT)==COILBIASBIT);
  the_NSR_Array[0].setPreSig((modeFlags & PRESIGBIT)==PRESIGBIT); 
  the_NSR_Array[0].setPreAmpSelect(k); 
  the_NSR_Array[0].setTRSelect(k);
  the_NSR_Array[0].setMixerHighBand(0,hibandflag&1);
  the_NSR_Array[0].setMixerGain(0,gain[0]);
  auxNSRControl.setPreAmpSelect(0); 
  auxNSRControl.setTRSelect(0);
      
  switch (observe1ch)
     {
      case 2:
        if (lockflag == 0) 
           the_NSR_Array[0].setMixerInput(0,1); // normal ch2 (low band) input.
        else 
           the_NSR_Array[0].setMixerInput(0,2);  // lk input
        break;
      case 3:
        the_NSR_Array[0].setMixerInput(0,3);
        the_NSR_Array[0].setMixerInput(1,0); 
        the_NSR_Array[0].setMixerHighBand(1,0);
        break;
      case 4: 
        the_NSR_Array[0].setMixerInput(0,3);
	the_NSR_Array[0].setMixerInput(1,3);
        the_NSR_Array[0].setMixerHighBand(1,1);
        the_NSR_Array[0].setMixerInput(2,0);
        the_NSR_Array[0].setMixerHighBand(2,0);
        break;
      case 5: 
        the_NSR_Array[0].setMixerInput(0,3);
	the_NSR_Array[0].setMixerInput(1,3);
        the_NSR_Array[0].setMixerHighBand(1,1);
        the_NSR_Array[0].setMixerInput(2,3);
        the_NSR_Array[0].setMixerHighBand(2,1); /* plug into 5514 */
        /* install 5th pre-amp to 2nd nsr controller to power pre-amp */
        auxNSRControl.setCoilBias((modeFlags & COILBIASBIT)==COILBIASBIT);
        auxNSRControl.setPreSig((modeFlags & PRESIGBIT)==PRESIGBIT); 
        auxNSRControl.setPreAmpSelect(1); 
        auxNSRControl.setTRSelect(1);
        break;
      case 1:
      default:
        the_NSR_Array[0].setMixerInput(0,0);   // ch1 input
        break;
     }
   }

void TR_INTERCONNECT::setA56Receiver()
{
  int k;
  k = 1 << (observe1ch-1); // a mask
  the_NSR_Array[0].setCoilBias((modeFlags & COILBIASBIT)==COILBIASBIT);
  the_NSR_Array[0].setPreSig((modeFlags & PRESIGBIT)==PRESIGBIT); 
  the_NSR_Array[0].setPreAmpSelect(k); 
  the_NSR_Array[0].setTRSelect(k);
  the_NSR_Array[0].setMixerHighBand(0,hibandflag&1);
  the_NSR_Array[0].setMixerGain(0,gain[0]);
  auxNSRControl.setPreAmpSelect(0); 
  auxNSRControl.setTRSelect(0);
        
  switch (observe1ch)
     {
      case 2:
        if (lockflag == 0) 
           the_NSR_Array[0].setMixerInput(0,1);  // normal ch2 (low band) input.
        else 
           the_NSR_Array[0].setMixerInput(0,2);  // lk input
        break;
      case 3:
        text_error("No channel 3 observe in this system\n");
        break;
      case 1:
      default:
        the_NSR_Array[0].setMixerInput(0,0);   // ch1 input
        break;
     }
}

void TR_INTERCONNECT::setA56_3ch_Receiver()
{
  int k;
  k = 1 << (observe1ch-1); // a mask
  the_NSR_Array[0].setCoilBias((modeFlags & COILBIASBIT)==COILBIASBIT);
  the_NSR_Array[0].setPreSig((modeFlags & PRESIGBIT)==PRESIGBIT);
  the_NSR_Array[0].setPreAmpSelect(k);
  the_NSR_Array[0].setTRSelect(k);

  // select the receive mixer band
  the_NSR_Array[0].setMixerHighBand(0,hibandflag&1);   // ProPulse INNOVA

  the_NSR_Array[0].setMixerGain(0,gain[0]);
  auxNSRControl.setPreAmpSelect(0);
  auxNSRControl.setTRSelect(0);

  // initialize tune switch to port 3
  the_NSR_Array[0].setMixerInput(1,1);     // tune selector port 3
  the_NSR_Array[0].setMixerHighBand(1,1);  // tune selector port 3
  the_NSR_Array[0].setMixerGain(1,0);      // tune selector port 3

  int cryo_usage = P2TheConsole->getConsoleCryoUsage();

  switch (observe1ch)
     {
      case 2:
        if (lockflag == 0)
        {
           if (cryo_usage == 0)
           {
             the_NSR_Array[0].setMixerInput(0,1);              // normal ch2 (low band) input.
             // fprintf(stdout,"Ch2      selects                               mixer port 1\n");
           }
           else if (cryo_usage == 1)  // cryo usage for LB
           {
             // selects port 5 of cryo selector switch
             the_NSR_Array[0].setMixerInput(2,3);              // cryo selector switch
             the_NSR_Array[0].setMixerHighBand(2,1);           // cryo selector switch
             the_NSR_Array[0].setMixerGain(2,2);               // cryo selector switch

             // selects port 3 of tune selector switch
             the_NSR_Array[0].setMixerInput(1,1);
             the_NSR_Array[0].setMixerHighBand(1,1);
             the_NSR_Array[0].setMixerGain(1,0);

             // selects port 3 of mixer switch
             the_NSR_Array[0].setMixerInput(0,3);              // mixer input to select the tune mix output
             // fprintf(stdout,"Ch2 Cryo selects cryo sel port 5, tune port 3, mixer port 3\n");
           }
           else
           {
              abort_message("invalid value for parameter probestyle on ProPulse INNOVA system. abort!\n");
           }
        }
        else
        {
           the_NSR_Array[0].setMixerInput(0,2);  // tn='lk' input
           // fprintf(stdout,"Ch2      selects   tn=lk case                  mixer port 2\n");
        }
        break;

      case 3:
        abort_message("RF channel 3 cannot be set as observe on ProPulse INNOVA system\n");
        break;

      case 1:
      default:
        if (cryo_usage == 0)
        {
           the_NSR_Array[0].setMixerInput(0,0);              // normal ch1 (hi band) input.
           // fprintf(stdout,"Ch1      selects                               mixer port 0\n");
        }
        else if (cryo_usage == 1)   // cryo usage for HB
        {

           // selects port 4 of cryo selector switch
           the_NSR_Array[0].setMixerInput(2,3);              // cryo selector switch
           the_NSR_Array[0].setMixerHighBand(2,0);           // cryo selector switch
           the_NSR_Array[0].setMixerGain(2,0);               // cryo selector switch

           // selects port 3 of tune selector switch
           the_NSR_Array[0].setMixerInput(1,1);
           the_NSR_Array[0].setMixerHighBand(1,1);
           the_NSR_Array[0].setMixerGain(1,0);

           // selects port 3 of mixer switch
           the_NSR_Array[0].setMixerInput(0,3);              // mixer input to select the tune mix output
           // fprintf(stdout,"Ch1 Cryo selects cryo sel port 4, tune port 3, mixer port 3\n");
        }
        break;
     }
}

// phase array and multi-mouse imaging
void TR_INTERCONNECT::setArrayedRcvrs()
{
   int i,j,k,m,trMask,preampMask,mxr;
   // build common masks
   trMask = 0;
   for (i=0; i< numRcvrs; i++)  // build a mask..
    if (rcvrS[i] == 'y') trMask |= 1<<i;
   // these apply to all preamps.
   preampMask = trMask; 
   auxNSRControl.setCoilBias((modeFlags & COILBIASBIT)==COILBIASBIT);
   auxNSRControl.setPreSig((modeFlags & PRESIGBIT)==PRESIGBIT);
   auxNSRControl.setPreAmpSelect(0); 
   auxNSRControl.setTRSelect(0);  
   auxNSRControl.setMixerInput(0,0); 
   auxNSRControl.setMixerHighBand(0,0);
   // mxr sets mixer input..
   k = OBSch;
   if (modeFlags & IMGWLOCKBIT) k += 10; // page 32 w lock requires
   if (!(modeFlags & VOLUMERCVRBIT)) k = 0;
   if (lockflag)                k = 9;
   switch (k) {
        case 0:  // ALL phased array i.e. volumercv == n
                 k = trMask & 0xf;
                 auxNSRControl.setPreAmpSelect(k); 
                 auxNSRControl.setTRSelect(k);   
                 preampMask &= 0xfffffff0; // normal pre-amps off for rf isolation.
                 trMask     &= 0xfffffff0; //
                 mxr = 1;
                 break;
        case 1:  case 11:
	         // high band volume receive channel 1
                 preampMask = trMask = 1;  // yyyyy -> ynnnn 
                 mxr = 0; 
                 break;
        case 2:   // low band volume receive channel 2
                 preampMask = trMask = 2;  // yyyyy -> ynnnn 
                 mxr = 2;
	         break;
        case 12: // low band volume receive channel 2
                 preampMask = trMask = 2;  // yyyyy -> ynnnn 
                 mxr = 3;
	         break;
        case 13: 
                 preampMask = trMask = 4;  // yyyyy -> ynnnn 
                 mxr = 3;
                 auxNSRControl.setMixerInput(0,3); 
                 auxNSRControl.setMixerHighBand(0,1);
                 auxNSRControl.setMixerInput(1,0); 
                 auxNSRControl.setMixerHighBand(1,0);
                 break;
        case 14: 
                 text_error("phased array chan 4 not proven");
                 mxr = 3;
                 preampMask = trMask = 8;  // yyyyy -> ynnnn 
                 auxNSRControl.setMixerInput(0,3); 
                 auxNSRControl.setMixerHighBand(0,1);
                 auxNSRControl.setMixerInput(1,3); 
                 auxNSRControl.setMixerHighBand(1,1);
                 break;
        case 9:
                // lock case..
                auxNSRControl.setTRSelect(2);  
                preampMask = 0; // lock has his own...
                trMask = 2;
                mxr = 2;
                break;
        default:
                abort_message("phased array configuration error"); 
   }
   
   // sorts out the pre-amps TR switch drivers.
 
    for (i=0;i<numNSRS; i++)
	{
	   k = trMask & 0xf;
	   trMask >>= 4; 
       m = preampMask & 0xf;
       preampMask >>= 4;
       the_NSR_Array[i].setCoilBias((modeFlags & COILBIASBIT)==COILBIASBIT);
       the_NSR_Array[i].setPreSig((modeFlags & PRESIGBIT)==PRESIGBIT); 
       the_NSR_Array[i].setPreAmpSelect(m); 
       the_NSR_Array[i].setTRSelect(k);
       // pseudo mixer #4 (all together) is set but unused.. 
       for (j=0; j < 5; j++) 
       {  // initial pseudo mixer 4 too.. ONLY USE FOR RT GAIN
        the_NSR_Array[i].setMixerHighBand(j,hibandflag&1); 
        the_NSR_Array[i].setMixerInput(j,mxr);
        the_NSR_Array[i].setMixerGain(j,gain[0]);
       }
    }  
}

// simultaneous multi-nuclear configuration single use
void TR_INTERCONNECT::setMultiNucSingle()
{  
   int k;
   k = 1 << (observe1ch -1); // mask
   the_NSR_Array[0].setCoilBias((modeFlags & COILBIASBIT)==COILBIASBIT);
   the_NSR_Array[0].setPreSig((modeFlags & PRESIGBIT)==PRESIGBIT);
   the_NSR_Array[0].setPreAmpSelect(k);
   the_NSR_Array[0].setTRSelect(k);
   the_NSR_Array[0].setMixerHighBand(0,hibandflag&1);
   the_NSR_Array[0].setMixerGain(0,gain[0]);
   switch (observe1ch)
   {
      case 2:
        if (lockflag == 0)
          the_NSR_Array[0].setMixerInput(0,1);       // ch2 normal input.
        else
          the_NSR_Array[0].setMixerInput(0,2);       // lk input
        break;

      case 3:                                        // LO from rf3
        the_NSR_Array[0].setMixerHighBand(1,hibandflag&1);      
        the_NSR_Array[0].setMixerInput(1,1);         // mixer #2 and ch2 input
        the_NSR_Array[0].setMixerGain(1,gain[0]);    // mixer #2
        break;

      case 4:
        the_NSR_Array[0].setMixerInput(0,3);
        auxNSRControl.setMixerInput(0,3);
        auxNSRControl.setMixerInput(1,0);
        auxNSRControl.setMixerHighBand(0,1);
        auxNSRControl.setMixerHighBand(1,0);
        the_NSR_Array[0].setMixerGain(0,gain[0]);      // mixer 1
        break;
      case 5: 
        the_NSR_Array[0].setMixerInput(0,3);
	the_NSR_Array[0].setMixerInput(1,3);
        the_NSR_Array[0].setMixerHighBand(1,1);
        the_NSR_Array[0].setMixerInput(2,3);
        the_NSR_Array[0].setMixerHighBand(2,1); /* plug into 5514 */
        /* install 5th pre-amp to 2nd nsr controller to power pre-amp */
        auxNSRControl.setCoilBias((modeFlags & COILBIASBIT)==COILBIASBIT);
        auxNSRControl.setPreSig((modeFlags & PRESIGBIT)==PRESIGBIT); 
        auxNSRControl.setPreAmpSelect(1); 
        auxNSRControl.setTRSelect(1);
        break;

      case 1:
      default:
        the_NSR_Array[0].setMixerHighBand(0,hibandflag);
        the_NSR_Array[0].setMixerInput(0,0);
        the_NSR_Array[0].setMixerGain(0,gain[0]);     // mixer 1
        break;
   }
}
// simultaneous multi-nuclear configuration 2 rcvrs used
void TR_INTERCONNECT::setMultiNuc2()    
{   
   int k,jj;
   if (observe2ch != 3 )
     abort_message("TRInterconnect::initialize() DECch must be 3 in multi nuc receiver multiple acquir mode unless probeConnect");
   if (observe1ch == 3)
     abort_message("TRInterconnect::initialize() OBSch can not be set to 3 in multi nuc receiver multiple acquire mode. interchange tn & dn2?");
   k  = 1 << (observe1ch-1);
   jj = 1 << (observe2ch-1); //  always 4!!??
   the_NSR_Array[0].setCoilBias((modeFlags & COILBIASBIT)==COILBIASBIT);
   the_NSR_Array[0].setPreSig((modeFlags & PRESIGBIT)==PRESIGBIT);
   the_NSR_Array[0].setPreAmpSelect(k|jj);
   the_NSR_Array[0].setTRSelect(k|jj);

   the_NSR_Array[0].setMixerGain(0,gain[0]);        // mixer 1
   // observe2ch (decch) = 3
   the_NSR_Array[0].setMixerInput(1,1);           // mixer 2  ch2 input
   the_NSR_Array[0].setMixerGain(1,gain[1]);      // mixer 2  gain 
   the_NSR_Array[0].setMixerHighBand(1,(hibandflag>>1)&1);// mixer 2 - ch2 input
   the_NSR_Array[0].setMixerHighBand(0,(hibandflag & 1)); 
   switch (observe1ch)
   {
      case 1:
        the_NSR_Array[0].setMixerInput(0,0);      // mixer 1 ch1 input
        break;
      
      case 2:
        the_NSR_Array[0].setMixerInput(0,1);      // mixer 1 ch2 input
        break;

      case 4:
        the_NSR_Array[0].setMixerInput(0,3);
        auxNSRControl.setMixerInput(0,3);
        auxNSRControl.setMixerInput(1,0);
        auxNSRControl.setMixerHighBand(0,1);
        auxNSRControl.setMixerHighBand(1,0);
        break;
      case 5: 
        the_NSR_Array[0].setMixerInput(0,3);
	the_NSR_Array[0].setMixerInput(1,3);
        the_NSR_Array[0].setMixerHighBand(1,1);
        the_NSR_Array[0].setMixerInput(2,3);
        the_NSR_Array[0].setMixerHighBand(2,1); /* plug into 5514 */
        /* install 5th pre-amp to 2nd nsr controller to power pre-amp */
        k >>= 4; 
        auxNSRControl.setCoilBias((modeFlags & COILBIASBIT)==COILBIASBIT);
        auxNSRControl.setPreSig((modeFlags & PRESIGBIT)==PRESIGBIT); 
        auxNSRControl.setPreAmpSelect(k); 
        auxNSRControl.setTRSelect(k);
        break;

      case 3:
      default:
        abort_message("Configuration error swap tn & dn(?)");
        break;

   }
}  

int TR_INTERCONNECT::initialize()
{
   int k;
   numNSRS = ((numRcvrs+3) / 4); 
   if (numNSRS > MAXNUMNSRS)
     abort_message("Number of receivers configured is more than maximum supported(32). Abort!\n");

   //cout << "numReceivers " << numReceivers << endl;
   //cout << "numNSRS " << numNSRS << endl;
  
   the_NSR_Array[0].setTuneModeSelect(0); // tune off..  
   //power on active preamps (hi/low??)
   // lock atten lock hi/low diplexer only CHANNEL 1...
   k = 1 << (observe1ch-1);

   // Determine the console RF Interface
   int rfIntfType = P2TheConsole->getConsoleRFInterface();
   if (rfIntfType == 1)        // VNMRS, 400MR, DD2
      the_LO_Selector.select_LO(observe1ch); 
   else if (rfIntfType == 2)   // ProPulse
      the_LO_Selector.select_LO_A56(observe1ch); 
   else if (rfIntfType == 3)  // ProPulse INNOVA
   {
     the_LO_Selector.select_LO_A56(observe1ch);
     the_NSR_Array[0].setPP2HFandCryoInterface();
   }
   else
      abort_message("invalid value for rfinterface global parameter. abort!");

   the_TR_Selector.select_TR(observe1ch); 
   the_NSR_Array[0].setLockHiLow((modeFlags & HBLOCKBIT)==HBLOCKBIT);
   the_NSR_Array[0].setDiplexRelay(lockflag==1);
   the_NSR_Array[0].setLockAtten((modeFlags & LOCKATTENBIT)==LOCKATTENBIT);

   // Receiver settings
   switch (usageFlag) { 

   	 case MULTIMGRCVR_SINGACQ:
   	 case MULTIMGRCVR_MULTACQ: setArrayedRcvrs();    break;
   	 case MULTNUCRCVR_SINGACQ: setMultiNucSingle();  break;
   	 case MULTNUCRCVR_MULTACQ: setMultiNuc2();       break;
   	 case SINGRCVR_SINGACQ:
   	 default:  if (rfIntfType == 3)
                     setA56_3ch_Receiver();
                   else if (rfIntfType == 2)
                     setA56Receiver();
                   else if (rfIntfType == 1)
                     setSingleReceiver(); 
                   else
                     abort_message("invalid setting for rfinterface parameter. abort!");
   } 
   return(0);
}

int TR_INTERCONNECT::setRTGainData(int gain)
{
  int i,k;
  k = (int) (gain+0.49);
  for (i=0; i < numRcvrs; i++) the_NSR_Array[i/4].setMixerGain(i%4,k);
  return(numRcvrs);
}

int TR_INTERCONNECT::setRTGainOneData(int igain, int choice)
{  
  the_NSR_Array[choice/4].setMixerGain(choice%4,igain);
  return(the_NSR_Array[choice/4].getNSRWord(choice%4+3));
}

int TR_INTERCONNECT::getGainCode(int rcvrNum)
{
  // cout << " getGainCode [" << rcvrNum << ",  " << rcvrNum / 4 << "    "<< rcvrNum  << "]" << endl;
   return(the_NSR_Array[rcvrNum/4].getNSRWord(3+(rcvrNum%4)));
}

void TR_INTERCONNECT::setBankGainCode(int igain, int nsrNum)
{
   the_NSR_Array[nsrNum].setMixerGain(4,igain);
}

int TR_INTERCONNECT::getBankGainCode(int nsrNum)
{
  return(the_NSR_Array[nsrNum].getNSRWord(7)); // pseudo mixer 4
}

//
// words to send returned 
//
// acode uses number of remaining words in first word
//
int TR_INTERCONNECT::packOutputBuffer(int *destination, int tuneflag)
{
  // ignore tune for the moment...
  int *pT,count,gain,k;
  pT = destination;
  // single receiver prototype...
  gain = the_NSR_Array[0].getGainSetting(0);
  *pT++ = (gain << 16) | 9;                 // gain | number words.
  *pT++ = the_NSR_Array[0].getNSRWord(2);  // TR preamp settings - skip narrow band
  *pT++ = the_NSR_Array[0].getNSRWord(3);  // Mixer 1 gain and input select. 
  *pT++ = the_NSR_Array[0].getNSRWord(4);  // Tune/Obs select 1  first tune 2nd port.
  *pT++ = the_NSR_Array[0].getNSRWord(5);  // Tune/Obs select 2.
  *pT++ = the_NSR_Array[0].getNSRWord(6);  // Tune/Obs select 3.
  *pT++ = the_NSR_Array[0].getNSRWord(8);  // Tune mode..
  *pT++ = the_NSR_Array[0].getNSRWord(11); // presig .
  *pT++ = the_NSR_Array[0].getNSRWord(12); // lock hi/low 
  *pT++ = the_NSR_Array[0].getNSRWord(14); // diplexer.
  *pT++ = auxNSRControl.getNSRWord(2);     // preamps & tr's
   count = (int) (pT - destination);
  *destination = (gain << 16) | (count-1);
  if (numRcvrs == 1)
     return(count);
  *pT++ = auxNSRControl.getNSRWord(3);     // tune selector + tn='lk' 
  *pT++ = auxNSRControl.getNSRWord(4);     // tune selector #2
  *pT++ = auxNSRControl.getNSRWord(11);    // presig
  for (k=1; k < numNSRS; k++)              // remaining NSRS!!!  
    {
      *pT++ = the_NSR_Array[k].getNSRWord(2);  // TR preamp settings - skip narrow band
      *pT++ = the_NSR_Array[k].getNSRWord(3);  // Mixer 0 gain and input select. 
      *pT++ = the_NSR_Array[k].getNSRWord(4);  // Mixer 1 
      *pT++ = the_NSR_Array[k].getNSRWord(5);  // Mixer 2
      *pT++ = the_NSR_Array[k].getNSRWord(6);  // Mixer 3  ..   
      *pT++ = the_NSR_Array[k].getNSRWord(11); // presig
    }
  count = (int) (pT - destination);
  *destination = (gain << 16) | (count-1);
  return(count);
}
void TR_INTERCONNECT::tell()
{
	
	cout << "number of receivers " << numRcvrs << endl;
	cout << "number of nsrs " << numNSRS << endl;
	
	if (modeFlags & PRESIGBIT) cout << "presig active ";
	if (modeFlags & HBLOCKBIT) cout << "high band lock active ";
	if (modeFlags & LOCKATTENBIT) cout << "lock attenuator set ";
	cout << "mode word " << hex << modeFlags << dec << endl;
	cout << "receivers = " << rcvrS << endl;
	cout << "observe 1 chan: " << observe1ch << endl;
	cout << "mixer 1 gain = " << gain[0] << endl;
	if (usageFlag==MULTNUCRCVR_MULTACQ) 
	{
	  cout << "observe 2 chan:  " << observe2ch << endl;
	  cout << "mixer 2 gain = " << gain[1] << endl;
	}
	if (usageFlag>=MULTIMGRCVR_SINGACQ)
	{
	  if (modeFlags & VOLUMERCVRBIT)
	    cout << "volume receiver in use channel 1 " << endl;
	  else
	    cout << "local receiver in use channel 1" << endl;
	}
}
 
MasterController::MasterController(const char *name, int flags):Controller(name,flags)/*,myRouter()*/
{
  kind = MASTER_TAG;
  //cout << "new master Controller" << endl;
  LKMode = 0;
  Extra = 0;
  shimset = -1;
  blafMode = 0;
}

//
//
void MasterController::setGates(int GatePattern)
{
   add2Stream(GATEKEY | (GatePattern & 0xffffff));
}

//
//
int MasterController::xgate(int xcounts)
{
  // switch time base to external and count "xcounts"
  add2Stream(encode_MASTERSelectTimeBase(0,1));
  add2Stream(encode_MASTERSetDuration(1,xcounts));

  // switch timebase to internal 80 MHz clock
  add2Stream(encode_MASTERSelectTimeBase(0,0));
  add2Stream(encode_MASTERSetDuration(1,LATCHDELAYTICKS));

  // now issue the sync signal to all controllers
  // WAIT4SYNC is 0x80
  add2Stream(DURATIONKEY | LATCHDELAYTICKS);
  add2Stream(LATCHKEY | GATEKEY | (0x80 << 12) | 0x80);
  add2Stream(DURATIONKEY | LATCHDELAYTICKS);
  add2Stream(LATCHKEY | GATEKEY | (0x80 << 12)       );

  // adjust MasterController BigTicker to account for total delay
  int totalTicks   = 3 * LATCHDELAYTICKS ;
  bigTicker   += totalTicks;
  smallTicker += totalTicks;

  // return totalTicks to sync all other Controllers
  return (totalTicks);
}

//     void setStatus(char ch);   // master might need this??
//
//
void MasterController::wait4XGate(int num)
{
   add2Stream( MXGATE | (num & 0xffff) | LATCHKEY);
}

void MasterController::rotorSync(int arg1, int arg2) // look this up..
{
   add2Stream( ROTOSYNC | (arg1& 0xffff) | LATCHKEY);
}
// 
//
//
void MasterController::giveSyncSignal()
{
   add2Stream( GATEKEY | 1 );
}

void MasterController::SystemSync(int num, int prepflag) // master overrides with 
{
   int buffer[2];
   buffer[0] = num;
   buffer[1] = prepflag;
   outputACode(SENDSYNC,2,buffer);
}

void MasterController::SystemSync(int num) // master overrides with 
{
   int buffer[2];
   buffer[0] = num;
   if (prepScan)
     buffer[1] = 1;
   else
     buffer[1] = 0;

   outputACode(SENDSYNC,2,buffer);
}

//SAS
void MasterController::setNoDelay(double dur)
{
  long long ticks;
    ticks = calcTicks(dur);
      // omitting this advances the Gradient
        // DelayAccumulator += ticks;
          bigTicker   += ticks;
            smallTicker = ticks;  // seves as clear and add...
            }
//ENDSAS


//
//  Start up the experiment .. very extensive method..
// 
extern "C" int getlkfreqDDsi(double,int,int *);
int MasterController::initializeExpStates(int setupflag)
{
  char estring[MAXSTR+9];
  double tmpval;
  double lockpower;
  int buffer[64];  // lots of room for NSR codes..
  int *codeBuffer;
  int i, k;
  int h1freq = 0;
  int lkflt_fast,lkflt_slow;
  int lkmode = 0;
  int numrfch;
  int vtrange;
  int spintype;
  int safeAuxByte;
  int NSRModeFlag;
  union cham {
    double freq;
    int ifreq[2];
  } fword;
  if (pAcodeBuf == NULL)
  {
    cout << "pAcodeBuf is NULL in MasterController object\n" ;
    exit(-1);
  }
  NSRModeFlag = 0;
  strcpy(estring,"   ");
  // Do this first, avoid race condition
  if ( P_getstring(GLOBAL, "gradtype", estring, 1, MAXSTR) < 0)
     abort_message("PSG: Cannot find gradtype");
  else
  {  if ( strcmp(estring,"rrr") == 0 ) 
     {  buffer[0] = 0xF0000;	// why F? Because V is not a hex value
        XYZgradflag = 1;
     }
     else
     {  buffer[0] = 0x0;
        XYZgradflag = 0;
     }
     buffer[1] = 0;
#ifdef XXX
     if (estring[2] == 'p')
     {
        /* ISI interlock */
        if ( P_getstring(GLOBAL, "isiin", estring, 1, MAXSTR) == 0)
        {
           if (estring[0] == 'y')
              buffer[1] = 1;
        }
     }
#endif
  }
  outputACode(MASTER_CHECK,2,buffer);
  if (P_getstring(CURRENT,"rollcall",estring,1,MAXSTR) < 0)
  {
    RollCall(RollCallString);
  }
  else
  {
    RollCall(estring);
  }
  if ( (setupflag == SU) && option_check("pause") )
  {
     double val;

     /* Pause at least 1 second */
     if ( P_getreal(CURRENT,"pad", &val,1) )
        val = 1.0;
     if ( val < 1.0)
        val = 1.0;
     P2TheConsole->newEvent();         // clear previous active
     clearSmallTicker();               // clear ticker count
     setActive();                      // make this cntl active
     setDelay(val);
     P2TheConsole->update4Sync(getSmallTicker());
     buffer[0] = ACQ_PAD;
     outputACode(SETACQSTATE, 1, buffer);
     return( 0 );
  }
  setGates(0);
  setDelay(LATCHDELAY); 
  // RF RELAY CONTROL

  // set relay on rf3 for routing LO to LO selector or LO#2 OUT

  setAux(0 << 8 | 1);  // RF relay #1  for rf3 LO to NO


  //  a stub for locktc.. lock servo loop time constant.
  //  LOCK TC STUFF ...
  //  ---------------------------------------
  if ( P_getreal(CURRENT, "locktc", &tmpval, 1) < 0 )
  {
     if ( P_getreal(GLOBAL, "locktc", &tmpval, 1) < 0 )
        tmpval = 1.0;
  }
  lkflt_fast = (int) (tmpval + 0.5);
  if ( P_getreal(CURRENT, "lockacqtc", &tmpval, 1) < 0 )
  {
     if ( P_getreal(GLOBAL, "lockacqtc", &tmpval, 1) < 0 )
        tmpval = 4.0;
  }
  lkflt_slow = (int) (tmpval + 0.5);
  if ( (lkflt_fast < 1) || (lkflt_fast > 4) ) lkflt_fast=(int)1;
  if ( (lkflt_slow < 1) || (lkflt_slow > 4) ) lkflt_slow=(int)4;
  if ( P_getstring(CURRENT, "alock", estring, 1, MAXSTR) < 0)
     abort_message("PSG: Cannot find alock");
  else
     lkmode = (estring[0]=='u') ? 4 : 0;
  P_getstring(CURRENT,"tn",estring,1,MAXSTR);
  /* condition the observe channel */
  if ( ! strcmp(estring,"lk"))
  {  lkmode |= 0x8;		/* so this is set already */
     outputACode(TNLK,0,buffer);
  }
  safeAuxByte = ((lkflt_slow - 1) & 3) | lkmode; 
  //  user bits.
  //  setAux(2 << 8 | 0);  
  //  T/R gate select
   
  setAux(3 << 8 | 1);  
  // 
  //  
  // lock parameters
  // 
  shimset = init_shimnames( GLOBAL );
  buffer[0] = shimset;
  if ( P_getstring(GLOBAL, "gradtype", estring, 1, MAXSTR) < 0)
     abort_message("PSG: Cannot find gradtype");
  else
  {  if ( strcmp(estring,"rrr") == 0 ) 
     {  buffer[0] |=  0xF0000;	// why F? Because V is not a hex value
        XYZgradflag = 1;
     }
     else
     {
        XYZgradflag = 0;
     }
  }
  if ( P_getreal(GLOBAL, "z0", &tmpval, 1) < 0 )
       abort_message("PSG: Cannot find z0");
    else
       buffer[1] = (int) sign_add(tmpval,0.5);
  if ( P_getreal(GLOBAL, "lockgain", &tmpval, 1) < 0 )
       abort_message("PSG: Cannot find lockgain");
    else
       buffer[2] = (int) (tmpval + 0.5);
  if ( P_getreal(GLOBAL, "lockpower", &tmpval, 1) < 0 )
       abort_message("PSG: Cannot find lockpower");
    else
       buffer[3] = (int) (tmpval + 0.5);
  lockpower = tmpval;
  if ( P_getreal(GLOBAL, "lockphase", &tmpval, 1) < 0 )
       abort_message("PSG: Cannot find lockphase");
    else
       buffer[4] = (int) (tmpval + 0.5);

  if ( P_getreal(GLOBAL, "h1freq", &tmpval, 1) < 0)
       abort_message("PSG: Cannot find h1freq");
  else
       h1freq = int(tmpval);
  if ( P_getreal(GLOBAL, "lockfreq", &tmpval, 1) < 0)
       abort_message("PSG: Cannot find lockfreq");
  else
       getlkfreqDDsi(tmpval,h1freq,fword.ifreq);
  // fword.freq = 7.678123L;
  buffer[5] = fword.ifreq[0];
  buffer[6] = fword.ifreq[1];
  buffer[7] = 0xa5000000 | (safeAuxByte << 16) |
                           ((lkflt_fast-1)<<8) | 
                           (lkflt_slow-1);
  outputACode(LOCKINFO,8,buffer);
  
  /********************************/
  /* Tune Information             */
  /********************************/
  spintype = spinnerStringToInt();;
  if (P_getreal(GLOBAL,"numrfch",&tmpval,1) < 0)
     numrfch=0;
  else
     numrfch = (int)tmpval;
  buffer[0] = numrfch;
  for (i=1; i<= numrfch; i++)                             //for all rf cntrls
  {
      buffer[i] = ((RFController *)(P2TheConsole->RFUserTable[i-1]) ) ->
		 getBaseFrequency() > 405.0? 0:1;        //store the mixer band
  }
  i = numrfch+1;
  /* and store the bearing air on level 'bearLevel' */
  if ( P_getreal(GLOBAL,"liqbear", &tmpval,1) < 0)
     buffer[i++] = 0xa000;	// default to 0xa000
  else
     buffer[i++] = (int)(tmpval + 0.5);
  /* and store the VT air flow */
  if ( P_getreal(GLOBAL,"vtflowrange", &tmpval,1) < 0)
  {
     vtrange = 25;
  }
  else
  {
     vtrange = (int) (tmpval + 0.5);
     if (vtrange < 25)
        vtrange = 25;
  }
  if ( P_getreal(GLOBAL,"vtairflow", &tmpval,1) < 0)
     buffer[i++] = 10 * 25 / vtrange;		// default to  10 l/min
  else
     buffer[i++] = (int)(tmpval + 0.5) * 25 / vtrange;
  /* and store the VT air limit */
  if ( P_getreal(GLOBAL,"vtairlimits", &tmpval,1) < 0)
  {
     if ( (spintype==MAS_SPINNER) || (spintype==SOLIDS_SPINNER) )
     {
        buffer[i++] = 0x0200;   // default to 0x200 for solids
     }
     else
     { 
        double tmpflow;

        if ( (P_getreal(GLOBAL,"vtflowrange", &tmpflow,1) < 0) || (tmpflow < 30.0) )
        {
           buffer[i++] = 0x0307;	// vtflowrange 0 - 25
        }
        else
        {
           buffer[i++] = 0x0201;	// vtflowrange 0 - 50
        }
     }
  }
  else
     buffer[i++] = (int)(tmpval + 0.5);
  /* set the spinner type */
  buffer[i++] = spintype;
  outputACode(TINFO,i,buffer);
  if ( P_getstring(CURRENT,"pin",estring,1,MAXSTR) < 0)
     buffer[0]=HARD_ERROR;
  else
     switch( estring[0] )
     {
     case 'n':
         buffer[0]=0;
         break;
     case 'y':
         buffer[0]=HARD_ERROR;
         break;
     case 'w':
         buffer[0]=WARNING_MSG;
         break;
     }
  outputACode(PNEUTEST,1,buffer);
  /***********************************/
  /* Load Gradient Duty Cycle Limits */
  /***********************************/
  if ( P_getstring(GLOBAL, "gradtype", estring, 1, MAXSTR) < 0)
     abort_message("PSG: Cannot find gradtype");
  if ( (strcmp(estring,"rrr") == 0) || (strcmp(estring,"www") == 0) )
     download_master_decc_values("master1");

  /********************************/
  /*       Load Shims             */
  /********************************/
  codeBuffer = loadshims();
  /********************************/
  /*       Get Sample             */
  /********************************/
  /* getsamp();  insuffient time and resource, so it back here */


  /* Only allow loc=0 with the change command */
  if ( (loc == 0) && (setupflag != CHANGE) )
    locActive = 0;
  if ( (locActive != 0) && ((setupflag == GO) || (setupflag >= CHANGE)) )
  {
    double traymax, tmpval;
    buffer[0] = loc;
    if (getparmd("traymax","real",GLOBAL,&tmpval,1))
    {
      traymax=0;
    }
    else
    {
      traymax= (int) (tmpval + 0.5);
    }
    buffer[1] = ((traymax == 48) || (traymax == 96)) ? 1 : 0;
    /* This is for the new SCARA robot */
    if ( (traymax == 12) || (traymax == 97) )
       buffer[1] = 2;
    if ( (buffer[1] == 2) &&  ! P_getstring(GLOBAL,"sin",estring,1,MAXSTR))
    {
       if (estring[0] == 'y')
          buffer[1] = 1;
    }
    buffer[2] = ok2bumpflag;
 
    P2TheConsole->broadcastCodes(GETSAMP,3,buffer);
    /* delay(0.100); */  /* delays added to prevent DDR from reboot when spin=value, GMB 1/18/05 */
  }    
       
  /********************************/
  /*       Set Temperature        */
  /********************************/
  setvt();		/* And it includes fifoStop/Sync */
  /* At this point, get the temperature close but ignore any errors.
   * Subsequent eject air and sample insert and spinning changes may
   * knock VT out of regulation. Check again after those events.
   */
  if ((setupflag == GO) || (setupflag >= CHANGE))
     wait4vt(0);		/* And this includes fifoStop/Sync */
  oldvttemp = vttemp;		/* if arrayed, vttemp will get new value */



  /********************************/
  /*       Load Sample            */
  /********************************/
  /* loadsamp();  insuffient time and resource, so it back here */

  if ( (locActive != 0) && (loc > 0) && ((setupflag == GO) || (setupflag >= CHANGE)) )
  {
    double traymax, tmpval;

    buffer[0] = loc;

    if (getparmd("traymax","real",GLOBAL,&tmpval,1))
    {
      traymax=0;
    }
    else
    {
      traymax= (int) (tmpval + 0.5);
    }
    buffer[1] = ((traymax == 48) || (traymax == 96)) ? 1 : 0;
    /* This is for the new SCARA robot */
    if ( (traymax == 12) || (traymax == 97) )
       buffer[1] = 2;
    if ( (buffer[1] == 2) &&  ! P_getstring(GLOBAL,"sin",estring,1,MAXSTR))
    {
       if (estring[0] == 'y')
          buffer[1] = 1;
    }

    buffer[2] = spinactive;
    buffer[3] = ok2bumpflag;
    P2TheConsole->broadcastCodes(LOADSAMP,4,buffer);
    /* delay(0.100); */  /* delays added to prevent DDR from reboot when spin=value, GMB 1/18/05 */
  } 


  /********************************/
  /*       Set Spin               */
  /********************************/
  setspin();		/* And it includes fifoStop/Sync */
  wait4spin();		/* And this includes fifoStop/Sync */
  oldspin = spin;

  /* At this point, the other events that affect VT are done.
   * Make sure it is regulated and enable errors.
   */
  if ((setupflag == GO) || (setupflag >= CHANGE))
     wait4vt(1);		/* And this includes fifoStop/Sync */

  /********************************/
  /*       Set NSR States         */
  /********************************/
  k = trInterconnect.packOutputBuffer(buffer,0);
  outputACode(NSRVSET,k,buffer);    
  setAux(trInterconnect.the_TR_Selector.getCode()); // programs the TR selector
  setGates(0);
  // hipwrampenable is y to use - physical channel.
  if (P_getstring(GLOBAL,"hipwrampenable",estring,1,MAXSTR) < 0) {
      // default clause - no hipwrampenable - volumexmt active??
      i=0;
      if (P_getstring(CURRENT,"volumexmt",estring,1,MAXSTR) >= 0)
      {
        if (estring[0] != 'n') i = 1; else i = 0; 
      }
  }
  else {
     if (estring[0] == 'y')  i = 1; else i = 0;  // Physical channel 1
     if (estring[1] == 'y')  i |= 2;
     if (estring[2] == 'y')  i |= 4;
     if (estring[3] == 'y')  i |= 8;
  }

  // if ProPulse setAux(0x501); ?? 
  // equivalent to hipwrampenable='ynnnnn'

  setAux((5<<8) | i);
  i = P2TheConsole->getConsoleRFInterface();
  if ((i == 2) || (i == 3))
  {
     // printf("Solids Relay AUX instruction set to 1\n");
     setAux(0x501);     // ProPulse LO Hi bit
  }
  setDelay(LATCHDELAY);
  setAux(trInterconnect.the_LO_Selector.getCode());
  setDelay(LATCHDELAY); 

  /******************/
  /* Autolock       */
  /******************/
  doAutolock();

  /******************/
  /* Autoshim       */
  /******************/
  translate_string(shimset);
  int shimbuffer[1024];
  shimbuffer[2] = 0;
  initwshim(shimbuffer);
  if (shimbuffer[2] > 0)
  {
     P2TheConsole->broadcastCodes(SHIMAUTO,shimbuffer[2] + 3,shimbuffer);  
  }
  
  /*
   * now that autolock and autoshim are done set verify lock acode
   *
   */
  if ( (interLock[0] == 'y') || (interLock[0] == 'Y') )
      buffer[0] = HARD_ERROR;
  else if ( (interLock[0] =='w') || (interLock[0] == 'W') )
      buffer[0] = WARNING_MSG;
  else
      buffer[0] = 0;
  if (setupflag) buffer[0]=0;
  outputACode(LOCKCHECK, 1, buffer);
  /******************/
  /* Autogain       */
  /******************/

  /************************/
  /******** BLAF SETUP ****/
  /************************/
  if (setupflag == GO) 
  {
     blafMode = 0; 
     if (P_getstring(GLOBAL, "hdwshim", estring, 1, 3) == 0) 
     {
       if (estring[ 0 ] == 'y' || estring[ 0 ] == 'Y')
         blafMode = 1;
       if (estring[ 0 ] == 'p' || estring[ 0 ] == 'P')
         blafMode = 2;
      }
     if (blafMode > 0)
     { 
        strcpy( estring, "" );
        k = 0;
        if (P_getstring(GLOBAL, "hdwshimlist",estring, 1, MAXSTR) == 0) 
        {
          if (strstr(estring,"z1") != 0) k |= 4;
          if (strstr(estring,"z2") != 0) k |= 0x10;
          if (strstr(estring,"z3") != 0) k |= 0x40;
          if (strstr(estring,"x1") != 0) k |= 0x10000;
          if (strstr(estring,"y1") != 0) k |= 0x20000;
          if (strstr(estring,"xy") != 0) k |= 0x80000;
          if (strstr(estring,"x2y2") != 0) k |= 0x100000;
          if (k == 0) k = 2;
        }
        buffer[0] = 0x101; /* standard */
        buffer[1] = k;
        buffer[2] = 0; 
        outputACode(SSHA,3,buffer);
     }
     if ( option_check("autogain") )
     {
        buffer[0] = ACQ_HOSTGAIN;
        outputACode(SETACQSTATE, 1, buffer);
     }
     else if ( option_check("shim") )
     {
        buffer[0] = ACQ_HOSTSHIM;
        outputACode(SETACQSTATE, 1, buffer);
     }
     else if ( option_check("tune") )
     {
        buffer[0] = ACQ_PROBETUNE;
        outputACode(SETACQSTATE, 1, buffer);
     }
     else if ( option_check("findz0") )
     {
        buffer[0] = ACQ_FINDZ0;
        outputACode(SETACQSTATE, 1, buffer);
     }
  }
  return((long long) 8);
}

// only support for all gains the same
// num rcvrs > 4 causes master to return delay longer....
// 
int MasterController::initializeIncrementStates(int total)
{
  double gain;
  int itemp,i,j,k;
  int gain2Defined=0;
  long long ticks;
  clearSmallTicker();
  setGates(0);  

  gain = getval("gain"); 
  // check if different..
  itemp = (int) (gain+0.5);

  if (var_active("gain2",CURRENT)==1)
    gain2Defined = 1;

  if (trInterconnect.the_NSR_Array[0].getGainSetting(0) != itemp)
  {
    //cout << "RT GAIN UPDATE " <<  endl;
    trInterconnect.setRTGainData(itemp);
    if (trInterconnect.numRcvrs == 1)
    {
      // can't use mixer 4 cause it will alter observe/tune selectors too.
      k = trInterconnect.getGainCode(0);
      RT_spi(1, k);
      setDelay(LATCHDELAY);
    }
    else
    {
      for (i=0; i < trInterconnect.numNSRS; i++)
      {
          if (trInterconnect.usageFlag != MULTNUCRCVR_MULTACQ)
          {
            trInterconnect.setBankGainCode(itemp,i);
            k = trInterconnect.getBankGainCode(i);
            RT_spi(1, k);
            if (i != (trInterconnect.numNSRS-1))  // skip the last one
               setDelay(0.000020); // 4 usec extra...
          }
          else
          {
             if ( gain2Defined )
             {
                  k = trInterconnect.getGainCode(0);
                  RT_spi(1, k);
                  setDelay(0.000020);
             }
             else
             {
               for (j=0; j < trInterconnect.numRcvrs; j++)
               {
                  k = trInterconnect.getGainCode(j);
                  RT_spi(1, k);
                  setDelay(0.000020);
               }
             }
          }
        }
    }
  }

  /* Blaf code */
  if (blafMode & 1)  // armed for first USER delay (via Bridge)
  {
      blafMode |= 4; // auto off
      add2Stream(GATEKEY| 0x20020); // 
  }

  ticks = getSmallTicker();
  if (ticks > total) 
    {
      return((int) ticks);
    }
  else
     setTickDelay((long long)(total - ticks));  

  clearSmallTicker();
  return(0);
}

int MasterController::endExperimentState()
{
  return(2);
}

/* this is a config parameter - it takes no real time */

void MasterController::RollCall(char *configString)
{
  int i,j,  buffer[MAX_ROLLCALL_STRLEN/4];
  for (i=0; i < MAX_ROLLCALL_STRLEN/4; i++) 
    buffer[i] = 0;  // safety nulls..
  i = strlen(configString);
  if (i > MAX_ROLLCALL_STRLEN )
    exit(-1);  // string too large 
  strcpy((char *)&(buffer[1]),configString);
  j = (i/4 + 1);  //  integers..
#ifdef LINUX
  for (i=0; i <= j; i++)
     buffer[i] = htonl(buffer[i]);
#endif
  buffer[0] = j;
  outputACode(ROLLCALL,j+1,buffer);
  return;
}

/*------------------------------------------------------------------
|	loadshims()/0
|	Load shims if load == 'y'.  Will be set by arrayfuncs if
|	any shims are arrayed.
+-----------------------------------------------------------------*/
int * MasterController::loadshims()
{
char  load[MAXSTR];
const char *sh_name;
double tmpdelay;
int   buffer[20];
int   index,ticks;
int   num_shims;
long long ldshimdelay, onshimdelay;
   if ((P_getstring(CURRENT,"load",load,1,15) >= 0) &&
      ((load[0] == 'y') || (load[0] == 'Y')) )
   {
      /* declare new event */
      P2TheConsole->newEvent(); // clear previous active
      clearSmallTicker();               // clear ticker count
      setActive();                      // make this cntl active

      setDelay(0.000028);//protect from prior calls...
      P2TheConsole->update4Sync(getSmallTicker());

      /* declare new event */
      P2TheConsole->newEvent(); // clear previous active
      clearSmallTicker();               // clear ticker count
      setActive();                      // make this cntl active

      
      /* get  number of shim for thsi shimset value, it is used several times */
      num_shims = shimCount();

      /* set load back to 'n' */
      P_setstring(CURRENT,"load","n",0);

      /* deal wiith ldshimdelay and oneshimdelay here */
      /* Shimset 1,2,10 use 16 bit, for 15 shims (Z0 is not set here) */
      /* RRI, use count * onshimdelay (sec), but GradientController   */
      /*      sets X,Y,Z1C; sync delay is adjusted for that by 3*17*80*/
      /*      GC uses SPI, 16 bit                                     */
      /* Other shimset  use 24 bit, for 39 shims (Z0 is not set here) */
      /* Then the 80 ticks of 12.5nsec = 1 usec	 		      */
      if ( (shimset<3) || (shimset==10) )
      {  oneshimdelay = 17 * 80;		/* usec/shim * 8 */
         ldshimdelay  = 15 * oneshimdelay;
      }
      else if ( isRRI() )
      {  oneshimdelay = (int)(0.3 * 80000000.0);
         ldshimdelay  = num_shims * oneshimdelay; /* 11.7 second */
      }
      else
      {  oneshimdelay = 25 * 80;		/* usec/shim * 8 */
         ldshimdelay  = 39 * oneshimdelay;
      }
      tmpdelay = -1.0;
      if ( P_getreal(CURRENT,"ldshimdelay",&tmpdelay,1) < 0 )
      {
         P_getreal(GLOBAL,"ldshimdelay",&tmpdelay,1);
      }
      if (tmpdelay > 0.0)
      {   ldshimdelay = (long long)(tmpdelay * 80000000.0);
          onshimdelay = ldshimdelay / num_shims;
      }
      tmpdelay = -1.0;
      /* Get parser synchronization delay, for one shim for testing  */
      if ( P_getreal(CURRENT,"oneshimdelay",&tmpdelay,1) < 0 )
      {
         P_getreal(GLOBAL,"oneshimdelay",&tmpdelay,1);
      }
      if (tmpdelay > 0.0)
      {  oneshimdelay = (long long)(tmpdelay * 80000000.0);
      }
      if (oneshimdelay > 0x3ffffff) 
         codes[0] = 0x3ffffff;
      else
         codes[0] = (int)oneshimdelay;

      /* Get the shims, 47 of them, skip z0 */
      codes[Z0] = MAX_SHIMS ;	/* use this slot, it is skipped */
      for (index= Z0 + 1; index < MAX_SHIMS; index++)
      {
         if ((sh_name = get_shimname(index)) != NULL)
         {
            codes[index] =  (int) sign_add(getval(sh_name), 0.0005);
         }
         else {
           codes[index] = 0;
         }
      }
      outputACode(SETSHIMS, 48, codes);
      if ( (isRRI()  || XYZgradflag ) && (shimset != 5) )
      {  // set X,Y,Z1C through grad controller if present
         // get GradController(), write acodes for shims
         GradientController *gc =  
                 (GradientController *)P2TheConsole->getControllerByID("grad1");
         if (gc != 0)
         {  buffer[0] = 17 * 80;		/* time is 17 usec, hardcoded */
            buffer[1]=  3;		/* load 3 shims */
            if (isRRI()) {
               buffer[2] = 3;		/* Z1C for RRI shim */
               buffer[3] = codes[3];  
            }
            else {
               buffer[2] = 2;		/* Z1 for Varian shim */
               buffer[3] = codes[2];
            }
            buffer[4] = 16;		/* X shim */
            buffer[5] = codes[16];  
            buffer[6] = 17;		/* Y shim */
            buffer[7] = codes[17];  
            gc->outputACode(SETSHIMS, 8, buffer);
//            gc->noteDelay(3 * 20 * 80);	// multiple of 4 usec, > 16 
         }
         else
         {  abort_message("shimset is RRI, but no gradient controller configured");
         }
      }
      /* Shimset 1,2,10 use 16 bit, for 15 shims (Z0 is not set here)	*/
      /* RRI, use count * onshimdelay (sec), but GradientController	*/
      /*      sets X,Y,Z1C; sync delay is adjusted for that by 3*17*8	*/
      /*      GC uses SPI, 16 bit					*/
      /* Other shimset  use 24 bit, for 39 shims (Z0 is not set here)	*/
      if ( (shimset<3) || (shimset==10) )
      {  ticks = num_shims * oneshimdelay;	/* shimCount * usec/shim * 80 */
      }
      else if ( isRRI() )
      {   // putting a long delay cause sync problems
          // The value here is excessive. It is not used anyway
          // For now set it small (would zero work?), left as placeholder
	  // At leaset 4 usec tho
          // ticks = (int)(num_shims * oneshimdelay);
          ticks = 320;
      }
      else
      {  ticks = 39 * oneshimdelay;	 /* 39 * usec/shim * 80 */
      }

      noteDelay(ticks);			// set big/small ticker

      P2TheConsole->update4Sync(ticks);

   }
   return(NULL);
}
//
//  the spi takes about 30 usec to run - repeated calls
//  with smaller delays will fail. Timing is done in the
//  calling context.
//  SPI 0 is the shim supply and SPI 1 is the signal router 
//
//  SPI 1 uses 16 bits delay of 20 usec used 16 is too short.
//    multiple routers  
//  CHIP SELECT IS embedded in value...

int MasterController::RT_spi(int number, int value)
{
  int tempw;
  tempw = (value & 0x0fffffff) | ((number & 1) << 28) |  0x40000000;
  add2Stream(tempw);
  return(0);
}  

/*set4TuneA56:
   chan is the lo source
   select chan usually equals chan but shared mode uses 
   different
*/
int MasterController::set4TuneA56(int chan, int gain, int hiband, int cryo)
{
  // LO selection is here! 
  // 
  // printf("set4TuneA56 called with chan=%d  hiband=%d  cryo=%d\n",chan,hiband,cryo);
  setAux(trInterconnect.the_LO_Selector.select_LO_A56(chan));  
  trInterconnect.the_NSR_Array[0].setPreAmpSelect(0); /* turn off the preamp's */
  trInterconnect.the_NSR_Array[0].setTRSelect(0);     /* all in T */
  trInterconnect.the_NSR_Array[0].setMixerHighBand(0,hiband);  
  trInterconnect.the_NSR_Array[0].setMixerGain(0,gain);
  trInterconnect.the_NSR_Array[0].setLockAtten(0); 
  trInterconnect.the_NSR_Array[0].setMixerInput(0,3);  // tune selector port
  if (cryo == 1) chan += 10;
  switch (chan) {
  case 1:
     trInterconnect.the_NSR_Array[0].setMixerInput(1,3);  // tune selector port
     trInterconnect.the_NSR_Array[0].setMixerHighBand(1,0);  // tune ports.
     trInterconnect.the_NSR_Array[0].setMixerGain(1,0);
     //trInterconnect.the_NSR_Array[0].setMixerGain(1,2);
     break;
  case 2:
     trInterconnect.the_NSR_Array[0].setMixerInput(1,3);  // tune selector port
     trInterconnect.the_NSR_Array[0].setMixerHighBand(1,1);  // tune ports.
     trInterconnect.the_NSR_Array[0].setMixerGain(1,2);
     //trInterconnect.the_NSR_Array[0].setMixerGain(1,0);
     break;
  case 3:
  case 13:
     trInterconnect.the_NSR_Array[0].setMixerInput(1,2);  // tune selector port
     trInterconnect.the_NSR_Array[0].setMixerHighBand(1,1);  // tune ports.
     trInterconnect.the_NSR_Array[0].setMixerGain(1,2);
     break; 
  case 11:
     trInterconnect.the_NSR_Array[0].setMixerInput(1,1);  // tune selector port
     trInterconnect.the_NSR_Array[0].setMixerHighBand(1,0);  // tune ports.
     trInterconnect.the_NSR_Array[0].setMixerGain(1,2);
     trInterconnect.the_NSR_Array[0].setMixerInput(2,2);  // tune selector port
     trInterconnect.the_NSR_Array[0].setMixerHighBand(2,1);  // tune ports.
     trInterconnect.the_NSR_Array[0].setMixerGain(2,2);
     break;
  case 12:
     trInterconnect.the_NSR_Array[0].setMixerInput(1,1);  // tune selector port
     trInterconnect.the_NSR_Array[0].setMixerHighBand(1,1);  // tune ports.
     trInterconnect.the_NSR_Array[0].setMixerGain(1,2);
     trInterconnect.the_NSR_Array[0].setMixerInput(2,1);  // tune selector port
     trInterconnect.the_NSR_Array[0].setMixerHighBand(2,1);  // tune ports.
     trInterconnect.the_NSR_Array[0].setMixerGain(2,2);
     break;
   }
//
  setTickDelay(1600); //  20 microseconds (1 extra) 
  RT_spi(1,trInterconnect.the_NSR_Array[0].getNSRWord(2));  //  necessary ... 
  setTickDelay(1600); //  20 microseconds (1 extra)  
  RT_spi(1,trInterconnect.the_NSR_Array[0].getNSRWord(3));
  setTickDelay(1600); //  20 microseconds (1 extra) 
  RT_spi(1,trInterconnect.the_NSR_Array[0].getNSRWord(4));
  setTickDelay(1600); //  20 microseconds (1 extra)
  RT_spi(1,trInterconnect.the_NSR_Array[0].getNSRWord(5));
  setTickDelay(1600); //  20 microseconds (1 extra)
  return 0;
}


/*
   set4TuneA56_3ch
   chan is the lo source
   select chan usually equals chan but shared mode uses different
*/
int MasterController::set4TuneA56_3ch(int chan, int gain, int hiband, int cryo)
{
  // LO selection is here! 
  // 
  // printf("set4TuneA56_3ch called with chan=%d  hiband=%d  cryo=%d\n",chan,hiband,cryo);
  setAux(trInterconnect.the_LO_Selector.select_LO_A56(chan));  
  trInterconnect.the_NSR_Array[0].setPreAmpSelect(0); /* turn off the preamp's */
  trInterconnect.the_NSR_Array[0].setTRSelect(0);     /* all in T */
  trInterconnect.the_NSR_Array[0].setMixerHighBand(0,hiband);  
  trInterconnect.the_NSR_Array[0].setMixerGain(0,gain);
  trInterconnect.the_NSR_Array[0].setLockAtten(0); 
  trInterconnect.the_NSR_Array[0].setMixerInput(0,3);  // tune selector port
  if (cryo == 1) chan += 10;

  switch (chan) {

  case 1:                                                       // High Band
     if (P2TheConsole->getConsoleHFMode() == 'c')               // H/F Combined mode
     {
        trInterconnect.the_NSR_Array[0].setPP2Ch3TuneSelect(1); // Ch3 Tune to HB
        trInterconnect.the_NSR_Array[0].setMixerInput(1,2);     // tune selector port
        trInterconnect.the_NSR_Array[0].setMixerHighBand(1,1);  // tune selector port
        trInterconnect.the_NSR_Array[0].setMixerGain(1,0);      // tune selector port
        // fprintf(stdout,"Tune Ch1 combined mode select Ch3TUNE to HB, tune port 2, mixer port 3\n");
     }
     else                                                       // H/F Independent mode
     {
        trInterconnect.the_NSR_Array[0].setMixerInput(1,3);     // tune selector port
        trInterconnect.the_NSR_Array[0].setMixerHighBand(1,0);  // tune selector port
        trInterconnect.the_NSR_Array[0].setMixerGain(1,0);      // tune selector port
        // fprintf(stdout,"Tune Ch1 indepent mode select              , tune port 4, mixer port 3\n");
     }
     break;

  case 2:
     trInterconnect.the_NSR_Array[0].setMixerInput(1,3);  // tune selector port
     trInterconnect.the_NSR_Array[0].setMixerHighBand(1,1);  // tune ports.
     trInterconnect.the_NSR_Array[0].setMixerGain(1,2);
     // fprintf(stdout,"Tune Ch2          mode select                tune port 5, mixer port 3\n");
     break;

  case 3:
  case 13:
     if ( hiband )           // RF channel 3 is HB channel and will always pass thru combiner
     {
        trInterconnect.the_NSR_Array[0].setPP2Ch3TuneSelect(1); // Ch3 Tune to HB
        trInterconnect.the_NSR_Array[0].setMixerInput(1,2);     // tune selector port
        trInterconnect.the_NSR_Array[0].setMixerHighBand(1,1);  // tune selector port
        trInterconnect.the_NSR_Array[0].setMixerGain(1,0);      // tune selector port
        // fprintf(stdout,"Tune Ch3 HB       mode select Ch3TUNE to HB, tune port 2, mixer port 3\n");
     }
     else                                                    // RF channel 3 is LB channel
     {
        trInterconnect.the_NSR_Array[0].setPP2Ch3TuneSelect(0); // Ch3 Tune to LB
        trInterconnect.the_NSR_Array[0].setMixerInput(1,2);     // tune selector port
        trInterconnect.the_NSR_Array[0].setMixerHighBand(1,1);  // tune selector port
        trInterconnect.the_NSR_Array[0].setMixerGain(1,0);      // tune selector port
        // fprintf(stdout,"Tune Ch3 LB       mode select Ch3TUNE to LB, tune port 2, mixer port 3\n");
     }
     break;

  case 11:
     // selects port 3 of cryo select switch
     trInterconnect.the_NSR_Array[0].setMixerInput(2,1);     // cryo selector port
     trInterconnect.the_NSR_Array[0].setMixerHighBand(2,1);  // cryo selector port
     trInterconnect.the_NSR_Array[0].setMixerGain(2,0);      // cryo selector port

     // selects port 3 of tune select switch
     trInterconnect.the_NSR_Array[0].setMixerInput(1,1);     // tune selector port
     trInterconnect.the_NSR_Array[0].setMixerHighBand(1,1);  // tune selector port
     trInterconnect.the_NSR_Array[0].setMixerGain(1,0);      // tune selector port
     // fprintf(stdout,"Tune Ch1 cryo     mode select cryo port 2 ,  tune port 3, mixer port 3 CORRECT???\n");
     break;

  case 12:
     // selects port 2 of cryo select switch
     trInterconnect.the_NSR_Array[0].setMixerInput(2,2);     // cryo selector port
     trInterconnect.the_NSR_Array[0].setMixerHighBand(2,1);  // cryo selector port
     trInterconnect.the_NSR_Array[0].setMixerGain(2,0);      // cryo selector port

     // selects port 3 of tune select switch
     trInterconnect.the_NSR_Array[0].setMixerInput(1,1);     // tune selector port
     trInterconnect.the_NSR_Array[0].setMixerHighBand(1,1);  // tune selector port
     trInterconnect.the_NSR_Array[0].setMixerGain(1,0);      // tune selector port
     // fprintf(stdout,"Tune Ch2 cryo     mode select cryo port 3 ,  tune port 3, mixer port 3 CORRECT???\n");
     break;

  default:
    abort_message("invalid RF channel assignment. abort!\n");
    break;
   }
//
  setTickDelay(1600); //  20 microseconds (1 extra) 
  RT_spi(1,trInterconnect.the_NSR_Array[0].getNSRWord(2));  //  necessary ... 
  setTickDelay(1600); //  20 microseconds (1 extra)  
  RT_spi(1,trInterconnect.the_NSR_Array[0].getNSRWord(3));
  setTickDelay(1600); //  20 microseconds (1 extra) 
  RT_spi(1,trInterconnect.the_NSR_Array[0].getNSRWord(4));
  setTickDelay(1600); //  20 microseconds (1 extra)
  RT_spi(1,trInterconnect.the_NSR_Array[0].getNSRWord(5));
  setTickDelay(1600); //  20 microseconds (1 extra)

  RT_spi(1,trInterconnect.the_NSR_Array[0].getNSRWord(14)); // ProPulse INNOVA settings
  setTickDelay(1600); //  20 microseconds (1 extra)

  return 0;
}


/* set4Tune:
   chan is the lo source
   select chan usually equals chan but shared mode uses 
   different
*/
int MasterController::set4Tune(int chan, int selectchan, int gain, int hiband)
{
  // LO selection is here! 
  // printf("set4Tune called with chan=%d  hiband=%d\n",chan,hiband);
 
  int rfInterface = P2TheConsole->getConsoleRFInterface();

  if (rfInterface == 2)
  {
     set4TuneA56(chan,gain,hiband,0);
     return 0;
  }
  else if (rfInterface == 3)
  {
     set4TuneA56_3ch(chan,gain,hiband,P2TheConsole->getConsoleCryoUsage());
     return 0;
  }

  setAux(trInterconnect.the_LO_Selector.select_LO(chan));  
  trInterconnect.the_NSR_Array[0].setPreAmpSelect(0); /* turn off the preamp's */
  trInterconnect.the_NSR_Array[0].setTRSelect(0);     /* all in T */
  trInterconnect.the_NSR_Array[0].setMixerHighBand(0,hiband);  
  trInterconnect.the_NSR_Array[0].setMixerGain(0,gain);
  trInterconnect.the_NSR_Array[0].setLockAtten(0); //<<<< IMPROVE 
  trInterconnect.the_NSR_Array[0].setMixerInput(0,3);  // tune selector port
  // -- mixer 0 set.. 
  // -- buid tune selector #1
  trInterconnect.the_NSR_Array[0].setMixerHighBand(1,1);  // tune ports.
  // if (trInterconnect.numRcvrs == 1) 
    {
      switch (selectchan) {
        case 1:  trInterconnect.the_NSR_Array[0].setMixerInput(1,0);   break;
        case 2:  trInterconnect.the_NSR_Array[0].setMixerInput(1,1);   break;
        case 3:  trInterconnect.the_NSR_Array[0].setMixerInput(1,2);   break;
        case 4:  trInterconnect.the_NSR_Array[0].setMixerInput(1,3);    // NEXT TUNE SELECTOR
                 trInterconnect.the_NSR_Array[0].setMixerHighBand(2,1); // tune ports.
                 trInterconnect.the_NSR_Array[0].setMixerInput(2,0);    // #2 port 1.
	         break; 
        case 5:  trInterconnect.the_NSR_Array[0].setMixerInput(1,3);    // NEXT TUNE SELECTOR
                 trInterconnect.the_NSR_Array[0].setMixerHighBand(2,1); // tune ports.
                 trInterconnect.the_NSR_Array[0].setMixerInput(2,1);    // #2 port 2.
	         break;
// case 9 is experimental
        case 9:  trInterconnect.the_NSR_Array[0].setMixerInput(1,3);    // NEXT TUNE SELECTOR
                 trInterconnect.the_NSR_Array[0].setMixerHighBand(2,1); // tune ports.
                 trInterconnect.the_NSR_Array[0].setMixerInput(2,3);    // #2 port 3.
	         break;

        default:  abort_message("bad tune channel ID\n");
       } 
    }
    //else
    {   // Multi Receiver SUPPORT
       trInterconnect.auxNSRControl.setMixerHighBand(0,1);  // tune ports on the selector..
       if (trInterconnect.usageFlag ==	MULTNUCRCVR_SINGACQ)
           setAux(0 << 8 | 0);  // RF relay #1  for rf3 LO to NC

       switch (selectchan)
       {
         case 1:  trInterconnect.auxNSRControl.setMixerInput(0,0);  break;
         case 2:  trInterconnect.auxNSRControl.setMixerInput(0,1);  break;
         case 3:  trInterconnect.auxNSRControl.setMixerInput(0,2);  break;
         case 4:  trInterconnect.auxNSRControl.setMixerInput(0,3); // NEXT TUNE SELECTOR
                  trInterconnect.auxNSRControl.setMixerHighBand(1,1); // tune ports.
                  trInterconnect.auxNSRControl.setMixerInput(1,0); // #2 port 1.
	          break; 
         case 5:  trInterconnect.auxNSRControl.setMixerInput(0,3); // NEXT TUNE SELECTOR
                  trInterconnect.auxNSRControl.setMixerHighBand(1,1); // tune ports.
                  trInterconnect.auxNSRControl.setMixerInput(1,1); // #2 port 1.
	   break;
//  == experimental
         case 9:  trInterconnect.auxNSRControl.setMixerInput(0,3); // NEXT TUNE SELECTOR
                  trInterconnect.auxNSRControl.setMixerHighBand(1,1); // tune ports.
                  trInterconnect.auxNSRControl.setMixerInput(1,3); // #2 port 4.
	   break;


         default:  abort_message("bad tune channel ID\n");
        } 
   }

  setTickDelay(1600); //  20 microseconds (1 extra) 
  RT_spi(1,trInterconnect.the_NSR_Array[0].getNSRWord(2));  //  necessary ... 
  setTickDelay(1600); //  20 microseconds (1 extra)  
  //if (trInterconnect.numRcvrs == 1) 
    {
       RT_spi(1,trInterconnect.the_NSR_Array[0].getNSRWord(3));
       setTickDelay(1600); //  20 microseconds (1 extra) 
       RT_spi(1,trInterconnect.the_NSR_Array[0].getNSRWord(4));
       setTickDelay(1600); //  20 microseconds (1 extra)
       RT_spi(1,trInterconnect.the_NSR_Array[0].getNSRWord(5));
       setTickDelay(1600); //  20 microseconds (1 extra)
    }
    //else
     {
       RT_spi(1,trInterconnect.auxNSRControl.getNSRWord(3));
       setTickDelay(1600); //  20 microseconds (1 extra) 
       RT_spi(1,trInterconnect.auxNSRControl.getNSRWord(4));
       setTickDelay(1600); //  20 microseconds (1 extra)
    }   
  return 0;
}

int MasterController::setFPowerMagnus(int gain)
{ // hiband hard coded!
  trInterconnect.the_NSR_Array[0].setPreAmpSelect(0); /* turn off the preamp's */
  trInterconnect.the_NSR_Array[0].setTRSelect(0);     /* all in T */
  trInterconnect.the_NSR_Array[0].setMixerHighBand(0,1);  
  trInterconnect.the_NSR_Array[0].setMixerGain(0,gain);
  trInterconnect.the_NSR_Array[0].setMixerInput(0,1);  // magnus forward power port..
  setTickDelay(1600); //  20 microseconds (1 extra) 
  RT_spi(1,trInterconnect.the_NSR_Array[0].getNSRWord(2));  //  necessary ... 
  setTickDelay(1600); //  20 microseconds (1 extra)  
  RT_spi(1,trInterconnect.the_NSR_Array[0].getNSRWord(3));  // necessary ..
  setTickDelay(1600); //  20 microseconds (1 extra)
  return 0;
}
//
// this element sorts out the NSR, LO selection, and the TR gates 
// it is temporarily confined to 2 observe channels.
//
// adds gain, pre-sig coil bias lock atten
void MasterController::setConsoleMap(int kind, int obschannel, int decchannel, int hiband, int lkflag)
{
  int nRcvrs,NSRModeFlag;
  char rcvrStr[MAXSTR],presigStr[MAXSTR];
  double dtmp;
 
  NSRModeFlag = 0; // off..
  if (P_getreal(GLOBAL,"numrcvrs", &dtmp, 1) < 0)
      nRcvrs = 1;
  else 
     nRcvrs = (int) (dtmp + 0.49);
  trInterconnect.usageFlag = kind;
  trInterconnect.numRcvrs = nRcvrs;
  getStringSetDefault(CURRENT,"rcvrs",rcvrStr,"ynnnn");
  strcpy(trInterconnect.rcvrS,rcvrStr);
  getStringSetDefault(CURRENT,"presig",presigStr,"nnnn");
  if (strncmp(presigStr, "h",1) == 0)
     NSRModeFlag |= PRESIGBIT;
  getStringSetDefault(CURRENT,"hblock",presigStr,"nnnn");
  if (strncmp(presigStr, "h",1) == 0)
     NSRModeFlag |= HBLOCKBIT;
  getStringSetDefault(CURRENT,"volumercv",presigStr,"ynnn");
  if ((presigStr[0] == 'y') || (strlen(presigStr) == 0))
     NSRModeFlag |= VOLUMERCVRBIT;
  getStringSetDefault(GLOBAL,"imgwlock",presigStr,"nnnn");
  if ((presigStr[0] == 'y') || (strlen(presigStr) == 0))
     NSRModeFlag |= IMGWLOCKBIT;
  trInterconnect.gain[0] = (int) (0.49+getval("gain"));
  trInterconnect.gain[1] = trInterconnect.gain[0];
  if (var_active("gain2",CURRENT)==1)
    trInterconnect.gain[1] = (int) (0.49+getvalnwarn("gain2"));

  if ( P_getreal(GLOBAL, "lockpower", &dtmp, 1) < 0 )
       abort_message("PSG: Cannot find lockpower");
  if (dtmp > 48.0) 
    NSRModeFlag |= LOCKATTENBIT;
  trInterconnect.modeFlags = NSRModeFlag;
  trInterconnect.hibandflag = hiband;
  trInterconnect.lockflag = lkflag;
  trInterconnect.observe1ch = obschannel;
  trInterconnect.observe2ch = 3; // was decchannel
  trInterconnect.initialize();   // maps out but no output
  //trInterconnect.tell();
}

void MasterController::homospoilGradient(char axis, double value)
{
int  ampI,codeStream[10];
  switch (axis)
  {
    case 'x': case 'X': case 'y': case 'Y':
      abort_message("homospoil gradient %c not available\n",axis);
      break;
    case 'z': case 'Z': break;
    default: ;
  }
  ampI = (int)(value+0.5);
  if (ampI)	/* homospoil_on */
  {
     codeStream[0] = 1;
  }
  else		/* homospoil_off */
  {
     codeStream[0] = 0;
  }
  outputACode(HOMOSPOIL,1,codeStream);
  // grad_flag = TRUE;


}

#define Z1_DAC  2	/* Using shims for gradients */
#define Z1C_DAC 3	/* Using shims for gradients */
#define X1_DAC 16	/* Using shims for gradients */
#define Y1_DAC 17	/* Using shims for gradients */

void MasterController::shimGradient(char axis, double value)
{
double shimD = 0.0;
int    dac_num = 0;
int    shimI,codeStream[10];
  // cout << "shimGradient: axis=" << axis << "value=" << value << endl;

  if (shimset < 0)		// not likely, but...
     shimset = init_shimnames( GLOBAL );
     
  switch (axis) {
  case 'x': case 'X':  shimD = getval("x1"); dac_num = X1_DAC; break;
  case 'y': case 'Y':  shimD = getval("y1"); dac_num = Y1_DAC; break;
  case 'z': case 'Z':
      switch (shimset) {
      case 1: case 2: case 5: case 10:
          shimD = getval("z1c"); dac_num = Z1C_DAC; break;
      default:
          shimD = getval("z1");  dac_num = Z1_DAC;  break;
      }
      break;
  default:
      abort_message("shimgradient %c not available\n",axis);
  }

  shimI = (shimD >= 0) ? (int)(shimD+0.5) :(int)(shimD-0.5);
  shimI += (int)(value+0.5);
  
  switch (shimset) {
  case 1: case 2: case 10: case 11: shimI = bound(axis, shimI,12);
  default: shimI = bound(axis, shimI, 16);
  }

  codeStream[0] = 1;		// set one shim
  codeStream[1] = dac_num;	// which shimdac
  codeStream[2] = shimI;	// to which value
  outputACode(SYNCSETSHIMS,3,codeStream);
}

int MasterController::bound(char axis, int value, int limit)
{
  int origValue,upper, lower, clip=0;

  origValue = value;
  upper = (1<<(limit-1)) - 1;
  lower = -(1<<(limit-1));
  if (value>upper)
  {
     clip = value = upper;
  }
  else if (value < lower)
  {
     clip = value = lower;
  }
  if (clip)
  {
     text_error("Gradient %c set out of range, %d clipped  to %d\n",
			axis,origValue,clip);
  }
  return(value);
}
//
// serRTGain sets mixer# to gain 
// it takes 20 usec. the usual mixer 1 is mixer 0 thing.
//

int MasterController::setRTGain(int mixer, double gain)
{
  int j,k;
  if (mixer > 0)
    {
      k = trInterconnect.setRTGainOneData((int)(gain+0.5),mixer-1);
      RT_spi(1,k);
      setTickDelay(1600); //  20 microseconds (1 extra)
      return(1600);
    }
  if (mixer == 0) 
    {
      for (j=0;j<trInterconnect.numNSRS; j++) 
	{
	  trInterconnect.setBankGainCode((int)(gain+0.5), j);
          RT_spi(1,trInterconnect.getBankGainCode(j));
	  setTickDelay(1600);
        }
      return(trInterconnect.numNSRS*1600);
    }
  return(0);
}


void MasterController::setDiplexerRelay(int state)
{
  trInterconnect.the_NSR_Array[0].setDiplexRelay(state);
  RT_spi(1,trInterconnect.the_NSR_Array[0].getNSRWord(14));
}

void MasterController::enableBlaf()
{
   if (blafMode & 1) blafMode |= 4;
}

/* arg=1 use 1st delay arg = 2 user specified start */
void MasterController::armBlaf(int arg)
{
  int tst;
  tst = ((arg==1) && (blafMode & 4)) || ((arg == 2) && (blafMode & 2));
  if (!tst)
    return;
  blafMode |= 4;
  add2Stream(GATEKEY| 0x20020); /* no time */
}

void MasterController::clearBlaf()
{
  if (!(blafMode & 4)) 
    return;
  blafMode &= 3;   // just one off...
  add2Stream(GATEKEY| 0x20000); /* no time */
}


void MasterController::setVDelayList(cPatternEntry *vdelayPattern, int *list, int nvals)
{
   /* nothing to do for Master Controller */
}


cPatternEntry * MasterController::createVDelayPattern(int *list, int nvals)
{
        int *pBuffer = &list[0];
        if (pBuffer == NULL)
                abort_message("vdelay_list is not defined or empty. abort!\n");

        pWaveformBuf->startSubSection(PATTERNHEADER);
        pWaveformBuf->putCodes(pBuffer, nvals);

        int listId;

        cPatternEntry *vdelayPattern = new cPatternEntry("vdlylist",99,nvals,nvals);
        if (vdelayPattern != NULL)
        {
                addPattern(vdelayPattern);
                sendPattern(vdelayPattern);
                listId = vdelayPattern->getReferenceID();
                if ( (listId < 1) || (listId > 0x7FFFFF) )
                        abort_message("unable to determine the vdelay_list id. abort!\n");
        }
        else
                abort_message("error in creating vdelay_list. abort!\n");

        return vdelayPattern;
}


