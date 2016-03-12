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

/* iCAT Internal System Flash  (ISF)  */
/* Author: Greg Brissey  1/20/2010 */

/*
DESCRIPTION

   functions to handle the iCAT Internal System Flash (ISF) 
   reading and writing, etc..

*/

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif

#include <string.h>
#include <stdioLib.h>
#include <vxWorks.h>

#include "md5.h"
#include "assert.h"

#include "icatSpiFifo.h"
#include "icatISF.h"

static int iCATFFSVersion = -1; // 1 - old FFS all 4 block size, 2 = new: 48 1-blk, 8 4-blk, 2 24-blk
static int Max_Num_Files = 0; // max number of files, 6 - old FFS, 58 - new FFS layout
 
// Directory table has a fixed length of 10 files  (configuration table sets)
// the FPGA bitstream file has only two fixed locations

// Spartan-3AN FPGA 700AN
// The flash memory is arranged in pages (264 bytes) , 
//    blocks = 8 pages, sector = 256 pages or 32 blocks
//
// The FPGA bit stream have two fixed loactions, dependent on the version of the sparten FPGA
// 1st bitstream  sector 0, page 0 (0x00_0000) through sector 1 page 207 (0x01_9E00)
// 2nd bitstream  ???
//
// Spartan-3AN FPGA 700AN
//
// totals: 4096 pages, 512 blocks, 16 sectors
// Bytes per   Block  2,112
//            Sector 67,584
// FPGA bitstream uncompressed: 2,732,640 bytes, requires 1,294 pages
// User Data:  5,917,824 bytes available
// if Using multiboot bitstreams, 2,162,688 bytes
// See Page 8, Table 1-1 in ug.pdf
//
// User data space also has fixed starting locations, 
//
// design parameters:   
// 1. configuration tables files  always start at a new block, this allows block erase
//    without worry of deleting another file accedentely 
//

//
// User data, or the configuration table will start at Sector 12
// page 3,071 or 0x18_0000
// 24-bit = BFF
// 24-bit  mask 0x00FFFFFF
// sector  mask 0x001E0000
// block   mask 0x001FF000
// address mask 0x001FFFFF
//   page  mask 0x001FFE00
//   byte  mask 0x000001FF

// files are fixed lengh , in units of blocks (2113 bytes, 8 pages)

/*
 * Determine whether the iCAT FFS layout is the old or new version
 * scan the possible old tables, check headers, if any are found 
 * that exceed the new smaller size for FFS 2, then it's the old FFS otherwise
 * it is or at least comparible with the new FFS layout.
 *
*/
determineIcatFFSVer()
{
    unsigned long calcStrtPage4FFS1(int index);
    int readFileHeader(unsigned long startPage,char *name, char* md5, UINT32 *len);
    int result,index;
    unsigned long pageAddr,lenBytes;
    char name[36],md5[36];

    iCATFFSVersion = -1;
    for (index = 0; index < MAX_NUM_TBL_FILES; index++)
    {
        pageAddr = calcStrtPage4FFS1(index);
        result = readFileHeader(pageAddr,name,md5,(UINT32*) &lenBytes);
        IPRINT2(0,"determineIcatFFSVer: pageAddr: %d, result: %d\n",pageAddr,result);
        // printf("determineIcatFFSVer: pageAddr: %d, result: %d\n",pageAddr,result);
        if (result != -1)
        {
           // printf("index: %d, file: '%s', size: %ld, MaxSize: %ld\n",
           IPRINT4(1,"index: %d, file: '%s', size: %ld, MaxSize: %ld\n",
                   index,name,lenBytes,((FILE_SIZE_1_IN_PAGES * BYTES_PER_PAGE) - sizeof(ISFFILEHEADER)));
           if (lenBytes > ((FILE_SIZE_1_IN_PAGES * BYTES_PER_PAGE) - sizeof(ISFFILEHEADER)))
           {
              iCATFFSVersion = 1;  // old FFS version
              Max_Num_Files = MAX_NUM_TBL_FILES;
              break;
           }
        }
    }
    // if still unassigned must be version 2 FFS
    if (iCATFFSVersion == -1)
    {
       iCATFFSVersion = 2;  // new FFS layout
       Max_Num_Files = MAX_NUM_FILE_INDICES;
    }
    return iCATFFSVersion;
}

/*
 * allow deriectly setting the version number
 * for testing only
 */
int setISFLayoutVer(int ver)
{
   if (ver == 1)
   {
      iCATFFSVersion = 1;  // old FFS version
      Max_Num_Files = MAX_NUM_TBL_FILES;
   }
   else if (ver == 2)
   {
       iCATFFSVersion = 2;  // new FFS layout
       Max_Num_Files = MAX_NUM_FILE_INDICES;
   }
   else
     printf ("only version of 1 or 2 are valid\n");

   return 0;
}

/*
 * return the ISF FSS layout version
 */
int getISFLayoutVer()
{
   if (iCATFFSVersion == -1 )
      determineIcatFFSVer();

   return (iCATFFSVersion);
}

/*
 * get the maximum number of files, based on the ISF layout version
 */
int getISFMaxNumFiles()
{
   if (Max_Num_Files == 0 )
      determineIcatFFSVer();

   return (Max_Num_Files);
}

/*
 * return ISFFILEHEADER Size
 */
int getISFFileHeaderSize()
{
   return sizeof(ISFFILEHEADER);
}

