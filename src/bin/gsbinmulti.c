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
/***********************************************************************

 gsbinmulti - Generate binary (float) fid data, multi block output

Details: 
	Input varian fid file directory; Output binary (float) data files.
	Data arranged in re-imag pairs
    If no of blocks specified multiple files with .block_no extension will
    be generated. A check is done to verify no_of_blocks match with BLOCKS
    // If no_of_blocks not specified, a single file is output 
    i.e. similar to gsbin.c - Not implemented
    To compile: cc -o gsbinmulti -o gsbinmult.c -m32 -DLINUX
Usage:  gsbinmulti ~/vnmrsys/data/test rawfile no_of_blocks     assume test.fid file
    

Author: S. Sukumar 

Version: 20060811

************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "data.h"

#define NUM_ARGS	3   /* number of input arguments */
#define NP              main_header1.np
#define NBLOCKS         main_header1.nblocks
#define NTRACES         main_header1.ntraces
#define EBYTES          main_header1.ebytes

struct datafilehead main_header1, main_header2;
struct datablockhead  *block_header1, *block_header2;

typedef struct datablockhead DATABLOCKHEAD;

main(argc, argv)
int	argc;
char	*argv[];
{
	char	dir1[128],outdir[128];
	char	fid1[128],outfid[128];
	char	**dataout,**data1;
	int	infile1,outfile,malloc_size;
	int	i,j,k;
	float   *f1,*fd1,re,im;
	char	*fbuf;
	int	blockflag,fp_data,bits;
    int blocks;
    char num[80];
	long	*ld1;
	unsigned char b[4],e[4];
	unsigned char *c;
	float   *f,*f2;
	long    *l,*l2;
	
	/* Check arguments */
	if ((argc-1 < NUM_ARGS-1)||(argc-1 > NUM_ARGS))
	{
		printf("Usage: gsbinmulti input_directory output_filename no_of_blocks");
		exit(1);
	}

	strcpy(dir1, argv[1]);      /* input dir, _____.fid */
	strcpy(outdir, argv[2]);    /* output filename */

	strcpy(fid1, dir1);
	strcpy(outfid, outdir);      /* output filename for binaries */
	strcat(fid1, ".fid/fid");        /* raw vnmr, .fid, data */

	/* open input files */
	if ((infile1 = open(fid1, O_RDONLY)) < 0)
	{
		printf("gsbinmulti: cannot open input file #1\n");
		exit(1);
	}
	blockflag = 0;	/* multiple blocks flag */
	if (argc-1 == NUM_ARGS)
	{
        blocks=atoi(argv[3]);
		blockflag = 1;    
	}
    else
    {
	  if ((outfile = open(outfid, O_CREAT|O_TRUNC|O_WRONLY, 0644)) < 0)
	  {
		printf("gsbinmulti: cannot open output file \n");
		exit(1);
	  }
    }
	/* read main header */
	malloc_size = sizeof(main_header1);
	if (read(infile1, &main_header1, malloc_size) != malloc_size)
	{
		printf("gsbinmulti: cannot read header #1");
		exit(1);
	}
	DATAFILEHEADER_CONVERT_HTON(&main_header1);  /* byte swap if LINUX */
	/* allocate space for block header */
	if ((block_header1 = (DATABLOCKHEAD *)malloc(NBLOCKS*sizeof(DATABLOCKHEAD)))
						== NULL)
	{
		printf("gsbinmulti: unable to allocate space for datablock headers\n");
		exit(1);
	}
	/* allocate space for input data traces */
	if (( data1 = (char **) malloc(NBLOCKS*sizeof(char*))) == NULL)
	{
		printf("gsbinmulti: unable to allocate space for input data");
		exit(1);
	}
	/* check data type via status; inova=32b (0)integer; vnmrs=32b (1)float */
	fp_data = (main_header1.status & 0x8);   /* 1=float, 0=int */
	bits = (main_header1.status & 0x4);     /* if integer: 0=16bit, 1 32bit */
    /* check no of blocks */
    if (blocks != NBLOCKS) 
    {
      printf("%d blocks specified. Blocks in fid file = %d",blocks,NBLOCKS);
      exit(1);
    }

    /* allocate space for each block of the data  */
    malloc_size = NP*NTRACES*EBYTES*sizeof(char);
    if ((fbuf = (char *)malloc(malloc_size)) == NULL)
    {
      printf("gsbinmulti: unable to allocate space for input data file #1");
      exit(1);
    }

	/* Read in data and block_headers */
	for (i=0; i<NBLOCKS; i++)
	{
		if (read(infile1, &block_header1[i], sizeof(DATABLOCKHEAD)) < 0)
		{
			printf("gsbinmulti: unable to read block header, file #1\n");
			exit(1);
		}

		DATABLOCKHEADER_CONVERT_HTON(&block_header1[i]); /* byte swap if LINUX */
		/* allocate space for each line of the data and read it in */
		if ((data1[i] = (char *)malloc(malloc_size)) == NULL)
		{
			printf("gsbinmulti: unable to allocate space for input data file #1");
			exit(1);
		}	
		if (read(infile1, data1[i], malloc_size) != malloc_size)
		{
			printf("gsbinmulti: unable to read input data file #1\n");
			exit(1);
		}
	}
		/**
		printf("np=%d, ntraces=%d, nblocks=%d,\n",NP,NTRACES,NBLOCKS);
		exit(1); **/ 
          /* Read data and save in dataout buffer */ 
        for (i=0; i<NBLOCKS; i++) 
        {	
                /* float, dc correct, and write the data */ 
		f1 = (float *)fbuf;
		c = (unsigned char *)data1[i];
                if(fp_data) 
		  fd1 = (float *)data1[i];
		else
		  if(bits)
		   ld1 = (long *)data1[i];
		  else
		  {
		    printf("16bit data (dp='n') not supported");
		    exit(1);
		  }
			
		for (j=0,k=0; j<NP*NTRACES; j=j+2)
		{
#ifdef LINUX 
			/* byte swap if linux */
			b[3] = c[k++];
			b[2] = c[k++];
			b[1] = c[k++];
			b[0] = c[k++];

			e[3] = c[k++];
			e[2] = c[k++];
			e[1] = c[k++];
			e[0] = c[k++];

			if(fp_data)
			{
  				f = (float *)b;
  				f2 = (float *)e;
				re = *f;
				im = *f2;
			}
			else
			{
  				l = (long *)b;
  				l2 = (long *)e;
  				re = *l;
  				im = *l2;
			}
#else

		
			if(fp_data)
			{
			  re = fd1[j];
			  im = fd1[j+1];
			}
			else
			{
			  re = ld1[j];
			  im = ld1[j+1];
			}	  
#endif 
			f1[j] = re;
			f1[j+1] = im;			
		}
		/* write out the data only from input buffer */
        if(blockflag == 0)
        {
		  if (write(outfile, fbuf, NP*EBYTES*NTRACES*sizeof(char)) <0)
		  {
			printf("gsbinmulti: unable to write data \n");
			exit(1);
		  }
        }
        else
        {    
          sprintf(num,"%c%d",'.',i);
          strcpy(outfid, outdir);
          strcat(outfid, num);
	      if ((outfile = open(outfid, O_CREAT|O_TRUNC|O_WRONLY, 0644)) < 0)
	      {
	        printf("gsbinmulti: cannot open output file \n");
		    exit(1);
	      } 
		  if (write(outfile, fbuf, NP*EBYTES*NTRACES*sizeof(char)) <0)
		  {
			printf("gsbinmulti: unable to write data \n");
			exit(1);
		  }
	      if( close(outfile) )
	      {
	        printf("gsbinmulti: problems closing output file\n");
	        exit(1);
	      }
        }          		
	}
	/* clean up */
	if( close(infile1) )
	{
		printf("gsbinmulti: problems closing input file #1\n");
		exit(1);
	}
    if(blockflag == 0)
    {
	  if( close(outfile) )
      {
		printf("gsbinmulti: problems closing output file\n");
		exit(1);
	  }
    }
}	

	
/***************************************************************************
                   MODIFICATION HISTORY

06Oct2005(ss) - Check status bit for float or integer data type
09Dec2005     - code for LINUX (byteswap) added.

****************************************************************************/
