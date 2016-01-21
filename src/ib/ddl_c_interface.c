/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
/* 
 */

#include <stdio.h>
#ifdef LINUX
#include <strstream>
#else
#include <stream.h>
#include <strstream.h>
#endif
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "ddllib.h"
#include "ddl_c_interface.h"

int DDLNodeList::refs = 0;
int DDLNodeLink::refs = 0;

static void convertdata(float *in, u_char *out, int datasize,
			float vmin, float vmax);
static void convertdata(double *in, u_char *out, int datasize,
			float vmin, float vmax);
static void convertdata(int *in, u_char *out, int datasize,
			float vmin, float vmax);
static void convertdata(unsigned short *in, u_char *out, int datasize,
			float vmin, float vmax);
static int get_data_type(DDLSymbolTable *st, char **errmsg);
static void get_min_max(DDLSymbolTable *st, float *vmin, float *vmax);
static void get_histogram(DDLSymbolTable *st, float min, float max,
	      int *histogram, int nbins);
static void autovscale(DDLSymbolTable *, float *vmin, float *vmax);

#define DDL_INT8 1
#define DDL_INT16 2
#define DDL_INT32 3
#define DDL_FLOAT32 4
#define DDL_FLOAT64 5

/* temporary defines until a defaults panel is constructed	*/
#define	CHAR_MAX	255
#define SHORT_MAX	65535
#define INT_MAX		2147483647.0

DDLSymbolTable *
read_ddl_file(char *filename)
{
    DDLSymbolTable *st = NULL;	// Contains the ddl symbol table

    // Check for file type
    if (is_fdf_magic_number(filename, NULL)){
	/* Create a DDL symbol table and load the data */
	st = ParseDDLFile(filename);
	if (st){
	    st->MallocData();
	    st->SetValue("dirpath", "/");
	    st->SetValue("filename", filename);
	}
    }
    return st;
}

float *
get_ddl_data(DDLSymbolTable *st)
{
    return (float *)st->GetData();
}

int
get_header_int(DDLSymbolTable *st, char *name, int *pval)
{
    if (!st){
	return FALSE;
    }else{
	return st->GetValue(name, *pval);
    }
}

int
get_header_double(DDLSymbolTable *st, char *name, double *pval)
{
    if (!st){
	return FALSE;
    }else{
	return st->GetValue(name, *pval);
    }
}

int
get_header_string(DDLSymbolTable *st, char *name, char **pval)
{
    if (!st){
	return FALSE;
    }else{
	return st->GetValue(name, *pval);
    }
}

int
get_header_array_int(DDLSymbolTable *st, char *name, int n, int *pval)
{
    if (!st){
	return FALSE;
    }else{
	return st->GetValue(name, *pval, n);
    }
}

int
get_header_array_double(DDLSymbolTable *st, char *name, int n, double *pval)
{
    if (!st){
	return FALSE;
    }else{
	return st->GetValue(name, *pval, n);
    }
}

int
get_header_array_string(DDLSymbolTable *st, char *name, int n, char **pval)
{
    if (!st){
	return FALSE;
    }else{
	return st->GetValue(name, *pval, n);
    }
}

int
get_image_width(DDLSymbolTable *st)
{
    int ret = 0;

    if (st){
	st->GetValue("matrix", ret, 0);
    }
    return ret;
}

int
get_image_height(DDLSymbolTable *st)
{
    int ret = 1;
    int rank = 1;

    if (st){
	st->GetValue("rank", rank);
	if (rank > 1){
	    st->GetValue("matrix", ret, 1);
	}
    }
    return ret;
}

int
get_image_depth(DDLSymbolTable *st)
{
    int ret = 1;
    int rank = 1;

    if (st){
	st->GetValue("rank", rank);
	if (rank > 2){
	    st->GetValue("matrix", ret, 2);
	}
    }
    return ret;
}

double
get_object_width(DDLSymbolTable *st)
{
    double ret = 0;
    if (st) st->GetValue("roi", ret, 0);
    return ret;
}

double
get_object_height(DDLSymbolTable *st)
{
    double ret = 0;
    if (st) st->GetValue("roi", ret, 1);
    return ret;
}

double
get_object_depth(DDLSymbolTable *st)
{
    double ret = 0;
    if (st) st->GetValue("roi", ret, 2);
    return ret;
}

void
write_ddl_data(DDLSymbolTable *st, char *fname)
{
    st->SaveSymbolsAndData(fname);
}

DDLSymbolTable *
clone_ddl(DDLSymbolTable *st, int dataflag)
{
    return (DDLSymbolTable *)st->CloneList(dataflag);
}

