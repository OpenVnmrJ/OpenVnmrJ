/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "data.h"		/* Vnmr file format */
#include "fileio.h"		/* 3D file format */

extern datafileheader *readDATAheader();

typedef struct{
    float *data;		/* Pointer to the trace data */
    int datatype;		/* REAL=1, COMPLEX=2, HYPERCOMPLEX=4 */
    int npts;			/* Number of points of type "datatype" */
    char direction[3];		/* "f1", "f2", or "f3" */
    int fastindex;		/* Trace location in f3 or f1 dimension */
    int slowindex;		/* Trace location in f1 or f2 dimension */
} Trace;

/*
 * Free memory belonging to a Trace structure.
 */
void
destructTrace(Trace **trace)
{
    if (*trace){
	if ((*trace)->data){
	    free((*trace)->data);
	}
	free(*trace);
	*trace = NULL;
    }
}

/*
 * Allocate memory for a Trace structure.
 */
Trace *
constructTrace(int npts, int datatype)
{
    Trace *trace;
    int i;

    trace = (Trace *)malloc(sizeof(Trace));
    if (!trace){
	return NULL;
    }
    trace->data = (float *)malloc(npts * datatype * sizeof(float));
    if (!trace->data){
	destructTrace(&trace);
	return NULL;
    }

    trace->datatype = datatype;
    trace->npts = npts;
    return trace;
}

Trace *
extract3Dtrace(char *datadir3d,	/* E.g.: ".../exp1/datadir3d" */
	       char *tracedir,	/* Direction of trace: "f1", "f2", or "f3" */
	       int fastindx,	/* Trace number for f3 or f1 */
	       int slowindx)	/* Trace number for f1 or f2 */
{
    char filepath[MAXPATHLEN];
    int fd;
    int i;
    int j;
    int n_f1_pts;
    int n_f2_pts;
    int n_f3_pts;
    int datatype;
    int n_data_files;
    int databytes_per_file;
    int headerbytes;
    int npts;
    float *src;
    float *dst;
    float *dat3d;
    datafileheader *fhead3d;
    Trace *trace;

    /*
     * Read header from the first data file
     */
    sprintf(filepath,"%s/data/data1", datadir3d);
    fd = open(filepath, O_RDONLY);
    if (!fd){
	Werrprintf("\nextract3Dtrace: cannot open data file \"%s\"", filepath);
	return NULL;
    }
    fhead3d = readDATAheader(fd, NULL);
    if (!fhead3d){
	close(fd);
	return NULL;
    }
    close(fd);

    /*
     * Set useful parameters
     */
    n_f1_pts = fhead3d->Vfilehead.ntraces;
    n_f2_pts = fhead3d->Vfilehead.nblocks;
    n_f3_pts = fhead3d->f3blockinfo.hcptspertrace * n_data_files;
    datatype = fhead3d->Vfilehead.np / n_f3_pts;
    n_data_files = fhead3d->ndatafiles;
    databytes_per_file = ((n_f1_pts * n_f2_pts * n_f3_pts
			   * datatype * sizeof(float))/ n_data_files);
    headerbytes = fhead3d->nheadbytes;
    if (strcasecmp(tracedir,"f1") == 0){
	npts = n_f1_pts;
    }else if (strcasecmp(tracedir,"f2") == 0){
	npts = n_f2_pts;
    }else if (strcasecmp(tracedir,"f3") == 0){
	npts = n_f3_pts;
    }else{
	Werrprintf("\nextract3Dtrace: unknown trace type: \"%s\"", tracedir);
	return NULL;
    }

    /*
     * Get the data
     */
    trace = constructTrace(npts, datatype);
    if (!trace){
	return NULL;
    }
    if (strcasecmp(tracedir,"f1") == 0 || strcasecmp(tracedir,"f2")){
	/* Get f1 or f2 trace.  It's all in one data file. */
	int fileno;
	int start;
	int incr;

	/* Mmap the 3d data file */
	fileno = 1 + (fastindx * n_data_files) / n_f3_pts;
	sprintf(filepath,"%s/data/data%d", datadir3d, fileno);
	fd = open(filepath, O_RDONLY);
	if (!fd){
	    Werrprintf("\nextract3Dtrace: cannot open data file \"%s\"",
		       filepath);
	    destructTrace(&trace);
	    return NULL;
	}
	dat3d = (float *)mmap(0, databytes_per_file,
			      PROT_READ, 0, fd, headerbytes);
	if (!dat3d){
	    Werrprintf("\nextract3Dtrace: cannot mmap data file \"%s\"",
		       filepath);
	    close(fd);
	    destructTrace(&trace);
	    return NULL;
	}
	close(fd);

	/* Pick out the data and put it into the "trace" */
	if (strcasecmp(tracedir,"f1") == 0){
	    start = ((fastindx * n_data_files) % n_f3_pts
		     + slowindx * n_f3_pts * datatype * n_f1_pts);
	    incr = n_f3_pts * datatype - datatype;
	}else{			/* tracedir == "f2" */
	    start = ((fastindx + n_data_files) / n_f3_pts
		     + slowindx * n_f3_pts * datatype);
	    incr = n_f3_pts * datatype * n_f1_pts - datatype;
	}
	for (i=0, src=dat3d+start, dst=trace->data; i<npts; i++, src += incr){
	    for (j=0; j<datatype; j++){
		*dst++ = *src++;
	    }
	}
	munmap((char *)dat3d, databytes_per_file);
    }else if (strcasecmp(tracedir,"f3")){
	/* Get f3 trace.  It's spread across all the data files. */
	int fileno;
	int start;

	start = ((fastindx + slowindx * n_f1_pts) * datatype * n_f3_pts
		 / n_data_files);
	for (fileno=0; fileno<n_data_files; fileno++){
	    /* Mmap next 3d data file */
	    sprintf(filepath,"%s/data/data%d", datadir3d, fileno);
	    fd = open(filepath, O_RDONLY);
	    if (!fd){
		Werrprintf("\nextract3Dtrace: cannot open data file \"%s\"",
			   filepath);
		destructTrace(&trace);
		return NULL;
	    }
	    dat3d = (float *)mmap(0, databytes_per_file,
				  PROT_READ, 0, fd, headerbytes);
	    if (!dat3d){
		Werrprintf("\nextract3Dtrace: cannot mmap data file \"%s\"",
			   filepath);
		close(fd);
		destructTrace(&trace);
		return NULL;
	    }
	    close(fd);

	    /* Xfer data from this file into the "trace" */
	    for (i=0, src=dat3d+start; i<npts/n_data_files; i++){
		for (j=0; j<datatype; j++){
		    *dst++ = *src++;
		}
	    }
	    munmap((char *)dat3d, databytes_per_file);
	}
    }else{
	Werrprintf("\nextract3Dtrace: unknown trace type \"%s\"", tracedir);
	destructTrace(&trace);
	return NULL;
    }
    return trace;
}
