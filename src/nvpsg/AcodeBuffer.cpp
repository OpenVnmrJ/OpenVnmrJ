/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* AcodeBuffer.cpp */

#include <iomanip>
using namespace std;
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <arpa/inet.h>
#include "cpsg.h"
#include "FFKEYS.h"
#include "ACode32.h"
#include "AcodeBuffer.h"

extern "C" {

#include "safestring.h"

};

extern int bgflag;
extern int checkflag;


AcodeBuffer::AcodeBuffer(size_t array_size, char *array_name, char *stage, int uID)
{
  int *data;

  currentSize   = array_size;
  incrementSize = array_size;
  data = new int[currentSize];
  
  if ( ! data ) 
     abort_message("psg internal acode buffer memory allocation failed. abort!\n");

  pData = data;
  int i;
  for(i=0; i< (int) currentSize; i++)
  {
    *(pData+i) = 0;
  }

  wind = 0;	// (next) write to this index
  rind = 0;	// (next) read from this index
  ssStart = 0;
  ssWind  = 0;
  ssIndex = 0;
  subSectionInUse=false;
  OSTRCPY( acodeName, sizeof(acodeName), array_name);
  OSTRCPY( acodeStage, sizeof(acodeStage), stage);
  acodeUniqueID = uID;
  cHeader.comboID_and_Number = UNDEFINEDHEADER;
  cHeader.sizeInBytes = 0;
  ssWriteFlag   = false;
  realWriteFlag = false;
  outputControl = 0;
  if (bgflag) { cout << "\nAcodeBuffer " << acodeName << "acodeUniqueID: " << acodeUniqueID << " created" << endl; }
}


// Destructor
AcodeBuffer::~AcodeBuffer()
{
     delete pData;
     wind=0;
     rind=0;
     ssStart=0;
     ssWind =0;
     ssIndex=0;
     subSectionInUse= false;
     ssWriteFlag    = false;
     realWriteFlag  = false;
     outputControl  = 0;
}


size_t AcodeBuffer::getAcodeSize(void)
{
  size_t acodesize;
  acodesize  = wind-ssStart;
  return acodesize;
}


char* AcodeBuffer::getName(void)
{
   return acodeName;
}


void AcodeBuffer::putCode(int code)
{
  if (wind >= (int) currentSize)
  {
     // need to reallocate new bigger array 
     pData = (int *) realloc(pData,(currentSize+incrementSize)*sizeof(int));
     currentSize    += incrementSize;
     if (bgflag) { cout << "AcodeBuffer size reallocation done for " << getName() << " new size = " << currentSize << endl; }
  }

  *(pData + wind) = code;	// write acode
  wind++;			// increment write pointer
  ssWind=wind-ssStart;		// set the sub-section write pointer
  ssWriteFlag=true;
}


void AcodeBuffer::putCodeAt(int code, int pos)
{
  if (pos < 0) 
     abort_message("psg internal acode buffer code write position is invalid (negative) for %s. abort!\n",acodeName);
  else if ( ((pos+ssStart) >= wind) || ((pos+ssStart) >= (int) currentSize) )
     abort_message("psg internal acode buffer code write position is invalid (beyond the write index wind or current size) for %s. abort!\n",acodeName);


  *(pData + (pos+ssStart)) = code;       // write acode
					 // no need to update wind & ssWind
  ssWriteFlag=true;
}



void AcodeBuffer::putCodes(int* pWords, int num)
{

  if ((wind+num) >= (int) currentSize)
  {
     // need to reallocate new bigger array
     int higherIndex = incrementSize;
     if (num >= (int) incrementSize) higherIndex += num;
     pData = (int *) realloc(pData,(currentSize+higherIndex)*sizeof(int));
     currentSize += higherIndex;
     if (bgflag) { cout << "AcodeBuffer size reallocation done for " << getName() <<" new size = " << currentSize <<  endl; }
  }

  for(int j=wind; j<(wind+num); j++)
  {
    *(pData + j) = *(pWords);       // write acode
    pWords++;
  }
  wind   = wind + num;
  ssWind = wind - ssStart;
  ssWriteFlag=true;
}