void
init_ddl(DDLSymbolTable *st, int w, int h, int d)
{
    int i;

    if (w<1 || h<1 || d<1){
	return;
    }

    int filesize = sizeof(float) * w * h * d;

    int rank;
    if (d > 1){
	rank = 3;
    }else if (h > 1){
	rank = 2;
    }else if (w > 1){
	rank = 1;
    }else{
	return;
    }
    st->SetValue("rank", rank);
    switch (rank){
      case 1:
	st->SetValue("spatial_rank", "1dfov");
	break;
      case 2:
	st->SetValue("spatial_rank", "2dfov");
	break;
      case 3:
	st->SetValue("spatial_rank", "3dfov");
	break;
    }

    char *data = new char[filesize];
    if (!data){
	return;
    }
    st->SetData(data, filesize);

    st->CreateArray("matrix");
    if (rank >= 1) st->AppendElement("matrix", w);
    if (rank >= 2) st->AppendElement("matrix", h);
    if (rank >= 3) st->AppendElement("matrix", d);

    st->CreateArray("abscissa");
    for (i=0; i<rank; i++){
	st->AppendElement("abscissa", "cm");
    }
}

DDLSymbolTable *
create_ddl(int w, int h, int d)
{
    int i;
    DDLSymbolTable *st;
    int rank;
    int filesize;

    if (w<1 || h<1 || d<1){
	return NULL;
    }
    filesize = sizeof(float) * w * h * d;

    if (d > 1){
	rank = 3;
    }else if (h > 1){
	rank = 2;
    }else if (w > 1){
	rank = 1;
    }else{
	return NULL;
    }
    st = new DDLSymbolTable();
    if (!st){
	return NULL;
    }
    st->SetValue("rank", rank);
    switch (rank){
      case 2:
	st->SetValue("spatial_rank", "2dfov");
	break;
      case 3:
	st->SetValue("spatial_rank", "3dfov");
	break;
    }

    st->SetValue("bits", 32);

    st->SetValue("storage", "float");

    char *data = new char[filesize];
    if (!data){
	return NULL;
    }
    st->SetData(data, filesize);

    st->CreateArray("matrix");
    if (rank >= 1) st->AppendElement("matrix", w);
    if (rank >= 2) st->AppendElement("matrix", h);
    if (rank >= 3) st->AppendElement("matrix", d);

    st->CreateArray("abscissa");
    for (i=0; i<rank; i++){
	st->AppendElement("abscissa", "cm");
    }

    st->CreateArray("ordinate");
    st->AppendElement("ordinate", "intensity");

    st->CreateArray("span");
    for (i=0; i<rank; i++){
	st->AppendElement("span", 10.0);
    }
  
    st->CreateArray("origin");
    for (i=0; i<rank; i++){
	st->AppendElement("origin", 0.0);
    }
  
    st->CreateArray("nucleus");
    st->AppendElement("nucleus", "H1");
    st->AppendElement("nucleus", "H1");
  
    st->CreateArray("nucfreq");
    st->AppendElement("nucfreq", 200.0);
    st->AppendElement("nucfreq", 200.0);
  
    st->CreateArray("location");
    st->AppendElement("location", 0.0);
    st->AppendElement("location", 0.0);
    st->AppendElement("location", 0.0);
  
    st->CreateArray("roi");
    st->AppendElement("roi", 10.0);
    st->AppendElement("roi", 10.0);
    st->AppendElement("roi", 10.0);
  
    st->CreateArray("orientation");
    st->AppendElement("orientation", 1.0);
    st->AppendElement("orientation", 0.0);
    st->AppendElement("orientation", 0.0);
    st->AppendElement("orientation", 0.0);
    st->AppendElement("orientation", 1.0);
    st->AppendElement("orientation", 0.0);
    st->AppendElement("orientation", 0.0);
    st->AppendElement("orientation", 0.0);
    st->AppendElement("orientation", 1.0);
  
    return st;
}

int
is_fdf_magic_number(char *filename, char *magic_string_list[])
{
    ifstream f;
    char header[1024];
    struct stat fname_stat;
    static char *fdf_magic_strings[] = {"sisdata", "fdf/startup", NULL};

    
    if (stat(filename, &fname_stat) != 0){
	return FALSE;
    }
    
    if ((fname_stat.st_mode & S_IFMT) != S_IFREG){
	return FALSE;
    }
    
    f.open(filename, ios::in);
    
    if (!f){
	return FALSE;
    }

    // Allow default magic strings
    if (magic_string_list == NULL){
	magic_string_list = fdf_magic_strings;
    }

    // Read in the header line (first line)
    f.getline(header, sizeof(header));

    // Check against all magic strings in list
    int rtn = FALSE;
    char **str = magic_string_list;
    for (str=magic_string_list; *str; str++){
	if (strstr(header, *str)){
	    rtn = TRUE;		// Found a match
	    break;
	}
    }
    f.close();
    return rtn;
}

