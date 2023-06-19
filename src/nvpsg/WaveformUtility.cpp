/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* WaveformUtility.cpp */

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "WaveformUtility.h"
#include "cpsg.h"
#include "FFKEYS.h"

extern "C" {
    
#include "safestring.h"

}

#define OFFSETPULSE_TIMESLICE (0.00000020L)

extern int bgflag;


WaveformUtility::WaveformUtility()
{ }

cPatternEntry *WaveformUtility::readRFWaveform(char *name, RFController *rf1, unsigned int *pStartPos, int rawwfgsize, double totalDuration)
{
  /* read in a waveform file .RF and populate the incoming array & the cPatternEntry
     Waveform is not added to Pattern Store in Control                            */

  int i,l;
  double f1,f2,f3,f4,f5,scalef;
  double pwrf;
  int wcount, eventcount, repeats;
  int phaseWord, ampWord, gateWord;
  char tname[200];
  cPatternEntry *tmp;

  pwrf=0.0;
  wcount = 0; eventcount = 0;
  scalef = 4095.0/1023.0;

  OSTRCPY( tname, sizeof(tname), name);
     i = rf1->findPatternFile(tname);
     if (i != 1)
       {
         text_error("%s could not find %s", "WaveformUtility readRFWaveform", tname);
         psg_abort(1);
       }

     wcount=0; eventcount=0;
     unsigned int *pOut = pStartPos;

     // read in the file
     do
    {
       l = rf1->scanLine(&f1,&f2,&f3,&f4,&f5);
       if ((l > 2) && (f3 < 0.5))
       {
            scalef = 4095.0/f2;
            continue;
       }

       repeats = 1;
       if (l > 2) // 255 is low??
         repeats = ((int) f3) & 0xff;

       gateWord = GATEKEY | (RFGATEON(X_OUT)  & 0xffffff);

       switch (l)
       {

         case 4:
           if (((int)f4) & 0x1)
             {
               gateWord = GATEKEY | (RFGATEON(X_OUT)  & 0xffffff);
             }
           else
             {
               gateWord = GATEKEY | (RFGATEOFF(X_OUT) & 0xffffff);
             }

         case 3:

         case 2:
           ampWord = RFAMPKEY | rf1->amp2Binary(f2*scalef);

           phaseWord = (rf1->degrees2Binary(f1) | RFPHASEKEY);

           do
             {
               *pOut++ = gateWord;
               *pOut++ = ampWord;
               *pOut++ = phaseWord;
               *pOut++ = LATCHKEY | DURATIONKEY;  // no duration value yet
               eventcount++;
               wcount += 4;
               repeats--;
             } while (repeats > 0);
       }

       if (wcount > rawwfgsize)
        {
          abort_message("waveform too large for swift_acquire. create parameter largewfgsize and set it to value over %d",rawwfgsize);
        }
    }  while (l > -1);

     // fill in the duration in words with DURATION KEY
     double duration = totalDuration/eventcount;

     for (int i=0; i<wcount; i++)
       {
         if ( (*(pStartPos+i) & (31<<26)) == DURATIONKEY)
         {
           *(pStartPos+i) |= rf1->calcTicks(duration);
         }
       }
   if (eventcount > 0)
   {
      pwrf = sqrt((pwrf)/((double) eventcount));
      tmp = new cPatternEntry(name,1 /* flag */, eventcount,wcount);
      tmp->setPowerFraction(pwrf);  // use this in calling code.
   }
   else
      abort_message("RF waveform file %s has no events",name);
  return(tmp);
}



cPatternEntry *WaveformUtility::readDECWaveform(char *name, RFController *rf1, unsigned int *array)
{
  /* read in a waveform file .RF and populate the incoming array & the cPatternEntry
     Waveform is not added to Pattern Store in Controller                         */

  return new cPatternEntry("abc",1, 1, 1024, 1024);
}


