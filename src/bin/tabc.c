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
/* static char SCCSid[] = "@(#)tabc.c 9.1 4/16/93  (C)1991 Spectroscopy Imaging Systems"; */

/* TABC.C */

/******************************************************************************
  This program unscrambles 2D data which has been acquired using an
  external AP table.  Both the raw fid file and the external table are
  read in, and then the fid data is written back out in an order defined
  by the table after sorting monotonically from small to large values.

  The "spare" field in the datafilehead structure is used to record the
  rearrangement process.  If this field is zero, no rearrangement has
  been performed.  A 1 in this element indicates that the tabc process
  has already been executed on the data, and should not be repeated.

  The sorting process works on either 2D or 3D data.  The acceptable formats
  for 2D data at this time are:

      1.  Arrayed and non-arrayed data using ni (arraydim >= 1).
      2.  Compressed multi-slice using nf and ni (nf > 1).
      3.  Arrayed compressed multislice using nf, ni and arrayed parameter(s)
	  (nf > 1  &  arraydim > 1).

  3D data is expected to be in the "compressed/standard" format, in which
  there are ni standard planes of data, each consisting of nf compressed fids.

  This routine will not work on data which is not compatible with 2D
  reconstruction in vnmr, i.e., compressed 2D data must first be reformatted 
  with flashc, after which it may be reordered by tabc.

  The one case which is not handled by flashc at this time is the
  "compressed-compressed" case, in which arrayed or kinetic data is obtained
  by multiple repetititions via two nested real-time loops.  tabc therefore
  will not work on this form of data.

  The array dimension is also required, even if it is unity.  This is
  currently obtained as a command line argument, but could be obtained from
  the arraydim parameter in vnmr.  There are no provisions for arrayed 3D.
     
  Usage:  tabc  dataformat  table_name  arraydim  current_experiment_dir

  A.R.Rath   Version  1:  11-22-91
	     Version  2:  1-13-92   Modified to include 3D data
******************************************************************************/

#include <sys/file.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <netinet/in.h>

/* Removed 5/22/97 Matt Howitt, will create parameter in tabc macro 	*/
/* tab_converted.  This is similar to the flash_converted param.	*/ 
/* #define TABC_DONE   8  /*set in main_header.spare1 means already converted*/
#define NUM_ARGS    4   /* # of command-line args not counting command name */
#define NP          ntohl(main_header.np)
#define NBLOCKS     ntohl(main_header.nblocks)
#define NTRACES     ntohl(main_header.ntraces)
#define EBYTES      ntohl(main_header.ebytes)

#include "data.h" 

struct datafilehead main_header;

struct datablockhead *block_header;
typedef  struct datablockhead  DATABLOCKHEAD;

int *table;

/*****************************************************************
*  This routine is used by the qsort function to provide the
*  comparison for sorting.  It compares the two entries in
*  "table" array which have indeces defined by the values of the 
*  elements in the "index" array.
*****************************************************************/
static  int intcompare(i,j)
int *i, *j;
{
    return(table[*i] - table[*j]);
}