static int
get_data_type(DDLSymbolTable *st, char **errmsg)
{
    int type = 0;

    int bits=0;
    st->GetValue("bits", bits);

    char* storage = "";
    st->GetValue("storage", storage);

    int isint = strcmp(storage, "integer") == 0 || strcmp(storage, "int") == 0;
    int ischar = strcmp(storage, "char") == 0;
    int isshort = strcmp(storage, "short") == 0;
    int isfloat = strcmp(storage, "float") == 0;

    if (bits == 8 && (isint || ischar)){
	type = DDL_INT8;
    }else if (bits == 16 && (isint || isshort)){
	type = DDL_INT16;
    }else if (bits == 32 && isint){
	type = DDL_INT32;
    }else if (bits == 32 && isfloat){
	type = DDL_FLOAT32;
    }else if (bits = 64 && isfloat){
	type = DDL_FLOAT64;
    }else if (!isint && !ischar && !isshort && !isfloat){
	*errmsg = "Invalid \"storage\" field.";
    }else{
	*errmsg = "Invalid \"bits\" field for storage type.";
    }
    return type;
}


u_char *
get_uchar_data(DDLSymbolTable *st,
	       double vmin, double vmax, char **errmsg, u_char *outdata)
{
    void *indata = (void *)st->GetData();

    int type = get_data_type(st, errmsg);
    if (!type){
	return NULL;
    }

    int bits=0;
    st->GetValue("bits", bits);
    int datasize = st->DataLength() / (bits/8);  
    if (type != DDL_INT8){
	if (!outdata){
	    outdata = new u_char[datasize];
	}
	if (!outdata){
	    *errmsg = "Unable to allocate memory.";
	    return NULL;
	}
	if (vmin == vmax){
	    // Auto scale
	    /*autovscale(st, &vmin, &vmax);*/
	    vmin = 0;
	    vmax = 1;
	}
    }

    switch (type){
      case DDL_INT8:
	// No conversion to do (IGNORE SCALING ALSO)
	outdata = (u_char *)indata;
	break;
      case DDL_INT16:
	convertdata((u_short *)indata, outdata, datasize, vmin, vmax);
	break;
      case DDL_INT32:
	convertdata( (int *)indata, outdata, datasize, vmin, vmax);
	break;
      case DDL_FLOAT32:
	convertdata( (float *)indata, outdata, datasize, vmin, vmax);
	break;
      case DDL_FLOAT64:
	convertdata( (double *)indata, outdata, datasize, vmin, vmax);
	break;
    }

    return outdata;
}

double
quiet_nan(long)
{
    double x;
    int *pi;

    pi = (int *)(&x);
    pi[0] = 0x7fffffff;
    pi[1] = 0xffffffff;

    return x;
}

/*
 * Note: the text of all the "convertdata()"s is the same, except for the
 * type of the "in" argument.
 */
static void
convertdata(float *in, u_char *out, int datasize, float vmin, float vmax)
{
    float vs = CHAR_MAX / (vmax - vmin);
    while (datasize-- > 0){
	*out++ = *in < vmin ? 0 : (*in > vmax ? CHAR_MAX :
				   (u_char)((*in - vmin) * vs));
	in++;
    }
}

static void
convertdata(double *in, u_char *out, int datasize, float vmin, float vmax)
{
    float vs = CHAR_MAX / (vmax - vmin);
    while (datasize-- > 0){
	*out++ = *in < vmin ? 0 : (*in > vmax ? CHAR_MAX :
				   (u_char)((*in - vmin) * vs));
	in++;
    }
}

static void
convertdata(int *in, u_char *out, int datasize, float vmin, float vmax)
{
    float vs = CHAR_MAX / (vmax - vmin);
    while (datasize-- > 0){
	*out++ = *in < vmin ? 0 : (*in > vmax ? CHAR_MAX :
				   (u_char)((*in - vmin) * vs));
	in++;
    }
}

static void
convertdata(unsigned short *in, u_char *out, int datasize, float vmin, float vmax)
{
    float vs = CHAR_MAX / (vmax - vmin);
    while (datasize-- > 0){
	*out++ = *in < vmin ? 0 : (*in > vmax ? CHAR_MAX :
				   (u_char)((*in - vmin) * vs));
	in++;
    }
}

