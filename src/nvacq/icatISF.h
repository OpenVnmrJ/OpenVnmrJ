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

#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */

#ifndef  icatIsfSpih
#define  icatIsfSpih

// Spartan-3AN FPGA 700AN
// The flash memory is arranged in pages (264 bytes) 
//    blocks = 8 pages, 
//    sector = 256 pages or 32 blocks
// The FPGA bit stream files have two fixed loactions, 
//   dependent on the version of the sparten FPGA
//
// 700AN
// 1st bitstream  sector 0, page 0 (0x00_0000) through sector 5, page 1,293 (0x0A_1A00)
// 2nd bitstream  sector 6, page 1,536  (0x0C_0000) through sector 11, page 2,829 (0x16_1A00)
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
// User data space also has fixed starting locations,  page 1,294 - 1,535 & page 2,830-3,071
// sector aligned starts, sector 12 or pages 3,072 - 4,095
//
// design parameters:   
// 1. configuration tables files  always start at a new block, this allows block erase
//    without worry of deleting another file accedentely 
//

/*   a directory just seem to much for this app, so we just going to include a file header 
 *   describing the file, since configuration files will be fixed length any way.
 */
/*
* typedef struct
* {
*    UINT32 PageAddr;
*    UINT32 LenBytes;
*    char Name[36];
*    char Md5Sig[36];
*    UINT32 Next;
*    UINT32 Reserved[1];
* }  ISFFILEENTRY;
*/

/*
 * the configuration table file-header
 */
typedef struct
{
   UINT32 Valid;
   UINT32 LenBytes;
   char Name[36];
   char Md5Sig[36];
   UINT32 Reserved[1];
}  ISFFILEHEADER;


//
// iCAT SPI ISF command layout
//

// enable to protect sectors of ISFlash
#define SECTOR_PROTECTION_ENABLE  0x3D2A7FA9
#define SECTOR_PROTECTION_DISABLE 0x3D2A7F9A  

//
// to selectively protect sectors
// bytes of SECTOR_PROTECTION_REGISTER values to protect   (0x00 - unprotected)
// sector 0     - 0xF0  (byte 0)
// sector 1-15  - 0xFF  (byte 1 - 15)
#define SECTOR_PROTECTION_READ    0x32000000
#define ISF_PROTECT_REG_SIZE_BYTES (16)
#define SECTOR_PROTECTION_ERASE   0x3D2A7FCF
#define SECTOR_PROTECTION_PROG    0x3D2A7FFC
//
// byte1 command, bytes2-4, 24-bit address, byte5 - dummy, Byte 6 - Data byte0, etc...
//
#define FAST_READ   0xB0000000

// byte1 command, bytes2-4, 24-bit address, Byte 5 - Data byte0, etc...
#define RANDOM_READ 0x03000000

// erase a sector of Flash Memeory
#define SECTOR_ERASE  0x7C000000

// erase a block of Flash Memeory
#define BLOCK_ERASE   0x50000000

// erase a page of Flash Memeory
#define PAGE_ERASE    0x81000000

// copy data to a page buffer(1 or 2)
#define BUFFER1_WRITE  0x84000000
#define BUFFER2_WRITE  0x87000000

// read data from a page buffer
#define BUFFER1_READ   0xD1000000
#define BUFFER2_READ   0xD3000000

// verify page buffer contents with a page of ISFlash
#define BUFFER1_VERIFY 0x60000000
#define BUFFER2_VERIFY 0x61000000

// write buffer to page on Flash, without erasing the page on flash
#define BUFFER1_PAGEwoE 0x88000000
#define BUFFER2_PAGEwoE 0x89000000

// read a page on Flash into a buffer
#define BUFFER1_PAGErd  0x53000000
#define BUFFER2_PAGErd  0x55000000

// 24-bit addressing MASKs
#define ISF_ADDRESS_MASK   0x00FFFFFF
#define ISF_SECTOR_MASK    0x001E0000
#define ISF_BLOCK_MASK     0x001FF000
#define ISF_PAGENBYTE_MASK 0x001FFFFF
#define ISF_PAGE_MASK      0x001FFE00
#define ISF_BYTE_MASK      0x000001FF

// 700AN ISF memory allocation
#define BYTES_PER_PAGE  (264)
#define PAGES_PER_BLOCK (8)
#define BLOCKS_PER_SECTOR (32)
#define PAGES_PER_SECTOR (BLOCKS_PER_SECTOR*PAGES_PER_BLOCK)
#define WORDS_PER_PAGE  (BYTES_PER_PAGE/sizeof(UINT32))
#define BYTES_PER_BLOCK (PAGES_PER_BLOCK*BYTES_PER_PAGE)
#define WORDS_PER_BLOCK (BYTES_PER_BLOCK/sizeof(UINT32))
#define BYTES_PER_SECTOR (BLOCKS_PER_SECTOR*PAGES_PER_BLOCK*BYTES_PER_PAGE)
#define PAGE_ADDRESS_SIZE (512)
#define BLOCK_ADDRESS_SIZE (PAGE_ADDRESS_SIZE*PAGES_PER_BLOCK)
#define SECTOR_ADDRESS_SIZE (BLOCK_ADDRESS_SIZE*BLOCK_PER_SECTOR)

