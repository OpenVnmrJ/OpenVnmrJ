/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* AcodeBuffer.h */

#ifndef INC_ACODEBUFFER_H
#define INC_ACODEBUFFER_H

using namespace std;

#include <iostream>
#include <fstream>
#include <stdlib.h>

#include "PSGFileHeader.h"


class AcodeBuffer
{

  public:
	// constructor
	AcodeBuffer(size_t size, char *name, char *acodeStage, int uID);

	// Destructor
	 ~AcodeBuffer(void);


	// Setter
	void putCode(int code);

        void putCodeAt(int code, int pos);

        void putCodes(int* pWords, int num);

        void putCodesAt(int* pWords, int num, int pos);

        // Duplicator

        int duplicate(int min_size, int ssize);

	// Accessor
        char* getName(void);

	int getCode(void);
  
	size_t getAcodeSize(void);



	int matchID(char *key);

	void describe(void);

        void describe(char *message);

	int closeAcodeFile(int option);

	int openAcodeFile(int option);

	void writeIncrement(int j);

        void setWriteIndex(int p);

        void incrWriteIndexBy(int p);

        void putHeaderInfo(ComboHeader *ch);

        void setHeaderWord(int wordType, int value);


	// sub section related

        void startSubSection(int typeOfHeader);

        void endSubSection();


        int * getpData();

        struct _COMBOHEADER_ cHeader;


  private:
	size_t currentSize;
	size_t incrementSize;
	char acodeName[32];
        char acodeStage[32];
        int acodeUniqueID;
        int *pData;
        int wind;	// (next) write to this index
        int rind;	// (next) read from this index
        int ssStart;    // start index of current section
        int ssWind;     // (next) write to this index of sub-section
	int ssIndex;
        bool subSectionInUse;
        bool ssWriteFlag;
        bool realWriteFlag;
	ofstream ofs;
        int outputControl;
} ;

#endif
