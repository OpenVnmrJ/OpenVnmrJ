/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */

/*
modification history
--------------------
1-22-2010,gmb  created
*/

/*
DESCRIPTION

   functions to handle the high level aspect of updating iCAT ISF 
   and programm the iCAT FPGA registers

*/

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <string.h>
#include <stdioLib.h>
#include <vxWorks.h>
 
#include "logMsgLib.h"
#include "rf.h"
#include "rf_fifo.h"
#include "nvhardware.h"
#include "md5.h"
#include "icatSpiFifo.h"
#include "icatISF.h"
#include "simon.h"
#include "icat_reg.h"

extern int ConsoleTypeFlag;

extern int RfType;

uint64_t iCAT_DNA = 0LL;

/************************************************/


/*
 * test the Controllers FPGA to determine if it is
 * icat capable
 * GMB
 */
int fpgaIcatEnabled()
{

   int FPGA_Id, FPGA_Rev, FPGA_Chksum;
   int status;
   FPGA_Id = get_field(RF,ID);
   FPGA_Rev = get_field(RF,revision);
   FPGA_Chksum = get_field(RF,checksum);
   diagPrint(NULL,"  FPGA ID: %d, Rev: %d, Chksum: %ld\n",
                  FPGA_Id,FPGA_Rev,FPGA_Chksum);
   if ( (FPGA_Id  == 1 /* RF */ )  && ( FPGA_Rev > 0 ) )
   {
       diagPrint(NULL," FPGA is iCAT enabled\n");
       status= 1;
   }
   else
   {
       diagPrint(NULL," FPGA is NOT iCAT enabled\n");
       status= 0;
   }
   return status;
}


/*
 * setup the RF FPGA registers based on 
 * icat status and console type
 */
int setupRfType()
{
   uint64_t readDNA(void);
   int status;
   int icatFpgaFlag;
   // determine if RF Controller FPGA is icat capable
   icatFpgaFlag = fpgaIcatEnabled();
   if (icatFpgaFlag > 0)
   {
       int icatID;
       // determine if icat RF is attached to the controller
       icatID = getIcatId();
       diagPrint(NULL," ICAT ID: 0x%lx\n",icatID);
       if (icatID != 0xa)
       {
          diagPrint(NULL," iCAT NOT Attached\n");
          set_field(RF,transmitter_mode,0);  // Non-ICAT  RF
          RfType = 0;
       }
       else
       {
          diagPrint(NULL,"iCAT Attached\n");
          RfType = 1;

          /* obtain the unique identifier for iCAT board */
          /* used within the fail files */
          iCAT_DNA = readDNA();
          diagPrint(NULL," ICAT DNA: 0x%llx\n",iCAT_DNA);
       }
       if (ConsoleTypeFlag != 1) {
          diagPrint(NULL," Setting for VNMRS Console\n");
          // printf("Setting for VNMRS Console\n");
          set_field(RF,console_type,0);  // VNMRS = 0, 400-MR = 1
       }
       else {
          diagPrint(NULL," Setting for 400-MR Console\n");
          // printf("Setting for 400-MR Console\n");
          set_field(RF,console_type,1);  // VNMRS = 0, 400-MR = 1
       }

       set_field(RF,p2_disable,0); // This will then connect the interface on the P2 connector
   }
   return RfType;
}

int getRfType()
{
    return RfType;
}


setRFTransMode(int mode)
{
  set_field(RF,transmitter_mode,mode);  // 0 =  Non-ICAT  RF, 1 - iCAT RF
}

setRFConsoleType(int type)
{
  set_field(RF,console_type,type);  // VNMRS = 0, 400-MR = 1
}

RfSettings()
{
   int value;
   value = get_field(RF,transmitter_mode);  // Non-ICAT  RF
   printf("Transmitter mode = 0x%x (0-std, 1-iCAT)\n",value);
   value = get_field(RF,console_type);  // Non-ICAT  RF
   printf("Console Type= 0x%x (0-VNMRS, 1-400MR)\n",value);
}

selectRFAmp(int band)
{
  if (band == 1)
    {
      set_field(RF,disable_high_band_amplifier,0);
      set_field(RF,disable_low_band_amplifier,1);
printf("disabled low band amplifier\n");
    }
  else
    {
      set_field(RF,disable_high_band_amplifier,1);
      set_field(RF,disable_low_band_amplifier,0);
printf("disabled high band amplifier\n");
    }
}