// ISF Memory Page, etc.. Address for App
//
// the location for the second FPGA bit-stream
// The only one that is field updated, 
// the FPGA bitstream one at sector 0 is a fail-safe version
// Update 4/20 due to hardware issues, decided to field update 
// the primary FPGA image, not secondary FPGA image.
// If we get this wrong, then the iCAT board will have to be sent back!
//
#define FPGA_SIZE_IN_SECTORS (16)

#define BASESECTOR_FPGA_BITSTREAM (6)
#define BASEBLOCK_FPGA_BITSTREAM (BASESECTOR_FPGA_BITSTREAM*BLOCKS_PER_SECTOR)
#define BASEPAGE_FPGA_BITSTREAM (BASEBLOCK_FPGA_BITSTREAM*PAGES_PER_BLOCK) // Bitstream page address
#define BASEBLOCK_FPGA_FILEHEADER (BASEBLOCK_FPGA_BITSTREAM-1) // one block before bitstream
#define BASEPAGE_FPGA_FILEHEADER (BASEBLOCK_FPGA_FILEHEADER*PAGES_PER_BLOCK) // Header page address

#define BASESECTOR_PRIMFPGA_BITSTREAM (0)
#define BASEBLOCK_PRIMFPGA_BITSTREAM (BASESECTOR_PRIMFPGA_BITSTREAM*BLOCKS_PER_SECTOR)
#define BASEPAGE_PRIMFPGA_BITSTREAM (BASEBLOCK_PRIMFPGA_BITSTREAM*PAGES_PER_BLOCK) // Bitstream page address
#define BASEBLOCK_PRIMFPGA_FILEHEADER (BASEBLOCK_FPGA_FILEHEADER-1) //  One block before secondary header
#define BASEPAGE_PRIMFPGA_FILEHEADER (BASEBLOCK_PRIMFPGA_FILEHEADER*PAGES_PER_BLOCK) //  Bitstream header in pages

#define BASEUSER_DATA_SECTOR 12
#define BASEUSER_DATA_PAGEADDR  (BASEUSER_DATA_SECTOR*PAGES_PER_SECTOR)

#define BASEUSER_DATA_SIZE_IN_BYTES ((FPGA_SIZE_IN_SECTORS-BASEUSER_DATA_SECTOR)*BYTES_PER_SECTOR)

#define FPGABITSTREAM_SIZE_IN_BLOCKS (162)   // (161.75 blocks) index starts at zero
#define FPGABITSTREAM_SIZE_IN_PAGES (1294)   // index starts at zero
#define FPGABITSTREAM_SIZE_IN_BYTES (FPGABITSTREAM_SIZE_IN_PAGES*BYTES_PER_PAGE)

#define FILE_SIZE_1_STRT_INDEX (0)   // index starts at zero
#define FILE_SIZE_1_END_INDEX (47)   
#define FILE_SIZE_1_IN_BLOCKS (1)   
#define FILE_SIZE_1_IN_PAGES (FILE_SIZE_1_IN_BLOCKS * PAGES_PER_BLOCK) 

#define FILE_SIZE_2_STRT_INDEX (48)   // index starts at 48 for 4 blk files
#define FILE_SIZE_2_END_INDEX (55)   // index ends at 55 
#define FILE_SIZE_2_IN_BLOCKS (4)
#define FILE_SIZE_2_IN_PAGES (FILE_SIZE_2_IN_BLOCKS * PAGES_PER_BLOCK) 

#define FILE_SIZE_3_STRT_INDEX (56)   // index starts at 56 for 24 blk files
#define FILE_SIZE_3_END_INDEX (57)   // index dns at 57
#define FILE_SIZE_3_IN_BLOCKS (24)
#define FILE_SIZE_3_IN_PAGES (FILE_SIZE_3_IN_BLOCKS * PAGES_PER_BLOCK) 

#define CONFIG_TABLE_SIZE_IN_BLOCKS (4)   // index starts at zero
#define CONFIG_TABLE_SIZE_IN_PAGES  (CONFIG_TABLE_SIZE_IN_BLOCKS*PAGES_PER_BLOCK)  // blocks 

#define MAX_NUM_FPGA_FILES (2)
#define MAX_NUM_TBL_FILES (6)

#define MAX_NUM_FILE_INDICES (58)

int getISFLayoutVer();
int getISFMaxNumFiles();
int getISFFileHeaderSize();
int getISFFileMaxSize();
int getConfigTableByName(char *filename);
int writeConfigTable(int index, char *name, char *md5, UINT32 len, char *buffer);
int readConfigTable(int index,char *name, char* md5, UINT32 *len, char *buffer, int *dataOffset);
int readConfigTableHeader(int index,char *name, char* md5, UINT32 *len);
int writeIcatFPGA(int index, char *name, char *md5, UINT32 len, char *buffer);
int readIcatFPGA(int index, char *name, char* md5, UINT32 *len, char *buffer);
int readIcatFPGAHeader(int index, char *name, char* md5, UINT32 *len);
int writeIsfFile(char *name, char *md5, UINT32 len, char *buffer, int overwrite);
char *readIsfFile(const char* filename, UINT32 *len, UINT32 *offset);

#endif /* icatIsfSpih */
