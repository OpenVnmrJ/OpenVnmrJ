/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef _SISFILE_H
#define	_SISFILE_H
/************************************************************************
*									
*
*************************************************************************
*									
*  Charly Gatot
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*									
*************************************************************************

 Data structure of (PROTOTYPE) SISFILE format. 		
 ----------------------------------------------
 A 'sisfile' format contains a header (Sisheader) followed by data.  The
 Sisheader contains all necessary information to describe the format, rank,
 type, and number sets of data.

 For rank = RANK_1D, the 'fast' variable will be used as the size of	
 spectrum, and the 'medium' variable will be used to indicate number 
 of slices.  For rank = RANK_2D, 'fast' and 'medium' indicate the size
 of image, and 'slow' indicates number of slices,  and so on for
 rank = RANK_3D.

 Each data will rounded to its nearest natural bit width. For example, 
 12 bit data will be stored as 16 bits per data.  No packing/compression
 method is necessary.  The exception is BIT_1 which will be packed into
 type of 'char'.

 Each dimensional data has its own ratio.  For rank = RANK_ID, ratio doesn't
 mean anything.  Example of ratio usage with rank = RANK_2D (ration_slow
 and ratio_hyperslow are not used):

 a.	fast = 1024
	medium = 256
	ratio_fast = 1.0
	ratio_medium = 1.0
   
   indicates that the proportional image size is 1.0 x 1.0 or square image,
   and 

 b.	fast = 256
	medium = 256
	ratio_fast = 4.0
	ratio_medium = 1.0

   indicates that the proportional image size is 1.0 X 4.0, which means 
   that the medium dimension of data should be multiple by 4.0 times.

  Since ratio indicates a propotional image size, example (a) can also
  be 256 X 256 or (1024 X 1024), and example (b) can be 256 X 1024 
  or 64 X 256 (medium X fast).

  In relation with Vnmr parameters lpe and lro, ratio_fast is equivalent
  with lpe and ratio_medium is equivalent with lro.

*************************************************************************/

#include <malloc.h>

/* Rank of data */
typedef enum
{
   RANK_1D,	/* Not supported yet */
   RANK_2D,
   RANK_3D,	/* Not supported yet */
   RANK_4D	/* Not supported yet */
}Sisfile_rank;

/* Number of bits per data */
typedef enum
{
   BIT_1,	/* Not supported yet */
   BIT_8,	/* Not supported yet */
   BIT_12,
   BIT_16,	/* Not supported yet */
   BIT_32,
   BIT_64	/* Not supported yet */
} Sisfile_bit;

/* Type of data */
typedef enum
{
   TYPE_CHAR,	/* Not supported yet */
   TYPE_SHORT,
   TYPE_INT,	/* Not supported yet */
   TYPE_FLOAT,
   TYPE_DOUBLE,	/* Not supported yet */
   TYPE_ANY	/* Any of the above is OK */
} Sisfile_type;

  
#define	SIS_MAGIC	0xabcdef01
typedef struct _sisheader
{
  long magic_num;	/* magic number to indicate SIS data file */
  Sisfile_rank rank;	/* rank of data */
  Sisfile_bit bit;	/* number of data bits */
  Sisfile_type type;	/* type of data */
  int fast;		/* # of data points in fast dimension */
  int medium;		/* # of data points in medium dimension */
  int slow;		/* # of data points in slow dimension */
  int hyperslow;	/* # of data points in the last dimension */
  float ratio_fast;	/* ratio at fast dimension */
  float ratio_medium;	/* ratio at medium dimension */
  float ratio_slow;	/* ratio at slow dimension */
  float ratio_hyperslow;	/* ratio at hyperslow dimension */
} Sisheader;

/* Definition of source data allocated from */
typedef enum
{
  DATA_UNDEFINED,       /* Data has not been read in yet */
  DATA_MMAP,		/* Data is not read-in but it is mmap-ed */
  DATA_MALLOC		/* Data is read-in and stored in memory (malloc) */
} Datasrc;

/* Details Sisfile information */
class  Sisfile_info
{
   char *dirpath;	/* directory path */
   char *filename;	/* filename of this file */
 public:
   Sisheader header;	/* header of sisifile */
   char *data;		/* a pointer to the data */
   int datasize;	/* data size */
   Datasrc dsrc;	/* indicate where data coming from */
   
