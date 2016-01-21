/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

static char *Sid(){
    return "@(#)sisfile.c 18.1 03/21/08 (c)1991-93 SISCO";
}

/************************************************************************
*									*
*  Charly Gatot								*
*  Spectroscopy Imaging Systems Corporation				*
*  Fremont, CA	94538							* 
*									*
*************************************************************************
*									*
*  Description								*
*  -----------								*
*									*
*  Routine related to sisfile header and data.				*
*									*
*************************************************************************/
#include <stdio.h>
// #include <stream.h>
#ifdef LINUX
// #include <strstream>
#else
// #include <strstream.h>
#endif
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "params.h"
#include "graphics.h"
#include "gframe.h"
#include "gtools.h"
#include "imginfo.h"
#include "stderr.h"
#include "ddllib.h"
#include "ddlfile.h"
#include "msgprt.h"

#ifndef __OSFCN_H
#ifndef __SYSENT_H
/* This is needed if osfcn.h or sysent.h is not included anywhere */
//extern "C" int ftruncate(int, int);
#endif
#endif

static int file_header_info(File_header *, Sisheader *, char *);

/************************************************************************
*									*
*  Convert Vnmr phasefile to sisfile.  Note that Vnmr phasefile must be	*
*  saved as trace='f1' and it Vnmr phasefile should NOT be multi-slices.*
*  Return SUCCESS or ERROR.						*
*									*/
Sisfile_info *
Sisfile_info::sisdata_phase2sis(char *inname,  float rwd, float rht, 
							char *errmsg)
{
   int infilesize;      /* size of input file */
   int outfilesize;     /* size of output file */
   int fd;              /* file descriptor */
   char *inptr;         /* inpu (mmap) pointer */
   char *outptr;        /* output (mmap) pointer */
   Sisfile_info  *sisfile;   /* Sisfile_info pointer */
   int nblock;          /* nth block */
   File_header *fheader; /* Vnmr phasefile file header */
   Block_header *bheader;/* Vnmr phasefile block header */
   int trace;           /* loop counter for rows of data */
   int ndata;           /* number of data per trace */
   char *pin;           /* pointer to input data */
   char *pout;          /* pointer to output data */
   struct stat fbuf;	/* structire of stat buffer */
    
   /* Open input file and mmap for reading */
   if ((fd = open(inname,O_RDONLY|O_NDELAY, 0644)) == -1)
   {
      if (errmsg)
         sprintf(errmsg,"ssidata_phase2sis:Couldn't open %s for reading",
		 inname);
      return(NULL); 
   }

   /* Gte the file size */
   if (fstat(fd, &fbuf) == -1)
   {
      (void)close(fd);
      if (errmsg)
         sprintf(errmsg,"ssidata_phase2sis:Couldn't stat %s", inname);
      return(NULL);
   }
   else
      infilesize = (int)fbuf.st_size;
    
   if ((inptr = (char *)mmap((caddr_t)0, infilesize, PROT_READ,
      MAP_SHARED, fd, (off_t)0)) == (char *) -1)
   {
      (void)close(fd);
      if (errmsg)
         sprintf(errmsg,
		 "sisdata_phase2sis:Couldn't mmap %s for reading",
		 inname);
      return(NULL);
   }
   (void)close(fd);

   if (infilesize <= sizeof(File_header) )
   {
      if (errmsg)
         sprintf(errmsg,
		"sisdata_phase2sis: Error, no data in file.");
      munmap(inptr, infilesize);
      return(NULL);
   }

    

   sisfile = new Sisfile_info;

   /* This function will set some of sisfile item values */
   if (file_header_info(fheader=(File_header *)inptr, &sisfile->header, errmsg)
      == ERROR)
   {
      munmap(inptr, infilesize);
      return(NULL);
   }
    
   sisfile->header.magic_num = SIS_MAGIC;
   sisfile->header.ratio_fast = rwd;
   sisfile->header.ratio_medium = rht;
   sisfile->header.ratio_slow = 1.0;            /* NOT used */
   sisfile->header.ratio_hyperslow = 1.0;       /* NOT used */
    
    
   /* Create data storage memory */
   outfilesize = (int)(fheader->nblocks *
		       (fheader->bbytes - sizeof(Block_header)));

   if ((outptr = (char *)malloc(outfilesize)) == (char *)0 )
   {
      if (errmsg)
         sprintf(errmsg,
		"sisdata_phase2sis:Couldn't malloc data buffer");
      munmap(inptr, infilesize);
      return(NULL);
   }
   (void)close(fd);

   /* set data pointer */
   sisfile->data=outptr;
   sisfile->datasize=outfilesize;
    
    
   bheader = (Block_header *)((char *)fheader + sizeof(File_header));
    
   ndata = (int)fheader->np * sizeof(float);
 
   /* Note that Vnmr phasefile data starts from the last trace to the */
   /* first trace.  Hence we have to reverse the data for new format  */
   pout = outptr + outfilesize - ndata;
   for (nblock=0; nblock < fheader->nblocks;
        bheader = (Block_header *)((char *)bheader + fheader->bbytes))
   {
      if (bheader->status)      // If this block contains data
      {
         for (trace=0, pin = (char *)bheader + sizeof(Block_header);
              trace < fheader->ntraces;
              trace++, pin += ndata)
         {
            memcpy(pout, pin, ndata);
            pout -= ndata;
         }
         nblock++;
      }
      else
      {
      	if (errmsg)
            sprintf(errmsg,
		"sisdata_phase2sis:Read Error. Wrong type of data?");
      	munmap(inptr, infilesize);
      	return(NULL);
      }
   }
   munmap(inptr, infilesize);
   return(sisfile);
} 

