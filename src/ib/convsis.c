/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

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
*  This file contains codes to convert Vnmr phasefile into a more simple*
*  file format as described in "sisfile.h".  				*
*									*
*************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <strings.h>
#include <memory.h>
#include "sisfile.h"

extern "C"
{
   int ftruncate(int, int);
};

#ifndef OK
#define	OK	0
#define	NOT_OK	-1
#endif

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

static void usage_info(char **);
static int check_sisfile(char *);
static void convert_phase2sis(char *, char *, float, float);
static int file_header_info(File_header *, Sisheader *);
static int get_filesize(int);
static char *rankstr(Sisfile_rank);
static char *bitstr(Sisfile_bit);
static char *typestr(Sisfile_type);

/************************************************************************
*									*
*  MAIN PROGRAM.							*
*									*/
main(int argc, char **argv)
{
   char infile[128];	/* input filename */
   char outfile[128];	/* output filename */
   float ratio_wd=1.0;	/* ratio for image width */
   float ratio_ht=1.0;	/* ratio for image height */

   Sid = Sid;

   if (argc == 2)
   {
      /* Check if input file is sisfile */
      if (check_sisfile(argv[1]) == OK)
	 exit(0);

      (void)strcpy(infile, argv[1]);
      (void)sprintf(outfile, "%s.sis", infile);
   }
   else if ((argc == 3) || (argc == 5))
   {
      (void)strcpy(infile, argv[1]);
      (void)strcpy(outfile, argv[2]);

      if (argc == 5)
      {
	 if (sscanf(argv[3], "%f", &ratio_wd) != 1)
	    usage_info(argv);

	 if (sscanf(argv[4], "%f", &ratio_ht) != 1)
	    usage_info(argv);

	 /* Make sure one of the proprotional ratio is 1.0 */
	 if (ratio_wd > ratio_ht)
	 {
	    ratio_wd /= ratio_ht;
	    ratio_ht = 1.0;
	 }
	 else
	 {
	    ratio_ht /= ratio_wd;
	    ratio_wd = 1.0;
	 }
      }
   }
   else
      usage_info(argv);

   fprintf(stderr," Converting %s to %s begins ......", infile, outfile);
   convert_phase2sis(infile, outfile, ratio_wd, ratio_ht);
   fprintf(stderr," Done.\n");

   exit(0);
}

