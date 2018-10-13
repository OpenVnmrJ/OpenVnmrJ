/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* AcodeManager.h */

#ifndef INC_ACODEMANAGER_H
#define INC_ACODEMANAGER_H

#include <iostream>
#include <stdlib.h>
#include "AcodeBuffer.h"

#define ACODE_INIT 0
#define ACODE_PRE  1
#define ACODE_PS   2
#define ACODE_POST 3
#define WFG_FLAG   4


class AcodeManager
{

  public:
        int acodeStage;

        int acodeStageWriteFlag[5];

        static AcodeManager* makeInstance(int);

        static AcodeManager* getInstance(void);

	AcodeBuffer *setAcodeBuffer(char *name, int uID);

	AcodeBuffer *allocAcodeBuffer(char *name, char *stage, int uID);
	
	AcodeBuffer *getAcodeBufferByIndex(int ind);

	AcodeBuffer *getAcodeBufferByID(char *key);

        void incrementAcodeStage();

        void startSubSection(int typeOfHeader);

        void endSubSection();
   
        void setAcodeStageWriteFlag(int stage, int flag);

        int  getAcodeStageWriteFlag(int stage);

	void describe(void);

  private:
        AcodeBuffer **pArrayAcodeBufs;
        int cntrlr;
        int numcntrlrs;
	size_t acodesz;
        char acodeStageNames[4][16];
        int outputControl;
   
        static bool instanceAMflag;
        static AcodeManager *acodeManager;

        // private constructors
        AcodeManager();
        AcodeManager(int num);
        
} ;

#endif