prtIcatDNA()
{
  uint64_t readDNA(void);
  /* obtain the unique identifier for iCAT board */
  /* used within the fail files */
  uint64_t tDNA = 0LL;
  tDNA = readDNA();
  printf("iCAT_DNA Parameter: 0x%llx\n",iCAT_DNA);
  printf("Read      ICAT DNA: 0x%llx\n",tDNA);
}

int chk4IcatFiles()
{
    char buffer[2048];
    int nfound;
    nfound = fffind("icat_top*.bit", buffer, 2048);
    IPRINT2(1,"found: %d, names: '%s'\n", nfound,buffer);
    return nfound;
}

//
// check for the fail files created if the primary iCAT FGPA image
// failed to be copied or loaded properly
//
int chk4IcatFailFiles()
{
    char buffer[2048];
    int nfound;
    nfound = fffind("*.fail", buffer, 2048);
    IPRINT2(1,"found: %d, names: '%s'\n", nfound,buffer);
    return nfound;
}

/*
 * calculate the md5 signiture of the buffer content
 * GMB
 */
int calcMd5(char *buffer,UINT32 len, char *md5sig)
{
   md5_state_t state;
   md5_byte_t digest[16];
   // char hex_output[16*2 + 1];
   int di;

   /* ---------------------------*/
   /* calc MD5 checksum signiture */
   md5_init(&state);
   md5_append(&state, (const md5_byte_t *)buffer, len);
   md5_finish(&state, digest);
   /* generate md5 string */
   for (di = 0; di < 16; ++di)   
        sprintf(md5sig + di * 2, "%02x", digest[di]);
   /* ---------------------------*/
   return 0;
}

/*
 * For the icat configuration files, extract the Index from the filename
 * index range from 0 to 6 (max?)
 * If no digits are found within the file name return -1
 * Author: Geg Brissey
 */
int extractIndex(char *buffer)
{
   char name[64];
   char *pstr,*endaddr;
   int i,len,val;
   strncpy(name,buffer,63);
   pstr = name;
   len = strlen(name);
   endaddr = pstr + len;
   // printf("addr: 0x%lx, len: %d, end addr: 0x%lx\n", pstr, len, pstr+len);

   while ( (! isdigit(*pstr++))  && (pstr <= endaddr ) );  // scan to 1st digit
/*
   {
       printf("addr: 0x%lx, char: '%c', isdigit: %d\n", pstr,*pstr,isdigit(*pstr));
       taskDelay(30);
   }
*/
   pstr = pstr - 1;
   len = strlen(pstr);
   // printf("str: '%s', %d\n",pstr,len);
   if (len > 0)
   {
      for(i = len; i >= 0; i--)
      {
         // printf("i: %d, char: '%c'\n", i, *(pstr+i-1));
         if ( isdigit( *(pstr+i-1) ) )
            break;
      }
      // printf("i=%d\n",i);
     *(pstr + i) = 0;
     // printf("pstr: '%s' \n",pstr);
     val = strtol(pstr,NULL,0);
   }
   else
     val = -1;

   return val;
}

createFailFile(char *name, char *strmsg)
{
    int len,status;
    len = strlen(strmsg) + 1; // include null 
    //cpBuf2FF(char *filename,char *buffer, unsigned long size)
    status = cpBuf2FF(name,strmsg,len);
    return status;
}

/*
 * transfer an iCAT file from the controller's flash file system to the iCAT ISF
 * e.g. icat configuration files:  icat_config00.tbl, icat_config01.tbl, etc..
 *  GMB
 */
