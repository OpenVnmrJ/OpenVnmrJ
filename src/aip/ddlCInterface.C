/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "ddlSymbol.h"

#include <stdio.h>
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "ddlCInterface.h"


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

    float *data = new float[w * h * d];
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
clone_ddl(DDLSymbolTable *st, int dataflag)
{
    return (DDLSymbolTable *)st->CloneList(dataflag);
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

    float *data = new float[w * h * d];
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

float *
get_ddl_data(DDLSymbolTable *st)
{
    return (float *)st->GetData();
}

int
get_header_int(DDLSymbolTable *st, const char *name, int *pval)
{
    if (!st){
	return false;
    }else{
	return st->GetValue(name, *pval);
    }
}

int
get_header_double(DDLSymbolTable *st, const char *name, double *pval)
{
    if (!st){
	return false;
    }else{
	return st->GetValue(name, *pval);
    }
}

int
get_header_string(DDLSymbolTable *st, const char *name, char **pval)
{
    if (!st){
	return false;
    }else{
	return st->GetValue(name, *pval);
    }
}

int
get_header_array_int(DDLSymbolTable *st, const char *name, int n, int *pval)
{
    if (!st){
	return false;
    }else{
	return st->GetValue(name, *pval, n);
    }
}

int
get_header_array_double(DDLSymbolTable *st, const char *name, int n,
                        double *pval)
{
    if (!st){
	return false;
    }else{
	return st->GetValue(name, *pval, n);
    }
}

int
get_header_array_string(DDLSymbolTable *st, const char *name, int n,
                        char **pval)
{
    if (!st){
	return false;
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

int
is_fdf_magic_number(const char *filename, const char *magic_string_list[])
{
    ifstream f;
    char header[1024];
    struct stat fname_stat;
    static const char *fdf_magic_strings[] = {"sisdata", "fdf/startup", NULL};

    
    if (stat(filename, &fname_stat) != 0){
	return false;
    }
    
    if ((fname_stat.st_mode & S_IFMT) != S_IFREG){
	return false;
    }
    
    f.open(filename, ios::in);
    
    if (!f){
	return false;
    }

    // Allow default magic strings
    if (magic_string_list == NULL){
	magic_string_list = fdf_magic_strings;
    }

    // Read in the header line (first line)
    f.getline(header, sizeof(header));

    // Check against all magic strings in list
    int rtn = false;
    const char **str = magic_string_list;
    for (str=magic_string_list; *str; str++){
	if (strstr(header, *str)){
	    rtn = true;		// Found a match
	    break;
	}
    }
    f.close();
    return rtn;
}

double
quiet_nan(long)
{
    return strtod("NaN",NULL);
}