/*
 * return USER Max File Size
 */
int getISFFileMaxSize()
{
   int maxSize;
   int FFSType = getISFLayoutVer();
   if (FFSType == 1)
      maxSize = ( (CONFIG_TABLE_SIZE_IN_BLOCKS * BYTES_PER_BLOCK) - sizeof(ISFFILEHEADER));
   else
      maxSize =  ( (FILE_SIZE_3_IN_BLOCKS * BYTES_PER_BLOCK) - sizeof(ISFFILEHEADER));

   return maxSize;
}


/*
 * Base on file size determine the file index range most suitable for the file
 * 0 - 2028 ( 2112 - header 84 )
 * 2029 - 8364  ( 8448 - header 84 )
 * 8365 - 50604 ( 50688 - header )
*/
int indexRangeBySize(int size, int *startIndex, int *endIndex, int *maxLenBytes)
{
   int FFSVer = getISFLayoutVer();
  
   if (FFSVer == 1)
   {
       *startIndex = 0;
       *endIndex = 5;
       *maxLenBytes = ((CONFIG_TABLE_SIZE_IN_BLOCKS * BYTES_PER_BLOCK) - sizeof(ISFFILEHEADER));
       return 0;
   }

   // Else it's ver 2

   if ( size <= ( (FILE_SIZE_1_IN_BLOCKS * BYTES_PER_BLOCK) - sizeof(ISFFILEHEADER) ) )
   {
       *startIndex = FILE_SIZE_1_STRT_INDEX;
       *endIndex = FILE_SIZE_1_END_INDEX;
       *maxLenBytes = (FILE_SIZE_1_IN_PAGES * BYTES_PER_PAGE) - sizeof(ISFFILEHEADER);
       return 0;
   }
   else if ( size <= ( (FILE_SIZE_2_IN_BLOCKS * BYTES_PER_BLOCK) - sizeof(ISFFILEHEADER) ) )
   {
       *startIndex = FILE_SIZE_2_STRT_INDEX;
       *endIndex = FILE_SIZE_2_END_INDEX;
       *maxLenBytes = (FILE_SIZE_2_IN_PAGES * BYTES_PER_PAGE) - sizeof(ISFFILEHEADER);
       return 0;
   }
   else if ( size <= ( (FILE_SIZE_3_IN_BLOCKS * BYTES_PER_BLOCK) - sizeof(ISFFILEHEADER) ) )
   {
       *startIndex = FILE_SIZE_3_STRT_INDEX;
       *endIndex = FILE_SIZE_3_END_INDEX;
       *maxLenBytes = (FILE_SIZE_3_IN_PAGES * BYTES_PER_PAGE) - sizeof(ISFFILEHEADER) ;
       return 0;
   }
   else
   {
       *startIndex = -1;
       *endIndex = -1;
       *maxLenBytes = 0;
      return -1;
   }
}


/*
 * Calc the start page based on index, for the newer FFS layout
 */
unsigned long calcStrtPage4FFS2(int index)
{
   unsigned long pageOffset,startPage;
    // printf("Gindex: %d\n",index);
    if ( index <= FILE_SIZE_2_STRT_INDEX )
    {
       pageOffset = FILE_SIZE_1_IN_PAGES * index;
       // printf("index: %d\n",index);
    }
    else if ( index <= FILE_SIZE_3_STRT_INDEX )
    {
       // printf("index: %d\n",index - FILE_SIZE_2_STRT_INDEX);
       pageOffset = (FILE_SIZE_1_IN_PAGES * FILE_SIZE_2_STRT_INDEX) +
                    (FILE_SIZE_2_IN_PAGES * (index - FILE_SIZE_2_STRT_INDEX));
    }
    else
    {
       // printf("index: %d\n",index - FILE_SIZE_3_STRT_INDEX);
       pageOffset = (FILE_SIZE_1_IN_PAGES * FILE_SIZE_2_STRT_INDEX) +
                    (FILE_SIZE_2_IN_PAGES * (FILE_SIZE_3_STRT_INDEX - FILE_SIZE_2_STRT_INDEX)) +
                    (FILE_SIZE_3_IN_PAGES * (index - FILE_SIZE_3_STRT_INDEX));
    }
    startPage = pageOffset + BASEUSER_DATA_PAGEADDR;
    // printf("PageOffset: %d, PageAddres: %d\n",pageOffset, startPage);
    // IPRINT2(2,"startPage: %ld, 0x%lx\n",startPage, startPage);
    return startPage;
}

/*
 * Calc the start page based on index, for the older FFS layout
 */
unsigned long calcStrtPage4FFS1(int index)
{
     unsigned long pageOffset, startPage;
     pageOffset = CONFIG_TABLE_SIZE_IN_PAGES * index;   // index starts at zero
     startPage = (BASEUSER_DATA_PAGEADDR + pageOffset);
     // IPRINT2(2,"startPage: %ld, 0x%lx\n",startPage, startPage);
     return startPage;
}

/*
 * Calc the start page based on index
 */
unsigned long calcStartPage(int index)
{
     unsigned long startPage;
     int FFSVer;
     FFSVer = getISFLayoutVer();

     if (FFSVer == 1)
        startPage = calcStrtPage4FFS1(index);
     else if (FFSVer == 2)
        startPage = calcStrtPage4FFS2(index);
     else
        startPage = 3000;
     IPRINT2(2,"startPage: %ld, 0x%lx\n",startPage, startPage);
     return startPage;
}