int transferFile2Icat( char *filename )
{
   char md5sig[32 + 1];
   char md5sig2[32 + 1];
   char name[36], md5[36];
   int result, ret, di;
   unsigned long size, nSize, sizeBytes, len;
   int tableIndex,dataOffset,status,OverWriteYes;;
   char *xferfileBuffer;
   char *pFileExtType;

   OverWriteYes = 1;

   size = getFFSize(filename);
   IPRINT2(-1,"transferFile2Icat: File: '%s', Bytes: %ld\n",filename,size);
   if (size < 1)   /* return file not present */
    return(0);

   // need this extra length (headersize) for the readback  verification step
   xferfileBuffer = (char*) malloc(size + getISFFileHeaderSize() + 4); 
   if ( (xferfileBuffer == NULL) )
   {
      errLogRet(LOGIT,debugInfo, " ****** System Memory Malloc failed\n");
      return(-1);
   }

   /* transfer FFS into buffer */
   result =  cpFF2Buf(filename,xferfileBuffer,&nSize);
   if (result < 0) 
   {
      errLogRet(LOGIT,debugInfo, " ****** Read from Flash FileSystem failed\n");
      free(xferfileBuffer);
      return(-1);
   }

   /* ---------------------------*/
   /* calc MD5 checksum signiture */
   calcMd5(xferfileBuffer, nSize, md5sig);

   IPRINT3(1,"transferFile2Icat: file: '%s', nSize: %ld, md5: '%s'\n",
               filename,nSize, md5sig);

   // determine if this file is a FPGA bitstream *.bit or configuration file *.tbl
   pFileExtType = (char*) strrchr(filename,'.') + 1;
   IPRINT1(1,"transferFile2Icat: File Ext Type: '%s'\n",pFileExtType);
   tableIndex = extractIndex(filename);
   IPRINT1(1,"transferFile2Icat: tableIndex; %ld\n", tableIndex);
   // =======================================================================
   // Configuration table transfers
   //
   if ( (strcmp("tbl",pFileExtType) == 0) )
   {
      // =======================================================================
      // read conf file header from iCAT flash
      // If the file already exists on icat ISFlash don't re-copy to ISFlash
      tableIndex = getConfigTableByName(filename);  // no longer use index encoded in name, just find it.
      IPRINT2(-11,"transferFile2Icat: filename: '%s', tableIndex: %d\n", filename, tableIndex);
      if (tableIndex != -1)  // -1 means didn't find this named file
      {
         result = readConfigTableHeader(tableIndex, name, md5, (int*) &sizeBytes);
         if (result != -1)   // -1 == invalid table index, no file present
         {
             IPRINT2(1,"transferFile2Icat: 2cp md5: '%s'\n\t\t\t\t\t\t vs present md5: '%s'\n",
                      md5sig,md5);      
             ret = strcmp(md5sig, md5);
             if (ret == 0) 
             {
                IPRINT(-1,"transferFile2Icat: File already Present on Icat ISFlash\n\n");
                free(xferfileBuffer);
                return(0);
             }
         }
      }
      // =======================================================================
      result = writeIsfFile(filename, md5sig, nSize, xferfileBuffer, OverWriteYes);
      memset(xferfileBuffer,0,nSize);

      // now read it back and confirm proper writing to iCAT ISF
      len = nSize;
      // relocate file, there is no guarentee that same index was used.
      tableIndex = getConfigTableByName(filename);  // no longer use index encoded in name, just find it.
      IPRINT2(-11,"transferFile2Icat: filename: '%s', tableIndex: %d\n", filename, tableIndex);
      result = readConfigTable(tableIndex, name, md5, (UINT32*) &len, xferfileBuffer, (int*) &dataOffset);
      IPRINT4(-12,"transferFile2Icat() name: '%s', md5: '%s', len: %ld, dataOffset: %ld\n",name,md5,len,dataOffset);
   
   }
   // =======================================================================
   // iCAT FPAG file transfers
   //
   else if ( (strcmp("bit",pFileExtType) == 0) && ((tableIndex >= -1) && (tableIndex <= 1)) )
   {
      IPRINT3(1,"transferFile2Icat() bitstream: '%s', md5: '%s', len: %ld \n",
              filename,md5sig,nSize);
      // =======================================================================
      // read conf file header from iCAT flash
      // If file already exist on icat ISFlash don't re-copy to ISFlash
      if (tableIndex == -1)
         tableIndex = 0;
      result = readIcatFPGAHeader(tableIndex,name, md5, (int*)  &sizeBytes);
      if (result != -1)   // -1 == invalid table index, no file present
      {
          IPRINT2(1,"transferFile2Icat: 2cp md5: '%s'\n\t\t\t\t\t\t vs preset md5: '%s'\n",
                md5sig,md5);      
          ret = strcmp(md5sig, md5);
          if (ret == 0) 
          {
             IPRINT(-1,"transferFile2Icat: File already Present on Icat ISFlash\n\n");
	     free(xferfileBuffer);
             return(0);
          }
      }
      // =======================================================================
      // transfer FPGA bit-stream

      result = writeIcatFPGA(tableIndex, filename, md5sig, nSize, xferfileBuffer);
      memset(xferfileBuffer,0,nSize);
      // now read it back confirm proper writing to iCAT ISF
      len = nSize;
      result = readIcatFPGA(tableIndex, name, md5, (UINT32*) &len, xferfileBuffer);
      dataOffset = 0;   
   }
   /* calc MD5 checksum signiture */
   calcMd5((xferfileBuffer + dataOffset), len, md5sig2);
   IPRINT3(1,"transferFile2Icat: file: '%s', written/read md5: '%s'\n\t\t\t\t\t\t\t vs '%s'\n",
          filename, md5sig, md5sig2);

   ret = strcmp(md5sig, md5sig2);
   if (ret != 0) 
   {
      char errmsg[256];
      // failure on Primary or Seconadary iCAT FPGA image, must mark failure via a small file
      if ( (strcmp("bit",pFileExtType) == 0)  )
      {
          // do not alter format of string prior to '\n'
          sprintf(errmsg,"iCAT_DNA: 0x%llx \nImage failed to copy.",iCAT_DNA);
          if ( (tableIndex >= -1) && (tableIndex <= 0) ) 
          {
             createFailFile("pri_imagecp.fail",  errmsg);
             // deleting primary image is probably un-necessary , it's just not going to boot anymore
          }
          else
          {
             createFailFile("sec_imagecp.fail",  errmsg);
             isffpgadel(); // deleted secondary image
          }
          errLogRet(LOGIT,debugInfo, "transferFile2Icat(): '%s' iCAT FPGA image copy, failed.\n",filename);
      }
      else if ( (strcmp("tbl",pFileExtType) == 0) && ((tableIndex >= 0) && (tableIndex < 6)) )
      {  // maybe moot since we don't copy tables any more to the ISF flash
         isftbldel(tableIndex);	// delete table that failed to copy
      }

      errLogRet(LOGIT,debugInfo, "transferFile2Icat():  ****** ERROR: MD5 Checksum do NOT match\n");
      errLogRet(LOGIT,debugInfo, "transferFile2Icat():  True MD5 - '%s'\n",md5sig);
      errLogRet(LOGIT,debugInfo, "transferFile2Icat():  Calc MD5 - '%s'\n",md5sig2);
      free(xferfileBuffer);
      return(-1);
   }
   IPRINT(-1,"Transfer Succesful\n\n");
   free(xferfileBuffer);
   return 0;
}