/************************************************************************
*									*
*  Usage information.							*
*									*/
static void
usage_info(char **argv)
{
  fprintf(stderr, "\n Convert Vnmr (absolute value) phasefile data to sisfile data\n");
  fprintf(stderr, " Usage: \n");
  fprintf(stderr, "    %s <phasefile> <sisfile> [<ratio_wd>] [<ratio_ht>]\n", argv[0]);
  fprintf(stderr, "\tinput:<phasefile>, output:<sisfile>\n");
  fprintf(stderr, "\t<ratio_wd>,<ratio_ht>:ratio of image data size\n\n");
  fprintf(stderr, "    %s <phasefile> <sisfile>\n", argv[0]);
  fprintf(stderr, "\tinput:<phasefile>, output:<sisfile>\n\n");
  fprintf(stderr, "    %s <phasefile>\n", argv[0]);
  fprintf(stderr, "\tinput:<phasefile>, output:<phasefile>.sis\n\n");
  fprintf(stderr, "    %s <sisfile>\n", argv[0]);
  fprintf(stderr, "\tprint information about sisfile\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "   If you specify ratio_wd=6.0 and ratio_ht=8.0, it means\n");
  fprintf(stderr, "   that the ratio of your image is 8.0 X 6.0. The default\n");
  fprintf(stderr, "   ratio is 1.0 X 1.0 (if you don't specify any).  These\n");
  fprintf(stderr, "   ratio values have something to do with Vnmr parameters\n");
  fprintf(stderr, "   lpe and lro, where ratio_wd=lro and ratio_ht=lpe)\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "   NOTE that Vnmr phasefile must be saved when trace='f1'\n");
  fprintf(stderr, "   for this conversion to work correctly\n\n");
  exit(0);
}

/************************************************************************
*									*
*  Check and print information if it is a Sisheader.  			*
*  Return OK or NOT_OK.							*
*									*/
static int
check_sisfile(char *filename)
{
   Sisheader sisfile;
   int fd;	/* file descriptor */

   if ((fd = open(filename,O_RDONLY|O_NDELAY, 0644)) == -1)
   {
      fprintf(stderr,"Couldn't open %s for reading\n", filename);
      exit(1);
   }

   if (read(fd, (char *)&sisfile, sizeof(Sisheader)) != sizeof(Sisheader))
   {
      fprintf(stderr,"Couln'd read %s\n", filename);
      exit(1);
   }
   (void)close(fd);
   
   if (sisfile.magic_num == (long)SIS_MAGIC)
   {
      fprintf(stdout, " %s:\n", filename);
      fprintf(stdout, "   rank:\t    %s\n",rankstr(sisfile.rank));
      fprintf(stdout, "   bit:\t\t    %s\n",bitstr(sisfile.bit));
      fprintf(stdout, "   type:\t    %s\n",typestr(sisfile.type));
      fprintf(stdout, "   fast:\t    %d\n", sisfile.fast);
      fprintf(stdout, "   medium:\t    %d\n", sisfile.medium);
      fprintf(stdout, "   slow:\t    %d\n", sisfile.slow);
      fprintf(stdout, "   hyperslow:\t    %d\n", sisfile.hyperslow);
      fprintf(stdout, "   ratio_fast:\t    %f\n", sisfile.ratio_fast);
      fprintf(stdout, "   ratio_medium:    %f\n", sisfile.ratio_medium);
      fprintf(stdout, "   ratio_slow:\t    %f\n", sisfile.ratio_slow);
      fprintf(stdout, "   ratio_hyperslow: %f\n", sisfile.ratio_hyperslow);
      return(OK);
   }
   else
      return(NOT_OK);	/* Not a sisfile */
}

/************************************************************************
*									*
*  Convert phasefile to sisfile.					*
*									*/
static void
convert_phase2sis(char *inname, char *outname, float rwd, float rht)
{
   int infilesize;	/* size of input file */
   int outfilesize;	/* size of output file */
   int fd;		/* file descriptor */
   char *inptr;		/* inpu (mmap) pointer */
   char *outptr;	/* output (mmap) pointer */
   Sisheader sisfile;	/* sifile header */
   int nblock;		/* nth block */
   File_header *fheader; /* Vnmr phasefile file header */
   Block_header *bheader;/* Vnmr phasefile block header */
   int trace;		/* loop counter for rows of data */
   int ndata;		/* number of data per trace */
   char *pin;		/* pointer to input data */
   char *pout;		/* pointer to output data */

   /* Open input file and mmap for reading */
   if ((fd = open(inname,O_RDONLY|O_NDELAY, 0644)) == -1)
   {
      fprintf(stderr,"Couldn't open %s for reading\n", inname);
      exit(1);
   }
   infilesize = get_filesize(fd);

   if ((inptr = (char *)mmap((caddr_t)0, infilesize, PROT_READ,
      MAP_SHARED, fd, (off_t)0)) == (char *) -1)
   {
      fprintf(stderr,"Couldn't mmap %s for reading\n", inname);
      exit(1);
   }
   (void)close(fd);

   /* This function will set some of sisfile item values */
   if (file_header_info(fheader=(File_header *)inptr, &sisfile) == NOT_OK)
      exit(1);

   sisfile.magic_num = SIS_MAGIC;
   sisfile.ratio_fast = rwd;
   sisfile.ratio_medium = rht;
   sisfile.ratio_slow = 1.0;		/* NOT used */
   sisfile.ratio_hyperslow = 1.0;	/* NOT used */

   /* Open outfile for writing */
   if ((fd = open(outname,O_RDWR|O_CREAT, 0644)) == -1)
   {
      fprintf(stderr,"Couldn't open %s for writing\n", outname);
      exit(1);
   }

   /* Create data storage memory */
   outfilesize = (int)(sizeof(Sisheader) + 
      fheader->nblocks * (fheader->bbytes - sizeof(Block_header)));

   if (ftruncate(fd, outfilesize) == -1)
   {
      fprintf(stderr,"Couldn't create data storage\n");
      exit(1);
   }

   if ((outptr = (char *)mmap((caddr_t)0, outfilesize, PROT_WRITE,
      MAP_SHARED, fd, (off_t)0)) == (char *) -1)
   {
      fprintf(stderr,"Couldn't mmap %s for writing\n", outname);
      perror("mmap");
      exit(1);
   }
   (void)close(fd);

   /* Write data header file */
   memcpy(outptr, (char *)&sisfile, sizeof(Sisheader));

   bheader = (Block_header *)((char *)fheader + sizeof(File_header));

   ndata = (int)fheader->np * sizeof(float);

   // Note that Vnmr phasefile data starts from the last trace to the first
   // trace.  Hence we have to reverse the data for new format
   pout = outptr + outfilesize - ndata;
   for (nblock=0; nblock < fheader->nblocks; 
        bheader = (Block_header *)((char *)bheader + fheader->bbytes))
   {
      if (bheader->status)	// If this block contains data
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
   }
   munmap(inptr, infilesize);
   munmap(outptr, outfilesize);
}

/************************************************************************
*									*
*  Get the information of the file information from phasefile and put	*
*  it in Sisheader.							*
*  Return OK or NOT_OK.							*
*									*/
static int
file_header_info(File_header *fheader, Sisheader *sisfile)
{
   if ((fheader->status & S_DATA) &&
       (fheader->status & S_SPEC) &&
       (fheader->status & S_FLOAT) &&
       (fheader->status & S_ABSVAL) &&
       !(fheader->status & S_COMPLEX) )
   {
      /* Set the default value for some items in sisfile */
      sisfile->bit = BIT_32;
      sisfile->type = TYPE_FLOAT;
      sisfile->slow = 1;		/* NOT used */
      sisfile->hyperslow = 1;		/* NOT used */

      if (fheader->status & S_SECND)
      {
	 sisfile->rank = RANK_2D;
	 sisfile->fast = (int)fheader->np;
	 sisfile->medium = (int)(fheader->nblocks * fheader->ntraces);
      }
      else
      {
	 sisfile->rank = RANK_1D;
	 sisfile->fast = (int)fheader->np;
	 sisfile->medium = 1;
      }
   }
   else
   {
      fprintf(stderr," This is not Vnmr (absolute value) phasefile\n");
      return(NOT_OK);
   }
   return(OK);
}

/************************************************************************
*									*
*  Return the size of a file.						*
*									*/
static int
get_filesize(int fd)
{
   struct stat buf ;            /* structure of stat */
   if (fstat(fd, &buf) == -1)
   {
      fprintf(stderr,"Cannot stat for file descriptor %d\n", fd);
      exit(1);
   }
   return(buf.st_size);
}

/************************************************************************
*									*
*  Return an apropriate string.						*
*									*/
static char *
rankstr(Sisfile_rank rank)
{
   switch (rank)
   {
      case RANK_1D: return("RANK_1D");
      case RANK_2D: return("RANK_2D");
      case RANK_3D: return("RANK_3D");
      case RANK_4D: return("RANK_4D");
   }
}

static char *
bitstr(Sisfile_bit bit)
{
   switch (bit)
   {
      case BIT_1: return("BIT_1");
      case BIT_8: return("BIT_8");
      case BIT_12: return("BIT_12");
      case BIT_16: return("BIT_16");
      case BIT_32: return("BIT_32");
      case BIT_64: return("BIT_64");
   }
}

static char *
typestr(Sisfile_type type)
{
   switch (type)
   {
      case TYPE_CHAR: return("TYPE_CHAR");
      case TYPE_SHORT: return("TYEP_SHORT");
      case TYPE_INT: return("TYPE_INT");
      case TYPE_FLOAT: return("TYPE_FLOAT");
      case TYPE_DOUBLE: return("TYPE_DOUBLE");
   }
}
