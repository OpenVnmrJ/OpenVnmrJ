/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
	InitAcqObject.h
*/

#ifndef INC_INITACQOBJECT_H
#define INC_INITACQOBJECT_H

using namespace std;

#include <iostream>
#include <fstream>

#include "AcodeManager.h"


//
// InitAcqObject
//
class InitAcqObject 
{
  private:
    static int SeqId;
    char Name[200];
    int UniqueID;
    AcodeManager *pAcodeMgr;
    AcodeBuffer  *pAcodeBuf;


  public:
   InitAcqObject();
   InitAcqObject(const char *nm);

   void describe(const char *what);
   void setAcodeBuffer();
   void WriteWord(int word);
   void writeLCdata();
   void writeAutoStruct();
   void startTableSection(ComboHeader ch);
   void endTableSection();

   void closeAcodeFile(int index);

} ;

#endif