int chk4IcatImageCpFail()
{
    char buffer[64];
    int nfound;
    nfound = fffind("*_imagecp.fail", buffer, 64);
    IPRINT2(1,"found: %d, names: '%s'\n", nfound,buffer);
    return nfound;
}

int chk4IcatImageLdFail()
{
    char buffer[2048];
    int nfound;
    nfound = fffind("*_imageld.fail", buffer, 64);
    IPRINT2(1,"found: %d, names: '%s'\n", nfound,buffer);
    return nfound;
}

chk4IcatConfigFail()
{
    char buffer[2048];
    int nfound;
    nfound = fffind("icat_config.fail", buffer, 64);
    IPRINT2(1,"found: %d, names: '%s'\n", nfound,buffer);
    return nfound;
}

isfmd5(char *filename)
{
    char md5sig[32 + 1];
    char *buffer;
    UINT32 len,offset;

    buffer = readIsfFile(filename, &len, &offset);
    if (buffer != NULL)
    {
       calcMd5((buffer + offset), len, md5sig);
       printf("%s  %s\n",md5sig,filename);
       free(buffer);
    }
    else
       printf("File '%s' not found.\n",filename);
}

/*
 * find any icat files on Flash and initiate transfer of these files to the iCAT ISFLash
 */
transferIcatFiles()
{
    char buffer[2048],filename[36];
    char *pCList,*pStr,*pType;
    int nfound,index,result;
    nfound = fffind("icat_top*.bit", buffer, sizeof(buffer));
    IPRINT2(-1,"\ntransferIcatFiles()  found: %d, names: '%s'\n", nfound,buffer);
    pCList = buffer;
    filename[0] = 0;

    for (pStr = (char *) strtok_r(buffer,",",&pCList);
	      pStr != NULL;
	      pStr = (char *) strtok_r(NULL,",",&pCList))
    {
       if (pStr != NULL)
          strncpy(filename,pStr,35);

       IPRINT1(1,"transferIcatFiles() filename: '%s'\n",filename);
       pType = (char*) strrchr(filename,'.') + 1;
       IPRINT2(1,"type: 0x%lx, '%s'\n",pType,pType);
       index = extractIndex(filename);
       IPRINT1(1,"index: %ld \n",index);
       result = transferFile2Icat( filename );
       filename[0] = 0;
    }
}