/* pos: position wrt to current subsection */
void AcodeBuffer::putCodesAt(int* pWords, int num, int pos)
{

  if ((pos < 0) || (num<=0))
     abort_message("psg internal acode buffer code write position or numwords is invalid (negative) for %s. abort!\n",acodeName);
  else if ((pos+ssStart) >= wind)
     abort_message("psg internal acode buffer code write position is invalid (beyond the write index wind) for %s. abort!\n",acodeName);


  if ((pos+ssStart+num) >= (int) currentSize)
  {
     // need to reallocate new bigger array
     int higherIndex = incrementSize;
     if ((pos+ssStart+num) >= (int)(currentSize+incrementSize)) higherIndex += num;
     pData = (int *) realloc(pData,(currentSize+higherIndex)*sizeof(int));
     currentSize += higherIndex;
     if (bgflag) { cout << "AcodeBuffer size reallocation done for " << getName() << " new size = " << currentSize << endl; }
  }

  for(int j=pos+ssStart; j<(pos+ssStart+num); j++)
  {
    *(pData + j) = *(pWords);       // write acode
    pWords++;
  }

  if ((pos+ssStart+num) > wind)
  {
     wind = pos+ssStart+num;
     ssWind = wind - ssStart;
  }
  ssWriteFlag=true;
}

int AcodeBuffer::duplicate(int min_size,int ssize)
{
   int nshort;
   int buildsize;
   int count;
   int i;
   int *p;
   nshort = ssize;
   buildsize = ssize;
   count = 1;
   p = pData+wind-ssize;  
   while (buildsize < min_size) {
     for (i=0; i < nshort; i++)
     {
        //cout << hex << *(p+i) << endl;
        *(pData+wind) = *(p+i);
        wind++; 
     }
     //cout << dec;
     buildsize += nshort;
     count++;
   }
   ssWind=wind-ssStart;		// set the sub-section write pointer
   return(count);
}

int AcodeBuffer::getCode(void)
{
  int code;
  code = *(pData+rind);
  rind++;
  return code;
}


int AcodeBuffer::matchID(char *key)
{
  if (!strncmp(key,acodeName,strlen(acodeName)))
    return(1);
  else
    return(0);
}

int AcodeBuffer::openAcodeFile(int option)
{
  char buffer[200], infopath[256];

  if (P_getstring(CURRENT,"goid",infopath,1,255) < 0)
     abort_message("psg internal acode buffer unable to get goid for %s. abort!\n",acodeName);

  if (checkflag)
  {
     ofs.open("/dev/null",ios::out|ios::binary);
  }
  else
  {
     OSPRINTF( buffer, sizeof(buffer), "%s.%s.%s", infopath, acodeStage, acodeName);
     ofs.open(buffer,ios::out|ios::binary);
  }

  if ( ! ofs.is_open() )
  {
     if (bgflag) { cout << "AcodeBuffer:: acode buffer open FAILED for " << buffer << endl; }
     abort_message("psg internal acode buffer unable to open file for %s. abort!\n",acodeName);
  }

  if (bgflag)  { cout << "AcodeBuffer:: opened Acode file "<< buffer << endl; }
  return(acodeUniqueID);
}


int AcodeBuffer::closeAcodeFile(int option)
{
  if (realWriteFlag)
  {
    openAcodeFile(option);
    writeIncrement(option);
    if ( ofs.is_open() )
    {
      ofs.close();
      return(0);
    }
    else
    {
      text_message("psg internal acode buffer output file stream is null for %s\n",acodeName);
      if (bgflag) { cout << " #### AcodeBuffer:: ERROR: ofs is NULL ! " << endl; }
      return(3);
    }   
  }
  else
  {
    if (bgflag) 
      cout << " #### WARNING: AcodeBuffer for " << acodeName << " is empty. No file written\n"; 

    return(-1);
  }
}


void AcodeBuffer::describe(void)
{
 if (bgflag)
 {
   cout << "\nAcode Buffer : \n" << "   acodeName: " << acodeName << "   acodeUniqueID: " << acodeUniqueID << endl;
   cout << "   pointer to Data Start(pData) : " << pData << endl;
   cout << "   pointer to Data Current(Data): " << (pData+wind) << endl;
   cout << "   write pointer : " << wind << endl;
   for(int i=0;i<wind;i=i+4)
     cout <<  *(pData+i) << "  " << *(pData+i+1) << "  " << *(pData+i+2) << "  " << *(pData+i +3) << endl;
 }
}


