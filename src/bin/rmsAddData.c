/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 * proprietary and confidential information of Varian, Inc. and its
 * contributors.  Use, disclosure and reproduction is prohibited
 * without prior consent.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <sys/param.h>

/*
 * File headers from data.h:
 */
/*  The file headers are defined as follows:                     */

/*****************/
struct datafilehead
/*****************/
/* Used at the beginning of each data file (fid's, spectra, 2D)  */
{
   long    nblocks;       /* number of blocks in file			*/
   long    ntraces;       /* number of traces per block			*/
   long    np;            /* number of elements per trace		*/
   long    ebytes;        /* number of bytes per element		*/
   long    tbytes;        /* number of bytes per trace			*/
   long    bbytes;        /* number of bytes per block			*/
   short   vers_id;       /* software version and file_id status bits	*/
   short   status;        /* status of whole file			*/
   long	   nbheaders;	  /* number of block headers			*/
};

/*******************/
struct datablockhead
/*******************/
/* Each file block contains the following header        */
{
   short 	  scale;	/* scaling factor                   */
   short 	  status;	/* status of data in block          */
   short 	  index;	/* block index                      */
   short	  mode;		/* mode of data in block	    */
   long		  ctcount;	/* ct value for FID		    */
   float 	  lpval;	/* F2 left phase in phasefile       */
   float 	  rpval;	/* F2 right phase in phasefile      */
   float 	  lvl;		/* F2 level drift correction        */
   float 	  tlt;		/* F2 tilt drift correction         */
};

static void
usage(char *progname)
{
    fprintf(stderr,"Usage: %s dir infile wt [infile wt] ... outfile", progname);
}

int
main(int argc, char **argv)
{
    int i, j, k;
    int nb;
    int ne;
    int nfiles;
    int sumfile;
    int *infile;
    int fhdrsize;
    long ntraces;
    long np;
    long ebytes;
    long bbytes;
    long nblocks;
    char fpath[MAXPATHLEN];
    char *dir;
    char *blkbuf;
    char *sumbuf;
    float *indata;
    float *sumdata;
    float *wt;
    float sumwt;
    struct datafilehead *inhdr;

    if (argc < 5 || (argc % 2) != 1) {
	usage(argv[0]);
	return -1;
    }
    fhdrsize = sizeof(struct datafilehead);

    /*
     * Open files
     */
    nfiles = (argc - 3) / 2;
    infile = (int *)malloc(sizeof(int) * nfiles);
    wt = (float *)malloc(sizeof(float) * nfiles);
    if (!infile || !wt) {
	fprintf(stderr,"%s: malloc() failed\n", argv[0]);
	return -1;
    }
    dir = argv[1];
    for (i=0, j=2; i<nfiles; i++, j+=2) {
	sprintf(fpath,"%s/%s", dir, argv[j]);
	infile[i] = open(fpath, O_RDONLY);
	if (infile[i] == -1) {
	    fprintf(stderr,"Source data file \"%s\" not found.\n", fpath);
	    return -1;
	}
	if (sscanf(argv[j+1], "%f", &wt[i]) != 1) {
	    fprintf(stderr,"Bad weight for %d'th input file: %s\n",
		    i+1, argv[j+1]);
	    return -1;
	}
    }

    sprintf(fpath,"%s/%s", dir, argv[j]);
    sumfile = open(fpath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (sumfile == -1) {
	fprintf(stderr,"Sum file \"%s\" cannot be created.\n", *argv[j]);
	return -1;
    }

    /*
     * Check headers for consistency
     */
    inhdr = (struct datafilehead *)malloc(sizeof(struct datafilehead) * nfiles);
    if (!inhdr) {
	fprintf(stderr,"%s: malloc() failed\n", argv[0]);
	return -1;
    }
    for (i=0; i<nfiles; i++) {
	if (read(infile[i], inhdr+i, fhdrsize) != fhdrsize) {
	    fprintf(stderr,"Error getting file header for data file %s.\n",
		    argv[i+1]);
	    return -1;
	}
    }

    ntraces = inhdr[0].ntraces;
    for (i=1; i<nfiles; i++) {
	if (ntraces != inhdr[i].ntraces) {
	    fprintf(stderr,"ntraces mismatch: data 1: %d vs. data %d: %d\n",
		    ntraces, i+1, inhdr[i].ntraces);
	    return -1;
	}
    }
    np = inhdr[0].np;
    for (i=1; i<nfiles; i++) {
	if (np != inhdr[i].np) {
	    fprintf(stderr,"np mismatch: data 1: %d vs. data %d: %d\n",
		    np, i+1, inhdr[i].np);
	    return -1;
	}
    }
    ebytes = inhdr[0].ebytes;
    for (i=1; i<nfiles; i++) {
	if (ebytes != inhdr[i].ebytes) {
	    fprintf(stderr,"ebytes mismatch: data 1: %d vs. data %d: %d\n",
		    ebytes, i+1, inhdr[i].ebytes);
	    return -1;
	}
    }
    bbytes = inhdr[0].bbytes;
    for (i=1; i<nfiles; i++) {
	if (bbytes != inhdr[i].bbytes) {
	    fprintf(stderr,"bbytes mismatch: data 1: %d vs. data %d: %d\n",
		    bbytes, i+1, inhdr[i].bbytes);
	    return -1;
	}
    }
    nblocks = inhdr[0].nblocks;
    for (i=1; i<nfiles; i++) {
	if (nblocks != inhdr[i].nblocks) {
	    fprintf(stderr,"nblocks mismatch: data 1: %d vs. data %d: %d\n",
		    nblocks, i+1, inhdr[i].nblocks);
	    return -1;
	}
    }

    blkbuf = (char *)malloc(bbytes);
    sumbuf = (char *)malloc(bbytes);
    if (blkbuf == NULL || sumbuf == NULL) {
	fprintf(stderr,"Malloc failed\n");
	return -1;
    }

    indata = (float *)(blkbuf + sizeof(struct datablockhead));
    sumdata = (float *)(sumbuf + sizeof(struct datablockhead));

    write(sumfile, &inhdr[0], sizeof(inhdr[0])); /* Write file header */

    /* Prepare weighting parameters */
    sumwt = 0;
    for (j=0; j<nfiles; j++) {
	wt[j] *= wt[j];		/* Square the weighting factors */
	sumwt += wt[j];
    }
    sumwt /= nfiles;
    if (sumwt == 0) sumwt = 1;

    /*
     * Crunch the data
     */
    ne = np * ntraces;
    for (i=0; i<nblocks; i++) {
	for (j=0; j<nfiles; j++) {
	    nb = read(infile[j], blkbuf, bbytes);
	    if (nb == -1) {
		perror("Read error getting data block");
		return -1;
	    } else if (nb != bbytes) {
		fprintf(stderr,"Read of data block failed: got %d bytes\n", nb);
		return -1;
	    }
	    if (j == 0) {
		for (k=0; k<ne; k++) {
		    sumdata[k] = indata[k] * indata[k] * wt[j];
		}
	    } else {
		for (k=0; k<ne; k++) {
		    sumdata[k] += indata[k] * indata[k] * wt[j];
		}
	    }
	}
	memcpy(sumbuf, blkbuf, sizeof(struct datablockhead));
	for (k=0; k<ne; k++) {
	    sumdata[k] = sqrt(sumdata[k] / sumwt);
	}
	write(sumfile, sumbuf, bbytes);
    }

    return 0;
}
