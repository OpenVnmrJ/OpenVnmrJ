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

 gsvtobin - Generate binary (float) fid data

Details: 
	Input varian data file; Output binary (float) data file.
	Data arranged in re-imag pairs

Usage:  gsvtobin input_varian_float_file binary_file <dc> 
        (note: gsbin expects .fid/fid file; assume FT'ed varian data, i.e. float)    

Author: S. Sukumar 

************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define NUM_ARGS	3   /* number of input arguments */
#define NP              main_header1.np
#define NBLOCKS         main_header1.nblocks
#define NTRACES         main_header1.ntraces
#define EBYTES          main_header1.ebytes

struct datafilehead
{
	long nblocks;		/* no of blocks in file */
	long ntraces;		/* no of traces per block */
	long np;		/* no elements re+im per trace */
	long ebytes;		/* no of byets per element */
	long tbytes;		/* no of bytes per trace */
	long bbytes;		/* no of bytes per block */
	short transf;		/* transposed storage flag */
	short status;		/* status of whole file */
	long spare1;		/* spare */
} main_header1, main_header2;

struct datablockhead
{
	short scale;		/* scaling factor */
	short status; 		/* status of data in block */
	short index;		/* block index */
	short spare3;		/* spare */
	long ctcount;		/* completed transients */
	float lpval;		/* left phase in phasefile */
	float rpval;		/* right phase in phasefile */
	float lvl;		/* level drift correction */
	float tlt;		/* tilt drift correction */
} *block_header1, *block_header2;

typedef struct datablockhead DATABLOCKHEAD;

int main(argc, argv)
int	argc;
char	*argv[];
{
	char	dir1[128],outdir[128];
	char	file1[128],outfid[128];
	char	**dataout,**data1;
	int	infile1,outfile,malloc_size;
	int	i,j,k,l,m;
	float   *d1;
	float   *f1;
	char	*fbuf;
	int	dcflag;
	int	t1sz,t2sz;
	
	/* Check arguments */
	if ((argc-1 < NUM_ARGS-1)||(argc-1 > NUM_ARGS))
	{
		printf("Usage: stripv input_directory output_filename <dc>");
		exit(1);
	}

	strcpy(dir1, argv[1]);      /* input file */
	strcpy(outdir, argv[2]);    /* output filename */

	strcpy(file1, dir1);
	strcpy(outfid, outdir);      /* output filename for binaries */

	/* open input files */
	if ((infile1 = open(file1, O_RDONLY)) < 0)
	{
		printf("stripv: cannot open input file #1\n");
		exit(1);
	}
	if ((outfile = open(outfid, O_CREAT|O_TRUNC|O_WRONLY, 0644)) < 0)
	{
		printf("stripv: cannot open output file \n");
		exit(1);
	}

	/* read main header */
	malloc_size = sizeof(main_header1);
	if (read(infile1, &main_header1, malloc_size) != malloc_size)
	{
		printf("stripv: cannot read header #1");
		exit(1);
	}
	/* allocate space for block header */
	if ((block_header1 = (DATABLOCKHEAD *)malloc(NBLOCKS*sizeof(DATABLOCKHEAD)))
						== NULL)
	{
		printf("stripv: unable to allocate space for datablock headers\n");
		exit(1);
	}
	/* allocate space for input data traces */
	if (( data1 = (char **) malloc(NBLOCKS*sizeof(char*))) == NULL)
	{
		printf("stripv: unable to allocate space for input data");
		exit(1);
	}

	/* Read in data and block_headers */
	malloc_size = NP*EBYTES*NTRACES*sizeof(char);
	for (i=0; i<NBLOCKS; i++)
	{
		if (read(infile1, &block_header1[i], sizeof(DATABLOCKHEAD)) < 0)
		{
			printf("stripv: unable to read block header, file #1\n");
			exit(1);
		}
		/* allocate space for each line of the data and read it in */
		if ((data1[i] = (char *)malloc(malloc_size)) == NULL)
		{
			printf("stripv: unable to allocate space for input data file #1");
			exit(1);
		}	
		if (read(infile1, data1[i], malloc_size) != malloc_size)
		{
			printf("stripv: unable to read input data file #1\n");
			exit(1);
		}

	}

		/**
		printf("np=%d, ntraces=%d, nblocks=%d,\n",NP,NTRACES,NBLOCKS);
		exit(1);
		**/
                /* allocate space for each block of the data  */
                malloc_size = NP*NTRACES*EBYTES*sizeof(char);
                if ((fbuf = (char *)malloc(malloc_size)) == NULL)
                {
                        printf("pcepi: unable to allocate space for input data file #1");
                        exit(1);
                }               			
	/* Read data and save in dataout buffer */
	/** Assumes NBLOCKS = 1 **/
	 
	for (i=0; i<NBLOCKS; i++)
	{	
		/* float, dc correct, and write the data */
		d1 = (float *)data1[i];  
		f1 = (float *)fbuf;
		k=0;
		t2sz=NTRACES;
		t1sz=NP/2;
		/* after ft2d, np refers to pe dimension */
		/*  ntraces refers to ro dim. */
		for (m=0; m<NP/2; m++)  
		{
		  for (j=0; j<NTRACES; j++)    
		  {	    
			f1[k] = d1[k];
			f1[k+1] = d1[k+1];
			k=k+2;	
		  }
		
		}
		/* write out the data only from input buffer */
		if (write(outfile, fbuf, NP*EBYTES*NTRACES,sizeof(char)) <0)
		{
			printf("stripv: unable to write data \n");
			exit(1);
		}		
	}
	/* clean up */
	if( close(infile1) )
	{
		printf("stripv: problems closing input file #1\n");
		exit(1);
	}
	if( close(outfile) )
	{
		printf("stripv: problems closing output file\n");
		exit(1);
	}
}	

	
/***************************************************************************
                   MODIFICATION HISTORY



****************************************************************************/