cPatternEntry *WaveformUtility::makeOffsetPattern(unsigned int *inp, unsigned int *out, RFController *rf1, int nInc, double phss, double pacc, int flag, char mode, char *tag, char *emsg, int action)
{
  /* Frequency shift an input waveform by adding phase ramp
     Waveform is not added to Pattern Store in Controller                         */

  return new cPatternEntry("abc",1, 1, 1024, 1024);
}

cPatternEntry *WaveformUtility::weaveGatePattern(unsigned int *pFinalWfgOut, unsigned int *pInp, int num_rawwords, unsigned int *pGatePattern, int num_gatewords, long long totalDurationTicks)
{
  /* Weave in a gate pattern into an input waveform array
     Waveform is not added to Pattern Store in Controller                         */

    int wcount, eventcount;
    unsigned int *pRawWfgStart;
    unsigned int *pOut;

    //printf("WU:wGP num_rawwords=%d num_gatewords=%d totaDurationTicks=%lld\n",num_rawwords,num_gatewords,totalDurationTicks);

 // weave two waveform patterns together to form an output pattern

    int A_DUR_WORD=0, A_GATE_WORD=0, A_PHASE_WORD=0, A_AMP_WORD=0;
    int B_DUR_WORD=0, B_GATE_WORD=0;
    int runWord  = 0;
    unsigned int *pA, *pB;
    pA = pInp;
    pB = pGatePattern;
    pRawWfgStart = pInp;
    pOut = pFinalWfgOut;

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

    if ( pB >= (pGatePattern + num_gatewords) )
      pB = pGatePattern;

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
    abort_message("psg internal error in swift waveform merge duration word is 0. abort!\n");
  }

  // Now do the merging of the two streams

     // logic to determine if we need to stop!

     if ( A_DUR_WORD <=  B_DUR_WORD )
        runWord = A_DUR_WORD;  // choose shortest
     else
        runWord = B_DUR_WORD;
//
     if (totalDurationTicks <= runWord)
     {
         runWord = totalDurationTicks;
         stopLoad = 1;
     }
     else
         stopLoad = 0;

     if (A_AMP_WORD != lastAmpWord)
     {
        //putPattern(A_AMP_WORD);
        (*pOut++) = (A_AMP_WORD);
        wcount++;
        lastAmpWord = A_AMP_WORD;
     }

     if (A_PHASE_WORD != lastPhaseWord)
     {
        //putPattern(A_PHASE_WORD);
        (*pOut++) = (A_PHASE_WORD);
        wcount++;
        lastPhaseWord = A_PHASE_WORD;
     }

     // gateWord comes from the B stream (gate array)
     if (B_GATE_WORD != lastGateWord)
     {
        //putPattern(B_GATE_WORD);
        (*pOut++) = (B_GATE_WORD);
        wcount++;
        lastGateWord = B_GATE_WORD;
     }

     // Now examine the two durations: A_DUR_WORD or B_DUR_WORD, which is shorter?
     // GATEWORD from B stream (gate array) has precedence

     if (runWord <= 1)   // catch a single or zero tick
         runWord = 2;

     if (runWord >= 2)  // insure legal time..
     {
       //putPattern(LATCHKEY | DURATIONKEY | runWord);
       (*pOut++) = (LATCHKEY | DURATIONKEY | runWord);
       wcount++;
       eventcount++;
     }
     else
    	 printf("psg internal error in swift waveform merge duration word invalid. abort!\n");

     totalDurationTicks -= runWord;
     B_DUR_WORD   -= runWord;
     A_DUR_WORD   -= runWord;

     if (A_DUR_WORD < 0) A_DUR_WORD = 0;
     if (B_DUR_WORD < 0) B_DUR_WORD = 0;

     if (stopLoad || (totalDurationTicks <= 0) ) break;
   }
   // add the waveform to Pattern List

     cPatternEntry *tmp;
     if (eventcount > 0)
     {
        tmp = new cPatternEntry("swift",1,eventcount,wcount);
     }
     else
        abort_message("pattern swift has no events");

  return tmp;
}