/************************************************************************
*                                                                       *
*  Get the information of the file information from phasefile and put   *
*  it in Sisheader.                                                     *
*  Return SUCCESS or ERROR.						*
*                                                                       */
static int
file_header_info(File_header *fheader, Sisheader *sisfile, char *errmsg)
{
    if (!(fheader->status & S_DATA) && errmsg){
	msgerr_print("Warning: file_header_info(): No data in file.");
    }
    if (!(fheader->status & S_FLOAT) && errmsg){
	msgerr_print("Warning: file_header_info(): Data type not FLOAT.");
    }
    
    /* Set the default value for some items in sisfile */
    sisfile->bit = BIT_32;
    sisfile->type = TYPE_FLOAT;
    sisfile->slow = 1;                /* NOT used */
    sisfile->hyperslow = 1;           /* NOT used */

    // Varian release 4.1 uses incompatible file header status word.
    // Need to check version to get the correct rank.
    // With Rel 4.1, version # is encoded in old "transf" header word.
    int rank_bit;
    if (fheader->transf == 0 || fheader->transf == 1){
	rank_bit = S_SECND;	// SISCO format
    }else{
	rank_bit = 0x100;	// New Varian format
    }
    if (fheader->status & rank_bit){
	sisfile->rank = RANK_2D;
	sisfile->fast = (int)fheader->np;
	sisfile->medium = (int)(fheader->nblocks * fheader->ntraces);
    }else{
	sisfile->rank = RANK_1D;
	sisfile->fast = (int)fheader->np;
	sisfile->medium = 1;
    }
    
    return(SUCCESS);
}

/************************************************************************
*                                                                       *
*	Converts from number of bits to the bit code.			*
*	Returns a "Sisfile_bit" code. On error returns ERROR, which	*
*	should not be a valid Sisfile_bit.				*
*									*/
Sisfile_bit
nbits_to_bitcode(int bits)
{
    Sisfile_bit rtn;
    
    switch (bits){
      case 1:
	rtn = BIT_1;
	break;

      case 8:
	rtn = BIT_8;
	break;

      case 12:
	rtn = BIT_12;
	break;

      case 16:
	rtn = BIT_16;
	break;

      case 32:
	rtn = BIT_32;
	break;

      case 64:
	rtn = BIT_64;
	break;

      default:
	rtn = (Sisfile_bit)ERROR;
	break;
    }
    return rtn;
}

/************************************************************************
*									*
*  Unmap the file and destroy Sisfile_info
*									*/

Sisfile_info::~Sisfile_info()
{
   if (data)
      free((char *) data);

   if (dirpath) free(dirpath);
   if (filename) free (filename);

}

 /*
  *  This Sisfile_info constructor accepts as its argument another Sisfile_Info
  *  and returns a carbon copy of it
  *
  */

Sisfile_info::Sisfile_info(Sisfile_info* original)
{

   /* Allocate memory for data + header */
   if (original == NULL) {
     return;
   };
   
   data = (char *) malloc(original->datasize);

   /* Header file */
     header.magic_num = original->header.magic_num;
     header.rank = original->header.rank;
     header.bit = original->header.bit;
     header.type = original->header.type;
     header.fast = original->header.fast;
     header.medium = original->header.medium;
     header.slow = original->header.slow;
     header.hyperslow = original->header.hyperslow;
     header.ratio_fast = original->header.ratio_fast;
     header.ratio_medium = original->header.ratio_medium;
     header.ratio_slow = original->header.ratio_slow;
     header.ratio_hyperslow = original->header.ratio_hyperslow;


   SetDirpath(original->GetDirpath());
   SetFilename(original->GetFilename());
   data = original->data;
   dsrc = original->dsrc;
   return;
}