static void
get_min_max(DDLSymbolTable *st, float *vmin, float *vmax)
{
    char *errmsg;
    int npts = (get_image_width(st) * get_image_height(st)
		* get_image_depth(st));
    int type = get_data_type(st, &errmsg);
    switch (type){
      case DDL_INT8:
	{
	    u_char *data = (u_char *)get_ddl_data(st);
	    u_char *end = data + npts;
	    u_char min = *data;
	    u_char max = *data++;
	    u_char x;
	    while (data < end){
		if ((x=*data++) < min){
		    min = x;
		}else if (x > max){
		    max = x;
		}
	    }
	    *vmin = (float)min;
	    *vmax = (float)max;
	}
	break;
      case DDL_INT16:
	{
	    u_short *data = (u_short *)get_ddl_data(st);
	    u_short *end = data + npts;
	    u_short min = *data;
	    u_short max = *data++;
	    u_short x;
	    while (data < end){
		if ((x=*data++) < min){
		    min = x;
		}else if (x > max){
		    max = x;
		}
	    }
	    *vmin = (float)min;
	    *vmax = (float)max;
	}
	break;
      case DDL_INT32:
	{
	    int *data = (int *)get_ddl_data(st);
	    int *end = data + npts;
	    int min = *data;
	    int max = *data++;
	    int x;
	    while (data < end){
		if ((x=*data++) < min){
		    min = x;
		}else if (x > max){
		    max = x;
		}
	    }
	    *vmin = (float)min;
	    *vmax = (float)max;
	}
	break;
      case DDL_FLOAT32:
	{
	    float *data = (float *)get_ddl_data(st);
	    float *end = data + npts;
	    float min = *data;
	    float max = *data++;
	    float x;
	    while (data < end){
		if ((x=*data++) < min){
		    min = x;
		}else if (x > max){
		    max = x;
		}
	    }
	    *vmin = (float)min;
	    *vmax = (float)max;
	}
	break;
      case DDL_FLOAT64:
	{
	    double *data = (double *)get_ddl_data(st);
	    double *end = data + npts;
	    double min = *data;
	    double max = *data++;
	    double x;
	    while (data < end){
		if ((x=*data++) < min){
		    min = x;
		}else if (x > max){
		    max = x;
		}
	    }
	    *vmin = (float)min;
	    *vmax = (float)max;
	}
	break;
    }
}

static void
get_histogram(DDLSymbolTable *st, float min, float max,
	      int *histogram, int nbins)
{
    int i;
    char *errmsg;
    int npts = (get_image_width(st) * get_image_height(st)
		* get_image_depth(st));
    float scale = (nbins - 1) / (max - min);
    for (i=0; i<nbins; i++){
	histogram[i] = 0;
    }
    int type = get_data_type(st, &errmsg);
    switch (type){
      case DDL_INT8:
	{
	    u_char *data = (u_char *)get_ddl_data(st);
	    u_char *end = data + npts;
	    while (data < end){
		histogram[(int)((*data++ - min) * scale)]++;
	    }
	}
	break;
      case DDL_INT16:
	{
	    u_short *data = (u_short *)get_ddl_data(st);
	    u_short *end = data + npts;
	    while (data < end){
		histogram[(int)((*data++ - min) * scale)]++;
	    }
	}
	break;
      case DDL_INT32:
	{
	    int *data = (int *)get_ddl_data(st);
	    int *end = data + npts;
	    while (data < end){
		histogram[(int)((*data++ - min) * scale)]++;
	    }
	}
	break;
      case DDL_FLOAT32:
	{
	    float *data = (float *)get_ddl_data(st);
	    float *end = data + npts;
	    while (data < end){
		histogram[(int)((*data++ - min) * scale)]++;
	    }
	}
	break;
      case DDL_FLOAT64:
	{
	    double *data = (double *)get_ddl_data(st);
	    double *end = data + npts;
	    while (data < end){
		histogram[(int)((*data++ - min) * scale)]++;
	    }
	}
	break;
    }
}


static void
autovscale(DDLSymbolTable *st, float *vmin, float *vmax)
{
    int i;
    int n;
    float min;
    float max;

    // Find max and min values in the data
    int npts = (get_image_width(st) * get_image_height(st)
		* get_image_depth(st));
    get_min_max(st, &min, &max);

    // Get intensity distribution
    int nbins = 10000;
    int *histogram = new int[nbins];
    get_histogram(st, min, max, histogram, nbins);

    // Estimate percentile points from distribution
    int percentile = (int)(0.01 * npts);
    if (percentile < 1) percentile = 1;
    float scale = (nbins - 1) / (max - min);
    for (i=n=0; n<percentile && i<nbins; n += histogram[i++]);
    *vmin = min + (i-1) / scale;
    for (i=nbins, n=0; n<percentile && i>0; n += histogram[--i]);
    *vmax = min + i / scale;

    // Set vmin to 0 if it looks plausible
    if (*vmin > 0 && *vmin / *vmax < 0.05){
	*vmin = 0;
    }

    delete [] histogram;
}   