/*
 * Calc the start page based on index
 */
unsigned long calcFPGAPage(int index)
{
   unsigned long pageOffset, startPage;
   // index = 0, is the primary iCAT FPGA image
   if (index == 0)
   {
       startPage = BASEPAGE_PRIMFPGA_BITSTREAM;
   } 
   // index = 1, is the secondary iCAT FPGA image
   else
   {
      startPage = BASEPAGE_FPGA_BITSTREAM;
   }
   IPRINT3(2,"calcFPGAPage(): index; %ld, startPage: %ld, 0x%lx\n",index,startPage, startPage);
   return startPage;
}

/*
 * Calc the start page based on index
 */
unsigned long calcFPGAHeaderPage(int index)
{
   unsigned long headerStartPage;
   // index = 0, is the primary iCAT FPGA image
   if (index == 0)
   {
       headerStartPage = BASEPAGE_PRIMFPGA_FILEHEADER;
   } 
   // index = 1, is the secondary iCAT FPGA image
   else
   {
      headerStartPage = BASEPAGE_FPGA_FILEHEADER;
   }
   IPRINT2(2,"startPage: %ld, 0x%lx\n",headerStartPage, headerStartPage);
   return headerStartPage;
}

/*
 * read from iCAT flash based on index
 */
readISF(unsigned long address,char *buffer, unsigned long len)
{
    unsigned long readcmd;

    readcmd = 0;
    readcmd = RANDOM_READ | (address * PAGE_ADDRESS_SIZE);
    IPRINT4(1,"readISF() RandomRead: 0x%lx, Cmd: 0x%lx, PageAddr: %ld, ByteAddr: %ld\n", 
	    readcmd, readcmd >> 24, ((readcmd & ISF_PAGE_MASK) / PAGE_ADDRESS_SIZE),(readcmd & ISF_BYTE_MASK));
    // into the FIFO , and out to the iCAT 
    readIcatFifo(readcmd, (UINT32*) buffer, len);

    return 0;
}

