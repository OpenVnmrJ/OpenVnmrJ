/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* AcodeManager.cpp */

#include <iostream>
#include <string.h>
#include <stdlib.h>

#include "Console.h"
#include "cpsg.h"

extern "C" {

#include "safestring.h"

}


extern int bgflag;

bool          AcodeManager::instanceAMflag = false;
AcodeManager* AcodeManager::acodeManager   = NULL;


AcodeManager* AcodeManager::makeInstance(int numcntrls)
{
  if (! instanceAMflag)
  {
     acodeManager = new AcodeManager(numcntrls);
     instanceAMflag = true;
     return acodeManager;
  }
  else
  {
     return acodeManager;
  }
}

AcodeManager* AcodeManager::getInstance()
{
  if (instanceAMflag)
     return acodeManager;
  else
     return NULL;
}

AcodeManager::AcodeManager()
{
  pArrayAcodeBufs = NULL;
  acodeStage = 0;
  cntrlr = 0;
  numcntrlrs = 0;
  acodesz   = 4096;
  acodeStageNames[0][0] = 0;
  acodeStageNames[1][0] = 0;
  acodeStageNames[2][0] = 0;
  acodeStageNames[3][0] = 0;
  acodeStageWriteFlag[ACODE_INIT]=0;
  acodeStageWriteFlag[ACODE_PRE] =0;
  acodeStageWriteFlag[ACODE_PS]  =0;
  acodeStageWriteFlag[ACODE_POST]=0;
  acodeStageWriteFlag[WFG_FLAG]  =0;
  outputControl = 0;
}

AcodeManager::AcodeManager(int numcntrls)
{
  pArrayAcodeBufs = (AcodeBuffer **) malloc(2*numcntrls*sizeof(AcodeBuffer *));
  if ( ! pArrayAcodeBufs ) 
    abort_message("memory allocation failed for acode manager. abort!\n");

  acodeStage = 0;
  cntrlr=0;
  numcntrlrs = numcntrls;
  acodesz   = 4096;
  OSTRCPY( acodeStageNames[0], sizeof(acodeStageNames[0]), "init");
  OSTRCPY( acodeStageNames[1], sizeof(acodeStageNames[1]), "pre");
  OSTRCPY( acodeStageNames[2], sizeof(acodeStageNames[2]), "ps");
  OSTRCPY( acodeStageNames[3], sizeof(acodeStageNames[3]), "post");
  acodeStageWriteFlag[ACODE_INIT]=0;
  acodeStageWriteFlag[ACODE_PRE] =0;
  acodeStageWriteFlag[ACODE_PS]  =0;
  acodeStageWriteFlag[ACODE_POST]=0;
  acodeStageWriteFlag[WFG_FLAG]  =0;
  outputControl = 0;
}


void AcodeManager::setAcodeStageWriteFlag(int stage, int flag)
{
   acodeStageWriteFlag[stage] = flag ;
}

  
int AcodeManager::getAcodeStageWriteFlag(int stage)
{
   return acodeStageWriteFlag[stage] ;
}


AcodeBuffer * AcodeManager::setAcodeBuffer(char *name, int uID)
{
  AcodeBuffer *pAcodeBuf;
  pAcodeBuf = allocAcodeBuffer(name, acodeStageNames[acodeStage], uID);
  return pAcodeBuf;
}


AcodeBuffer * AcodeManager::allocAcodeBuffer(char *name, char *stage, int uID)
{
  AcodeBuffer *pAcodeBuf;
  pAcodeBuf = new AcodeBuffer(acodesz,name,stage,uID);
  if ( pAcodeBuf == NULL )
     abort_message("unable to allocate acode buffer for %s. abort!\n",name);

  pArrayAcodeBufs[cntrlr]=pAcodeBuf;
  
  if (bgflag) cout << "AcodeManager:allocAcodeBuffer(): new allocation of AcodeBuffer for "<< name << "  stage "<< stage<< "  uID = " << uID <<  "  cntrlr = " << cntrlr << endl;

  cntrlr++;
  return pAcodeBuf; 
}


AcodeBuffer * AcodeManager::getAcodeBufferByIndex(int k)
{
 if ((k >= 0) && (k < numcntrlrs))
    return(pArrayAcodeBufs[k]);
  else
    exit(-1);
}


AcodeBuffer * AcodeManager::getAcodeBufferByID(char *name)
{
  int i;
  for (i=0; i < numcntrlrs; i++)
    {
        if (pArrayAcodeBufs[i]->matchID(name))
          return(pArrayAcodeBufs[i]);
    }
  exit(-1);  
}


void AcodeManager::incrementAcodeStage()
{
  P2TheConsole->flushAllEvents();
  int i;
  for (i=0; i<numcntrlrs; i++)
  {
    if ( pArrayAcodeBufs[i] != NULL ) 
    {
       int wflag = pArrayAcodeBufs[i]->closeAcodeFile(0);
       if (wflag == 0)
       {
         setAcodeStageWriteFlag(acodeStage,1);
       }
    }
    else
    {
       if (bgflag) cout << "AcodeBuffer["<<i<<"] is NULL" << endl;
       text_message("psg internal acode buffer is null for controller %d\n",i);
    }
  }

  if (bgflag) P2TheConsole->printSyncStatus();

  if (strcmp(acodeStageNames[acodeStage],"ps") == 0) 
  { 
     P2TheConsole->verifySync("End of Pulse Sequence");
     P2TheConsole->closeWaveformFiles();
  }

  for (i=0; i<numcntrlrs; i++)
  {
    pArrayAcodeBufs[i] = NULL;
  }


  acodeStage++;               // increment to the next Acode Stage
  
  if (acodeStage > ACODE_POST) return;

  if (bgflag)
  {
    cout << "\n\n\n $$$$$$$$$$$   INCREMENT ACODE STAGE  TO " << acodeStage << "  $$$$$$$$$$$$$$$$\n";
    cout << "    0=init   1=pre    2=ps     3=post  \n";
  }

  P2TheConsole->resetSync();

  cntrlr=0;
  P2TheConsole->setAcodeBuffers();
  if (strcmp(acodeStageNames[acodeStage],"ps") == 0)
  {
    P2TheConsole->setWaveformBuffers();
  }
}


// The start.. and end.. SubSection() does not affect the InitAcqObject 
void AcodeManager::startSubSection(int typeOfHeader)
{
  int i;
  for (i=0; i<numcntrlrs; i++)
  {
    if ( pArrayAcodeBufs[i] != NULL )
       pArrayAcodeBufs[i]->startSubSection(typeOfHeader);
    else
    {
       if (bgflag) cout << "AcodeBuffer["<<i<<"] is NULL" << endl;
       text_message("psg internal acode buffer is null for controller %d\n",i);
    }
  }
}


// The start.. and end.. SubSection() does not affect the InitAcqObject 
void AcodeManager::endSubSection()
{
  P2TheConsole->flushAllControllerDload();

  for (int i=0; i<numcntrlrs; i++)
  {
    if ( pArrayAcodeBufs[i] != NULL )
       pArrayAcodeBufs[i]->endSubSection();
    else
    {
       if (bgflag) cout << "AcodeBuffer["<<i<<"] is NULL" << endl;
       text_message("psg internal acode buffer is null for controller %d\n",i);
    }
  }
}



void AcodeManager::describe(void)
{
   fprintf(stdout,"psg internal number of acode streams = %d\n",numcntrlrs);
}

