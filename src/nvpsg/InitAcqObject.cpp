/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
   InitAcqObject.cpp
*/

#include <iostream>
#include <string.h>
#include <iomanip>
#include <fstream>
#include "InitAcqObject.h"
#include "cpsg.h"
#include "lc.h"
#include "PSGFileHeader.h"


extern "C" {

  int * init_acodes(Acqparams *);
  void initautostruct(void);

#include "safestring.h"

}


InitAcqObject::InitAcqObject()
{
  OSTRCPY( Name, sizeof(Name), "InitAcqObject");
  UniqueID = 9;
  pAcodeMgr = NULL;
  pAcodeBuf = NULL;
} 

InitAcqObject::InitAcqObject(const char *nm)
{
  OSTRCPY( Name, sizeof(Name), nm);
  UniqueID = 9;
  pAcodeMgr = AcodeManager::getInstance();
  setAcodeBuffer();
}

void InitAcqObject::setAcodeBuffer()
{
  pAcodeBuf = pAcodeMgr->setAcodeBuffer(Name,UniqueID);
  //pAcodeBuf->openAcodeFile(1);
}

void InitAcqObject::describe(const char *what) 
{
   cout << "InitAcqObject: " << what << endl;
   (*pAcodeBuf).describe();
}

void InitAcqObject::WriteWord(int word)
{
  pAcodeBuf->putCode(word);
}

void InitAcqObject::writeLCdata()
{
  pAcodeBuf->startSubSection(INITIALHEADER);
  init_acodes( (Acqparams *) (pAcodeBuf->getpData()) );
  initacqparms(1);
  int nWordsLC = sizeof(Acqparams)/sizeof(int);
  pAcodeBuf->incrWriteIndexBy(nWordsLC);
  // dont closeAcodefile(), since autod structure has to be appended 
}
  
void InitAcqObject::writeAutoStruct()
{
  initautostruct();
  int nWordsAuto = sizeof(autodata)/sizeof(int);
  pAcodeBuf->incrWriteIndexBy(nWordsAuto);
  // dont closeAcodefile(), since tables have to be appended 
  // pAcodeBuf->closeAcodeFile(0);
  pAcodeBuf->endSubSection();
}
   
void InitAcqObject::startTableSection(ComboHeader ch)
{
  pAcodeBuf->startSubSection(TABLEHEADER);
  pAcodeBuf->putHeaderInfo(&ch);
  pAcodeBuf->cHeader = ch;
}


void InitAcqObject::endTableSection()
{
  pAcodeBuf->endSubSection();
}


void InitAcqObject::closeAcodeFile(int index)
{
  pAcodeBuf->closeAcodeFile(index);
}