int main(int argc, char *argv[])
{
    int     *index, i, j, in_file, out_file, tablesize, malloc_size;
    int     arraydim, dataformat;
    char    table_name[128], input_fid[128], output_fid[128];
    char    orig_fid[128], vnmruser_dir[128], vnmrsystem_dir[128];
    char    curexpdir[128];  /* delete this declaration if built into vnmr */
    char    **data;
    FILE    *table_ptr, *fopen();

    /****
    * Check for NUM_ARGS command-line arguments in addition to command name.
    ****/
    if (argc - 1 != NUM_ARGS) {
	printf("Usage: tabc  dataformat  table_name  arraydim  curexpdir\n");
	exit(1);
    }

    /****
    * dataformat is an integer which describes what type of data is in the
    * fid file.  At present this may be either standard 2D data (dataformat=2),
    * or 3D data consisting of a compressed 2nd dimension and a standard 
    * third dimension (dataformat=3).
    ****/
    dataformat = (int)atof(argv[1]);  

    /****
    * Generate a path to the table in $vnmruser/tablib.
    ****/
    strcpy(vnmruser_dir, getenv("vnmruser"));
    strcpy(table_name, vnmruser_dir);
    strcat(table_name, "/tablib/");
    strcat(table_name, argv[2]);

    /****
    * arraydim is obtained as a command line argument.  If tabc is built
    * in to vnmr, then this value can be obtained as it is in flash.c
    ****/
    arraydim = (int)atof(argv[3]);  

    /****
    * This next statement, which gets the current experiment directory from
    * the command line, can go away if tabc is built into vnmr, which already
    * has direct access to this name.
    ****/
    strcpy(curexpdir, argv[4]);

    /****
    * Generate paths to the fid file and a new, temporary file.
    ****/
    strcpy(input_fid, curexpdir);
    strcpy(output_fid, curexpdir);
    strcat(input_fid, "/acqfil/fid");
    strcat(output_fid, "/acqfil/tabc.temp");

    /****
    * Open table file:
    ****/
    if ((table_ptr = fopen(table_name, "r")) == NULL) {
	
    	/****
   	 * Generate a path to the table in $vnmrsystem/tablib.
    	****/
    	strcpy(vnmrsystem_dir, getenv("vnmrsystem"));
    	strcpy(table_name, vnmrsystem_dir);
    	strcat(table_name, "/tablib/");
    	strcat(table_name, argv[2]);
        if ((table_ptr = fopen(table_name, "r")) == NULL) {
	   perror("Open");
           printf("tabc: can't open table file in user or system dir.\n");
           exit(1);
	} 
    }

    /****
    * Open input fid file:
    ****/
    if ((in_file = open(input_fid, O_RDONLY)) < 0) {
        perror("Open");
        printf("tabc: can't open input file.\n");
        exit(1);   
    }

    /****
    * Read main_header.
    ****/
    malloc_size = sizeof(main_header);
    if (read(in_file, &main_header, malloc_size) != malloc_size) {
        perror ("read");
        exit(1);   
    }

    /****
    * Info about the data in main_header:  EBYTES is 2 for single
    * precision, and 4 for double precision.  NBLOCKS is the total
    * number of data lines, which are separated by block headers,
    * and will typically be ni*arraydim.  NTRACES describes the number
    * of "fids" in each block, which will be greater than 1 for
    * acquisitions such as multislice and multiecho.
    *
    * For compressed 2D data (dataformat = 1) the NTRACES describes 
    * the number of compressed fids in the compressed dimension, which 
    * is the one which will be reordered.
    *
    * For 3D data (dataformat = 3) the NTRACES describes the number of
    * compressed fids in the compressed dimension, which is the one
    * which will be reordered.  NBLOCKS is the number of increments
    * in the third dimension, controlled by ni, and is not reordered
    * by this program.
    ****/
    if (dataformat == 2)
	tablesize = (int)NBLOCKS/arraydim;
    else if (dataformat == 3)
	tablesize = (int)NTRACES/arraydim;
    else if (dataformat == 1)		/* compressed 2d */
	tablesize = (int)NTRACES;
    else {
        printf("tabc:  Unrecognized dataformat.\n");
        exit(1);   
    }

    /****
    * Read in the table.  A separate index array is constructed at this
    * time.  This index array is sorted by qsort, using as criteria the
    * values pointed to in the table array by the entries in index.
    ****/
    table = (int *)malloc(tablesize*sizeof(int));
    index = (int *)malloc(tablesize*sizeof(int));
    while (fgetc(table_ptr) != '=') ;
    for (i=0; i<tablesize; i++) {
	if (fscanf(table_ptr, "%d", &table[i]) != 1) {
            perror("fscanf");
	    printf("tabc: table length inconsistent with data.\n");
	    exit(1);   
	}
	index[i] = i;
    }
    if (fclose(table_ptr)) {
        perror("fclose");
	printf("tabc: trouble closing table file.\n");
	exit(1);   
    }
    qsort(index, tablesize, sizeof(int), intcompare);

    /****
    * At this point the index array has been rearranged so that its
    * entries, in ascending order, hold the index values required to 
    * unscramble the table array, and therefore the data array.
    *
    * Example:           table =  2   0  -1   1  -2
    * index starts as:   index =  0   1   2   3   4
    * After sorting,     index =  4   2   1   3   0
    *
    * Now, we can write out the 4th element of data, followed by the
    * 2nd, 1st, 3rd, and 0th, resulting in data which appears as if it 
    * had been acquired in the order: -2  -1  0  1  2.
    ****/

    /****
    * Allocate space for block_headers.
    ****/
    if ((block_header =
      (DATABLOCKHEAD *)malloc(NBLOCKS*sizeof(DATABLOCKHEAD))) == NULL) {
        perror("malloc");
        printf("tabc: unable to malloc space for data block headers.\n");
        exit(1);   
    }

    /****
    * Open output fid file.
    ****/
    out_file = open(output_fid, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (out_file < 0) {
        perror("open");
        printf("tabc: unable to open output file.\n");
        exit(1);   
    }

    /****
    * Write out the main header.
    ****/
    if (write(out_file, &main_header, sizeof(main_header)) < 0) {
        perror("write");
        printf("tabc: unable to write main header.\n");
        exit(1);   
    }

    switch (dataformat) {
      case 2:   /* Standard 2D data, with headers separating each trace */
      /****
      * Allocate space for pointers to input data traces.
      ****/
      if ((data = (char **) malloc(NBLOCKS*sizeof(char*))) == NULL) {
          perror("malloc");
          printf("tabc: unable to malloc space for input data.\n");
          exit(1);   
      }

      /****
      * Read in data and block_headers.  
      ****/
      malloc_size = NP*EBYTES*NTRACES*sizeof(char);
      for (i=0 ; i<NBLOCKS; i++) {
          if (read(in_file, &block_header[i], sizeof(DATABLOCKHEAD)) < 0) {
              perror ("read");
              printf("tabc: unable to read data block headers.\n");
              exit(1);   
          }
	  /****
	  * Allocate space for each line of the data, and read it in.
	  ****/
          if ((data[i]=(char *)malloc(malloc_size)) == NULL) {
              perror("malloc");
              printf("tabc: unable to malloc space for input data.\n");
              exit(1);   
	  }
          if (read(in_file, data[i], malloc_size) != malloc_size) {
              perror ("read");
              printf("tabc: unable to read data.\n");
              exit(1);   
	  }
      }

      /****
      * Write out the data in correct monotonic order, interleaved with
      * the original block headers.  The second half of the data index
      * expression selects the appopriate array "block", and the first half
      * selects the appropriate array element within that block.
      ****/
      for (i=0; i<NBLOCKS; i++) {
          if (write(out_file, &block_header[i], sizeof(DATABLOCKHEAD)) < 0) {
              perror("write");
              printf("tabc: unable to write block header.\n");
              exit(1);   
	  }
          if (write(out_file, data[(i%arraydim+arraydim*index[i/arraydim])],
	    NP*EBYTES*NTRACES*sizeof(char)) < 0) {
              perror("write");
              printf("tabc: unable to write data.\n");
              exit(1);   
	  }
      }
      break;

      case 3:  /* 3D data, one dimension compressed, one standard */
        /****
        * Allocate space for pointers to input data traces, and then
	* for each line of the data in one compressed "plane".
        ****/
        if ((data = (char **) malloc(NTRACES*sizeof(char *))) == NULL) {
            perror("malloc");
            printf("tabc: unable to malloc space for input data.\n");
            exit(1);   
        }  
        malloc_size = NP*EBYTES*sizeof(char);
        for (j=0; j<NTRACES; j++) {
            if ((data[j] = (char *) malloc(malloc_size)) == NULL) {
                perror("malloc");
                printf("tabc: unable to malloc space for input data.\n");
                exit(1);   
            }
        }   
      
        /****
        * Read in block header and data for each compressed block in
	* the data, then write out the header and the reordered data.
        ****/
        for (i = 0; i < NBLOCKS; i++) {
            if (read(in_file, &block_header[i], sizeof(DATABLOCKHEAD)) < 0) {
                perror("read");
                printf("tabc: unable to read data block headers.\n");
                exit(1);   
            }
            if (write(out_file, &block_header[i], sizeof(DATABLOCKHEAD)) < 0) {
                perror("write");
                printf("tabc: unable to write block header.\n");
                exit(1);   
            }
            for (j = 0; j < NTRACES; j++) {
                if (read(in_file, data[j], malloc_size) != malloc_size) {
                    perror("read");
                    printf("tabc: unable to read data.\n");
                    exit(1);   
                }
            }
            for (j = 0; j < NTRACES; j++) {
                if (write(out_file,
		  data[(j % arraydim + arraydim*index[j/arraydim])],
                  NP * EBYTES * sizeof(char)) < 0) {
                    perror("write");
                    printf("tabc: unable to write data.\n");
                    exit(1);   
                }
            }
        }
	break;

      case 1:  /* 2D data, one dimension compressed, one standard */
        /****
        * Allocate space for pointers to input data traces, and then
	* for each line of the data in one compressed "plane".
        ****/
        if ((data = (char **) malloc(NTRACES*sizeof(char *))) == NULL) {
            perror("malloc");
            printf("tabc: unable to malloc space for input data.\n");
            exit(1);   
        }  
        malloc_size = NP*EBYTES*sizeof(char);
        for (j=0; j<NTRACES; j++) {
            if ((data[j] = (char *) malloc(malloc_size)) == NULL) {
                perror("malloc");
                printf("tabc: unable to malloc space for input data.\n");
                exit(1);   
            }
        }   
      
        /****
        * Read in block header and data for each compressed block in
	* the data, then write out the header and the reordered data.
        ****/
        for (i = 0; i < NBLOCKS; i++) {
            if (read(in_file, &block_header[i], sizeof(DATABLOCKHEAD)) < 0) {
                perror("read");
                printf("tabc: unable to read data block headers.\n");
                exit(1);   
            }
            if (write(out_file, &block_header[i], sizeof(DATABLOCKHEAD)) < 0) {
                perror("write");
                printf("tabc: unable to write block header.\n");
                exit(1);   
            }
            for (j = 0; j < NTRACES; j++) {
                if (read(in_file, data[j], malloc_size) != malloc_size) {
                    perror("read");
                    printf("tabc: unable to read data.\n");
                    exit(1);   
                }
            }
            for (j = 0; j < NTRACES; j++) {
                if (write(out_file,data[index[j]],
                  NP * EBYTES * sizeof(char)) < 0) {
                    perror("write");
                    printf("tabc: unable to write data.\n");
                    exit(1);   
                }
            }
        }
	break;
    }

    /****
    * Finished, so close everything up cleanly.
    ****/
    if (close(in_file)) {
        perror("close");
        printf("tabc: problems closing input file.\n");
        exit(1);   
    }

    if (close(out_file)) {
        perror("close");
        printf("tabc: problems closing output file.\n");
        exit(1);   
    }

    /****
    * Move the original fid file and replace it with the temporary file.
    * In this early version, the original fid file is still available
    * as the file "fid.orig" in the acqfil directory.  If the result of
    * unscrambling is unsatisfactory, you can retrieve the original data
    * manually by moving it back to the name "fid".
    *
    * When tabc has been excercised more thoroughly, and it is no longer
    * necessary to retain the original data for safety purposes, then
    * "rename" may be changed to "unlink" to delete it entirely.
    ****/
    strcpy(orig_fid, curexpdir);
    strcat(orig_fid, "/acqfil/fid.orig");
    if (rename(input_fid, orig_fid) != 0) {
        perror("rename");
        printf("tabc: unable to rename original fid file.\n");
        exit(1);   
    }
    if (rename(output_fid, input_fid) != 0) {
        perror("rename");
        printf("tabc: unable to rename new fid file.\n");
        exit(1);   
    }

    exit(0);
}