void AcodeBuffer::describe(char *message)
{
 if (bgflag)
 {
   cout << "\nAcode Buffer : at " << message << "\n" << "   acodeName: " << acodeName << "   acodeUniqueID: " << acodeUniqueID << endl;
   cout << "   pointer to Data Start(pData) : " << pData << endl;
   cout << "   pointer to Data Current(Data): " << (pData+wind) << endl;
   cout << "   write pointer : " << wind << endl;
   for(int i=0;i<wind;i=i+4)
      cout <<  *(pData+i) << "  " << *(pData+i+1) << "  " << *(pData+i+2) << "  " << *(pData+i +3) << endl;
 }
}


void AcodeBuffer::writeIncrement(int j)
{
#ifdef LINUX
  int *dptr;
  int  i;

  dptr = pData;
  for (i=0; i< wind; i++)
  {
     *dptr = htonl( *dptr );
     dptr++;
  }
#endif
  ofs.write( (char *) pData,    sizeof(int)*wind);

  if ( bgflag && (outputControl & 3))
  {
    for(int i=0;i<wind;i=i+4)
     cout <<  *(pData+i) << "  " << *(pData+i+1) << "  " << *(pData+i+2) << "  " << *(pData+i+3) << endl;
  }

  wind   = 0;
  rind   = 0;
  ssStart= 0;
  ssWind = 0;
  ssWriteFlag  =false;
  realWriteFlag=false;
}


void AcodeBuffer::setWriteIndex(int index)
{
   if (index > wind) ssWriteFlag=true;
   wind = index;
   ssWind = wind-ssStart;
}


void AcodeBuffer::incrWriteIndexBy(int index)
{
   if (index > 0) ssWriteFlag=true;
   wind = wind + index;
   ssWind = wind-ssStart;
}


// get pointer to the data array: breaks OO concepts, but more efficient

int * AcodeBuffer::getpData()    
{
  return (pData+wind) ;
}


void AcodeBuffer::startSubSection(int typeOfHeader)
{
  if (subSectionInUse)
  {
    if (bgflag) { cout << "AcodeBuffer::startSubSection(): subsection in use. cannot start sub section\n"; }
    abort_message("psg internal acode buffer subsection is already in use. cannot start sub section for %s. abort!\n",acodeName);
  }

  ssStart = wind;
  cHeader.comboID_and_Number = typeOfHeader;
  cHeader.sizeInBytes = 0;
  putCodes((int *)(&cHeader),sizeof(cHeader)/sizeof(int));
  ssIndex++;

  // only use 24 LSB to code the sub section index (e.g., fid number)
  if (ssIndex >= 0x1000000) 
  {
    if (bgflag) { cout << "AcodeBuffer::startSubSection(..): too many sub sections! \n"; }
    abort_message("psg internal acode buffer has too many sub sections %d for %s. abort!\n",ssIndex,acodeName);
  }
  subSectionInUse=true;
  ssWriteFlag=false;
}


void AcodeBuffer::endSubSection()
{
  if (! subSectionInUse)
  {
    if (bgflag) { cout << "AcodeBuffer::endSubSection(): subsection not in use. cannot end sub section\n"; }
    abort_message("psg internal acode buffer subsection not in use. cannot end sub section for %s. abort!\n",acodeName);
  }
  if (ssWriteFlag) realWriteFlag=true;
  cHeader.sizeInBytes = ( ssWind*sizeof(int) - sizeof(cHeader) ) ;
  putCodesAt((int *)(&cHeader),sizeof(cHeader)/sizeof(int),0);
  subSectionInUse=false;
}



void AcodeBuffer::putHeaderInfo(ComboHeader *pCH)
{
  putCodesAt((int *)pCH,sizeof(cHeader)/sizeof(int),0);
}


void AcodeBuffer::setHeaderWord(int wordType, int value)
{
  switch ( wordType ) {

    case COMBOID_NUM_WORD:
          cHeader.comboID_and_Number = value ;
          break;

    default:
          if (bgflag) 
          {
             cout << "AcodeBuffer::setHeaderWord(): invalid word type ! \n" ;
             text_message("psg internal acode buffer has invalid word type for header, for %s\n",acodeName);
          }
          
          break;
  }
}