/*
 * This routine assumes the format of the DNA line is 'DNA: 0x123456789abcdef0 \n'
 * where the number is a 64 bit hex value obtained from the iCAT board that had the error
 * 
 */
uint64_t getDNAfromFile(char *filename)
{
   int result;
   uint32_t size,nSize;
   uint64_t icatDNA;
   char* xferfileBuffer;
   char *pCList,*pStr,*pType;
   size = getFFSize(filename);
   xferfileBuffer = (char*) malloc(size + 4); 
   if (xferfileBuffer != NULL)
   {
      result =  cpFF2Buf(filename,xferfileBuffer,&nSize);
      xferfileBuffer[nSize] = 0;
      pStr = (char *) strtok_r(xferfileBuffer,":",&pCList);
	   pStr = (char *) strtok_r(NULL," ",&pCList);
      if ( pStr != NULL)
          sscanf(pStr,"%llx",&icatDNA);
      else
          icatDNA = 0LL;
      IPRINT2(-9,"File '%s': iCAT DNA: 0x%llx\n",filename,icatDNA);
      free(xferfileBuffer);
   }
   else
      icatDNA = 0LL;

   return icatDNA;
}

/*
 * Get a list of all *.fail files from the controllers flash
 * check the present iCAT DNA value against that in the fail file
 * if the do not match then delete the file, since the fail file
 * does not pertain to the present iCAT RF
 */
removeFailFiles(uint64_t icatDNA)
{
    char buffer[2048],filename[36];
    char *pCList,*pStr,*pType;
    int nfound,index,result;
    uint64_t fileIcatDNA;

    nfound = fffind("*.fail", buffer, sizeof(buffer));
    IPRINT2(-9,"\nremoveFailFiles()  found: %d, names: '%s'\n", nfound,buffer);
    pCList = buffer;
    filename[0] = 0;

    for (pStr = (char *) strtok_r(buffer,",",&pCList);
         pStr != NULL;
         pStr = (char *) strtok_r(NULL,",",&pCList))
    {
       if (pStr != NULL)
       {
          strncpy(filename,pStr,35);

          IPRINT1(-9,"\nremoveFailFiles() filename: '%s'\n",filename);
          fileIcatDNA = getDNAfromFile(filename);
          IPRINT3(-9,"\nremoveFailFiles() filename: '%s' - DNA: 0x%llx vs 0x%llx\n",filename,fileIcatDNA,icatDNA);
          // if the fail file is not for this iCAT then delete it.
          if (icatDNA != fileIcatDNA)
          {
             IPRINT1(-9,"\nremoveFailFiles():  DELETE file: '%s' \n",filename);
             ffdel(filename);
          }
       } 
       filename[0] = 0;
    }
}

/*
 * obatin the 64-bit DNA value from the iCAT Regs
 */
uint64_t readDNA(void)
{
  union
  {
    uint64_t lword;
    uint16_t word[4];
  } temp;

  temp.word[3] = icat_spi_read(iCAT_DNA0);
  temp.word[2] = icat_spi_read(iCAT_DNA1);
  temp.word[1] = icat_spi_read(iCAT_DNA2);
  temp.word[0] = icat_spi_read(iCAT_DNA3);

  return temp.lword;
}

/*
 * temporary test routines
 */
tstscan()
{
    uint64_t tstval;
    char *pVal = "0x123456789abcdef0";

    sscanf(pVal,"%llx",&tstval);
    printf("iCAT DNA: 0x%llx\n",tstval);
}

tstfaildna()
{
   char errmsg[256];
   iCAT_DNA = 0x123456789abcdef0LL;
   sprintf(errmsg,"iCAT_DNA: 0x%llx \nImage failed to copy.",iCAT_DNA);
   createFailFile("pri_imagecp.fail",  errmsg);
}