   char *GetFilename() {
     return filename;
   };
   
   char *GetDirpath() {
     return dirpath;
   };
   
   void SetFilename(char *f) {
     if (filename) free(filename);
     if (f) 
       filename = strdup(f);
     else
       filename = strdup("");
   };
   
   void SetDirpath(char *f) {
     if (dirpath) free(dirpath);
     if (f)
       dirpath = strdup(f);
     else
       dirpath = strdup("");
   };

   int DataLength() {
     return datasize ;
   }
   Sisfile_info() {
     dirpath = filename = 0;
     datasize = 0;
     data = (char *) 0;
     dsrc = DATA_UNDEFINED;
   };

   Sisfile_info(Sisfile_info* original);
   
   ~Sisfile_info();
   

   /************************************************************************
    *  
    *  Convert Vnmr phasefile to sisfile.  Note that Vnmr phasefile must be *
    *  saved as trace='f1' and Vnmr phasefile should NOT be multi-slices.   *
    *  It will put an error message into 'errmsg' if error occurs and if  
    *  'errmsg' is not equal to NULL.                                     
    *
    *  Return a pointer to Sisfile_info or NULL.                          
    *
    */
   static
   Sisfile_info*  sisdata_phase2sis(char *inname,    /* input filename */
					float rwd,   /* field of view (lpe) */
					float rht,   /* field of view (lro) */
					 char *errmsg);// error message buffer
   

};



/************************************************************************
*                                                                       *
*	Converts from number of bits to the bit code.			*
*	Returns a "Sisfile_bit" code. On error returns ERROR, which	*
*	should not be a valid Sisfile_bit.				*
*									*/
extern Sisfile_bit
nbits_to_bitcode(int bits);



/*========================================================================
 Data structure for Vnmr data header.  This is only used in a function
 to convert phasefile into sisfile format. (sisfile.c in
 function sisdata_phase2sis)
 Our new PROTOTYPE sisfile format doesn't use the following at all.
======================================================================== */
/* Used at the beginning of each Vnmr data file (fid's, spectra, 2D) */
typedef struct datafilehead
{
  long  nblocks;       /* number of blocks in file     */
  long  ntraces;       /* number of traces per block   */
  long  np;            /* number of elements per trace */
  long  ebytes;        /* number of bytes per element  */
  long  tbytes;        /* number of bytes per trace    */
  long  bbytes;        /* number of bytes per block (header +data) */
  short transf;        /* transposed storage flag      */
  short status;        /* status of whole file         */
  long  spare1;        /* reserved for future use      */
} File_header;
 
/* Each Vnmr file block contains the following header */
typedef struct datablockhead
{
  short scale;         /* scaling factor               */
  short status;        /* status of data in block      */
  short index;         /* block index                  */
  short spare3;        /* reserved for future use      */
  long  ctcount;       /* completed transients in fids */
  float lpval;         /* left phase in phasefile      */
  float rpval;         /* right phase in phasefile     */
  float lvl;           /* level drift correction       */
  float tlt;           /* tilt drift correction        */
} Block_header;
 
/* file status codes bits */
#define S_DATA      1  /* 0 = no data     1 = data there       */
#define S_SPEC      2  /* 0 = fid         1 = spectrum         */
#define S_32        4  /* 0 = 16 bit      1 = 32 bit           */
#define S_FLOAT     8  /* 0 = integer     1 = floating point   */
#define S_SECND    16  /* 0 = first ft    1 = second ft        */
#define S_ABSVAL   32  /* 0 = not absval  1 = absolute value   */
#define S_COMPLEX  64  /* 0 = not complex 1 = complex          */
#define S_ACQPAR  128  /* 0 = not acqpar  1 = acq. params      */
#define S_CM     2048  /* 1 = Chem. Magnetics data             */
#define S_GE     4096  /* 1 = GE data                          */
#define S_JEOL   8192  /* 1 = JEOL data                        */
#define S_BRU   16384  /* 1 = Bruker data                      */

#endif	_SISFILE_H