//
//   tables files are fixed size.
//
int writeISF(unsigned long address, char *buffer, int len)
{
     int i,j,remainder,nblocks,npages,status;
     char *pBuf;
     unsigned long writeBuffer[WORDS_PER_PAGE+1]; // 1 cmd word and 1 page (264 bytes)
     unsigned long startPage, startPageAddr,startBlock,eraseCmd;
     unsigned long BufferWriteCmd, Buffer2PageCmd, BufferWriteAddr;
     unsigned long VerifyWriteCmd;
     unsigned long pageaddr, byteaddr, addrtmp;

     status = 0;
     startPage = address; // calcStartPage(index);
     startBlock = startPage / PAGES_PER_BLOCK;
     IPRINT3(0,"writeISF() startBlockAddr: %ld, (0x%lx), Block Addr: 0x%lx \n", startBlock, startBlock, (startBlock << 12));
     eraseCmd = BLOCK_ERASE | ( startBlock << 12 );
    
     // first erase the blocks, speeds writing
     remainder = len % BYTES_PER_BLOCK;
     nblocks = len / BYTES_PER_BLOCK;
     nblocks = (remainder) ? nblocks + 1 : nblocks;
     IPRINT2(0,"writeISF() bytes: %ld,  blocks: %ld\n", len, nblocks);
     for ( i=0; i < nblocks; i++)
     {
        IPRINT4(0,"writeISF() %d eraseCmd : 0x%lx, cmd: 0x%lx, block: 0x%lx\n", 
               i+1, eraseCmd, eraseCmd >> 24, (eraseCmd & ISF_BLOCK_MASK) >> 12);
        writeIcatFifo((UINT32*) &eraseCmd,4);
        eraseCmd = eraseCmd + 0x1000; // increment by one block
     }

     // what size buffer require for write
     // write consistents of 
     // 1. Buffer Write [BUFFER1_WRITE] (byte address for buffer always Zero 
     // 2. One Page of Data, 264 bytes 
     // 3. Buffer2PageWoErase [BUFFER1_PAGEwoE] with page & byte address
     //
     pBuf = (char*) buffer;
     remainder = len % BYTES_PER_PAGE;
     npages = len / BYTES_PER_PAGE;
     npages = (remainder) ? npages + 1 : npages;
     IPRINT3(0,"writeISF() bufferAddr: 0x%lx, bytes: %ld,  pages: %ld\n", buffer, len, npages);

     for (i=0 ; i < npages  /* pages to write */; i++)
     {
        if (! (i % 8) )
        {
           IPRINT(1,"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n\n");
           IPRINT1(1,"Block Write %d\n",(i+8)/8);
           IPRINT(1,"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        }
         
        // BufferWriterCmd - page address is ignored and for our case the byte address always starts at zero
        BufferWriteCmd = BUFFER1_WRITE;  // address is Zero
        writeBuffer[0] = BufferWriteCmd;
        memcpy(writeBuffer+1,pBuf,BYTES_PER_PAGE);
        writeIcatFifo((UINT32*) writeBuffer,sizeof(writeBuffer[0])+BYTES_PER_PAGE);

        BufferWriteAddr = (startPage + i) * PAGE_ADDRESS_SIZE;
        Buffer2PageCmd = BUFFER1_PAGEwoE | BufferWriteAddr;

        pageaddr = ((Buffer2PageCmd & ISF_PAGE_MASK) / PAGE_ADDRESS_SIZE);
        byteaddr = (Buffer2PageCmd & ISF_BYTE_MASK);
        IPRINT5(0,"writeISF() %d Buf2Page: 0x%lx, Cmd: 0x%lx, PageAddr: %ld, ByteAddr: %ld\n", 
                i+1,Buffer2PageCmd, Buffer2PageCmd >> 24, pageaddr,byteaddr);

        writeIcatFifo((UINT32*) &Buffer2PageCmd,sizeof(Buffer2PageCmd));

        // verify that page was written to ISFlash witho error.
        VerifyWriteCmd = BUFFER1_VERIFY | BufferWriteAddr;
        status = writeIcatFifo((UINT32*) &VerifyWriteCmd,sizeof(VerifyWriteCmd));
        // compare bit-6, 1-failed, 0-OK
        if ((status & 0x40) != 0 )
        {
           pageaddr = ((VerifyWriteCmd & ISF_PAGE_MASK) >> 9);
           errLogRet(LOGIT,debugInfo, "Write Verify Failed for page %ld, abort writing\n",
                     pageaddr);
           return -1;
        }
          
        pBuf = pBuf + BYTES_PER_PAGE;

     }
     return 0;
}

/*
 * Enable the ISFlash Sector Protection scheme
 * Sectors are protected based on the value of the 
 * Sector Protection Register (16-bytes)
 */
enableIsfProt()
{
   unsigned long ISF_Cmd;
   int status;
   ISF_Cmd = SECTOR_PROTECTION_ENABLE;
   status = writeIcatFifo((UINT32*) &ISF_Cmd,4);
   return status;
}

/*
 * Disable the ISFlash Sector Protection scheme
 * All sectors are open to be erase and written to
 */
disableIsfProt()
{
   unsigned long ISF_Cmd;
   int status;
   ISF_Cmd = SECTOR_PROTECTION_DISABLE;
   status = writeIcatFifo((UINT32*) &ISF_Cmd,4);
   return status;
}

/*
 * erase the Sector Protection register
 * This needs to be done prior to programming it
 * Warning this erase/program is limited to 10,000 cycles
 * When erased all bytes are 0xFF which would protect all 
 * sectors
 */
eraseIsfProtReg()
{
   unsigned long ISF_Cmd;
   int status;
   ISF_Cmd = SECTOR_PROTECTION_ERASE;
   status = writeIcatFifo((UINT32*) &ISF_Cmd,4);
   return status;
}

writeIsfProtReg(UINT8 *buf)
{
   unsigned long ISF_Cmd;
   int status;
   UINT8 CmdAndData[4+16];

   ISF_Cmd = SECTOR_PROTECTION_PROG;
   memcpy(CmdAndData,&ISF_Cmd,sizeof(long));
   memcpy(&(CmdAndData[4]),buf,16);
   status = writeIcatFifo((UINT32*) CmdAndData,4+16);
   return status;
}

openAllSectors()
{
    UINT8 protRegData[ISF_PROTECT_REG_SIZE_BYTES];
    int i,status;

    for(i=0; i < 16; i++)  protRegData[i] = 0;
    eraseIsfProtReg();
    status = writeIsfProtReg(protRegData);
    
    return status;
}

readIsfProtReg(UINT8 *buf)
{
   unsigned long ISF_Cmd;
   ISF_Cmd = SECTOR_PROTECTION_READ;
   readIcatFifo(ISF_Cmd, (UINT32*) buf, ISF_PROTECT_REG_SIZE_BYTES);
}

prtIsfProtReg()
{
   int i;
   UINT8 buf[ISF_PROTECT_REG_SIZE_BYTES];
   readIsfProtReg(buf);
   printf("Protection Bytes (0-unprot, f-prot): \n");
   for (i=0; i < ISF_PROTECT_REG_SIZE_BYTES; i++)
   {
      printf("%d: 0x%x, ",i,buf[i]);
      if ( (i != 0) && !(i % 7) )
        printf("\n");
   }
   printf("\n\n");
}

/* 
  Test if a page has the write protection enabled on it.
  since we protect whole sectors at a time we need only to test the value
  for non zero
*/
isPageProtected( unsigned long page )
{
   int sector;
   UINT8 buf[ISF_PROTECT_REG_SIZE_BYTES];
   readIsfProtReg(buf);
   
    // for (i=0; i < ISF_PROTECT_REG_SIZE_BYTES; i++)
     //   printf("%d: 0x%x, ",i,buf[i]);
   // since we protect whole sectors at a time we need only to test the value
   // for non-zero value
   sector = page / PAGES_PER_SECTOR ; // (256 pages per sector)
   // printf("page: %lu, sector: %d, protbits: 0x%lx\n",page,sector,buf[sector]);
   return ( ( buf[sector]  == 0x00) ? 0 : 1 );
}
/*
 * bit-7 0-busy, 1-ready
 * bit-6 0-compare matched, 1-differ
 * bit-5-2: ISF Memory Size MBit: 0011 - 1, 0111 - 4, 1001- 8, 1011 - 16MBit
 * bit-1: 0 - open, 1- protected
 * bit-0: page size, 0=extended (default), 1-Power-of-2
 */
prtIsfStatus()
{
   int status;
   int busy,compare, size,mbits, prot,pagesize;
   status = getIcatSpiStatus();
   busy = (status >> 7) & 0x1;
   compare = (status >> 6) & 0x1;
   size = (status >> 2) & 0xF;
   prot = (status >> 1) & 0x1;
   pagesize = status & 0x1;
   printf("ISF Status: 0x%x\n",status);
   printf("ISF: %s\n", (busy == 1) ? "Ready" : "Busy");
   printf("ISF Compare: %s\n", (compare == 0) ? "Matches" : "Differs");
   mbits = 0;
   switch(size) {
    case 3: mbits = 1; break;
    case 7: mbits = 4; break;
    case 9: mbits = 8; break;
    case 11: mbits = 16; break;
   }
   printf("ISF Size MBit: %d\n", mbits);
   printf("ISF Sector Protected: %s\n", (prot == 1) ? "Protected" : "Open");
   printf("ISF PageSize: %s\n", (pagesize == 0) ? "Extended" : "Power-of-2");
    
}

/*
getProtRegBytes2Alter(UINT32 pageAddress, UINT32 SizeBytes, UINT32 *startByte, UINT32 *nBytes)
{
    int startSector, remainder, sectors;
    startSector = pageAddress / PAGES_PER_SECTOR;
    remainder = SizeBytes % BYTES_PER_SECTOR;
    sectors = SizeBytes / BYTES_PER_SECTOR;
    sectors = (remainder) ? sectors + 1 : sectors;
    // endSector = startSector + sectors;
    *startByte = startSector;
    *nBytes = sectors;

    return sectors;
}
*/

protectISFSectors(UINT32 pageAddress, UINT32 SizeBytes, int protectFlag)
{
    int i, status, startSector, remainder, sectors;
    int endSector;
    UINT8 protRegData[ISF_PROTECT_REG_SIZE_BYTES];
    // read the present values of trhe protection register
    readIsfProtReg(protRegData);
    // calc beginning sector, and end Sector for File
    // #define PAGES_PER_SECTOR (256)
    startSector = pageAddress / PAGES_PER_SECTOR;
    remainder = SizeBytes % BYTES_PER_SECTOR;
    sectors = SizeBytes / BYTES_PER_SECTOR;
    sectors = (remainder) ? sectors + 1 : sectors;
    endSector = startSector + sectors - 1;
    printf("StartPage: %ld, length: %ld\n", pageAddress, SizeBytes);
    printf("Start Sector: %d, nsectors: %d, endSector: %d\n",startSector,sectors,endSector);

/*
   for (i=0; i < ISF_PROTECT_REG_SIZE_BYTES; i++)
   {
      printf("%d: 0x%x, ",i,protRegData[i]);
      if ( (i != 0) && !(i % 7) )
        printf("\n");
   }
   printf("\n\n");
*/


    // secondary image (sector 6), must open sector 5 for header writting
    if (startSector == 6)  
        startSector = 5;
    for (i=startSector; i <= endSector ; i++)
    {
       if (protectFlag == 1)
           protRegData[i] = 0xff;
       else
           protRegData[i] = 0x00;
    }
    
/*
   for (i=0; i < ISF_PROTECT_REG_SIZE_BYTES; i++)
   {
      printf("%d: 0x%x, ",i,protRegData[i]);
      if ( (i != 0) && !(i % 7) )
        printf("\n");
   }
   printf("\n\n");
*/


   eraseIsfProtReg();
   status = writeIsfProtReg(protRegData);
    
   return status;
}


/*
 * Erase a page of the ISFlash
 * Do Not erase pages 0 - 1293  fail-safe icat FPGA boot image
 * Secondary FPGA image at 1536 - 2829
 * icat config table start at 3072
 */
eraseIsfPage(int startPage)
{
   unsigned long ISF_Cmd,pageaddr,byteaddr;
   if (startPage < 1294 )
      return -1;
   ISF_Cmd = PAGE_ERASE | (startPage << 9);
   pageaddr = ((ISF_Cmd & ISF_PAGE_MASK) >> 9);
   byteaddr = (ISF_Cmd & ISF_BYTE_MASK);
   // printf("eraseIsfPage() PageErase: 0x%lx, Cmd: 0x%lx, PageAddr: %ld, ByteAddr: %ld\n", 
   //              ISF_Cmd, ISF_Cmd >> 24, pageaddr,byteaddr);

   writeIcatFifo((UINT32*) &ISF_Cmd,4);
   return 0;
}

#define PROT_ON (1)
#define PROT_OFF (0)

isftbldel(int index)
{
   unsigned long startPage;
   int rdonly;

   startPage = calcStartPage(index);
   rdonly = isPageProtected( startPage );
   // protectISFSectors(3072,67584,0)
   if (rdonly > 0) {
       printf("Write protection removed.\n");
       protectISFSectors(startPage, (BYTES_PER_BLOCK * 2), PROT_OFF);  // turn off write protection
   }
   eraseIsfPage(startPage);
}

isffpgadel()
{
   unsigned long startPage;
   startPage = BASEPAGE_FPGA_FILEHEADER;

   protectISFSectors(BASEPAGE_FPGA_FILEHEADER, (BYTES_PER_BLOCK * 2), PROT_OFF);  // turn off write protection
   eraseIsfPage(startPage);
   taskDelay(5);
   startPage = BASEPAGE_FPGA_BITSTREAM;
   eraseIsfPage(startPage);
   protectISFSectors(BASEPAGE_FPGA_FILEHEADER, (WORDS_PER_BLOCK * 2), PROT_ON);  // turn on write protection
}


/*
 * read a ISFlash page
 * low level test routine
 */
readISFPage(int startPage)
{
   unsigned long ISF_Cmd;
   unsigned long buffer[128];
   unsigned long pageaddr, byteaddr;
   int i;

   ISF_Cmd = 0L;
   ISF_Cmd = RANDOM_READ | (startPage << 9); 
   pageaddr = ((ISF_Cmd & ISF_PAGE_MASK) >> 9);
   byteaddr = (ISF_Cmd & ISF_BYTE_MASK);
   printf("readISFPage() RandomRead: 0x%lx, Cmd: 0x%lx, PageAddr: %ld, ByteAddr: %ld\n", 
                ISF_Cmd, ISF_Cmd >> 24, pageaddr,byteaddr);
   // writeIcatFifo(&ISF_Cmd,4);
   readIcatFifo(ISF_Cmd, (UINT32*) buffer, BYTES_PER_PAGE);
   for (i=0; i < WORDS_PER_PAGE; i++)
      printf("buf[%d] = 0x%lx\n",i,buffer[i]);
}

isfdir()
{
   int readConfigTableHeader(int index,char *name, char* md5, UINT32 *len);
   char name[36], md5[36];
   unsigned long len;
   int i,result,cnt,totalSize,maxNumFiles;
   totalSize = cnt = 0;
   maxNumFiles = getISFMaxNumFiles();
   printf("Directory of icat ISFlash   [ FFS Version %d, Max. Files: %d ]\n",getISFLayoutVer(),maxNumFiles);
   result = readIcatFPGAHeader(0, name, md5, (UINT32*) &len);
   if (result != -1)
   {
      printf("%20s (pri)\t%10d bytes\tmd5: %s\n", name, len,md5);
      cnt++;
      totalSize = totalSize + len;
   }
   result = readIcatFPGAHeader(1, name, md5, (UINT32*) &len);
   if (result != -1)
   {
      printf("%20s (sec)\t%10d bytes\tmd5: %s\n", name, len,md5);
      cnt++;
      totalSize = totalSize + len;
   }
   for (i=0; i < maxNumFiles; i++)
   {
      result = readConfigTableHeader(i,name, md5, (UINT32*) &len);
      if (result != -1)
      {
        printf("%20s [%2d]\t%10d bytes\tmd5: %s\n", name, i,len,md5);
        cnt++;
        totalSize = totalSize + len;
      }
   }
   printf("\n   %5d file(s)\t\t%10d bytes\n",cnt,totalSize);
}

/*
  Function to find and read into a buffer the ISF file
  Buffer is malloc and returned, the calling must free the buffer
*/
char *readIsfFile(const char* filename, UINT32 *len, UINT32 *offset)
{
   int readConfigTableHeader(int index,char *name, char* md5, UINT32 *len);
   char name[36], md5[36];
   int i,result,maxNumFiles;
   char *buffer;
   UINT32 filelen, dataOffset;
   /* look at the FPGA images */
   for (i=0; i < MAX_NUM_FPGA_FILES; i++)
   {
      result = readIcatFPGAHeader(i, name, md5, (UINT32*) &filelen);
      if (result != -1)
      {
         // printf("%d filename: '%s' vs '%s'\n", i, filename, name);
         if (strcmp(filename,name) == 0)
            break;
      }
   }
   if ((i == 0) || (i == 1))
   {
      buffer = (char*) malloc(filelen);
      readIcatFPGA(i, name, md5, &filelen, buffer);
      *len = filelen;
      *offset = 0;
      return buffer;
   }
   /* move on to the tables */
   maxNumFiles = getISFMaxNumFiles();
   for (i=0; i < maxNumFiles; i++)
   {
      result = readConfigTableHeader(i,name, md5, (UINT32*) &filelen);
      if (result != -1)
      {
         // printf("%d filename: '%s' vs '%s'\n", i, filename, name);
         if (strcmp(filename,name) == 0)
            break;
      }
   }
   if ((i >= 0) && (i < maxNumFiles))
   {
      buffer = (char*) malloc(filelen + sizeof(ISFFILEHEADER));
      readConfigTable(i, name, md5, &filelen, buffer, &dataOffset);
      *len = filelen;
      *offset = sizeof(ISFFILEHEADER);
      return buffer;
   }
   *len = 0;
   *offset = 0;
   return NULL;
}

// used in sysUtils by pcp command
int getConfigTableByName(char *filename)
{
   int readConfigTableHeader(int index,char *name, char* md5, UINT32 *len);
   char name[36], md5[36];
   int i,result,index,maxNumFiles;
   UINT32 filelen;
   index = -1;
   maxNumFiles = getISFMaxNumFiles();
   for (i=0; i < maxNumFiles; i++)
   {
      result = readConfigTableHeader(i,name, md5, (UINT32*) &filelen);
      if (result != -1)
      {
         // printf("%d filename: '%s' vs '%s'\n", i, filename, name);
         // if (strcmp(filename,name) == 0)
         // this routine accepts the wild chars '?' and '*' within the filename string
         if (wc_strncmp(filename,name,1,0,1) == 0)
            break;
      }
   }
   if ((i >= 0) && (i < maxNumFiles))
   {
      index = i;
   }
   return index;
}
      
int writeIsfFile(char *name, char *md5, UINT32 len, char *buffer, int overwrite)
{
   int writeConfigTable(int index, char *name, char *md5, UINT32 len, char *buffer);
   int readConfigTableHeader(int index,char *name, char* md5, UINT32 *len);
   int result,index,i,deleteIndex;
   int strtIndex, endIndex, maxBytes;
   UINT32 filelen;
   char md5sig[36],filename[36];
   int maxNumFiles;

   deleteIndex = -1;
   maxNumFiles = getISFMaxNumFiles();
   result = indexRangeBySize(len, &strtIndex, &endIndex,&maxBytes);
   // printf("writeIsfFile: len: %ld, sindex: %d, eindex: %d, maxBytes: %d, max#files: %d\n",
   IPRINT5(2,"writeIsfFile: len: %ld, sindex: %d, eindex: %d, maxBytes: %d, max#files: %d\n",
           len, strtIndex, endIndex, maxBytes, maxNumFiles);
   if (result == -1)
      return -2;  // file too big 

   index = getConfigTableByName(name);
   // printf("writeIsfFile: getConfigTableByName: '%s', index: %d\n",name,index);
   IPRINT2(1,"writeIsfFile: getConfigTableByName: '%s', index: %d\n",name,index);
   if ( (index != -1) && (overwrite != 1) )
      return -1;  // name already in use

   // test to besure the the new file canbe safely written (i.e. overwrite) the present file
   // based on index / size limits
   if ( (index >= strtIndex) && (index <= endIndex) && (len <= maxBytes) )
   {
      // printf("writeIsfFile: writeConfigTable: '%s', index: %d\n",name,index);
      IPRINT2(1,"writeIsfFile: writeConfigTable: '%s', index: %d\n",name,index);
      writeConfigTable(index, name, md5, len, buffer);
      return 0;
   }
   else
   {
      // delete this file when new file is written, too small to overwrite in place
      deleteIndex = index;
   }
   // find an empty slot
   index = -1;
   for (i=strtIndex; i < maxNumFiles; i++)
   {
      result = readConfigTableHeader(i,filename, md5sig, (UINT32*) &filelen);
      if (result == -1)
      {
         index = i;
         break; 
      }
   }
   IPRINT1(1,"writeIsfFile: Found free index at: %d\n",index);
   if (index == -1)
   {
      return -3; // no empty slots
   }

   writeConfigTable(index, name, md5, len, buffer);

   IPRINT1(1,"writeIsfFile: deleteIndex: %d\n",deleteIndex);
   if (deleteIndex != -1) {
       isftbldel(deleteIndex);
   }

   return 0;
}

isfdel(char *filename)
{
    int index,prevIndex,forceDel;
    char name[36], md5[36];
    UINT32 filelen;

    if (filename == NULL)
      return -1;

    prevIndex = -1;
    while ( (index = getConfigTableByName(filename)) != -1)
    {
      // printf("filename: '%s', prevIndex: %d, index: %d \n", filename,prevIndex,index);
      readConfigTableHeader(index,name, md5, (UINT32*) &filelen);
      if (prevIndex == index) {
         printf("Failed to delete\n");
         printf("To forceable delete this file perform the following:\n");
         printf("  disableIsfProt \n");
         printf("  isfdel \"%s\"\n", name);
         printf("  enableIsfProt\n\n");
         return -1;
      }
      else {
         printf("Deleting: '%s'\n",name);
      }
      isftbldel(index);
      prevIndex = index;
    }

    return 0;
}

writeConfigTable(int index, char *name, char *md5, UINT32 len, char *buffer)
{
   ISFFILEHEADER *pFileheader;
   unsigned long startPage, bufSize;

   // malloc enough space for the header and the data and round out to an even multiple of page size. Init with zeros.
   // char *pbuf = (char*) calloc(((sizeof(ISFFILEHEADER)+len+BYTES_PER_PAGE-1)/BYTES_PER_PAGE),BYTES_PER_PAGE);
   bufSize = ((sizeof(ISFFILEHEADER)+len+BYTES_PER_PAGE-1)/BYTES_PER_PAGE) * BYTES_PER_PAGE;
   // printf("writeConfigTable: len: %ld, bufSize: %ld\n",len,bufSize);
   char *pbuf = (char*) malloc(bufSize);
   memset(pbuf,0xFF,bufSize);  // FF which is non write for FFS
   memset(pbuf,0x00,sizeof(ISFFILEHEADER)); // zero header
   startPage = calcStartPage(index);
   pFileheader = (ISFFILEHEADER*) pbuf;
   strncpy(pFileheader->Name,name,35);
   strncpy(pFileheader->Md5Sig,md5,35);
   pFileheader->LenBytes = len;
   pFileheader->Valid = 1;
   pFileheader->Reserved[0] = 0;
   memcpy(pbuf+sizeof(ISFFILEHEADER),buffer,len);

   IPRINT3(1,"writeConfigTable() name: '%s', md5: '%s', len: %ld\n",
           pFileheader->Name, pFileheader->Md5Sig,pFileheader->LenBytes);
   IPRINT4(1,"writeConfigTable: index: %d, startPage: %d, pbuf: 0x%lx, len: %ld\n",
     index,startPage, pbuf, len + sizeof(ISFFILEHEADER));
   writeISF(startPage, pbuf, len + sizeof(ISFFILEHEADER));
   free(pbuf);
   // cfree(pbuf);
}

/*
 * base function to read ISFlash file header of the ISFlash
 */
readFileHeader(unsigned long startPage,char *name, char* md5, UINT32 *len)
{
   ISFFILEHEADER FileHeader;
   ISFFILEHEADER *pFileHeader;
   int status;
   pFileHeader = &FileHeader;
   // startPage = calcStartPage(index);
   readISF(startPage,(char*) pFileHeader, sizeof(ISFFILEHEADER));
   name[0]=0;
   md5[0]=0;
   *len = 0;
   status = -1;
   if (pFileHeader->Valid == 1)
   {
      strcpy(name,pFileHeader->Name);
      strcpy(md5,pFileHeader->Md5Sig);
      *len = pFileHeader->LenBytes;
      status = 0;
   }
   return status; 
}

readConfigTableHeader(int index,char *name, char* md5, UINT32 *len)
{
   ISFFILEHEADER FileHeader;
   ISFFILEHEADER *pFileHeader;
   unsigned long startPage;
   int status;

   pFileHeader = &FileHeader;
   startPage = calcStartPage(index);
   status = readFileHeader(startPage,name, md5, len);
   return status; 
}

readConfigTable(int index,char *name, char* md5, UINT32 *len, char *buffer, int *dataOffset)
{
   ISFFILEHEADER *pFileheader;
   unsigned long startPage;
   startPage = calcStartPage(index);
   int status;
   IPRINT3(0,"readConfigTable() index: %d, data len: %ld, data+header: %ld\n",
           index, *len, *len + sizeof(ISFFILEHEADER));
   // taskDelay(60);
   readISF(startPage, buffer, *len + sizeof(ISFFILEHEADER));
   pFileheader = (ISFFILEHEADER*) buffer;
   name[0]=0;
   md5[0]=0;
   *len = 0;
   status = -1;
   if (pFileheader->Valid == 1)
   {
      strcpy(name,pFileheader->Name);
      strcpy(md5,pFileheader->Md5Sig);
      *len = pFileheader->LenBytes;
      *dataOffset = sizeof(ISFFILEHEADER);
      status = 0;
   }
   return status; 
}

/*
 * writes file to the iCAT ISF
*/
writeIcatFPGA(int index, char *name, char *md5, UINT32 len, char *buffer)
{
   ISFFILEHEADER Fileheader,*pFileheader;
   unsigned long headerStartPage,FPGAStartPage;
   int status;
   // index = 0, is the primary iCAT FPGA image
   headerStartPage = calcFPGAHeaderPage(index);
   FPGAStartPage = calcFPGAPage(index);
   
   pFileheader = &Fileheader;
   strncpy(pFileheader->Name,name,35);
   strncpy(pFileheader->Md5Sig,md5,35);
   pFileheader->LenBytes = len;
   pFileheader->Valid = 1;
   pFileheader->Reserved[0] = 0;
   
   IPRINT2(1,"writeIcatFPGA() write FPGA file header: name: '%s', data len: %ld\n",
        name, sizeof(ISFFILEHEADER));

   protectISFSectors(FPGAStartPage, len, PROT_OFF);  // turn off write protection

   status = writeISF(headerStartPage, (char*) pFileheader, sizeof(ISFFILEHEADER));

   IPRINT2(1,"writeIcatFPGA() write FPGA: StartPage: %ld, data len: %ld\n",
        FPGAStartPage,len);
   status = writeISF(FPGAStartPage, buffer, len);

   protectISFSectors(FPGAStartPage, len, PROT_ON);  // turn on write protection

   return status;
}

readIcatFPGAHeader(int index, char *name, char* md5, UINT32 *len)
{
   ISFFILEHEADER FileHeader;
   ISFFILEHEADER *pFileHeader;
   unsigned long startPage;
   int status;

   pFileHeader = &FileHeader;
   // index = 0, is the primary iCAT FPGA image
   startPage = calcFPGAHeaderPage(index);
   status = readFileHeader(startPage,name, md5, len);
   return status; 
}

readIcatFPGA(int index, char *name, char* md5, UINT32 *len, char *buffer)
{
   int status;
   ISFFILEHEADER Fileheader,*pFileheader;
   unsigned long headerStartPage,FPGAStartPage,nSize;

   headerStartPage = calcFPGAHeaderPage(index);
   FPGAStartPage = calcFPGAPage(index);
   pFileheader = &Fileheader;
   nSize = *len;

   IPRINT1(1,"readIcatFPGA() read FPGA file header data len: %ld\n",
         sizeof(ISFFILEHEADER));
   readISF(headerStartPage, (char*) pFileheader, sizeof(ISFFILEHEADER));
   name[0]=0;
   md5[0]=0;
   *len = 0;
   status = -1;
   if (pFileheader->Valid != 1)
   {
      errLogRet(LOGIT,debugInfo, "No Valid Table Entry for FPGA\n");
      return(-1);
   }

   strcpy(name,pFileheader->Name);
   strcpy(md5,pFileheader->Md5Sig);
   *len = pFileheader->LenBytes;

   // *dataOffset = sizeof(ISFFILEHEADER);

   IPRINT2(1,"readIcatFPGA() read FPGA file name: '%s', data len: %ld\n",
        name, nSize);
   readISF(FPGAStartPage, buffer, nSize);

   return 0;
}