mkallfail()
{
   char errmsg[256];
   int status;

   sprintf(errmsg,"iCAT_DNA: 0x%llx \nImage failed to copy.",iCAT_DNA);
   createFailFile("pri_imagecp.fail",  errmsg);
   createFailFile("sec_imagecp.fail",  errmsg);
   sprintf(errmsg,"iCAT_DNA: 0x%llx \nSecondary Image failed to load.",iCAT_DNA);
   createFailFile("sec_imageld.fail",  errmsg);

   status = 4242;
   sprintf(errmsg,"iCAT_DNA: 0x%llx \nError: %d \n",iCAT_DNA,status);
   createFailFile("icat_config.fail", errmsg);
}

catff(char *filename)
{
   int result;
   uint32_t size,nSize;
   uint64_t tstval;
   char* xferfileBuffer;
   char *pCList,*pStr,*pType;
   size = getFFSize(filename);
   xferfileBuffer = (char*) malloc(size + 4); 

   result =  cpFF2Buf(filename,xferfileBuffer,&nSize);
   xferfileBuffer[nSize] = 0;
   printf("'%s'\n",xferfileBuffer);
   pStr = (char *) strtok_r(xferfileBuffer,":",&pCList);
   printf("'%s'\n",pStr);
	pStr = (char *) strtok_r(NULL," ",&pCList);
   printf("'%s'\n",pStr);
   if ( pStr != NULL)
       sscanf(pStr,"%llx",&tstval);
   else
       tstval = 0LL;
   free(xferfileBuffer);
   printf("iCAT DNA in fail file: 0x%llx\n",tstval);
}

int loadSecondaryFPGA(void)
{
  int status;
  // primary FPGA image placed onto iCAT
  // Tell iCAT to reload FPGA, wait for about 5 seconds
  IPRINT(-1,"Rebooting iCAT\n");
  icat_spi_write(iCAT_RebootAddressHigh,0xc);
  icat_spi_write(iCAT_RebootAddressLow,0);
  status = rebootIcat();
  if (status == -1)  // failed to boot
    {
      char errmsg[256];
      // do not alter format of string prior to '\n'
      sprintf(errmsg,"iCAT_DNA: 0x%llx \nSecondary Image failed to load.",iCAT_DNA);
      createFailFile("sec_imageld.fail",  errmsg);
      errLogRet(LOGIT,debugInfo, "transferFile2Icat():  iCAT FPGA reload, failed.\n");
      return -1;
    }
  else
    {
      IPRINT(-1,"Reboot Successful\n");

      /* set console type and transmitter mode and p2 registers appropriately */
      setupRfType();
    }
}

/*
 * Main Initialization routine call from initBrdSpecific() in rf.c 
 * as part of RF controller's board specific bring-up
 * GMB
 */
initIcat()
{
    int iCatID, nfound, status;
   /* is the present RF FPGA iCAT enabled? */
   /* then continue on */
   if ( fpgaIcatEnabled() == 0 )
      return 0;

   /* Set console type and transmitter mode and p2 registers appropriately */
   /* If there an iCAT connected to this RF controller? */
   /* then continue on */
   if ( setupRfType() == 0)  // no icat just return
       return;

   /* enable the Global Sector Protection of the ISFlash */
   enableIsfProt();

   /* remove Fail Files if they do not match current iCAT DNA number */
   removeFailFiles(iCAT_DNA);

   /*  Are there any icat files on the controllers flash? */
   /*  if present then transfer these files to the iCAT flash */
   if ( (nfound = chk4IcatFiles()) != 0)
     transferIcatFiles();
     
   if (loadSecondaryFPGA() == -1)
     return;

   taskDelay(60);

   /* Some iCATs may still have the old config file in them. Remove
      that here so that it doesn't get loaded too. */
   isftbldel(5);		/* TEMPORARY */

   /*
    * Initialize the iCAT board.
    */
   status = simon_boot("load ffs:icat_config.4th");
   if (status != 0)
   {
      char errmsg[256];
      // do not alter format of string prior to '\n'
      sprintf(errmsg,"iCAT_DNA: 0x%llx \nError: %d \n",iCAT_DNA,status);
      createFailFile("icat_config.fail", errmsg);
      errLogRet(LOGIT,debugInfo, "initIcat():  FORTH iCAT Configuration Error: %d\n",status);
   }
}
